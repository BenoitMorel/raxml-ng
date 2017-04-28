#ifndef RAXML_PARALLELCONTEXT_HPP_
#define RAXML_PARALLELCONTEXT_HPP_

#include <vector>
#include <unordered_map>
#include <memory>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef _RAXML_MPI
#include <mpi.h>
#endif

#ifdef _RAXML_PTHREADS
#include <thread>
#include <mutex>
typedef std::thread ThreadType;
typedef std::thread::id ThreadIDType;
typedef std::mutex MutexType;
typedef std::unique_lock<std::mutex> LockType;
#else
typedef int ThreadType;
typedef size_t ThreadIDType;
typedef int MutexType;
typedef int LockType;
#endif

class Options;

class ParallelContext
{
public:
  static void init_mpi(int argc, char * argv[]);
  static void init_pthreads(const Options& opts, const std::function<void()>& thread_main);

  static void finalize(bool force = false);

  static size_t num_procs() { return _num_ranks * _num_threads; }
  static size_t num_threads() { return _num_threads; }
  static size_t num_ranks() { return _num_ranks; }

  static void parallel_reduce_cb(void * context, double * data, size_t size, int op);
  static void thread_reduce(double * data, size_t size, int op);
  static void thread_broadcast(size_t source_id, void * data, size_t size);
  void thread_send_master(size_t source_id, void * data, size_t size) const;

  static void mpi_gather_custom(std::function<int(void*,int)> prepare_send_cb,
                                std::function<void(void*,int)> process_recv_cb);

  static bool master() { return proc_id() == 0; }
  static bool master_rank() { return _rank_id == 0; }
  static bool master_thread() { return _thread_id == 0; }
  static size_t thread_id() { return _thread_id; }
  static size_t proc_id() { return _rank_id * _num_threads + _thread_id; }

  static void gather(double value, double *dest, unsigned int rank);
  static void barrier();
  static void thread_barrier();
  static void mpi_barrier();

  /* static singleton, no instantiation/copying/moving */
  ParallelContext() = delete;
  ParallelContext(const ParallelContext& other) = delete;
  ParallelContext(ParallelContext&& other) = delete;
  ParallelContext& operator=(const ParallelContext& other) = delete;
  ParallelContext& operator=(ParallelContext&& other) = delete;

  class UniqueLock
  {
  public:
    UniqueLock() : _lock(mtx) {}
  private:
    LockType _lock;
  };
public:
  static std::vector<ThreadType> _threads;
  static size_t _num_threads;
  static size_t _num_ranks;
  static std::vector<char> _parallel_buf;
  static std::unordered_map<ThreadIDType, ParallelContext> _thread_ctx_map;
  static MutexType mtx;

  static size_t _rank_id;
  static thread_local size_t _thread_id;

  static void start_thread(size_t thread_id, const std::function<void()>& thread_main);
  static void parallel_reduce(double * data, size_t size, int op);

public:
  static std::vector<long> _cpu_times;
  static std::vector<long> _waiting_times;
  static std::vector<timespec> _starts; 
  static std::vector<timespec> _ends;
  static long _total_cpu_time;
  static long _total_waiting_time;
  static long _total_min_waiting_time;
  static const char *_step;
  static void reinit_stats(const char *step);
  static void print_stats();
};

#endif /* RAXML_PARALLELCONTEXT_HPP_ */
