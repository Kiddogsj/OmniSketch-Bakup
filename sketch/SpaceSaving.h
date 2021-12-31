#ifndef SKETCHLAB_CPP_SPACESAVING_H
#define SKETCHLAB_CPP_SPACESAVING_H

#include "hash.h"
#include "util.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <map>

namespace SketchLab {
template <typename T, int32_t key_len> class SpaceSaving {
private:
  std::map<FlowKey<key_len>, T> counter_;
  int num_threshold_;

public:
  SpaceSaving(int num_threshold) : num_threshold_(num_threshold) {}
  ~SpaceSaving() {}
  void update(const FlowKey<key_len> &flowkey, T val);
  T query(const FlowKey<key_len> &flowkey) const;
  std::map<FlowKey<key_len>, T> getHeavyHitters(const T threshold_val) const;
  std::size_t size() const; // only estimated size
};
template <typename T, int32_t key_len>
void SpaceSaving<T, key_len>::update(const FlowKey<key_len> &flowkey, T val) {
  auto iter = counter_.find(flowkey);
  if (iter != counter_.end()) {
    iter->second += val;
  } else if (counter_.size() < num_threshold_) {
    counter_.emplace(flowkey, val);
  } else {
    // find the min element
    auto min_iter =
        std::min_element(counter_.begin(), counter_.end(),
                         [](const std::pair<FlowKey<key_len>, T> &left,
                            const std::pair<FlowKey<key_len>, T> &right) {
                           return left.second < right.second;
                         });
    counter_.emplace(flowkey, val + min_iter->second);
    counter_.erase(min_iter);
  }
}

template <typename T, int32_t key_len>
T SpaceSaving<T, key_len>::query(const FlowKey<key_len> &flowkey) const {
  auto iter = counter_.find(flowkey);
  if (iter != counter_.end()) {
    return iter->second;
  } else {
    return 0;
  }
}

template <typename T, int32_t key_len>
std::map<FlowKey<key_len>, T>
SpaceSaving<T, key_len>::getHeavyHitters(const T val_threshold) const {
  std::map<FlowKey<key_len>, T> heavy_hitters;
  for (auto &kv : counter_) {
    if (kv.second >= val_threshold) {
      heavy_hitters.emplace(kv.first, kv.second);
    }
  }
  return heavy_hitters;
}
// only estimated size
// 没有考虑map的每个节点的指针的大小
template <typename T, int32_t key_len>
std::size_t SpaceSaving<T, key_len>::size() const {
  return (sizeof(FlowKey<key_len>) + sizeof(T)) *
             num_threshold_ +
         sizeof(SpaceSaving<T, key_len>);
}
} // namespace SketchLab
#endif