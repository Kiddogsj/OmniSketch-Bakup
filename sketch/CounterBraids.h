#ifndef SKETCHLAB_CPP_COUNTERBRAIDS_H
#define SKETCHLAB_CPP_COUNTERBRAIDS_H

#include "hash.h"
#include "util.h"

#include <algorithm>
#include <cstddef>
#include <numeric>
#include <unordered_map>
#include <vector>

#define TOBYTES(n) (((n) + 7) >> 3)
#define TOINDEX(n) ((n) >> 3)
#define TOOFFSET(n) ((n)&0x7)
#define TOTAIL(n) ((((n)-1) & 0x7) + 1)

namespace SketchLab {

static void MessagePassing(int32_t *estimate, const int32_t *cnt,
                           std::unordered_map<int32_t, int32_t> *left,
                           int32_t lsize, std::vector<int32_t> *right,
                           int32_t rsize, int32_t T) {
  int32_t *ptr = new int32_t[lsize];
  // Iter
  for (int32_t i = 1; i <= T; ++i) {
    // Forward
    for (int32_t j = 0; j < rsize; ++j) {
      int32_t acc = 0;
      for (auto k : right[j]) {
        acc += estimate[k];
      }
      acc = cnt[j] - acc;
      for (auto k : right[j]) {
        left[k][j] = std::max(acc + estimate[k], (int32_t)1);
      }
    }
    // Backward
    for (int32_t j = 0; j < lsize; ++j) {
      if (left[j].empty()) {
        estimate[j] = 0;
      } else {
        int32_t maxi = 0, mini = 0x3FFFFFFF;
        for (auto &k : left[j]) {
          maxi = std::max(maxi, k.second);
          mini = std::min(mini, k.second);
        }
        if (i & 1) {
          estimate[j] = mini;
        } else {
          estimate[j] = maxi;
        }
      }
    } // End Backward
    if (i == T - 1) {
      std::copy(estimate, estimate + lsize, ptr);
    }
  } // End Iter
  for (int32_t i = 0; i < lsize; ++i) {
    estimate[i] = (estimate[i] + ptr[i]) >> 1;
  }
  delete[] ptr;
} // End MessagePassing

template <typename hash_t, int32_t key_len> class CounterBraids {

private:
  int32_t layer_;     // # of layers in CB
  int32_t *cntno_;    // # of counters within each layer of CB
  int32_t *cntdep_;   // # of bits within each layer of CB
  int32_t *hashno_;   // # of hash fns within each layer of CB
  hash_t **hash_fns_; // mimic a 2D arr of size [layer_, hashno_]
  uint8_t **counter_;
  uint8_t **status_;

  std::unordered_map<const FlowKey<key_len>, int32_t> flow_map_;
  // get status bit associated with a counter
  bool getStat(int32_t layer, int32_t cnt) {
    return (status_[layer][TOINDEX(cnt)] >> TOOFFSET(cnt)) & 1;
  }
  // get counter value
  int32_t getCounter(int32_t layer, int32_t cnt);
  // update a counter
  int32_t updateCnt(int32_t layer, int32_t cnt, int32_t val);
  // update a status bit
  void updateStat(int32_t layer, int32_t cnt);
  // recursively update from a layer
  void updateLayer(int32_t layer, int32_t cnt, int32_t val);
  // decode counter-counter layer
  int32_t *decodeLayer(int32_t layer, int32_t T, int32_t *cnt);
  // decode flow-counter layer
  void decodeFlow(int32_t T, int32_t *cnt);

public:
  CounterBraids(const int32_t layer, const int32_t *cntno,
                const int32_t *cntdep, const int32_t *hashno);
  ~CounterBraids();
  // online update
  void update(const FlowKey<key_len> &flowkey, int32_t val); // incremented by 1
  // offline decoding
  void decode(int32_t T);

  int32_t getVal(const FlowKey<key_len> &flowkey) {
    return flow_map_[flowkey];
  };
  size_t size() const;
  void clear();
};

template <typename hash_t, int32_t key_len>
CounterBraids<hash_t, key_len>::CounterBraids(const int32_t layer,
                                              const int32_t *cntno,
                                              const int32_t *cntdep,
                                              const int32_t *hashno)
    : layer_(layer), cntno_(new int32_t[layer]), cntdep_(new int32_t[layer]),
      hashno_(new int32_t[layer]) {
  // Avoid hash collision
  for (int32_t i = 0; i < layer; i++) {
    cntno_[i] = Util::NextPrime(cntno[i]);
  }
  std::copy(cntdep, cntdep + layer, cntdep_);
  std::copy(hashno, hashno + layer, hashno_);

  // Intialize hash fns
  hash_fns_ = new hash_t *[layer_];
  hash_fns_[0] = new hash_t[std::accumulate(hashno_, hashno_ + layer_, 0)];
  for (int32_t i = 1; i < layer_; ++i) {
    hash_fns_[i] = hash_fns_[i - 1] + hashno_[i - 1];
  }

  // Initialize counters
  counter_ = new uint8_t *[layer_];
  for (int32_t i = 0; i < layer_; ++i) {
    counter_[i] =
        new uint8_t[cntno_[i] * TOBYTES(cntdep_[i])](); // Init with zero
  }

  // Initialize status bits
  status_ = new uint8_t *[layer_];
  for (int32_t i = 0; i < layer_; ++i) {
    status_[i] = new uint8_t[TOBYTES(cntno_[i])](); // Init with zero
  } // bits associated with the last layer adds to a negligible storage overhead
}

template <typename hash_t, int32_t key_len>
CounterBraids<hash_t, key_len>::~CounterBraids() {
  delete[] cntno_;
  delete[] cntdep_;

  delete[] hashno_;
  delete[] hash_fns_[0];
  delete[] hash_fns_;

  for (int32_t i = 0; i < layer_; ++i) {
    delete[] counter_[i];
  }
  delete[] counter_;

  for (int32_t i = 0; i < layer_; ++i) {
    delete[] status_[i];
  }
  delete[] status_;
}

template <typename hash_t, int32_t key_len>
void CounterBraids<hash_t, key_len>::update(const FlowKey<key_len> &flowkey,
                                            int32_t val) {
  flow_map_.emplace(flowkey, 0);
  for (int32_t i = 0; i < hashno_[0]; ++i) {
    updateLayer(0, hash_fns_[0][i](flowkey) % cntno_[0], val);
  }
}

template <typename hash_t, int32_t key_len>
int32_t CounterBraids<hash_t, key_len>::getCounter(int32_t layer, int32_t cnt) {
  int32_t lower = TOBYTES(cntdep_[layer]) * cnt;
  int32_t upper = lower + TOBYTES(cntdep_[layer]);
  int32_t ret = 0, base = 1;
  while (lower < upper - 1) {
    ret += counter_[layer][lower] * base;
    lower++;
    base <<= 8;
  }
  ret += counter_[layer][upper - 1] * base;
  return ret;
}

template <typename hash_t, int32_t key_len>
int32_t CounterBraids<hash_t, key_len>::updateCnt(int32_t layer, int32_t cnt,
                                                  int32_t val) {
  int32_t lower = TOBYTES(cntdep_[layer]) * cnt;
  int32_t upper = lower + TOBYTES(cntdep_[layer]);
  uint32_t overflow = 0;
  while (lower < upper - 1) {
    uint8_t tmp = counter_[layer][lower] + overflow + (val & (uint8_t)0xFF);
    overflow = (counter_[layer][lower] + overflow + (val & (uint8_t)0xFF)) >> 8;
    counter_[layer][lower] = tmp;
    lower++;
    val >>= 8;
  }
  /*
  while (lower < upper - 1) {
    if (counter_[layer][lower] == (uint8_t)0xFF) {
      counter_[layer][lower] = 0;
      lower++;
    } else {
      counter_[layer][lower]++;
      return false;
    }
  }*/
  uint32_t off = TOTAIL(cntdep_[layer]);
  uint32_t bnd = (1 << off) - 1;
  uint8_t tmp = (counter_[layer][lower] + overflow + (val & bnd)) & bnd;
  overflow = (counter_[layer][lower] + overflow + (val & bnd)) >> off;
  counter_[layer][lower] = tmp;
  overflow += (val >> off);
  if (overflow) {
    updateStat(layer, cnt);
  }
  return overflow;
}

template <typename hash_t, int32_t key_len>
void CounterBraids<hash_t, key_len>::updateStat(int32_t layer, int32_t cnt) {
  uint32_t ind = TOINDEX(cnt), off = 1 << TOOFFSET(cnt);
  status_[layer][ind] |= off;
}

template <typename hash_t, int32_t key_len>
void CounterBraids<hash_t, key_len>::updateLayer(int32_t layer, int32_t cnt,
                                                 int32_t val) {
  int32_t carry = updateCnt(layer, cnt, val);
  if (layer == layer_ - 1)
    return;
  for (int32_t i = 0; i < hashno_[layer + 1]; ++i) {
    updateLayer(layer + 1, hash_fns_[layer + 1][i](cnt) % cntno_[layer + 1],
                carry);
  }
}

template <typename hash_t, int32_t key_len>
size_t CounterBraids<hash_t, key_len>::size() const {
  size_t tot = sizeof(CounterBraids<hash_t, key_len>); // instance
  for (int32_t i = 0; i < layer_; ++i) {
    tot += TOBYTES(cntno_[i] * (cntdep_[i] + 1)); // counter + status bits
    tot += hashno_[i] * sizeof(hash_t);           // hash_fns
  }

  return tot;
}

template <typename hash_t, int32_t key_len>
void CounterBraids<hash_t, key_len>::clear() {
  for (int32_t i = 0; i < layer_; ++i) {
    std::fill(counter_[i], counter_[i] + cntno_[i] * TOBYTES(cntdep_[i]), 0);
    std::fill(status_[i], status_[i] + TOBYTES(cntno_[i]), 0);
  }
  flow_map_.clear();
}

template <typename hash_t, int32_t key_len>
int32_t *CounterBraids<hash_t, key_len>::decodeLayer(int32_t layer, int32_t T,
                                                     int32_t *cnt) {
  // Build the graph
  std::unordered_map<int32_t, int32_t> *left =
      new std::unordered_map<int32_t, int32_t>[cntno_[layer]];
  std::vector<int32_t> *right = new std::vector<int32_t>[cntno_[layer + 1]];
  int32_t *est = new int32_t[cntno_[layer]]();
  for (int32_t i = 0; i < cntno_[layer]; ++i) {
    if (!getStat(layer, i))
      continue;
    for (int32_t j = 0; j < hashno_[layer + 1]; ++j) {
      int32_t k = hash_fns_[layer + 1][j](i) % cntno_[layer + 1];
      left[i].emplace(k, 0);
      right[k].push_back(i);
    }
  }
  // Message Passing
  MessagePassing(est, cnt, left, cntno_[layer], right, cntno_[layer + 1], T);
  delete[] left;
  delete[] right;
  return est;
}

template <typename hash_t, int32_t key_len>
void CounterBraids<hash_t, key_len>::decodeFlow(int32_t T, int32_t *cnt) {
  // Build the graph
  std::unordered_map<int32_t, int32_t> *left =
      new std::unordered_map<int32_t, int32_t>[flow_map_.size()];
  std::vector<int32_t> *right = new std::vector<int32_t>[cntno_[0]];
  int32_t *est = new int32_t[flow_map_.size()]();
  int32_t index = 0;
  for (auto &i : flow_map_) {
    for (int32_t j = 0; j < hashno_[0]; ++j) {
      int32_t k = hash_fns_[0][j](i.first) % cntno_[0];
      left[index].emplace(k, 0);
      right[k].push_back(index);
    }
    index++;
  }
  // Message Passing
  MessagePassing(est, cnt, left, flow_map_.size(), right, cntno_[0], T);
  delete[] left;
  delete[] right;
  // Store it to flow_map_
  index = 0;
  for (auto &i : flow_map_) {
    i.second = est[index];
    index++;
  }
}

template <typename hash_t, int32_t key_len>
void CounterBraids<hash_t, key_len>::decode(int32_t T) {
  int32_t *cnt = new int32_t[cntno_[layer_ - 1]];
  for (int32_t i = 0; i < cntno_[layer_ - 1]; ++i) {
    cnt[i] = getCounter(layer_ - 1, i);
  }
  // decode counter-counter layer
  for (int32_t i = layer_ - 2; i >= 0; --i) {
    int32_t *est = decodeLayer(i, T, cnt);
    int32_t nonzero = 0;
    for (int32_t j = 0; j < cntno_[i]; ++j) {
      nonzero += (est[j] != 0);
    }
    printf("nonzero: %d\n", nonzero);
    for (int32_t j = 0; j < cntno_[i]; ++j) {
      est[j] = (est[j] << cntdep_[i]) + getCounter(i, j);
    }
    delete[] cnt;
    cnt = est;
  }
  // decode flow-counter layer
  decodeFlow(T, cnt);
  delete[] cnt;
}

} // namespace SketchLab

#endif // SKETCHLAB_CPP_COUNTERBRAIDS_H