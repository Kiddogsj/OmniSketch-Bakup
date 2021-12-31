//
// Created by ztr on 2021/5/23
//

#ifndef SKETCHLAB_CPP_HYPERLOGLOG_H
#define SKETCHLAB_CPP_HYPERLOGLOG_H

#include <algorithm>
#include <cmath>
#include <memory>

#include "hash.h"
#include "util.h"

namespace SketchLab {

// Which type for float?

template <typename T, typename hash_t> class HyperLogLog {

  int32_t depth_;

  int32_t log2_depth_;

  double alpha_;

  hash_t *hash_fns_;

  int32_t *max_pos_;

public:
  HyperLogLog(int depth);
  ~HyperLogLog();

  template <int32_t key_len> void update(const FlowKey<key_len> &flowkey);
  T query() const;
  size_t size() const;
  void clear();
};

double alpha_HyperLogLog(int32_t m) {
  if (m == 16) {
    return 0.673;
  }

  if (m == 32) {
    return 0.697;
  }

  if (m == 64) {
    return 0.709;
  }

  if (m >= 128) {
    return 0.7213 / (1. + 1.079 / m);
  }

  return 0.;
}

template <typename T, typename hash_t>
HyperLogLog<T, hash_t>::HyperLogLog(int depth) : depth_(depth) {

  log2_depth_ = 4;

  while ((log2_depth_ < 16) && (depth_ > (1 << log2_depth_))) {
    log2_depth_++;
  }

  // set log2_depth_ to 4, 5, 6, ..., 16

  depth_ = (1 << log2_depth_);

  // set depth_ to 16, 32, 64, ..., 65536

  alpha_ = alpha_HyperLogLog(depth_);

  hash_fns_ = new hash_t[1];

  max_pos_ = new int32_t[depth_]();
}

template <typename T, typename hash_t> HyperLogLog<T, hash_t>::~HyperLogLog() {
  delete[] hash_fns_;

  delete[] max_pos_;
}

int32_t pos_HyperLogLog(uint32_t s, int32_t K) {
  if (!s) {
    return K + 1;
  }

  int32_t pos = 1;

  while ((s & 1) == 0) {
    pos++;
    s >>= 1;
  }

  return pos;
}

template <typename T, typename hash_t>
template <int32_t key_len>
void HyperLogLog<T, hash_t>::update(const FlowKey<key_len> &flowkey) {

  // ignore potential pkt_size because HyperLogLog focus on cardinality

  uint32_t hash_res_ = static_cast<uint32_t>(hash_fns_[0](flowkey));

  // set hash result to uint32_t

  int32_t index_ = hash_res_ & ((1 << log2_depth_) - 1);

  uint32_t string_ = hash_res_ >> log2_depth_;

  int32_t cur_pos_ = pos_HyperLogLog(string_, 32 - log2_depth_);

  max_pos_[index_] = std::max(max_pos_[index_], cur_pos_);
}

double negpwr2_HyperLogLog(int32_t e) {
  double negpwr2_ = 1.;
  while (e > 0) {
    negpwr2_ /= 2.;
    e--;
  }
  return negpwr2_;
}

template <typename T, typename hash_t> T HyperLogLog<T, hash_t>::query() const {
  double E_ = 0;

  for (int32_t index_ = 0; index_ < depth_; index_++) {
    E_ += negpwr2_HyperLogLog(max_pos_[index_]);
  }

  double depth_float_ = static_cast<double>(depth_);

  E_ = 1. / E_;

  E_ *= alpha_ * depth_float_ * depth_float_;

  double Boundary1_ = depth_float_ * 2.5;

  double Boundary2_ = 65536. * 65536. / 30.;

  double Estar_ = E_;

  if (E_ <= Boundary1_) {
    int32_t V_ = 0;

    for (int32_t index_ = 0; index_ < depth_; index_++) {
      if (max_pos_[index_] == 0)
        V_++;

      // count empty registers
    }

    if (V_ > 0) {
      Estar_ = log(depth_float_ / static_cast<double>(V_)) * depth_float_;
    }

    else {
      Estar_ = E_;
    }

    // small range correction
  }

  else if (E_ <= Boundary2_) {
    Estar_ = E_;

    // intermediate range - no correction
  }

  else {
    Estar_ = 0. - 65536. * 65536. * log(1. - E_ / 65536. / 65536.);

    // large range correction
  }

  return static_cast<T>(Estar_);
}

template <typename T, typename hash_t>
size_t HyperLogLog<T, hash_t>::size() const {
  return sizeof(HyperLogLog<T, hash_t>) // Instance
         + 1 * sizeof(hash_t)           // hash_fns
         + depth_ * sizeof(uint32_t);   // maximizer
}

template <typename T, typename hash_t> void HyperLogLog<T, hash_t>::clear() {
  for (int32_t index_ = 0; index_ < depth_; index_++) {
    max_pos_[index_] = 0;
  }
}

} // namespace SketchLab

#endif // SKETCHLAB_CPP_HyperLogLog_H