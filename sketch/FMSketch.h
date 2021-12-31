#ifndef SKETCHLAB_CPP_FMSKETCH_H
#define SKETCHLAB_CPP_FMSKETCH_H
#include "hash.h"
#include "util.h"
#include <cmath>
namespace SketchLab {
template <typename hash_t> class FMSketch {
private:
  uint64_t *arr_;
  int32_t depth_;
  hash_t *hash_fns_;
  int32_t zeroes(uint64_t num) const;
  int32_t ones(uint64_t num) const;

public:
  FMSketch(int32_t depth);
  ~FMSketch();
  template <int32_t key_len> void update(const FlowKey<key_len> &flowkey);
  int64_t query() const;
  std::size_t size() const;
  void clear();
};
template <typename hash_t>
FMSketch<hash_t>::FMSketch(int32_t depth) : depth_(depth) {
  arr_ = new uint64_t[depth_]();
  hash_fns_ = new hash_t[depth_];
}

template <typename hash_t> FMSketch<hash_t>::~FMSketch() {
  delete[] arr_;
  delete[] hash_fns_;
}

template <typename hash_t>
int32_t FMSketch<hash_t>::zeroes(uint64_t num) const {
  int32_t ret = 0;
  while (num) {
    if (!(num & 1)) {
      ++ret;
      num >>= 1;
    } else {
      return ret;
    }
  }
  return ret;
}

template <typename hash_t> int32_t FMSketch<hash_t>::ones(uint64_t num) const {
  int32_t ret = 0;
  while (num) {
    if (num & 1) {
      ++ret;
      num >>= 1;
    } else {
      return ret;
    }
  }
  return ret;
}

template <typename hash_t>
template <int32_t key_len>
void FMSketch<hash_t>::update(const FlowKey<key_len> &flowkey) {
  for (int32_t i = 0; i < depth_; ++i) {
    int32_t idx = zeroes(hash_fns_[i](flowkey));
    // int32_t idx = ones(hash_fns_[i](flowkey));
    if (idx > 0) {
      arr_[i] |= (1ULL << (idx - 1));
    }
  }
}

template <typename hash_t> int64_t FMSketch<hash_t>::query() const {
  // int32_t L = 0;
  // for (int32_t i = 0; i < depth_; ++i) {
  //   L += ones(arr_[i]);
  //   std::cout << ones(arr_[i]) << " ";
  // }
  // 选中位数
  int32_t *values = new int32_t[depth_];
  for (int32_t i = 0; i < depth_; ++i) {
    values[i] = ones(arr_[i]);
    std::cout << ones(arr_[i]) << " ";
  }
  std::cout << std::endl;
  std::sort(values, values + depth_);
  double p;
  if (!(depth_ & 1)) { //偶数
    p = std::abs((values[depth_ / 2 - 1] + values[depth_ / 2]) / 2);
  } else { //奇数
    p = std::abs(values[depth_ / 2]);
  }
  delete[] values;
  // double p = static_cast<double>(L) / depth_;
  int64_t cardinality = static_cast<int64_t>(1.2928 * pow(2.0, p));
  return cardinality;
}
template <typename hash_t> std::size_t FMSketch<hash_t>::size() const {
  return sizeof(FMSketch<hash_t>) +
         depth_ * (sizeof(uint64_t) + sizeof(hash_t));
}
template <typename hash_t> void FMSketch<hash_t>::clear() {
  std::fill(arr_, arr_ + depth_, 0);
}
} // namespace SketchLab

#endif