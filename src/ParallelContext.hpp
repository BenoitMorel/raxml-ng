#ifndef RAXML_PARALLELCONTEXT_HPP_
#define RAXML_PARALLELCONTEXT_HPP_

#include <vector>
#include <unordered_map>
#include <memory>

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

  static const ParallelContext& ctx();
  static size_t num_procs() { return _num_ranks * _num_threads; }
  static size_t num_threads() { return _num_threads; }
  static size_t num_ranks() { return _num_ranks; }

  static void parallel_reduce_cb(void * context, double * data, size_t size, int op);
  void thread_reduce(double * data, size_t size, int op) const;
  void thread_broadcast(size_t source_id, void * data, size_t size) const;
  void thread_send_master(size_t source_id, void * data, size_t size) const;

  static void mpi_gather_custom(std::function<int(void*,int)> prepare_send_cb,
                                std::function<void(void*,int)> process_recv_cb);

  static bool master_rank() { return _rank_id == 0; }
  static bool is_master() { return num_procs() == 1 || ctx().master(); }
  static void ctx_barrier() { ctx().barrier(); }

  size_t thread_id() const { return _thread_id; }
  size_t proc_id() const { return _rank_id * _num_threads + _thread_id; }
  bool master() const { return proc_id() == 0; }
  bool master_thread() const { return _thread_id == 0; }

  void barrier() const;
  void thread_barrier() const;
  void mpi_barrier() const;

  /* non-copyable ! */
//  ParallelContext(const ParallelContext& other) = delete;
//  ParallelContext(ParallelContext&& other) = delete;
//  ParallelContext& operator=(const ParallelContext& other) = delete;
//  ParallelContext& operator=(ParallelContext&& other) = delete;

  class UniqueLock
  {
  public:
    UniqueLock() : _lock(mtx) {}
  private:
    LockType _lock;
  };
private:
  static std::vector<ThreadType> _threads;
  static size_t _num_threads;
  static size_t _num_ranks;
  static std::vector<char> _parallel_buf;
  static std::unordered_map<ThreadIDType, ParallelContext> _thread_ctx_map;
  static MutexType mtx;

  static size_t _rank_id;
  size_t _thread_id;

  ParallelContext (size_t thread_id) : _thread_id(thread_id) {};

  void parallel_reduce(double * data, size_t size, int op) const;
};

#endif /* RAXML_PARALLELCONTEXT_HPP_ */
