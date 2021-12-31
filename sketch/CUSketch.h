#ifndef SKETCHLAB_CPP_CUSKETCH_H
#define SKETCHLAB_CPP_CUSKETCH_H

#include "hash.h"
#include "util.h"
#include <algorithm>
#include <limits>
#include <memory>

namespace SketchLab {

template <typename T, typename hash_t> class CUSketch {

  int32_t depth_;
  int32_t width_;

  hash_t *hash_fns_;

  T **counter_;
  int32_t *indexs_;

public:
  CUSketch(int32_t depth, int32_t width);
  ~CUSketch();

  template <int32_t key_len>
  void update(const FlowKey<key_len> &flowkey, T val);
  template <int32_t key_len> T query(const FlowKey<key_len> &flowkey) const;
  size_t size() const;
  void clear();
};

template <typename T, typename hash_t>
CUSketch<T, hash_t>::CUSketch(int32_t depth, int32_t width)
    : depth_(depth), width_(Util::NextPrime(width)) {

  hash_fns_ = new hash_t[depth_];

  // Allocate continuous memory
  counter_ = new T *[depth_];
  counter_[0] = new T[depth_ * width_](); // Init with zero
  for (int32_t i = 1; i < depth_; ++i) {
    counter_[i] = counter_[i - 1] + width_;
  }
  indexs_ = new int32_t[depth_];
}

template <typename T, typename hash_t> CUSketch<T, hash_t>::~CUSketch() {
  delete[] hash_fns_;

  delete[] counter_[0];
  delete[] counter_;
  delete[] indexs_;
}

template <typename T, typename hash_t>
template <int32_t key_len>
void CUSketch<T, hash_t>::update(const FlowKey<key_len> &flowkey, T val) {
  T min_val = std::numeric_limits<T>::max();
  for (int32_t i = 0; i < depth_; ++i) {
    int32_t idx = hash_fns_[i](flowkey) % width_;
    indexs_[i] = idx;
    min_val = std::min(min_val, counter_[i][idx]);
  }
  min_val += val;
  for (int32_t i = 0; i < depth_; ++i) {
    counter_[i][indexs_[i]] = std::max(min_val, counter_[i][indexs_[i]]);
  }
}

template <typename T, typename hash_t>
template <int32_t key_len>
T CUSketch<T, hash_t>::query(const FlowKey<key_len> &flowkey) const {
  T min_val = std::numeric_limits<T>::max();
  for (int32_t i = 0; i < depth_; ++i) {
    int32_t idx = hash_fns_[i](flowkey) % width_;
    min_val = std::min(min_val, counter_[i][idx]);
  }
  return min_val;
}

template <typename T, typename hash_t>
size_t CUSketch<T, hash_t>::size() const {
  return sizeof(CUSketch<T, hash_t>)    // Instance
         + depth_ * sizeof(hash_t)      // hash_fns
         + depth_ * width_ * sizeof(T); // counter
}

template <typename T, typename hash_t> void CUSketch<T, hash_t>::clear() {
  std::fill(counter_[0], counter_[0] + depth_ * width_, 0);
}

} // namespace SketchLab

#endif // SKETCHLAB_CPP_CUSketch_H
