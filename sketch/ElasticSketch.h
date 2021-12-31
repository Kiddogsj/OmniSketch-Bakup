#ifndef SKETCHLAB_CPP_ELASTICSKETCH_H
#define SKETCHLAB_CPP_ELASTICSKETCH_H

#include "CMSketch.h"
#include "hash.h"
#include "util.h"
#include <algorithm>
#include <memory>

#define JUDGE_IF_SWAP(min_val, guard_val)                                      \
  ((guard_val) > ((min_val) << 3)) // delta = 8

namespace SketchLab {

template <typename T, typename U, typename hash_t, int32_t key_len>
class ElasticSketch {
private:
  struct Entry {
    FlowKey<key_len> flowkey_;
    T val_;
    bool flag_;
    Entry() : flowkey_(), val_(0), flag_(false) {}
    bool isEmpty() {
      FlowKey<key_len> emptyKey{};
      return flowkey_ == emptyKey;
    }
  };
  // heavy part
  Entry **buckets_;
  int32_t num_buckets_;
  int32_t num_per_bucket_; // each bucket has num_per_bucket_ entries

  hash_t hash_h_;
  // light part
  CMSketch<U, hash_t> cm_;

public:
  ElasticSketch(int32_t num_buckets, int32_t num_per_bucket, int32_t l_depth,
                int32_t l_width);
  ~ElasticSketch();

  int heavypartInsert(const FlowKey<key_len> &flowkey, T val,
                      FlowKey<key_len> &swap_key, T &swap_val);

  void lightpartInsert(const FlowKey<key_len> &flowkey, T val);
  void update(const FlowKey<key_len> &flowkey, T val);
  T heavypartQuery(const FlowKey<key_len> &flowkey, bool &flag) const;
  T lightpartQuery(const FlowKey<key_len> &flowkey) const;
  T query(const FlowKey<key_len> &flowkey) const;
  size_t size() const;
};
template <typename T, typename U, typename hash_t, int32_t key_len>
ElasticSketch<T, U, hash_t, key_len>::ElasticSketch(int32_t num_buckets,
                                                    int32_t num_per_bucket,
                                                    int32_t l_depth,
                                                    int32_t l_width)
    : num_buckets_(Util::NextPrime(num_buckets)),
      num_per_bucket_(num_per_bucket), cm_(l_depth, l_width) {
  buckets_ = new Entry *[num_buckets_];
  buckets_[0] = new Entry[num_buckets_ * num_per_bucket_]();
  for (int i = 1; i < num_buckets_; ++i) {
    buckets_[i] = buckets_[i - 1] + num_per_bucket_;
  }
}

template <typename T, typename U, typename hash_t, int32_t key_len>
ElasticSketch<T, U, hash_t, key_len>::~ElasticSketch() {
  delete[] buckets_[0];
  delete[] buckets_;
}

template <typename T, typename U, typename hash_t, int32_t key_len>
int ElasticSketch<T, U, hash_t, key_len>::heavypartInsert(
    const FlowKey<key_len> &flowkey, T val, FlowKey<key_len> &swap_key,
    T &swap_val) {

  int32_t index = hash_h_(flowkey) % num_buckets_;
  int32_t matched = -1;
  int32_t empty = -1;
  int32_t min_counter = 0;
  T min_counter_val = buckets_[index][0].val_;
  for (int32_t i = 0; i < num_per_bucket_ - 1; ++i) {
    if (buckets_[index][i].flowkey_ == flowkey) { // flowkey is in the bucket
      matched = i;
      buckets_[index][i].val_ += val;
      return 0;
    }
    if (buckets_[index][i].isEmpty()) {
      buckets_[index][i].flowkey_ = flowkey;
      buckets_[index][i].val_ = val;
      return 0;
    }
    if (min_counter_val > buckets_[index][i].val_) {
      min_counter = i;
      min_counter_val = buckets_[index][i].val_;
    }
  }
  /* update guard val and comparison */
  T guard_val = buckets_[index][num_per_bucket_ - 1].val_;
  guard_val += 1;

  if (!JUDGE_IF_SWAP(min_counter_val, guard_val)) {
    buckets_[index][num_per_bucket_ - 1].val_ = guard_val;
    return 2;
  } else {
    swap_key = buckets_[index][min_counter].flowkey_;
    swap_val = buckets_[index][min_counter].val_;
    buckets_[index][num_per_bucket_ - 1].val_ = 0;
    buckets_[index][min_counter].flowkey_ = flowkey;
    buckets_[index][min_counter].val_ = val;
    buckets_[index][min_counter].flag_ = true;
    return 1;
  }
}

template <typename T, typename U, typename hash_t, int32_t key_len>
void ElasticSketch<T, U, hash_t, key_len>::lightpartInsert(
    const FlowKey<key_len> &flowkey, T val) {
  int64_t oldval = cm_.query(flowkey);
  int64_t newval = oldval + val;
  if (newval <= std::numeric_limits<U>::max()) { // assure not overflow
    cm_.update(flowkey, val);
  }
}

template <typename T, typename U, typename hash_t, int32_t key_len>
void ElasticSketch<T, U, hash_t, key_len>::update(
    const FlowKey<key_len> &flowkey, T val) {

  FlowKey<key_len> swap_key;
  T swap_val;
  int result = heavypartInsert(flowkey, val, swap_key, swap_val);
  switch (result) {
  case 0:
    return;
  case 1: {
    lightpartInsert(swap_key, swap_val);
    return;
  }
  case 2: {
    lightpartInsert(flowkey, val);
    return;
  }
  default:
    printf("error return value !\n");
    exit(1);
  }
}

template <typename T, typename U, typename hash_t, int32_t key_len>
T ElasticSketch<T, U, hash_t, key_len>::heavypartQuery(
    const FlowKey<key_len> &flowkey, bool &flag) const {
  int index = hash_h_(flowkey) % num_buckets_;
  for (int i = 0; i < num_per_bucket_ - 1; ++i) {
    if (buckets_[index][i].flowkey_ == flowkey) {
      flag = buckets_[index][i].flag_;
      return buckets_[index][i].val_;
    }
  }
  return 0;
}

template <typename T, typename U, typename hash_t, int32_t key_len>
T ElasticSketch<T, U, hash_t, key_len>::lightpartQuery(
    const FlowKey<key_len> &flowkey) const {
  return cm_.query(flowkey);
}

template <typename T, typename U, typename hash_t, int32_t key_len>
T ElasticSketch<T, U, hash_t, key_len>::query(
    const FlowKey<key_len> &flowkey) const {
  bool flag = false;
  T heavy_result = heavypartQuery(flowkey, flag);
  T light_result = 0;
  if (heavy_result == 0 || flag == true) {
    light_result = lightpartQuery(flowkey);
  }
  return heavy_result + light_result;
}

template <typename T, typename U, typename hash_t, int32_t key_len>
size_t ElasticSketch<T, U, hash_t, key_len>::size() const {
  size_t es = sizeof(ElasticSketch<T, U, hash_t, key_len>) + sizeof(hash_h_);
  size_t bucket_size = key_len + 1 + sizeof(U);
  size_t vk = bucket_size * num_buckets_ * num_per_bucket_;
  size_t ct = cm_.size();
  return es + vk + ct;
}

} // namespace SketchLab

#undef JUDGE_IF_SWAP
#endif
