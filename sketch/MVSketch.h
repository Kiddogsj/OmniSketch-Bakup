//
// Created by Ferric on 2021/04/20
//

#ifndef SKETCHLAB_CPP_MVSKETCH_H
#define SKETCHLAB_CPP_MVSKETCH_H

#include <algorithm>
#include <map>
#include <vector>

#include "hash.h"
#include "util.h"

namespace SketchLab {
template <typename T, typename hash_t, int32_t key_len> class MVSketch {
  int32_t depth_;
  int32_t width_;

  hash_t *hash_fns_;

  struct Bounds {
    T lower;
    T upper;
  };

  struct Bucket {
    T V;
    FlowKey<key_len> K;
    T C;
  };

  Bucket **counter_;

public:
  MVSketch(int depth, int width);
  MVSketch(const MVSketch<T, hash_t, key_len> &);
  MVSketch(MVSketch &&) = delete;
  ~MVSketch();
  MVSketch &operator=(const MVSketch &) = delete;
  MVSketch &operator=(MVSketch &&) = delete;

  void update(const FlowKey<key_len> &flow_key, T val);
  T query(const FlowKey<key_len> &flow_key) const;

  void clear();
  std::size_t size() const;

  Bounds queryBounds(const FlowKey<key_len> &flow_key) const;
  std::map<FlowKey<key_len>, T> heavyHitters(T threshold) const;
  std::map<FlowKey<key_len>, T> heavyChangers(T threshold,
                                              const MVSketch &other) const;
};

template <typename T, typename hash_t, int32_t key_len>
MVSketch<T, hash_t, key_len>::MVSketch(int depth, int width)
    : depth_(depth), width_(Util::NextPrime(width)) {
  hash_fns_ = new hash_t[depth_];

  // Allocate continuous memory
  counter_ = new Bucket *[depth_];
  counter_[0] = new Bucket[depth_ * width_]();
  for (int i = 1; i < depth_; ++i)
    counter_[i] = counter_[i - 1] + width_;
}

template <typename T, typename hash_t, int32_t key_len>
MVSketch<T, hash_t, key_len>::MVSketch(
    const MVSketch<T, hash_t, key_len> &other)
    : depth_(other.depth_), width_(other.width_) {
  hash_fns_ = new hash_t[depth_];
  std::copy(other.hash_fns_, other.hash_fns_ + depth_, hash_fns_);

  counter_ = new Bucket *[depth_];
  counter_[0] = new Bucket[depth_ * width_];
  for (int i = 1; i < depth_; ++i)
    counter_[i] = counter_[i - 1] + width_;
  std::copy(other.counter_[0], other.counter_[0] + depth_ * width_,
            counter_[0]);
}

template <typename T, typename hash_t, int32_t key_len>
MVSketch<T, hash_t, key_len>::~MVSketch() {
  delete[] hash_fns_;

  delete[] counter_[0];
  delete[] counter_;
}

template <typename T, typename hash_t, int32_t key_len>
void MVSketch<T, hash_t, key_len>::update(const FlowKey<key_len> &flow_key,
                                          T val) {
  for (int i = 0; i < depth_; ++i) {
    int index = hash_fns_[i](flow_key) % width_;
    counter_[i][index].V += val;
    if (counter_[i][index].K == flow_key)
      counter_[i][index].C += val;
    else {
      counter_[i][index].C -= val;
      if (counter_[i][index].C < 0) {
        counter_[i][index].K = flow_key;
        counter_[i][index].C *= -1;
      }
    }
  }
}

template <typename T, typename hash_t, int32_t key_len>
T MVSketch<T, hash_t, key_len>::query(const FlowKey<key_len> &flow_key) const {
  std::vector<T> S_cap(depth_);

  for (int i = 0; i < depth_; ++i) {
    int index = hash_fns_[i](flow_key) % width_;
    if (counter_[i][index].K == flow_key)
      S_cap[i] = (counter_[i][index].V + counter_[i][index].C) / 2;
    else
      S_cap[i] = (counter_[i][index].V - counter_[i][index].C) / 2;
  }

  return *min_element(S_cap.begin(), S_cap.end());
}

template <typename T, typename hash_t, int32_t key_len>
void MVSketch<T, hash_t, key_len>::clear() {
  std::fill(counter_[0], counter_[0] + depth_ * width_, {0, 0, 0});
}

template <typename T, typename hash_t, int32_t key_len>
std::size_t MVSketch<T, hash_t, key_len>::size() const {
  return sizeof(MVSketch<T, hash_t, key_len>) + // Instance
         depth_ * sizeof(hash_t) +              // hash_fns
         sizeof(Bucket *) * depth_ +            // counter
         sizeof(Bucket) * depth_ * width_;
}

template <typename T, typename hash_t, int32_t key_len>
typename MVSketch<T, hash_t, key_len>::Bounds
MVSketch<T, hash_t, key_len>::queryBounds(
    const FlowKey<key_len> &flow_key) const {
  std::vector<T> L(depth_);

  for (int i = 0; i < depth_; ++i) {
    int index = hash_fns_[i](flow_key) % width_;
    L[i] = counter_[i][index].K == flow_key ? counter_[i][index].C : 0;
  }

  return {*max_element(L.begin(), L.end()), query(flow_key)};
}

template <typename T, typename hash_t, int32_t key_len>
std::map<FlowKey<key_len>, T>
MVSketch<T, hash_t, key_len>::heavyHitters(T threshold) const {
  std::map<FlowKey<key_len>, T> heavy_hitters;

  for (int i = 0; i < depth_; ++i)
    for (int j = 0; j < width_; ++j) {
      if (counter_[i][j].V < threshold)
        continue;

      const FlowKey<key_len> &flow_key = counter_[i][j].K;
      if (heavy_hitters.find(flow_key) != heavy_hitters.end())
        continue;

      T S_cap = query(flow_key);
      if (S_cap >= threshold)
        heavy_hitters.emplace(flow_key, S_cap);
    }

  return heavy_hitters;
}

template <typename T, typename hash_t, int32_t key_len>
std::map<FlowKey<key_len>, T>
MVSketch<T, hash_t, key_len>::heavyChangers(T threshold,
                                            const MVSketch &other) const {
  auto d_cap = [this, &other](const FlowKey<key_len> &flow_key) {
    auto bounds = queryBounds(flow_key);
    auto other_bounds = other.queryBounds(flow_key);
    return std::max(std::abs(bounds.upper - other_bounds.lower),
                    std::abs(bounds.lower - other_bounds.upper));
  };

  std::map<FlowKey<key_len>, T> heavy_changers;

  for (int i = 0; i < depth_; ++i)
    for (int j = 0; j < width_; ++j) {
      if (counter_[i][j].V < threshold)
        continue;

      const FlowKey<key_len> &flow_key = counter_[i][j].K;
      if (heavy_changers.find(flow_key) != heavy_changers.end())
        continue;

      T D_cap = d_cap(flow_key);
      if (D_cap >= threshold)
        heavy_changers.emplace(flow_key, D_cap);
    }

  for (int i = 0; i < depth_; ++i)
    for (int j = 0; j < width_; ++j) {
      if (other.counter_[i][j].V < threshold)
        continue;

      const FlowKey<key_len> &flow_key = other.counter_[i][j].K;
      if (heavy_changers.find(flow_key) != heavy_changers.end())
        continue;

      T D_cap = d_cap(flow_key);
      if (D_cap >= threshold)
        heavy_changers.emplace(flow_key, D_cap);
    }

  return heavy_changers;
}

} // namespace SketchLab

#endif
