#ifndef SKETCHLAB_CPP_MISRAGRIES_H
#define SKETCHLAB_CPP_MISRAGRIES_H

#include "hash.h"
#include "util.h"

#include <algorithm>
#include <cstdint>
#include <map>
namespace SketchLab {
template <typename T, int32_t key_len> class MisraGries {
private:
  std::map<FlowKey<key_len>, T> counter_;
  int num_threshold_;
  int total_val_;

public:
  MisraGries(int num_threshold)
      : num_threshold_(num_threshold), total_val_(0) {}
  ~MisraGries() {}
  void update(const FlowKey<key_len> &flowkey);
  void update(const FlowKey<key_len> &flowkey, T val);
  T query(const FlowKey<key_len> &flowkey) const;
  std::map<FlowKey<key_len>, T>
  getHeavyHittersWithLowerBound(const T val_threshold) const;
  std::map<FlowKey<key_len>, T>
  getHeavyHittersWithUpperBound(const T val_threshold) const;
  std::size_t size() const; // only estimated size
};

// 默认val是1
template <typename T, int32_t key_len>
void MisraGries<T, key_len>::update(const FlowKey<key_len> &flowkey) {
  total_val_ += 1;
  auto iter = counter_.find(flowkey);
  if (iter != counter_.end()) {
    iter->second += 1;
  } else if (counter_.size() < num_threshold_) {
    counter_.emplace(flowkey, 1);
  } else {
    for (auto it = counter_.begin(); it != counter_.end();) {
      it->second -= 1;
      if (it->second == 0) {
        it = counter_.erase(it);
      } else {
        ++it;
      }
    }
  }
}

template <typename T, int32_t key_len>
void MisraGries<T, key_len>::update(const FlowKey<key_len> &flowkey, T val) {
  total_val_ += val;
  auto iter = counter_.find(flowkey);
  if (iter != counter_.end()) {
    iter->second += val;
  } else if (counter_.size() < num_threshold_) {
    counter_.emplace(flowkey, val);
  } else {
    auto min_val =
        std::min_element(counter_.begin(), counter_.end(),
                         [](const std::pair<FlowKey<key_len>, T> &left,
                            const std::pair<FlowKey<key_len>, T> &right) {
                           return left.second < right.second;
                         })
            ->second;
    if (val < min_val) {
      for (auto &kv : counter_) {
        kv.second -= val;
      }
    } else {
      for (auto it = counter_.begin(); it != counter_.end();) {
        if (it->second <= min_val) {
          it = counter_.erase(it);
        } else {
          it->second -= min_val;
          ++it;
        }
      }
      val -= min_val;
      if (val > 0) {
        counter_.emplace(flowkey, val);
      }
    }
  }
}

template <typename T, int32_t key_len>
T MisraGries<T, key_len>::query(const FlowKey<key_len> &flowkey) const {
  auto iter = counter_.find(flowkey);
  if (iter != counter_.end()) {
    return iter->second;
  } else {
    return 0;
  }
}

template <typename T, int32_t key_len>
std::map<FlowKey<key_len>, T>
MisraGries<T, key_len>::getHeavyHittersWithLowerBound(
    const T val_threshold) const {
  std::map<FlowKey<key_len>, T> heavy_hitters;
  for (auto &kv : counter_) {
    if (kv.second >= val_threshold) {
      heavy_hitters.emplace(kv.first, kv.second);
    }
  }
  return heavy_hitters;
}

template <typename T, int32_t key_len>
std::map<FlowKey<key_len>, T>
MisraGries<T, key_len>::getHeavyHittersWithUpperBound(
    const T val_threshold) const {
  std::map<FlowKey<key_len>, T> heavy_hitters;
  int delta = total_val_ / (num_threshold_ + 1);
  for (auto &kv : counter_) {
    if (kv.second + delta >= val_threshold) {
      heavy_hitters.emplace(kv.first, kv.second);
    }
  }
  return heavy_hitters;
}
// only estimated size
// 没有考虑map的每个节点的指针的大小
template <typename T, int32_t key_len>
std::size_t MisraGries<T, key_len>::size() const {
  return (sizeof(FlowKey<key_len>) + sizeof(T)) * num_threshold_ +
         sizeof(MisraGries<T, key_len>);
}
} // namespace SketchLab
#endif