//
// Created by Ferric on 2021/4/21.
//

#ifndef SKETCHLAB_CPP_LDSKETCH_H
#define SKETCHLAB_CPP_LDSKETCH_H

#include <algorithm>
#include <map>
#include <vector>

namespace SketchLab {

template <typename T, typename hash_t, int32_t key_len> class LDSketch {
  int32_t depth_, width_;
  T threshold_;
  double expansion_;

  hash_t *hash_fns_;

  struct Bounds {
    T lower, upper;
  };
  struct Bucket {
    T V, e;
    int32_t l;
    std::map<FlowKey<key_len>, T> A;

    Bucket();
    void update(const FlowKey<key_len> &flow_key, T val, double expansion);
    Bounds query(const FlowKey<key_len> &flow_key) const;
    void clear();
    std::size_t size();
  };
  Bucket **counter_;

public:
  LDSketch(int depth, int width, T threshold, double eps);
  LDSketch(const LDSketch &other);
  LDSketch(LDSketch &&other) = delete;
  ~LDSketch();
  LDSketch &operator=(const LDSketch &other) = delete;
  LDSketch &operator=(LDSketch &&other) = delete;

  void update(const FlowKey<key_len> &flow_key, T val);

  std::size_t size() const;
  void clear();

  std::map<FlowKey<key_len>, T> heavyHitters() const;
  std::map<FlowKey<key_len>, T> heavyChangers(const LDSketch &other) const;
};

template <typename T, typename hash_t, int32_t key_len>
LDSketch<T, hash_t, key_len>::Bucket::Bucket() : V(0), e(0), l(0) {}

template <typename T, typename hash_t, int32_t key_len>
void LDSketch<T, hash_t, key_len>::Bucket::update(
    const FlowKey<key_len> &flow_key, T val, double expansion) {
  V += val;

  auto it = A.find(flow_key);
  if (it != A.end())
    it->second += val;
  else if (A.size() < l)
    A.emplace(flow_key, val);
  else {
    T k = V / expansion;
    if ((k + 1) * (k + 2) - 1 <= l) {
      T e_cap = val;
      for (const auto &kv : A)
        e_cap = std::min(e_cap, kv.second);
      e += e_cap;
      for (auto it = A.begin(); it != A.end();) {
        it->second -= e_cap;
        if (it->second <= 0)
          it = A.erase(it);
        else
          ++it;
      }
      if (val > e_cap)
        A.emplace(flow_key, val - e_cap);
    } else {
      l = (k + 1) * (k + 2) - 1;
      A.emplace(flow_key, val);
    }
  }
}

template <typename T, typename hash_t, int32_t key_len>
typename LDSketch<T, hash_t, key_len>::Bounds
LDSketch<T, hash_t, key_len>::Bucket::query(
    const FlowKey<key_len> &flow_key) const {
  auto it = A.find(flow_key);
  if (it == A.end())
    return {0, e};
  else
    return {it->second, it->second + e};
}

template <typename T, typename hash_t, int32_t key_len>
void LDSketch<T, hash_t, key_len>::Bucket::clear() {
  V = e = 0;
  l = 0;
  A.clear();
}

template <typename T, typename hash_t, int32_t key_len>
std::size_t LDSketch<T, hash_t, key_len>::Bucket::size() {
  // Not accurate
  return sizeof(LDSketch<T, hash_t, key_len>::Bucket) +
         A.size() * (sizeof(FlowKey<key_len>) + sizeof(T));
}

template <typename T, typename hash_t, int32_t key_len>
LDSketch<T, hash_t, key_len>::LDSketch(int depth, int width, T threshold,
                                       double eps)
    : depth_(depth), width_(Util::NextPrime(width)), threshold_(threshold),
      expansion_(eps * threshold) {
  hash_fns_ = new hash_t[depth_];

  // Allocate continuous memory;
  counter_ = new Bucket *[depth_];
  counter_[0] = new Bucket[depth_ * width_]();
  for (int i = 1; i < depth_; ++i)
    counter_[i] = counter_[i - 1] + width_;
}

template <typename T, typename hash_t, int32_t key_len>
LDSketch<T, hash_t, key_len>::LDSketch(const LDSketch &other)
    : depth_(other.depth_), width_(other.width_), threshold_(other.threshold_),
      expansion_(other.expansion_) {
  hash_fns_ = new hash_t[depth_];
  std::copy(other.hash_fns_, other.hash_fns_ + depth_, hash_fns_);

  counter_ = new Bucket *[depth_];
  counter_[0] = new Bucket[depth_ * width_]();
  for (int i = 1; i < depth_; ++i)
    counter_[i] = counter_[i - 1] + width_;
  std::copy(other.counter_[0], other.counter_[0] + depth_ * width_,
            counter_[0]);
}

template <typename T, typename hash_t, int32_t key_len>
LDSketch<T, hash_t, key_len>::~LDSketch() {
  delete[] hash_fns_;

  delete[] counter_[0];
  delete[] counter_;
}

template <typename T, typename hash_t, int32_t key_len>
void LDSketch<T, hash_t, key_len>::update(const FlowKey<key_len> &flow_key,
                                          T val) {
  for (int i = 0; i < depth_; ++i) {
    int32_t index = hash_fns_[i](flow_key) % width_;
    counter_[i][index].update(flow_key, val, expansion_);
  }
}

template <typename T, typename hash_t, int32_t key_len>
void LDSketch<T, hash_t, key_len>::clear() {
  for (int i = 0; i < depth_; ++i)
    for (int j = 0; j < width_; ++j)
      counter_[i][j].clear();
}

template <typename T, typename hash_t, int32_t key_len>
std::size_t LDSketch<T, hash_t, key_len>::size() const {
  std::size_t s = sizeof(LDSketch<T, hash_t, key_len>) + // Instance
                  sizeof(hash_t) * depth_ +              // hash_fns
                  sizeof(Bucket *) * depth_;
  for (int i = 0; i < depth_; ++i)
    for (int j = 0; j < width_; ++j)
      s += counter_[i][j].size(); // Respective sizes
  return s;
}

template <typename T, typename hash_t, int32_t key_len>
std::map<FlowKey<key_len>, T>
LDSketch<T, hash_t, key_len>::heavyHitters() const {
  std::map<FlowKey<key_len>, T> heavy_hitters;

  for (int i = 0; i < depth_; ++i)
    for (int j = 0; j < width_; ++j) {
      const Bucket &bucket = counter_[i][j];
      if (bucket.V < threshold_)
        continue;

      for (const auto &kv : bucket.A) {
        if (heavy_hitters.find(kv.first) != heavy_hitters.end())
          continue;

        std::vector<T> upper_bounds(depth_);
        for (int k = 0; k < depth_; ++k) {
          int32_t index = hash_fns_[k](kv.first) % width_;
          upper_bounds[k] = counter_[k][index].query(kv.first).upper;
        }

        T u = *std::min_element(upper_bounds.begin(), upper_bounds.end());
        if (u >= threshold_)
          heavy_hitters.emplace(kv.first, u);
      }
    }

  return heavy_hitters;
}

template <typename T, typename hash_t, int32_t key_len>
std::map<FlowKey<key_len>, T>
LDSketch<T, hash_t, key_len>::heavyChangers(const LDSketch &other) const {
  auto d = [this, &other](FlowKey<key_len> flow_key) {
    std::vector<T> Ds(depth_);
    for (int i = 0; i < depth_; ++i) {
      int index = hash_fns_[i](flow_key) % width_;
      auto bounds = counter_[i][index].query(flow_key),
           other_bounds = other.counter_[i][index].query(flow_key);
      Ds[i] = std::max(bounds.upper - other_bounds.lower,
                       other_bounds.upper - bounds.lower);
    }
    return *std::min_element(Ds.begin(), Ds.end());
  };

  std::map<FlowKey<key_len>, T> heavy_changers;

  for (int i = 0; i < depth_; ++i)
    for (int j = 0; j < width_; ++j) {
      const Bucket &bucket = counter_[i][j];
      const Bucket &other_bucket = other.counter_[i][j];
      if (bucket.V < threshold_ && other_bucket.V < threshold_)
        continue;

      for (const auto &kv : bucket.A) {
        if (heavy_changers.find(kv.first) != heavy_changers.end())
          continue;

        T D = d(kv.first);
        if (D >= threshold_)
          heavy_changers.emplace(kv.first, D);
      }

      for (const auto &kv : other_bucket.A) {
        if (heavy_changers.find(kv.first) != heavy_changers.end())
          continue;

        T D = d(kv.first);
        if (D >= threshold_)
          heavy_changers.emplace(kv.first, D);
      }
    }

  return heavy_changers;
}

} // namespace SketchLab

#endif // SKETCHLAB_CPP_LDSKETCH_H
