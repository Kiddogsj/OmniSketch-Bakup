#ifndef SKETCHLAB_CPP_LOSSYCOUNT_H
#define SKETCHLAB_CPP_LOSSYCOUNT_H

#include <cmath>
#include <list>

#include "hash.h"
#include "util.h"

namespace SketchLab {

template <typename T, typename hash_t, int32_t key_len> class LossyCount {

private:
  int32_t width_;
  int32_t size_;
  int32_t bucket_current_;
  int32_t count_;

  hash_t hash_fn_;

  struct entry {
    FlowKey<key_len> flowkey_;
    T freq_;
    T error_;
    entry() {}
    entry(const FlowKey<key_len> &flowkey, T freq, T error)
        : flowkey_(flowkey), freq_(freq), error_(error) {}
  };

  std::list<entry> *arr_;

public:
  LossyCount(double eps, int size);
  ~LossyCount();
  LossyCount(const LossyCount &) = delete;
  LossyCount(LossyCount &&) = delete;
  LossyCount &operator=(LossyCount) = delete;

  void update(const FlowKey<key_len> &flowkey, T val);
  T query(const FlowKey<key_len> &flowkey) const;
  std::size_t size() const;
  void clear();
};

template <typename T, typename hash_t, int32_t key_len>
LossyCount<T, hash_t, key_len>::LossyCount(double eps, int size)
    : width_(ceil(1 / eps)), size_(Util::NextPrime(size)), bucket_current_(1), count_(0) {
  arr_ = new std::list<entry>[size_];
}

template <typename T, typename hash_t, int32_t key_len>
LossyCount<T, hash_t, key_len>::~LossyCount() {
  delete[] arr_;
}

template <typename T, typename hash_t, int32_t key_len>
void LossyCount<T, hash_t, key_len>::update(const FlowKey<key_len> &flowkey,
                                            T val) {
  int idx = hash_fn_(flowkey) % size_;
  bool lookup = 0; //表中是否能找到flowkey
  for (entry &e : arr_[idx]) {
    if (e.flowkey_ == flowkey) {
      e.freq_ += val;
      lookup = 1;
      break;
    }
  }
  if (!lookup) {
    arr_[idx].push_back(entry{flowkey, val, bucket_current_ - 1});
  }
  count_ += val;
  if (count_ >= width_) {
    for (int i = 0; i < size_; ++i) {
      arr_[i].remove_if(
          [&](entry &e) { return e.freq_ + e.error_ <= bucket_current_; });
    }
    bucket_current_ += count_ / width_;
    count_ %= width_;
  }
}

template <typename T, typename hash_t, int32_t key_len>
T LossyCount<T, hash_t, key_len>::query(const FlowKey<key_len> &flowkey) const {
  int idx = hash_fn_(flowkey) % size_;
  for (entry e : arr_[idx]) {
    if (e.flowkey_ == flowkey) {
      return e.freq_;
    }
  }
  return 0;
}

template <typename T, typename hash_t, int32_t key_len>
std::size_t LossyCount<T, hash_t, key_len>::size() const {
  int entry_num = 0;
  for (int i = 1; i < size_; i++) {
    entry_num += arr_[i].size();
  }
  return sizeof(LossyCount<T, hash_t, key_len>) // Instance
         + sizeof(hash_t)                       // hash_fn
         + entry_num * sizeof(entry);           // counter
}

template <typename T, typename hash_t, int32_t key_len>
void LossyCount<T, hash_t, key_len>::clear() {
  for (int i = 0; i < size_; ++i)
    arr_[i].clear();
  bucket_current_ = 1;
  count_ = 0;
}

} // namespace SketchLab

#endif