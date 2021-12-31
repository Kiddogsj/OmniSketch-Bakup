#ifndef SKETCHLAB_CPP_FLOWRADAR_H
#define SKETCHLAB_CPP_FLOWRADAR_H

#include "BloomFilter.h"
#include "FlowKey.h"
#include "hash.h"
#include "util.h"
#include <algorithm>
#include <map>
#include <memory>
namespace SketchLab {
template <typename T, typename hash_t, int32_t key_len> class FlowRadar {
  int32_t n_arr_;
  int32_t nhash_arr_;
  hash_t *hash_fns_;

  int32_t n_flows_;
  T *flow_arr_;
  T *size_arr_;
  FlowKey<key_len> *keys_;
  BloomFilter<hash_t> bf_;

public:
  FlowRadar(int32_t bf_nbits, int32_t bf_nhash, int32_t n_arr,
            int32_t nhash_arr);
  ~FlowRadar();

  void update(const FlowKey<key_len> &flowkey, T size);
  std::map<FlowKey<key_len>, T> decode();
  void clear();
  size_t size() const;
};

template <typename T, typename hash_t, int32_t key_len>
FlowRadar<T, hash_t, key_len>::FlowRadar(int32_t bf_nbits, int32_t bf_nhash,
                                         int32_t n_arr, int32_t nhash_arr)
    : bf_(bf_nbits, bf_nhash), n_arr_(Util::NextPrime(n_arr)),
      nhash_arr_(nhash_arr) {
  n_flows_ = 0;
  hash_fns_ = new hash_t[nhash_arr_];
  // init flow counters and size counters
  flow_arr_ = new T[n_arr_]();
  size_arr_ = new T[n_arr_]();
  // init XOR encoders
  keys_ = new FlowKey<key_len>[n_arr_]();
}

template <typename T, typename hash_t, int32_t key_len>
FlowRadar<T, hash_t, key_len>::~FlowRadar() {
  delete[] hash_fns_;
  delete[] flow_arr_;
  delete[] size_arr_;
  delete[] keys_;
}

template <typename T, typename hash_t, int32_t key_len>
void FlowRadar<T, hash_t, key_len>::update(const FlowKey<key_len> &flowkey,
                                           T size) {
  bool exist = bf_.query(flowkey);
  if (!exist) {
    bf_.insert(flowkey);
    ++n_flows_;
  }

  for (int32_t i = 0; i < nhash_arr_; i++) {
    int32_t index = hash_fns_[i](flowkey) % n_arr_;
    if (!exist) {
      ++flow_arr_[index];
      keys_[index] ^= flowkey;
    }
    size_arr_[index] += size;
  }
}

template <typename T, typename hash_t, int32_t key_len>
std::map<FlowKey<key_len>, T> FlowRadar<T, hash_t, key_len>::decode() {
  // int stop = 0;
  bool stop = false;
  int ret = 0;
  std::map<FlowKey<key_len>, T> ans;
  while (!stop) {
    int32_t index = 0;
    for (index = 0; index < n_arr_; index++) {
      if (flow_arr_[index] == 1) {
        break;
      }
    }

    if (index == n_arr_) {
      stop = true;
      break;
    } else {
      FlowKey<key_len> flowkey = keys_[index];
      T size = size_arr_[index];
      for (int i = 0; i < nhash_arr_; ++i) {
        int l = hash_fns_[i](flowkey) % n_arr_;
        flow_arr_[l]--;
        if (size_arr_[l] >= size) {
          size_arr_[l] -= size;
        } else {
          size = size_arr_[l];
          size_arr_[l] = 0;
        }
        keys_[l] ^= flowkey;
      }
      ans.emplace(flowkey, size);
    }
  }
  return ans;
}

template <typename T, typename hash_t, int32_t key_len>
void FlowRadar<T, hash_t, key_len>::clear() {
  n_flows_ = 0;
  bf_.clear();
  std::fill_n(flow_arr_, n_arr_, 0);
  std::fill_n(size_arr_, n_arr_, 0);
  FlowKey<key_len> emptykey{};
  for (int i = 0; i < n_arr_; ++i) {
    keys_[i] = emptykey;
  }
}

template <typename T, typename hash_t, int32_t key_len>
size_t FlowRadar<T, hash_t, key_len>::size() const {
  size_t admin = sizeof(FlowRadar<T, hash_t, key_len>);
  size_t hash = nhash_arr_ * sizeof(hash_t);
  size_t cnt = 2 * sizeof(T) * n_arr_;
  size_t encode = key_len * n_arr_;
  size_t bf = bf_.size();
  return admin + hash + bf + cnt + encode;
}

} // namespace SketchLab

#endif
