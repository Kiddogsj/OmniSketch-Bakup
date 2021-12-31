#ifndef SKETCHLAB_CPP_COUNTSKETCH_H
#define SKETCHLAB_CPP_COUNTSKETCH_H

#include <algorithm>
#include <memory>

#include "hash.h"
#include "util.h"

namespace SketchLab {

template <typename T, typename hash_t> class CountSketch {
private:
  int depth_;
  int width_;

  hash_t *hash_fns_;

  T **arr_;
  T *values_;

public:
  CountSketch(int depth, int width);
  ~CountSketch();
  CountSketch(const CountSketch &) = delete;
  CountSketch(CountSketch &&) = delete;
  CountSketch &operator=(CountSketch) = delete;

  template <int32_t key_len>
  void update(const FlowKey<key_len> &flowkey, T val);
  template <int32_t key_len> T query(const FlowKey<key_len> &flowkey) const;
  std::size_t size() const;
  void clear();
};

template <typename T, typename hash_t>
CountSketch<T, hash_t>::CountSketch(int depth, int width)
    : depth_(depth), width_(Util::NextPrime(width)) {

  // hash funcs
  hash_fns_ = new hash_t[depth_ << 1];

  // Allocate continuous memory
  arr_ = new T *[depth_];
  arr_[0] = new T[depth_ * width_];
  for (int i = 1; i < depth_; ++i) {
    arr_[i] = arr_[i - 1] + width_;
  }
  values_ = new T[depth_];
}

template <typename T, typename hash_t> CountSketch<T, hash_t>::~CountSketch() {
  delete[] hash_fns_;
  delete[] arr_[0];
  delete[] arr_;
  delete[] values_;
}

template <typename T, typename hash_t>
template <int32_t key_len>
void CountSketch<T, hash_t>::update(const FlowKey<key_len> &flowkey, T val) {
  for (int i = 0; i < depth_; ++i) {
    int idx = hash_fns_[i](flowkey) % width_;
    arr_[i][idx] +=
        val * (static_cast<int>(hash_fns_[depth_ + i](flowkey) & 1) * 2 - 1);
  }
}

template <typename T, typename hash_t>
template <int32_t key_len>
T CountSketch<T, hash_t>::query(const FlowKey<key_len> &flowkey) const {
  for (int i = 0; i < depth_; ++i) {
    int idx = hash_fns_[i](flowkey) % width_;
    values_[i] = arr_[i][idx] *
                 (static_cast<int>(hash_fns_[depth_ + i](flowkey) & 1) * 2 - 1);
  }
  std::sort(values_, values_ + depth_);
  if (!(depth_ & 1)) { //偶数
    return std::abs((values_[depth_ / 2 - 1] + values_[depth_ / 2]) / 2);
  } else { //奇数
    return std::abs(values_[depth_ / 2]);
  }
}

template <typename T, typename hash_t>
std::size_t CountSketch<T, hash_t>::size() const {
  return sizeof(CountSketch<T, hash_t>) + (depth_ << 1) * sizeof(hash_t) +
         depth_ * width_ * sizeof(T);
}

template <typename T, typename hash_t> void CountSketch<T, hash_t>::clear() {
  std::fill(arr_[0], arr_[0] + depth_ * width_, 0);
}
} // namespace SketchLab
#endif