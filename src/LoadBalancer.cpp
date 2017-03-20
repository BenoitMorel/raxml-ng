#include <assert.h>
#include <stack>
#include <algorithm>
#include <stdexcept>
#include <iostream>
#include "LoadBalancer.hpp"

using namespace std;

LoadBalancer::LoadBalancer ()
{
  // TODO Auto-generated constructor stub

}

LoadBalancer::~LoadBalancer ()
{
  // TODO Auto-generated destructor stub
}

PartitionAssignmentList LoadBalancer::get_all_assignments(const PartitionAssignment& part_sizes,
                                                          size_t num_procs)
{
  if (num_procs == 1)
    return PartitionAssignmentList(1, part_sizes);
  else
    return compute_assignments(part_sizes, num_procs);
}

PartitionAssignment LoadBalancer::get_proc_assignments(const PartitionAssignment& part_sizes,
                                                       size_t num_procs, size_t proc_id)
{
  if (proc_id >= num_procs)
    throw std::out_of_range("Process ID out of range");

  if (num_procs == 1)
    return part_sizes;
  else
    return compute_assignments(part_sizes, num_procs).at(proc_id);
}

PartitionAssignmentList SimpleLoadBalancer::compute_assignments(const PartitionAssignment& part_sizes,
                                                                    size_t num_procs)
{
  PartitionAssignmentList part_assign(num_procs);

  size_t proc_id = 0;
  for (auto& proc_assign: part_assign)
  {
    for (auto const& full_range: part_sizes)
    {
      const size_t total_sites = full_range.length;
      const size_t proc_sites = total_sites / num_procs;
      auto part_id = full_range.part_id;
      auto start = full_range.start + proc_id * proc_sites;
      auto length = (proc_id == num_procs-1) ? total_sites - start : proc_sites;
      proc_assign.assign_sites(part_id, start, length);
    }
    ++proc_id;
  }

  assert(proc_id == part_assign.size());

  return part_assign;
}

PartitionAssignmentList KassianLoadBalancer::compute_assignments(const PartitionAssignment& part_sizes,
                                                                 size_t num_procs)
{
  PartitionAssignmentList bins(num_procs);
  
  // Sort the partitions by size in ascending order
  vector<const PartitionRange*> sorted_partitions;
  for (auto const& range: part_sizes)
    sorted_partitions.push_back(&range);
  sort(sorted_partitions.begin(), sorted_partitions.end(),
       [](const PartitionRange *r1, const PartitionRange *r2) 
       { return (r1->weight() < r2->weight());} );

  // Compute the maximum number of sites per Bin
  double total_weight = 0;
  for (auto const& range: part_sizes)
  {
    total_weight += range.weight();
  }

  double  max_weight = total_weight / double(bins.size());
  unsigned int curr_part = 0; // index in sorted_partitons (AND NOT IN _partitions)
  unsigned int current_bin = 0;

//  vector<size_t> weights(num_procs, 0);
  vector<bool> full(num_procs, false);

  // Assign partitions in a cyclic manner to bins until one is too big
  for (; curr_part < sorted_partitions.size(); ++curr_part)
  {
    const PartitionRange *partition = sorted_partitions[curr_part];
    current_bin = curr_part % bins.size();
    if (partition->weight() + bins[current_bin].weight() > max_weight)
    {
      // the partition exceeds the current bin's size, go to the next step of the algo
      break;
    }
    // add the partition !
    bins[current_bin].assign_sites(partition->part_id, 0, partition->length, partition->weight_ratio);
  }

  stack<PartitionAssignment *> qlow_;
  stack<PartitionAssignment *> *qlow = &qlow_; // hack to assign qhigh to qlow when qlow is empty
  stack<PartitionAssignment *> qhigh;
  for (unsigned int i = 0; i < current_bin; ++i)
  {
    if (!full[current_bin])
    {
      qhigh.push(&bins[i]);
    }
  }
  for (unsigned int i = current_bin; i < bins.size(); ++i)
  {
    if (!full[current_bin])
    {
      qlow->push(&bins[i]);
    }
  }
  unsigned int remaining = sorted_partitions[curr_part]->length;
  while (curr_part < sorted_partitions.size() && (qlow->size() || qhigh.size()))
  {
    const PartitionRange *partition = sorted_partitions[curr_part];
    // try to dequeue a process from Qhigh and to fill it
    if (qhigh.size() && (qhigh.top()->weight() + remaining * partition->weight_ratio >= max_weight))
    {
      PartitionAssignment * bin = qhigh.top();
      qhigh.pop();
      unsigned int toassign = (max_weight - bin->weight()) / partition->weight_ratio;
      bin->assign_sites(partition->part_id, partition->length - remaining, toassign, partition->weight_ratio);
      remaining -= toassign;
    }
    else if ((qlow->top()->weight() + remaining * partition->weight_ratio >= max_weight))
    { // same with qlow
      PartitionAssignment * bin = qlow->top();
      qlow->pop();
      unsigned int toassign = (max_weight - bin->weight()) / partition->weight_ratio;
      bin->assign_sites(partition->part_id, partition->length - remaining, toassign, partition->weight_ratio);
      remaining -= toassign;
    }
    else
    {
      PartitionAssignment * bin = qlow->top();
      qlow->pop();
      bin->assign_sites(partition->part_id, partition->length - remaining, remaining, partition->weight_ratio);
      remaining = 0;
      qhigh.push(bin);
    }

    if (!qlow->size())
    {
      qlow = &qhigh;
    }
    if (!remaining)
    {
      if (++curr_part < sorted_partitions.size())
      {
        remaining = sorted_partitions[curr_part]->length;
      }
    }
  }
  std::cout << bins << std::endl;
  return bins;
}
