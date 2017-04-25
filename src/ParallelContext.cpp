#include "ParallelContext.hpp"

#include "Options.hpp"
#include <limits>
using namespace std;

// TODO: estimate based on #threads and #partitions?
#define PARALLEL_BUF_SIZE (128 * 1024)

size_t ParallelContext::_num_threads = 1;
size_t ParallelContext::_num_ranks = 1;
size_t ParallelContext::_rank_id = 0;
thread_local size_t ParallelContext::_thread_id = 0;
std::vector<ThreadType> ParallelContext::_threads;
std::vector<char> ParallelContext::_parallel_buf;
std::unordered_map<ThreadIDType, ParallelContext> ParallelContext::_thread_ctx_map;
MutexType ParallelContext::mtx;

std::vector<long> ParallelContext::_cpu_times;
std::vector<long> ParallelContext::_sync_times;
std::vector<long> ParallelContext::_reduce_times;
timespec ParallelContext::_last_time;
long ParallelContext::_total_cpu_time = 0;
long ParallelContext::_total_sync_time = 0;
long ParallelContext::_total_reduce_time = 0;
long ParallelContext::_total_min_sync_time = 0;
const char *ParallelContext::_step;

void ParallelContext::init_mpi(int argc, char * argv[])
{
#ifdef _RAXML_MPI
  {
    int tmp;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &tmp);
    _rank_id = (size_t) tmp;
    MPI_Comm_size(MPI_COMM_WORLD, &tmp);
    _num_ranks = (size_t) tmp;
//    printf("size: %lu, rank: %lu\n", _num_ranks, _rank_id);
  }
#else
  UNUSED(argc);
  UNUSED(argv);
#endif
}

void ParallelContext::start_thread(size_t thread_id, const std::function<void()>& thread_main)
{
  ParallelContext::_thread_id = thread_id;
  thread_main();
}

void ParallelContext::init_pthreads(const Options& opts, const std::function<void()>& thread_main)
{
  _num_threads = opts.num_threads;
  _parallel_buf.reserve(PARALLEL_BUF_SIZE);

#ifdef _RAXML_PTHREADS
  /* Launch threads */
  for (size_t i = 1; i < _num_threads; ++i)
  {
    _threads.emplace_back(ParallelContext::start_thread, i, thread_main);
  }
#endif
}

void ParallelContext::finalize(bool force)
{
#ifdef _RAXML_PTHREADS
  for (thread& t: _threads)
  {
    if (force)
      t.detach();
    else
      t.join();
  }
  _threads.clear();
#endif

#ifdef _RAXML_MPI
  if (force)
    MPI_Abort(MPI_COMM_WORLD, -1);
  else
    MPI_Barrier(MPI_COMM_WORLD);

  MPI_Finalize();
#endif
}

void ParallelContext::barrier()
{
#ifdef _RAXML_MPI
  mpi_barrier();
#endif

#ifdef _RAXML_PTHREADS
  thread_barrier();
#endif
}

void ParallelContext::mpi_barrier()
{
#ifdef _RAXML_MPI
  if (_thread_id == 0 && _num_ranks > 1)
    MPI_Barrier(MPI_COMM_WORLD);
#endif
}

void ParallelContext::thread_barrier()
{
  static volatile unsigned int barrier_counter = 0;
  static thread_local volatile int myCycle = 0;
  static volatile int proceed = 0;

  __sync_fetch_and_add( &barrier_counter, 1);

  if(_thread_id == 0)
  {
    while(barrier_counter != ParallelContext::_num_threads);
    barrier_counter = 0;
    proceed = !proceed;
  }
  else
  {
    while(myCycle == proceed);
    myCycle = !myCycle;
  }
}

long diff_time_ns(struct timespec start, struct timespec end) 
{
  struct timespec temp;
  temp.tv_sec = end.tv_sec-start.tv_sec;
  temp.tv_nsec = end.tv_nsec-start.tv_nsec;
  return (temp.tv_sec * 1000000000 + temp.tv_nsec);
}

void ParallelContext::thread_reduce(double * data, size_t size, int op)
{
  timespec new_time;
  if (_cpu_times.size()) {
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &new_time);
    _cpu_times[_thread_id] += diff_time_ns(_last_time, new_time);
    _last_time = new_time;
  }
  /* synchronize */
  thread_barrier();
 
  if (_cpu_times.size()) {
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &new_time);
    _sync_times[_thread_id] += diff_time_ns(_last_time, new_time);
    _last_time = new_time;
  }

  double *double_buf = (double*) _parallel_buf.data();

  /* collect data from threads */
  size_t i, j;
  for (i = 0; i < size; ++i)
    double_buf[_thread_id * size + i] = data[i];

  /* synchronize */
  thread_barrier();

  /* reduce */
  for (i = 0; i < size; ++i)
  {
    switch(op)
    {
      case PLLMOD_TREE_REDUCE_SUM:
      {
        data[i] = 0.;
        for (j = 0; j < ParallelContext::_num_threads; ++j)
          data[i] += double_buf[j * size + i];
      }
      break;
      case PLLMOD_TREE_REDUCE_MAX:
      {
        data[i] = _parallel_buf[i];
        for (j = 1; j < ParallelContext::_num_threads; ++j)
          data[i] = max(data[i], double_buf[j * size + i]);
      }
      break;
      case PLLMOD_TREE_REDUCE_MIN:
      {
        data[i] = _parallel_buf[i];
        for (j = 1; j < ParallelContext::_num_threads; ++j)
          data[i] = min(data[i], double_buf[j * size + i]);
      }
      break;
    }
  }

  if (_cpu_times.size()) {
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &new_time);
    _reduce_times[_thread_id] += diff_time_ns(_last_time, new_time);
    _last_time = new_time;
  }
  //needed?
//  parallel_barrier(useropt);
}


void ParallelContext::parallel_reduce(double * data, size_t size, int op)
{
  timespec new_time;
#ifdef _RAXML_PTHREADS
  if (_num_threads > 1)
    thread_reduce(data, size, op);
#endif

#ifdef _RAXML_MPI
  if (_num_ranks > 1)
  {
    if (_cpu_times.size()) {
      clock_gettime(CLOCK_THREAD_CPUTIME_ID, &new_time);
      _cpu_times[_rank_id] += diff_time_ns(_last_time, new_time);
      _last_time = new_time;
    }
    
    thread_barrier();
  
    /* this is temporary for load balancing statistics */
    mpi_barrier();
    if (_cpu_times.size()) {
      clock_gettime(CLOCK_THREAD_CPUTIME_ID, &new_time);
      _sync_times[_rank_id] += diff_time_ns(_last_time, new_time);
      _last_time = new_time;
    }
   

    if (_thread_id == 0)
    {
      MPI_Op reduce_op;
      if (op == PLLMOD_TREE_REDUCE_SUM)
        reduce_op = MPI_SUM;
      else if (op == PLLMOD_TREE_REDUCE_MAX)
        reduce_op = MPI_MAX;
      else if (op == PLLMOD_TREE_REDUCE_MIN)
        reduce_op = MPI_MIN;
      else
        assert(0);

#if 1
      MPI_Allreduce(MPI_IN_PLACE, data, size, MPI_DOUBLE, reduce_op, MPI_COMM_WORLD);
#else
      // not sure if MPI_IN_PLACE will work in all cases...
      MPI_Allreduce(data, _parallel_buf.data(), size, MPI_DOUBLE, reduce_op, MPI_COMM_WORLD);
      memcpy(data, _parallel_buf.data(), size * sizeof(double));
#endif
    }
    if (_cpu_times.size()) {
      clock_gettime(CLOCK_THREAD_CPUTIME_ID, &new_time);
      _reduce_times[_rank_id] += diff_time_ns(_last_time, new_time);
      _last_time = new_time;
    }
    

    if (_num_threads > 1)
      thread_broadcast(0, data, size * sizeof(double));
  }
#endif
}

void ParallelContext::parallel_reduce_cb(void * context, double * data, size_t size, int op)
{
  ParallelContext::parallel_reduce(data, size, op);
  UNUSED(context);
}

void ParallelContext::thread_broadcast(size_t source_id, void * data, size_t size)
{
  /* write to buf */
  if (_thread_id == source_id)
  {
    memcpy((void *) _parallel_buf.data(), data, size);
  }

  /* synchronize */
  thread_barrier();

//  pthread_barrier_wait(&barrier);
  __sync_synchronize();

  /* read from buf*/
  if (_thread_id != source_id)
  {
    memcpy(data, (void *) _parallel_buf.data(), size);
  }

  thread_barrier();
}

void ParallelContext::thread_send_master(size_t source_id, void * data, size_t size) const
{
  /* write to buf */
  if (_thread_id == source_id && data && size)
  {
    memcpy((void *) _parallel_buf.data(), data, size);
  }

  /* synchronize */
  barrier();

//  pthread_barrier_wait(&barrier);
  __sync_synchronize();

  /* read from buf*/
  if (_thread_id == 0)
  {
    memcpy(data, (void *) _parallel_buf.data(), size);
  }

  barrier();
}

void ParallelContext::mpi_gather_custom(std::function<int(void*,int)> prepare_send_cb,
                                        std::function<void(void*,int)> process_recv_cb)
{
#ifdef _RAXML_MPI
  /* we're gonna use _parallel_buf, so make sure other threads don't interfere... */
  UniqueLock lock;

  if (_rank_id == 0)
  {
    for (size_t r = 1; r < _num_ranks; ++r)
    {
      int recv_size;
      MPI_Status status;
      MPI_Probe(r, 0, MPI_COMM_WORLD, &status);
      MPI_Get_count(&status, MPI_BYTE, &recv_size);

//      printf("recv: %lu\n", recv_size);

      _parallel_buf.reserve(recv_size);

      MPI_Recv((void*) _parallel_buf.data(), recv_size, MPI_BYTE,
               r, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

      process_recv_cb(_parallel_buf.data(), recv_size);
    }
  }
  else
  {
    auto send_size = prepare_send_cb(_parallel_buf.data(), _parallel_buf.capacity());
//    printf("sent: %lu\n", send_size);

    MPI_Send(_parallel_buf.data(), send_size, MPI_BYTE, 0, 0, MPI_COMM_WORLD);
  }
#else
  UNUSED(prepare_send_cb);
  UNUSED(process_recv_cb);
#endif
}
  
void ParallelContext::reinit_stats(const char *step)
{
  barrier();
  unsigned int threads = std::max(_num_threads, _num_ranks);
    _cpu_times.resize(threads);
    _sync_times.resize(threads);
    _reduce_times.resize(threads);
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &_last_time);
    ParallelContext::_step = step;
    std::fill(_cpu_times.begin(), _cpu_times.end(), 0);
    std::fill(_sync_times.begin(), _sync_times.end(), 0);
    std::fill(_reduce_times.begin(), _reduce_times.end(), 0);
  barrier();
}

void ParallelContext::print_stats()
{

  MPI_Gather(&_cpu_times[_rank_id], 1, MPI_LONG_LONG, &_cpu_times[0], 1, MPI_LONG_LONG, 0, MPI_COMM_WORLD); 
  MPI_Gather(&_sync_times[_rank_id], 1, MPI_LONG_LONG, &_sync_times[0], 1, MPI_LONG_LONG, 0, MPI_COMM_WORLD); 
  MPI_Gather(&_reduce_times[_rank_id], 1, MPI_LONG_LONG, &_reduce_times[0], 1, MPI_LONG_LONG, 0, MPI_COMM_WORLD); 
  if (_thread_id || _rank_id ) {
    return;
  }

  long allcores_cpu_time = 0;
  long allcores_sync_time = 0;
  long allcores_reduce_time = 0;
  long min_sync_time = std::numeric_limits<long>::max();
  for (unsigned int i = 0; i < _cpu_times.size(); ++i) {
    allcores_cpu_time += _cpu_times[i];
    allcores_sync_time += _sync_times[i];
    allcores_reduce_time += _reduce_times[i];
    min_sync_time = std::min(min_sync_time, _sync_times[i]);
  }
  _total_cpu_time += allcores_cpu_time;
  _total_sync_time += allcores_sync_time;
  _total_reduce_time += allcores_reduce_time;
  _total_min_sync_time += min_sync_time;

  long allcores_time = allcores_cpu_time + allcores_sync_time + allcores_reduce_time;
  long total_time = _total_cpu_time + _total_sync_time + _total_reduce_time;
    
  LOG_PROGR << "    stats_step: " << _step << std::endl; 
  LOG_PROGR << "    step cpu ratio: "    << double(allcores_cpu_time) / double(allcores_time) << std::endl;
  LOG_PROGR << "    step sync ratio: "    << double(allcores_sync_time) / double(allcores_time) << std::endl;
  LOG_PROGR << "    step reduce ratio: "    << double(allcores_reduce_time) / double(allcores_time) << std::endl;
  LOG_PROGR << "    step irreductible sync ratio: "    << double(min_sync_time) / double(allcores_time) << std::endl;
  LOG_PROGR << "    total cpu ratio: "    << double(_total_cpu_time) / double(total_time) << std::endl;
  LOG_PROGR << "    total sync ratio: "    << double(_total_sync_time) / double(total_time) << std::endl;
  LOG_PROGR << "    total reduce ratio: "    << double(_total_reduce_time) / double(total_time) << std::endl;
  LOG_PROGR << "    total irreductible sync ratio: "    << double(_total_min_sync_time) / double(total_time) << std::endl;


  LOG_PROGR << "    detailed cpu_time_ms ";
  for (unsigned int i = 0; i < _cpu_times.size(); ++i) {
    LOG_PROGR << " (" << i << ", " << _cpu_times[i] / 1000000 << ")";
  }
  LOG_PROGR  << std::endl;
 
  LOG_PROGR << "    detailed sync_time_ms ";
  for (unsigned int i = 0; i < _sync_times.size(); ++i) {
    LOG_PROGR << " (" << i << ", " << _sync_times[i] / 1000000 << ")";
  }
  LOG_PROGR  << std::endl;
 
  LOG_PROGR << "    detailed reduce_time_ms ";
  for (unsigned int i = 0; i < _reduce_times.size(); ++i) {
    LOG_PROGR << " (" << i << ", " << _reduce_times[i] / 1000000 << ")";
  }
  LOG_PROGR  << std::endl;
 
}


