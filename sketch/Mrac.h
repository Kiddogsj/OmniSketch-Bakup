#ifndef SKETCHLAB_CPP_MRAC_H
#define SKETCHLAB_CPP_MRAC_H

#include "hash.h"
#include "util.h"
#include <algorithm>
#include <map>
namespace SketchLab {
template <typename T, typename hash_t> class Mrac {
private:
  int32_t width_;
  T *arr_;
  hash_t *hash_fns_;
  int64_t sum_;

public:
  Mrac(int32_t width);
  ~Mrac();

  template <int32_t key_len> void update(const FlowKey<key_len> &flowkey);
  std::map<T, double> estimateDistribution();
  size_t size() const;
  void clear();
};

template <typename T, typename hash_t>
Mrac<T, hash_t>::Mrac(int32_t width) : width_(Util::NextPrime(width)), sum_(0) {
  arr_ = new T[width_]();
  hash_fns_ = new hash_t;
}

template <typename T, typename hash_t> Mrac<T, hash_t>::~Mrac() {
  delete[] arr_;
  delete hash_fns_;
}

template <typename T, typename hash_t> size_t Mrac<T, hash_t>::size() const {
  return sizeof(Mrac<T, hash_t>) + sizeof(hash_t) + sizeof(T) * (width_);
}

template <typename T, typename hash_t>
template <int32_t key_len>
void Mrac<T, hash_t>::update(const FlowKey<key_len> &flowkey) {
  sum_ += 1;
  int32_t idx = (*hash_fns_)(flowkey) % width_;
  arr_[idx] += 1;
}

template <typename T, typename hash_t>
std::map<T, double> Mrac<T, hash_t>::estimateDistribution() {
  std::map<T, double> distri;
  for (int i = 0; i < width_; ++i) {
    auto iter = distri.find(arr_[i]);
    if (iter == distri.end()) {
      distri.emplace(arr_[i], 1);
    } else {
      iter->second += 1;
    }
  }
  for (auto &kv : distri) {
    kv.second /= sum_;
  }
  return distri;
}

template <typename T, typename hash_t> void Mrac<T, hash_t>::clear() {
  sum_ = 0;
  std::fill(arr_, arr_ + width_, 0);
}
} // namespace SketchLab
#endif