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



PartitionAssignmentList vector_assignment(const std::vector<std::vector<unsigned int> > &M)
{
  PartitionAssignmentList bins(M.size());
  for (unsigned int p = 0; p < M[0].size(); ++p) {
    unsigned int partition_offset = 0;
    for (unsigned int c = 0; c < M.size(); ++c) {
      if (M[c][p]) {
        bins[c].assign_sites(p, partition_offset, M[c][p]);
        partition_offset += M[c][p];
      }
    }
  }
  return bins;
}


// from modeo param 
PartitionAssignmentList hardcoded_assignment()
{ 
  unsigned int cores = 16;
  unsigned int partitions = 11;
  std::vector<std::vector<unsigned int> >M(cores,
      std::vector<unsigned int>(partitions, 0));
  M[0][7] = 353;
  M[0][8] = 99;
  M[1][1] = 362;
  M[1][3] = 109;
  M[2][1] = 111;
  M[2][4] = 339;
  M[3][1] = 74;
  M[3][9] = 431;
  M[4][0] = 466;
  M[5][0] = 466;
  M[6][0] = 465;
  M[7][0] = 53;
  M[7][2] = 408;
  M[8][2] = 469;
  M[9][2] = 414;
  M[9][10] = 47;
  M[10][10] = 467;
  M[11][6] = 61;
  M[11][10] = 404;
  M[12][6] = 488;
  M[13][5] = 256;
  M[13][6] = 176;
  M[14][5] = 462;
  M[15][1] = 113;
  M[15][7] = 347;
  return vector_assignment(M);
}

// from spr round
PartitionAssignmentList hardcoded_assignment_2()
{ 
  unsigned int cores = 16;
  unsigned int partitions = 11;
  std::vector<std::vector<unsigned int> >M(cores,
      std::vector<unsigned int>(partitions, 0));
 M[0][7] = 368;
 M[0][8] = 99;
 M[1][1] = 366;
 M[1][3] = 109;
 M[2][1] = 107;
 M[2][4] = 339;
 M[3][1] = 50;
 M[3][9] = 431;
 M[4][0] = 466;
 M[5][0] = 465;
 M[6][0] = 465;
 M[7][0] = 54;
 M[7][2] = 409;
 M[8][2] = 468;
 M[9][2] = 414;
 M[9][10] = 47;
 M[10][10] = 468;
 M[11][6] = 60;
 M[11][10] = 403;
 M[12][6] = 482;
 M[13][5] = 263;
 M[13][6] = 183;
 M[14][5] = 455;
 M[15][1] = 137;
 M[15][7] = 332;

  return vector_assignment(M);
}
// from spr round and swaps
PartitionAssignmentList hardcoded_assignment_3()
{ 
  unsigned int cores = 16;
  unsigned int partitions = 11;
  std::vector<std::vector<unsigned int> >M(cores,
      std::vector<unsigned int>(partitions, 0));
 M[0][4] = 339;
 M[0][8] = 99;
 M[1][1] = 510;
 M[2][1] = 102;
 M[2][7] = 358;
 M[3][1] = 48;
 M[3][2] = 407;
 M[4][0] = 495;
 M[5][0] = 494;
 M[6][10] = 450;
 M[7][0] = 50;
 M[7][2] = 405;
 M[8][2] = 479;
 M[9][6] = 478;
 M[10][10] = 468;
 M[11][0] = 411;
 M[11][6] = 50;
 M[12][9] = 431;
 M[13][5] = 243;
 M[13][6] = 197;
 M[14][5] = 475;
 M[15][3] = 109;
 M[15][7] = 342;
  return vector_assignment(M);
}

// from 1 partition 1 core mesasurments
PartitionAssignmentList hardcoded_assignment_4()
{ 
  unsigned int cores = 16;
  unsigned int partitions = 11;
  std::vector<std::vector<unsigned int> >M(cores,
      std::vector<unsigned int>(partitions, 0));
 M[0][7] = 250;
 M[0][8] = 99;
 M[1][0] = 246;
 M[1][3] = 109;
 M[2][1] = 312;
 M[2][6] = 129;
 M[3][1] = 12;
 M[3][9] = 431;
 M[4][0] = 602;
 M[5][0] = 602;
 M[6][4] = 339;
 M[7][2] = 571;
 M[8][10] = 570;
 M[9][2] = 571;
 M[10][10] = 348;
 M[11][1] = 277;
 M[11][2] = 149;
 M[12][6] = 534;
 M[13][5] = 356;
 M[13][6] = 62;
 M[14][5] = 362;
 M[14][7] = 60;
 M[15][1] = 59;
 M[15][7] = 390;
  return vector_assignment(M);
}


PartitionAssignmentList KassianLoadBalancer::compute_assignments(const PartitionAssignment& part_sizes,
    size_t num_procs)
{
 // std::cout << hardcoded_assignment_4() << std::endl;
 // return hardcoded_assignment_4() ;
  
  //const double hackweights[] = {2.73433, 2.7473, 2.77429, 2.74025, 2.82602, 2.91913, 2.77682, 2.78898, 2.8393, 2.63217, 2.76065}; 
  //const double hackweights[] = {7.47656, 7.54767, 7.69666, 7.50896, 7.98638, 8.52133, 7.71074, 7.7784, 8.06165, 6.92832, 7.62119};
  /*const double hackweights[] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};


  PartitionAssignment &hack = (PartitionAssignment &)part_sizes;
  for (struct PartitionRange & range: hack) {
    range.weight_ratio = hackweights[range.part_id]; 
  }*/
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
    std::cout << "range weight : " << range.weight() << std::endl;
    total_weight += range.weight();
  }

  double  max_weight = total_weight / double(bins.size());
  std::cout << "total weight : " << total_weight << std::endl;
  std::cout << "max weight : " << max_weight << std::endl;
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

PartitionAssignmentList FakeLoadBalancer::compute_assignments(const PartitionAssignment& part_sizes,
    size_t num_procs)
{
  //std::cout << hardcoded_assignment_4() << std::endl;
  //return hardcoded_assignment_4() ;
  
  //const double hackweights[] = {2.73433, 2.7473, 2.77429, 2.74025, 2.82602, 2.91913, 2.77682, 2.78898, 2.8393, 2.63217, 2.76065}; 
  //const double hackweights[] = {7.47656, 7.54767, 7.69666, 7.50896, 7.98638, 8.52133, 7.71074, 7.7784, 8.06165, 6.92832, 7.62119};
  const double hackweights[] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

  num_procs -= 10;
  PartitionAssignment secondbins[4];
  unsigned assigned = 0;

  PartitionAssignment &hack = (PartitionAssignment &)part_sizes;
  for (struct PartitionRange & range: hack) {
    range.weight_ratio = hackweights[range.part_id]; 
  }
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
    bins[current_bin].assign_sites(partition->part_id, 0, partition->length - 5, partition->weight_ratio);
    unsigned int index = assigned ? 1 : 0;
    index = assigned > 2 ? 2 : index;
    index = assigned > 5 ? 3 : index;
    secondbins[index].assign_sites(partition->part_id, partition->length - 5, 5, partition->weight_ratio);
    assigned++;
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
  bins.push_back(secondbins[0]);
  bins.push_back(secondbins[1]);
  bins.push_back(secondbins[2]);
  bins.push_back(secondbins[3]);
  std::cerr<< "PIF " << std::endl;
  bins.push_back(PartitionAssignment());
  bins.push_back(PartitionAssignment());
  bins.push_back(PartitionAssignment());
  bins.push_back(PartitionAssignment());
  bins.push_back(PartitionAssignment());
  bins.push_back(PartitionAssignment());
  std::cerr<< "PAF " << std::endl;
  std::cout << bins << std::endl;
  return bins;
}

PartitionAssignmentList CyclicLoadBalancer::compute_assignments(const PartitionAssignment& part_sizes,
    size_t num_procs)
{
  PartitionAssignmentList bins(num_procs);
  unsigned int sites_limit = 900;
  unsigned int current_bin = 0;
  for (auto const& range: part_sizes) {
    if (range.length < sites_limit) {
      bins[(current_bin++) % bins.size()].assign_sites(range.part_id, 0, range.length);
    } else {
      bins[(current_bin++) % bins.size()].assign_sites(range.part_id, 0, range.length / 2);
      bins[(current_bin++) % bins.size()].assign_sites(range.part_id, range.length / 2, range.length - range.length / 2);

    }
  }
  std::cout << bins << std::endl;
  return bins;
}
