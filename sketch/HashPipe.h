#ifndef SKETCHLAB_CPP_HASHPIPE_H
#define SKETCHLAB_CPP_HASHPIPE_H

#include "hash.h"
#include "util.h"
#include <cstdint>
#include <map>
#include <set>
namespace SketchLab {
template <typename T, typename hash_t, int32_t key_len> class HashPipe {
private:
  class Entry {
  public:
    FlowKey<key_len> flowkey_;
    T val_;
  };
  int32_t depth_;
  int32_t width_;

  hash_t *hash_fns_;
  Entry **arr_;

public:
  HashPipe(int depth, int width);
  ~HashPipe();
  void update(const FlowKey<key_len> &flowkey, T val);
  T query(const FlowKey<key_len> &flowkey) const;
  std::map<FlowKey<key_len>, T> getHeavyHitters(const T val_threshold) const;
  std::size_t size() const;
  void clear();
};
template <typename T, typename hash_t, int32_t key_len>
HashPipe<T, hash_t, key_len>::HashPipe(int depth, int width)
    : depth_(depth), width_(width) {
  width_ = Util::NextPrime(width_);
  // hash functions
  hash_fns_ = new hash_t[depth_];
  // allocate memorys
  arr_ = new Entry *[depth_];
  arr_[0] = new Entry[depth_ * width_]();
  for (int i = 1; i < depth_; ++i) {
    arr_[i] = arr_[i - 1] + width_;
  }
}

template <typename T, typename hash_t, int32_t key_len>
HashPipe<T, hash_t, key_len>::~HashPipe() {
  delete[] hash_fns_;
  delete[] arr_[0];
  delete[] arr_;
}

template <typename T, typename hash_t, int32_t key_len>
void HashPipe<T, hash_t, key_len>::update(const FlowKey<key_len> &flowkey,
                                          T val) {
  // first pipe
  int idx = hash_fns_[0](flowkey) % width_;
  FlowKey<key_len> emptykey;
  FlowKey<key_len> c_key;
  T c_val;
  if (arr_[0][idx].flowkey_ == flowkey) {
    // flowkey hit
    arr_[0][idx].val_ += val;
    return;
  } else if (arr_[0][idx].flowkey_ == emptykey) {
    // empty
    arr_[0][idx].val_ = val;
    arr_[0][idx].flowkey_ = flowkey;
    return;
  } else {
    // swap
    c_key = arr_[0][idx].flowkey_;
    c_val = arr_[0][idx].val_;
    arr_[0][idx].flowkey_ = flowkey;
    arr_[0][idx].val_ = val;
  }
  // second pipe
  for (int i = 1; i < depth_; ++i) {
    idx = hash_fns_[i](c_key) % width_;
    if (arr_[i][idx].flowkey_ == c_key) {
      arr_[i][idx].val_ += c_val;
      return;
    } else if (arr_[i][idx].flowkey_ == emptykey) {
      arr_[i][idx].flowkey_ = c_key;
      arr_[i][idx].val_ = c_val;
      return;
    } else if (arr_[i][idx].val_ < c_val) {
      auto tmpkey = c_key;
      c_key = arr_[i][idx].flowkey_;
      arr_[i][idx].flowkey_ = tmpkey;
      std::swap(c_val, arr_[i][idx].val_);
    }
  }
}
template <typename T, typename hash_t, int32_t key_len>
T HashPipe<T, hash_t, key_len>::query(const FlowKey<key_len> &flowkey) const {
  int ret = 0;
  for (int i = 0; i < depth_; ++i) {
    int idx = hash_fns_[i](flowkey) % width_;
    if (arr_[i][idx].flowkey_ == flowkey) {
      ret += arr_[i][idx].val_;
    }
  }
  return ret;
}

template <typename T, typename hash_t, int32_t key_len>
std::map<FlowKey<key_len>, T>
HashPipe<T, hash_t, key_len>::getHeavyHitters(const T val_threshold) const {
  std::map<FlowKey<key_len>, T> heavy_hitters;
  std::set<FlowKey<key_len>> checked;
  for (int i = 0; i < depth_; ++i) {
    for (int j = 0; j < width_; ++j) {
      const auto &flowkey = arr_[i][j].flowkey_;
      if (checked.find(flowkey) != checked.end()) {
        continue;
      }
      checked.insert(flowkey);
      auto estimate_val = query(flowkey);
      if (estimate_val >= val_threshold) {
        heavy_hitters.emplace(flowkey, estimate_val);
      }
    }
  }
  return heavy_hitters;
}

template <typename T, typename hash_t, int32_t key_len>
std::size_t HashPipe<T, hash_t, key_len>::size() const {
  return sizeof(HashPipe<T, hash_t, key_len>) + depth_ * sizeof(hash_t) +
         depth_ * width_ * sizeof(Entry);
}
template <typename T, typename hash_t, int32_t key_len>
void HashPipe<T, hash_t, key_len>::clear() {
  FlowKey<key_len> empty_key{};
  for (int i = 0; i < depth_; ++i) {
    for (int j = 0; j < width_; ++j) {
      arr_[i][j].flowkey_ = empty_key;
      arr_[i][j].val_ = 0;
    }
  }
}
} // namespace SketchLab
#endif