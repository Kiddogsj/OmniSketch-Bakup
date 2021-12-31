#ifndef SKETCHLAB_CPP_TWOLEVEL_H
#define SKETCHLAB_CPP_TWOLEVEL_H

#include "BloomFilter.h"
#include "hash.h"
#include "util.h"

#include <vector>

namespace SketchLab {

template <typename hash_t> class TwoLevel {
private:
  // distinct bf
  int32_t distinct_bf_num_hash_;
  int32_t distinct_bf_nbits_;

  // level 1
  int32_t bf_num_hash_;
  int32_t bf_nbits_;

  // level 2
  int32_t table_count_;
  int32_t table_num_hash_;
  int32_t table_nbits_;

  int32_t ss_width_;

  double r1_, r2_;
  double gamma_;
  int32_t w_;

  hash_t *hash_fns_;

  BloomFilter<hash_t> *distinct_bf_;
  BloomFilter<hash_t> *bf_;
  BloomFilter<hash_t> **table_;
  uint32_t *ss_;

  union Key {
    uint32_t src_dst_[2];
    FlowKey<8> flowkey_;
    Key(uint32_t src, uint32_t dst = 0) {
      src_dst_[0] = src;
      src_dst_[1] = dst;
    }
  };

public:
  TwoLevel(int distinct_bf_num_hash, int distinct_bf_nbits, int bf_num_hash,
           int bf_nbits, int table_count, int table_num_hash, int table_nbits,
           int ss_width, double r1, double r2, double gamma, int w);
  ~TwoLevel();
  TwoLevel(const TwoLevel &) = delete;
  TwoLevel(TwoLevel &&) = delete;
  TwoLevel &operator=(TwoLevel) = delete;

  void insert(uint32_t src, uint32_t dst);
  std::vector<uint32_t> query() const;
  size_t size() const;
  void clear();
};

template <typename hash_t>
TwoLevel<hash_t>::TwoLevel(int distinct_bf_num_hash, int distinct_bf_nbits,
                           int bf_num_hash, int bf_nbits, int table_count,
                           int table_num_hash, int table_nbits, int ss_width,
                           double r1, double r2, double gamma, int w)
    : distinct_bf_num_hash_(distinct_bf_num_hash),
      distinct_bf_nbits_(distinct_bf_nbits), bf_num_hash_(bf_num_hash),
      bf_nbits_(bf_nbits), table_count_(table_count),
      table_num_hash_(table_num_hash), table_nbits_(table_nbits),
      ss_width_(ss_width), r1_(r1), r2_(r2), gamma_(gamma), w_(w) {
  hash_fns_ = new hash_t[table_count_ + 2];

  // distinct bf
  distinct_bf_ =
      new BloomFilter<hash_t>(distinct_bf_nbits_, distinct_bf_num_hash_);

  // level 1
  bf_ = new BloomFilter<hash_t>(bf_nbits_, bf_num_hash_);

  // level 2
  table_ = new BloomFilter<hash_t> *[table_count_];
  for (int32_t i = 0; i < table_count_; ++i) {
    table_[i] = new BloomFilter<hash_t>(table_nbits_, table_num_hash_);
  }

  ss_width_ = Util::NextPrime(ss_width_);
  ss_ = new uint32_t[ss_width_]();
}

template <typename hash_t> TwoLevel<hash_t>::~TwoLevel() {
  delete[] hash_fns_;
  delete distinct_bf_;
  delete bf_;
  for (int32_t i = 0; i < table_count_; ++i) {
    delete table_[i];
  }
  delete[] table_;
  delete[] ss_;
}

template <typename hash_t>
void TwoLevel<hash_t>::insert(uint32_t src, uint32_t dst) {
  Key key(src, dst);
  int32_t edge1 = r1_ * 1000;
  int32_t edge2 = r2_ * 1000;
  int32_t edge3 = (1 / gamma_) * 1000;
  int32_t count = 0;
  uint32_t h1 = hash_fns_[0](key.flowkey_) % 1000;
  uint32_t h2 = hash_fns_[1](key.flowkey_) % 1000;
  uint32_t h3[table_count_];
  for (int32_t i = 0; i < table_count_; ++i) {
    h3[i] = hash_fns_[i + 2](key.flowkey_) % 1000;
  }
  if (!distinct_bf_->query(key.flowkey_)) {
    // step 1
    if (h2 < edge2) {
      if (bf_->query(FlowKey<4>(src))) {
        for (int32_t i = 0; i < table_count_; ++i) {
          if (h3[i] < edge3) {
            table_[i]->insert(FlowKey<4>(src));
            ++count;
          } else {
            count += table_[i]->query(FlowKey<4>(src));
          }
        }
        if (count >= w_) {
          int32_t ss_index = hash_fns_[0](FlowKey<4>(src)) % ss_width_;
          for (int32_t i = 0; i < ss_width_; ++i) {
            if (ss_[(ss_index + i) % ss_width_] == 0) {
              ss_[(ss_index + i) % ss_width_] = src;
              break;
            } else {
              if (ss_[(ss_index + i) % ss_width_] == src) {
                break;
              }
            }
          }
        }
      }
    }
    // step 2
    if (h1 < edge1) {
      bf_->insert(FlowKey<4>(src));
    }
    distinct_bf_->insert(key.flowkey_);
  }
}

template <typename hash_t>
std::vector<uint32_t> TwoLevel<hash_t>::query() const {
  std::vector<uint32_t> super_spreader;
  for (int32_t i = 0; i < ss_width_; ++i) {
    if (ss_[i]) {
      super_spreader.push_back(ss_[i]);
    }
  }
  return super_spreader;
}

template <typename hash_t> size_t TwoLevel<hash_t>::size() const {
  return sizeof(TwoLevel<hash_t>)              // Instance
         + (table_count_ + 2) * sizeof(hash_t) // hash_fns
         + distinct_bf_->size()                // distinct_bf_
         + bf_->size()                         // bf_
         + table_count_ * table_[0]->size()    // table_
         + ss_width_ * sizeof(uint32_t);       // ss_
}

template <typename hash_t> void TwoLevel<hash_t>::clear() {
  distinct_bf_->clear();
  bf_->clear();
  for (int32_t i = 0; i < table_count_; ++i) {
    table_[i]->clear();
  }
  std::fill(ss_, ss_ + ss_width_, 0);
}

} // namespace SketchLab

#endif // SKETCHLAB_CPP_TWOLEVEL_H