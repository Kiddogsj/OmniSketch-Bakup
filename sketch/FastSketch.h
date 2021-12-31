#ifndef SKETCHLAB_CPP_FASTSKETCH_H
#define SKETCHLAB_CPP_FASTSKETCH_H

#include "hash.h"
#include "util.h"

#include <map>
#include <vector>

namespace SketchLab {

template <typename T, typename hash_t, int32_t key_len> class FastSketch {
private:
  T sum_; // Count total traffic
  int32_t depth_;
  int32_t width_;
  int32_t num_hash_; // numbers of hash functions
  T **counter_;      // Counter table
  hash_t *hash_fns_;

  bool guessOne(int32_t i, T thresh, uint8_t *guess);
  void recover(uint8_t *q, int32_t i, int32_t j, uint8_t *guess);
  std::map<FlowKey<key_len>, T> detectAnomaly(T threshold);

public:
  FastSketch(int32_t depth, int32_t num_hash);
  FastSketch(const FastSketch &rhs);
  FastSketch(FastSketch &&rhs) noexcept;
  FastSketch &operator=(FastSketch rhs) noexcept;
  void swap(FastSketch &rhs) noexcept;
  ~FastSketch();

  void update(const FlowKey<key_len> &flowkey, T val);
  T query(const FlowKey<key_len> &flowkey) const;
  std::map<FlowKey<key_len>, T> heavyChangers(T threshold, const FastSketch &other) ;
  std::map<FlowKey<key_len>, T> heavyHitters(T threshold);
  
  size_t size() const;
  void clear();
  void merge(const FastSketch<T, hash_t, key_len> **fast_arr,
             int32_t size); // 将size个FastSketch的counter合
  T getCount() const;       // Return the total traffic
  T **getTable() const;
};
template <typename T, typename hash_t, int32_t key_len>
FastSketch<T, hash_t, key_len>::FastSketch(int32_t depth, int32_t num_hash)
    : depth_(depth), num_hash_(num_hash) {
  int32_t d = 1;
  while (d < depth_ && d > 0) {
    d = (d << 1);
  }
  depth_ = (d > 0) ? d : (1 << 30);
  // 结构：depth_ * (1 + log(n/depth_))， 其中n是flowkey的范围
  int32_t i = 1;
  for (; (1 << i) <= depth_; ++i)
    ;
  --i;
  int32_t key_bits = (key_len << 3);
  width_ = 1 + key_bits - i;

  sum_ = 0;

  counter_ = new T *[depth_];
  counter_[0] = new T[depth_ * width_]();
  for (int32_t j = 1; j < depth_; ++j) {
    counter_[j] = counter_[j - 1] + width_;
  }
  // num_hash_个哈希函数
  hash_fns_ = new hash_t[num_hash_];
}

template <typename T, typename hash_t, int32_t key_len>
FastSketch<T, hash_t, key_len>::FastSketch(const FastSketch &rhs)
    : sum_(rhs.sum_), depth_(rhs.depth_), width_(rhs.width_),
      num_hash_(rhs.num_hash_) {
  counter_ = new T *[depth_];
  counter_[0] = new T[depth_ * width_]();
  for (int32_t j = 1; j < depth_; ++j) {
    counter_[j] = counter_[j - 1] + width_;
  }
  std::copy(rhs.counter_[0], rhs.counter_[0] + depth_ * width_, counter_[0]);
  hash_fns_ = new hash_t[num_hash_];
  std::copy(rhs.hash_fns_, rhs.hash_fns_ + num_hash_, hash_fns_);
}

template <typename T, typename hash_t, int32_t key_len>
FastSketch<T, hash_t, key_len>::FastSketch(FastSketch &&rhs) noexcept
    : sum_(rhs.sum_), depth_(rhs.depth_), width_(rhs.width_),
      num_hash_(rhs.num_hash_) {
  counter_ = rhs.counter_;
  rhs.counter_ = nullptr;
  hash_fns_ = rhs.hash_fns_;
  rhs.hash_fns_ = nullptr;
}

template <typename T, typename hash_t, int32_t key_len>
FastSketch<T, hash_t, key_len> &
FastSketch<T, hash_t, key_len>::operator=(FastSketch rhs) noexcept {
  rhs.swap(*this);
  return *this;
}

template <typename T, typename hash_t, int32_t key_len>
void FastSketch<T, hash_t, key_len>::swap(FastSketch &rhs) noexcept {
  using std::swap;
  swap(sum_, rhs.sum_);
  swap(depth_, rhs.depth_);
  swap(width_, rhs.width_);
  swap(num_hash_, rhs.num_hash_);
  swap(counter_, rhs.counter_);
  swap(hash_fns_, rhs.hash_fns_);
}

template <typename T, typename hash_t, int32_t key_len>
FastSketch<T, hash_t, key_len>::~FastSketch() {
  delete[] hash_fns_;
  if (counter_ != nullptr) {
    delete[] counter_[0];
    delete[] counter_;
  }
}

template <typename T, typename hash_t, int32_t key_len>
void FastSketch<T, hash_t, key_len>::update(const FlowKey<key_len> &flowkey,
                                            T val) {
  sum_ += val;
  uint64_t key_val = 0;
  memcpy(&key_val, flowkey.cKey(), 8);
  uint64_t key_q = key_val / depth_;
  uint64_t key_mod = key_val % depth_;
  for (int32_t i = 0; i < num_hash_; ++i) {
    uint64_t bucket =
        (key_mod) ^ (hash_fns_[i]((uint8_t *)&key_q, key_len) % depth_);
    counter_[(int32_t)bucket][0] += val;
    // log insert
    for (int32_t j = 1; j < width_; ++j) {
      if (key_q & (1ULL << (j - 1))) {
        counter_[bucket][j] += val;
      }
    }
  }
}

template <typename T, typename hash_t, int32_t key_len>
T FastSketch<T, hash_t, key_len>::query(
    const FlowKey<key_len> &flowkey) const {
  T res = 0;
  uint64_t key_val = 0;
  memcpy(&key_val, flowkey.cKey(), key_len);
  // Update sketch

  uint64_t key_q = key_val / depth_;
  uint64_t key_mod = key_val % depth_;

  for (int32_t i = 0; i < num_hash_; ++i) {
    uint32_t bucket =
        (key_mod) ^ (hash_fns_[i]((uint8_t *)&key_q, key_len) % depth_);
    // loginsert
    if (i == 0) {
      res = counter_[bucket][0];
    } else {
      res = std::min(res, counter_[bucket][0]);
    }
    for (int32_t j = 1; j < width_; ++j) {
      if (key_q & (1ULL << (j - 1))) {
        res = std::min(res, counter_[bucket][j]);
      }
    }
  }
  return res;
}

template <typename T, typename hash_t, int32_t key_len>
bool FastSketch<T, hash_t, key_len>::guessOne(int32_t i, T thresh,
                                              uint8_t *guess) {
  T count0 = counter_[i][0];
  if (count0 < thresh) {
    return false;
  }
  for (int32_t k = 1; k < width_; ++k) {
    // Maintest: if one side is above threshold, the other side is not
    T countk = counter_[i][k];
    if (((count0 - countk < thresh) && (countk < thresh)) ||
        ((count0 - countk > thresh) && (countk > thresh))) {
      return false;
    }
    if (countk > thresh) {
      int32_t nbyte = (k - 1) / 8;
      int32_t nbits = (k - 1) % 8;
      guess[nbyte] |= (1 << nbits);
    }
  }
  return true;
}

// 假设一个flowkey被用第j个哈希函数映射到了第i行。现在要恢复这个flowkey
template <typename T, typename hash_t, int32_t key_len>
void FastSketch<T, hash_t, key_len>::recover(uint8_t *q, int32_t i, int32_t j,
                                             uint8_t *guess) {
  uint64_t bucket = hash_fns_[j](q, key_len) % depth_;
  uint64_t qint = 0;
  memcpy(&qint, q, key_len);
  uint64_t tmp = qint * depth_ + (i ^ bucket);
  memcpy(guess, &tmp, key_len);
}

template <typename T, typename hash_t, int32_t key_len>

std::map<FlowKey<key_len>, T> FastSketch<T, hash_t, key_len>::detectAnomaly(T thresh) {
  uint8_t guess[key_len];
  uint8_t q[key_len];
  T degree = 0;
  std::map<FlowKey<key_len>, T> cand_list;
  for (int32_t i = 0; i < depth_; ++i) {
    // Find one candidate
    memset(guess, 0, key_len);
    memset(q, 0, key_len);
    if (guessOne(i, thresh, q) == false) {
      continue;
    }

    for (int32_t j = 0; j < num_hash_; ++j) {
      degree = 0;
      recover(q, i, j, guess);
      uint64_t guessint = 0; // 是猜测的flowkey的整数表示
      memcpy(&guessint, guess, key_len);
      uint64_t guess_q = guessint / depth_;
      uint64_t guess_mod = guessint % depth_;
      uint32_t row = hash_fns_[j]((uint8_t *)&guess_q, key_len) % depth_;
      row = guess_mod ^ row;
      uint64_t qint = 0;
      memcpy(&qint, q, key_len);
      if ((row == (uint32_t)i) &&
          (guess_q == qint)) { // 用第j个哈希函数恢复出的key是对的
        int32_t pass = 0;
        for (int32_t k = 0; k < num_hash_; ++k) { // 计算flowkey对应的估计值
          uint32_t bucket = hash_fns_[k]((uint8_t *)&guess_q, key_len) % depth_;
          bucket = guess_mod ^ bucket;
          T deg = counter_[bucket][0];
          if (deg > thresh) {
            pass++;
            if (k == 0)
              degree = deg;
            else
              degree = std::min(degree, deg);
            for (int32_t t = 1; t < width_; ++t) {
              if (guess_q & (1ULL << (t - 1))) {
                degree = std::min(degree, counter_[bucket][t]);
              }
            }
          }
        }
        if (pass == num_hash_) {
          FlowKey<key_len> guesskey{guess};
          if (cand_list.find(guesskey) != cand_list.end()) {
            if (cand_list[guesskey] > degree) {
              cand_list[guesskey] = degree;
            }
          } else {
            cand_list[guesskey] = degree;
          }
        }
      }
    }
  }
  return cand_list;
}

template<typename T, typename hash_t, int32_t key_len>
std::map<FlowKey<key_len>, T> FastSketch<T, hash_t, key_len>::heavyHitters(T threshold){
  return detectAnomaly(threshold);
}

template<typename T, typename hash_t, int32_t key_len>
std::map<FlowKey<key_len>, T> FastSketch<T, hash_t, key_len>::heavyChangers(T threshold, const FastSketch &other){
  int32_t n = depth_ * width_;
  T *temp = new T[n];
  std::copy(counter_[0], counter_[0] + n, temp);
  for(int32_t i = 0; i < n; ++i){
    *(counter_[0] + i) = std::abs(*(counter_[0] + i) - *(other.counter_[0] + i));
  }
  T temp_sum = sum_;
  sum_ = std::abs(sum_ - other.sum_);
  auto heavy_changers = detectAnomaly(threshold);
  std::copy(temp, temp + n, counter_[0]);
  sum_ = temp_sum;
  delete []temp;
  return heavy_changers;
}


template <typename T, typename hash_t, int32_t key_len>
size_t FastSketch<T, hash_t, key_len>::size() const {
  return sizeof(FastSketch<T, hash_t, key_len>) // Instance
         + num_hash_ * sizeof(hash_t)           // hash_fns
         + depth_ * width_ * sizeof(T);         // counter
}

template <typename T, typename hash_t, int32_t key_len>
void FastSketch<T, hash_t, key_len>::clear() {
  sum_ = 0;
  std::fill(counter_[0], counter_[0] + depth_ * width_, 0);
}

// 将size个FastSketch的counter合并
template <typename T, typename hash_t, int32_t key_len>
void FastSketch<T, hash_t, key_len>::merge(
    const FastSketch<T, hash_t, key_len> **fast_arr, int32_t size) {
  for (int32_t k = 0; k < size; ++k) {
    T **counts = fast_arr[k]->getTable();
    for (int32_t i = 0; i < depth_; ++i) {
      for (int32_t j = 0; j < width_; ++j) {
        counter_[i][j] += counts[i][j];
      }
    }
  }
}

template <typename T, typename hash_t, int32_t key_len>
T FastSketch<T, hash_t, key_len>::getCount() const {
  return sum_;
}

template <typename T, typename hash_t, int32_t key_len>
T **FastSketch<T, hash_t, key_len>::getTable() const {
  return counter_;
}

} // namespace SketchLab

#endif // SKETCHLAB_CPP_FASTSKETCH_H