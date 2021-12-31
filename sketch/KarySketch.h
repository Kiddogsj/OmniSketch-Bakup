#ifndef SKETCHLAB_CPP_KARYSKETCH_H
#define SKETCHLAB_CPP_KARYSKETCH_H

#include <algorithm>

#include "hash.h"
#include "util.h"
namespace SketchLab {
template <typename T, typename hash_t> class KarySketch {
private:
  int32_t depth_;
  int32_t width_;
  hash_t *hash_fns_;
  T **arr_;
  int32_t sum_;
  T *values_;

public:
  KarySketch(int32_t depth, int32_t width);
  ~KarySketch();
  KarySketch(const KarySketch &) = delete;
  KarySketch(KarySketch &&) = delete;
  KarySketch &operator=(KarySketch) = delete;

  template <int32_t key_len>
  void update(const FlowKey<key_len> &flowkey, T val);
  template <int32_t key_len> T query(const FlowKey<key_len> &flowkey) const;
  size_t size() const;
  void clear();
};

template <typename T, typename hash_t>
KarySketch<T, hash_t>::KarySketch(int32_t depth, int32_t width)
    : depth_(depth), width_(Util::NextPrime(width)), sum_(0) {

  hash_fns_ = new hash_t[depth_];
  // Allocate continuous memory
  arr_ = new T *[depth_];
  arr_[0] = new T[depth_ * width_](); // Init with zero
  for (int32_t i = 1; i < depth_; ++i) {
    arr_[i] = arr_[i - 1] + width_;
  }
  values_ = new T[depth_];
}

template <typename T, typename hash_t> KarySketch<T, hash_t>::~KarySketch() {
  delete[] hash_fns_;

  delete[] arr_[0];
  delete[] arr_;
  delete[] values_;
}

template <typename T, typename hash_t>
template <int32_t key_len>
void KarySketch<T, hash_t>::update(const FlowKey<key_len> &flowkey, T val) {
  sum_ += val;
  for (int32_t i = 0; i < depth_; ++i) {
    int32_t index = hash_fns_[i](flowkey) % width_;
    arr_[i][index] += val;
  }
}

template <typename T, typename hash_t>
template <int32_t key_len>
T KarySketch<T, hash_t>::query(const FlowKey<key_len> &flowkey) const {
  for (int32_t i = 0; i < depth_; ++i) {
    int32_t idx = hash_fns_[i](flowkey) % width_;
    values_[i] = (arr_[i][idx] - 1. * sum_ / width_) / (1. - 1. / width_);
  }
  std::sort(values_, values_ + depth_);
  if (depth_ & 1) {
    return std::abs(values_[depth_ / 2]);
  } else {
    return std::abs(values_[depth_ / 2 - 1] + values_[depth_ / 2]) / 2;
  }
}

template <typename T, typename hash_t>
size_t KarySketch<T, hash_t>::size() const {
  return sizeof(KarySketch<T, hash_t>)  // Instance
         + depth_ * sizeof(hash_t)      // hash_fns
         + depth_ * width_ * sizeof(T); // counter
}

template <typename T, typename hash_t> void KarySketch<T, hash_t>::clear() {
  std::fill(arr_[0], arr_[0] + depth_ * width_, 0);
  sum_ = 0;
}
} // namespace SketchLab

#endif