#ifndef SKETCHLAB_CPP_COUNTINGBLOOMFILTER_H
#define SKETCHLAB_CPP_COUNTINGBLOOMFILTER_H

#include "hash.h"
#include "util.h"

#include <algorithm>
#include <cstddef>
namespace SketchLab {
template <typename hash_t> class CountingBloomFilter {

private:
  int32_t nbuckets_; //每个槽4bit
  int32_t num_hash_;
  int32_t nbytes_;
  uint8_t *arr_;
  hash_t *hash_fns_;

  inline void setVal(int32_t idx, uint8_t val);
  inline uint8_t getVal(int32_t idx) const;

public:
  CountingBloomFilter(int32_t nbuckets, int32_t num_hash);
  ~CountingBloomFilter();
  CountingBloomFilter(const CountingBloomFilter &) = delete;
  CountingBloomFilter(CountingBloomFilter &&) = delete;
  CountingBloomFilter &operator=(const CountingBloomFilter) = delete;

  template <int32_t key_len> void insert(const FlowKey<key_len> &flowkey);
  template <int32_t key_len> void remove(const FlowKey<key_len> &flowkey);
  template <int32_t key_len> bool query(const FlowKey<key_len> &flowkey) const;
  std::size_t size() const;
  void clear();
};

template <typename hash_t>
inline void CountingBloomFilter<hash_t>::setVal(int32_t idx, uint8_t val) {
  int32_t k = (idx >> 1);
  if (idx & 1) {
    arr_[k] &= 0xF0;
    arr_[k] |= val;
  } else {
    arr_[k] &= 0xF;
    arr_[k] |= (val << 4);
  }
}

template <typename hash_t>
inline uint8_t CountingBloomFilter<hash_t>::getVal(int32_t idx) const {
  int32_t k = (idx >> 1);
  return ((idx & 1) ? arr_[k] : (arr_[k] >> 4)) & 0xF;
}

template <typename hash_t>
CountingBloomFilter<hash_t>::CountingBloomFilter(int32_t nbuckets, int32_t num_hash)
    : nbuckets_(nbuckets), num_hash_(num_hash) {
  nbuckets_ = Util::NextPrime(nbuckets_);
  nbytes_ = (nbuckets_ + 1) >> 1; //大于2的质数一定是奇数
  hash_fns_ = new hash_t[num_hash_];
  // Allocate memory
  arr_ = new uint8_t[nbytes_]();
}

template <typename hash_t> CountingBloomFilter<hash_t>::~CountingBloomFilter() {
  delete[] hash_fns_;
  delete[] arr_;
}

template <typename hash_t>
template <int32_t key_len>
void CountingBloomFilter<hash_t>::insert(const FlowKey<key_len> &flowkey) {
  for (int32_t i = 0; i < num_hash_; ++i) {
    int32_t idx = hash_fns_[i](flowkey) % nbuckets_;
    uint8_t val = getVal(idx);
    if (val < 0xF) { //如果没满，就加1
      ++val;
    }
    setVal(idx, val);
  }
}

template <typename hash_t>
template <int32_t key_len>
void CountingBloomFilter<hash_t>::remove(const FlowKey<key_len> &flowkey) {
  for (int32_t i = 0; i < num_hash_; ++i) {
    int32_t idx = hash_fns_[i](flowkey) % nbuckets_;
    uint8_t val = getVal(idx);
    if (val > 0) {
      --val;
    }
    setVal(idx, val);
  }
}

template <typename hash_t>
template <int32_t key_len>
bool CountingBloomFilter<hash_t>::query(const FlowKey<key_len> &flowkey) const {
  for (int32_t i = 0; i < num_hash_; ++i) {
    int32_t idx = hash_fns_[i](flowkey) % nbuckets_;
    uint8_t val = getVal(idx);
    if (!val) {
      return false;
    }
  }
  return true;
}

template <typename hash_t>
std::size_t CountingBloomFilter<hash_t>::size() const {
  return sizeof(CountingBloomFilter<hash_t>) // Instance
         + nbytes_ * sizeof(uint8_t)         // arr_
         + num_hash_ * sizeof(hash_t);       // hash_fns
}

template <typename hash_t> void CountingBloomFilter<hash_t>::clear() {
  std::fill(arr_, arr_ + nbytes_, 0);
}
} // namespace SketchLab
#endif