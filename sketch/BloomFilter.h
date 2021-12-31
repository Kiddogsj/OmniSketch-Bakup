#ifndef SKETCHLAB_CPP_BLOOMFILTER_H
#define SKETCHLAB_CPP_BLOOMFILTER_H

#include "hash.h"
#include "util.h"

#include <algorithm>
#include <cstddef>
#define BYTE(n) ((n) >> 3)
#define BIT(n) ((n)&7)
namespace SketchLab {
template <typename hash_t> class BloomFilter {

private:
  int32_t nbits_;
  int32_t num_hash_;
  int32_t nbytes_;
  uint8_t *arr_;
  hash_t *hash_fns_;


  inline void setBit(int32_t pos) { arr_[BYTE(pos)] |= (1 << BIT(pos)); }
  inline uint8_t getBit(int32_t pos) const {
    return (arr_[BYTE(pos)] >> BIT(pos)) & 1;
  }

public:
  BloomFilter(int32_t nbits, int32_t num_hash);
  ~BloomFilter();
  BloomFilter(const BloomFilter &) = delete;
  BloomFilter(BloomFilter &&) = delete;
  BloomFilter &operator=(BloomFilter) = delete;

  template <int32_t key_len> void insert(const FlowKey<key_len> &flowkey);
  template <int32_t key_len> bool query(const FlowKey<key_len> &flowkey) const;
  std::size_t size() const;
  void clear();
};

template <typename hash_t>
BloomFilter<hash_t>::BloomFilter(int32_t nbits, int32_t num_hash)
    : nbits_(nbits), num_hash_(num_hash) {
  nbits_ = Util::NextPrime(nbits_);
  nbytes_ = (nbits_ & 7) == 0 ? (nbits_ >> 3) : (nbits_ >> 3) + 1;
  hash_fns_ = new hash_t[num_hash_];
  // Allocate memory
  arr_ = new uint8_t[nbytes_]();
}
template <typename hash_t> BloomFilter<hash_t>::~BloomFilter() {
  delete[] hash_fns_;
  delete[] arr_;
}
template <typename hash_t>
template <int32_t key_len>
void BloomFilter<hash_t>::insert(const FlowKey<key_len> &flowkey) {
  for (int32_t i = 0; i < num_hash_; ++i) {
    int32_t idx = hash_fns_[i](flowkey) % nbits_;
    setBit(idx);
  }
}
template <typename hash_t>
template <int32_t key_len>
bool BloomFilter<hash_t>::query(const FlowKey<key_len> &flowkey) const {
  for (int32_t i = 0; i < num_hash_; ++i) {
    int32_t idx = hash_fns_[i](flowkey) % nbits_;
    if (getBit(idx) == 0) {
      return false;
    }
  }
  return true;
}
template <typename hash_t> std::size_t BloomFilter<hash_t>::size() const {
  return sizeof(BloomFilter<hash_t>)   // Instance
         + nbytes_ * sizeof(uint8_t)   // arr_
         + num_hash_ * sizeof(hash_t); // hash_fns
}
template <typename hash_t> void BloomFilter<hash_t>::clear() {
  std::fill(arr_, arr_ + nbytes_, 0);
}
} // namespace SketchLab
#undef BYTE
#undef BIT
#endif