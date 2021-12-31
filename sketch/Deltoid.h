#ifndef SKETCHLAB_CPP_DELTOID_H
#define SKETCHLAB_CPP_DELTOID_H

#include "hash.h"
#include "util.h"
#include <map>
namespace SketchLab {
template <typename T, typename hash_t, int32_t key_len> class Deltoid {

public:
  Deltoid(int32_t num_hash, int32_t num_group);
  Deltoid(const Deltoid &rhs);
  Deltoid(Deltoid &&rhs) noexcept;
  Deltoid &operator=(Deltoid rhs) noexcept;
  void swap(Deltoid &rhs) noexcept;
  ~Deltoid();

  void update(const FlowKey<key_len> &flowkey, T val);
  std::map<FlowKey<key_len>, T> heavyChangers(T threshold,
                                              const Deltoid &other) const;
  std::map<FlowKey<key_len>, T> heavyHitters(T threshold) const;

  T query(const FlowKey<key_len> &flowkey) const;
  size_t size() const;
  void clear();

private:
  T sum_;
  int32_t num_hash_;
  int32_t num_group_;
  int32_t nbits_;
  T ***
      arr1_; //三维矩阵:
             // num_hash*num_group*(nbits_+1)。对应于论文中的T_{a,b,c}
             //和论文中不一样的是，这里用arr1_[a][b][nbits_]代表论文中定义的T_{a,
             // b, 0}
  T ***arr0_; //三维矩阵:
              // num_hash*num_group*(nbits_)。对应于论文中的T'_{a,b,c}
  hash_t *hash_fns_; // hash funcs
};
template <typename T, typename hash_t, int32_t key_len>
Deltoid<T, hash_t, key_len>::Deltoid(int32_t num_hash, int32_t num_group)
    : num_hash_(num_hash), num_group_(Util::NextPrime(num_group)),
      nbits_(key_len * 8), sum_(0) {

  // allocate continuous memory
  arr1_ = new T **[num_hash_];
  for (int32_t i = 0; i < num_hash_; ++i) {
    arr1_[i] = new T *[num_group_];
  }
  arr1_[0][0] = new T[num_hash_ * num_group_ * (nbits_ + 1)]();
  for (int32_t j = 1; j < num_group_; ++j) {
    arr1_[0][j] = arr1_[0][j - 1] + (nbits_ + 1);
  }
  for (int32_t i = 1; i < num_hash_; ++i) {
    arr1_[i][0] = arr1_[i - 1][0] + num_group_ * (nbits_ + 1);
    for (int32_t j = 1; j < num_group_; ++j) {
      arr1_[i][j] = arr1_[i][j - 1] + (nbits_ + 1);
    }
  }
  arr0_ = new T **[num_hash_];
  for (int32_t i = 0; i < num_hash_; ++i) {
    arr0_[i] = new T *[num_group_];
  }
  arr0_[0][0] = new T[num_hash_ * num_group_ * nbits_]();
  for (int32_t j = 1; j < num_group_; ++j) {
    arr0_[0][j] = arr0_[0][j - 1] + nbits_;
  }
  for (int32_t i = 1; i < num_hash_; ++i) {
    arr0_[i][0] = arr0_[i - 1][0] + num_group_ * nbits_;
    for (int32_t j = 1; j < num_group_; ++j) {
      arr0_[i][j] = arr0_[i][j - 1] + nbits_;
    }
  }
  // hash functions
  hash_fns_ = new hash_t[num_hash_];
}

template <typename T, typename hash_t, int32_t key_len>
Deltoid<T, hash_t, key_len>::Deltoid(const Deltoid &rhs)
    : sum_(rhs.sum_), num_hash_(rhs.num_hash_), num_group_(rhs.num_group_),
      nbits_(rhs.nbits_) {

  arr1_ = new T **[num_hash_];
  for (int32_t i = 0; i < num_hash_; ++i) {
    arr1_[i] = new T *[num_group_];
  }
  arr1_[0][0] = new T[num_hash_ * num_group_ * (nbits_ + 1)]();
  for (int32_t j = 1; j < num_group_; ++j) {
    arr1_[0][j] = arr1_[0][j - 1] + (nbits_ + 1);
  }
  for (int32_t i = 1; i < num_hash_; ++i) {
    arr1_[i][0] = arr1_[i - 1][0] + num_group_ * (nbits_ + 1);
    for (int32_t j = 1; j < num_group_; ++j) {
      arr1_[i][j] = arr1_[i][j - 1] + (nbits_ + 1);
    }
  }
  std::copy(rhs.arr1_[0][0],
            rhs.arr1_[0][0] + num_hash_ * num_group_ * (nbits_ + 1),
            arr1_[0][0]);

  arr0_ = new T **[num_hash_];
  for (int32_t i = 0; i < num_hash_; ++i) {
    arr0_[i] = new T *[num_group_];
  }
  arr0_[0][0] = new T[num_hash_ * num_group_ * nbits_]();
  for (int32_t j = 1; j < num_group_; ++j) {
    arr0_[0][j] = arr0_[0][j - 1] + nbits_;
  }
  for (int32_t i = 1; i < num_hash_; ++i) {
    arr0_[i][0] = arr0_[i - 1][0] + num_group_ * nbits_;
    for (int32_t j = 1; j < num_group_; ++j) {
      arr0_[i][j] = arr0_[i][j - 1] + nbits_;
    }
  }
  std::copy(rhs.arr0_[0][0], rhs.arr0_[0][0] + num_hash_ * num_group_ * nbits_,
            arr0_[0][0]);

  hash_fns_ = new hash_t[num_hash_];
  std::copy(rhs.hash_fns_, rhs.hash_fns_ + num_hash_, hash_fns_);
}

template <typename T, typename hash_t, int32_t key_len>
Deltoid<T, hash_t, key_len>::Deltoid(Deltoid &&rhs) noexcept
    : sum_(rhs.sum_), num_hash_(rhs.num_hash_), num_group_(rhs.num_group_),
      nbits_(rhs.nbits_) {
  arr0_ = rhs.arr0_;
  rhs.arr0_ = nullptr;
  arr1_ = rhs.arr1_;
  rhs.arr1_ = nullptr;
  hash_fns_ = rhs.hash_fns_;
  rhs.hash_fns_ = nullptr;
}

template <typename T, typename hash_t, int32_t key_len>
Deltoid<T, hash_t, key_len> &
Deltoid<T, hash_t, key_len>::operator=(Deltoid rhs) noexcept {
  rhs.swap(*this);
  return *this;
}

template <typename T, typename hash_t, int32_t key_len>
void Deltoid<T, hash_t, key_len>::swap(Deltoid &rhs) noexcept {
  using std::swap;
  swap(sum_, rhs.sum_);
  swap(num_hash_, rhs.num_hash_);
  swap(num_group_, rhs.num_group_);
  swap(nbits_, rhs.nbits_);
  swap(arr0_, rhs.arr0_);
  swap(arr1_, rhs.arr1_);
  swap(hash_fns_, rhs.hash_fns_);
}

template <typename T, typename hash_t, int32_t key_len>
Deltoid<T, hash_t, key_len>::~Deltoid() {
  if (arr1_ != nullptr) {
    delete[] arr1_[0][0];
    for (int32_t i = 0; i < num_hash_; ++i) {
      delete[] arr1_[i];
    }
    delete[] arr1_;
  }
  if (arr0_ != nullptr) {
    delete[] arr0_[0][0];
    for (int32_t i = 0; i < num_hash_; ++i) {
      delete[] arr0_[i];
    }
    delete[] arr0_;
  }
  delete[] hash_fns_;
}

template <typename T, typename hash_t, int32_t key_len>
void Deltoid<T, hash_t, key_len>::update(const FlowKey<key_len> &flowkey,
                                         const T val) {
  sum_ += val;
  for (int32_t i = 0; i < num_hash_; ++i) {
    int32_t idx = hash_fns_[i](flowkey) % num_group_;
    for (int32_t j = 0; j < nbits_; ++j) {
      if (flowkey.getBit(j)) {
        arr1_[i][idx][j] += val;
      } else {
        arr0_[i][idx][j] += val;
      }
    }
    arr1_[i][idx][nbits_] += val;
  }
}

template <typename T, typename hash_t, int32_t key_len>
T Deltoid<T, hash_t, key_len>::query(const FlowKey<key_len> &flowkey) const {
  T min_val = std::numeric_limits<T>::max();
  for (int32_t i = 0; i < num_hash_; ++i) {
    int32_t idx = hash_fns_[i](flowkey) % num_group_;
    for (int32_t j = 0; j < nbits_; ++j) {
      if (flowkey.getBit(j)) {
        min_val = std::min(min_val, arr1_[i][idx][j]);
      } else {
        min_val = std::min(min_val, arr0_[i][idx][j]);
      }
    }
  }
  return min_val;
}

template <typename T, typename hash_t, int32_t key_len>
std::map<FlowKey<key_len>, T>
Deltoid<T, hash_t, key_len>::heavyHitters(T threshold) const {
  double val1 = 0;
  double val0 = 0;
  std::map<FlowKey<key_len>, T> heavy_hitters;
  for (int32_t i = 0; i < num_hash_; i++) {
    for (int32_t j = 0; j < num_group_; j++) {
      // val1 = static_cast<double>(
      //     std::abs(arr1_[i][j][nbits_] - other.arr1_[i][j][nbits_]));
      if (arr1_[i][j][nbits_] <= threshold) { //在这个组中没有heavyhitter
        continue;
      }
      FlowKey<key_len> fk{}; //建一个全为0的flowkey
      bool reject = false;
      for (int32_t k = 0; k < nbits_; k++) {

        bool t1 = (arr1_[i][j][k] > threshold);
        bool t0 = (arr0_[i][j][k] > threshold);
        if (t1 == t0) {
          reject = true;
          break;
        }
        if (t1) {
          fk.setBit(k, true);
        }
      }
      if (reject) {
        continue;
      } else if (heavy_hitters.find(fk) == heavy_hitters.end()) {
        T esti_val = query(fk);
        heavy_hitters.emplace(fk, esti_val);
      }
    }
  }
  return heavy_hitters;
}

template <typename T, typename hash_t, int32_t key_len>
std::map<FlowKey<key_len>, T>
Deltoid<T, hash_t, key_len>::heavyChangers(T threshold,
                                           const Deltoid &other) const {
  double val1 = 0;
  double val0 = 0;
  std::map<FlowKey<key_len>, T> heavy_changers;
  for (int32_t i = 0; i < num_hash_; i++) {
    for (int32_t j = 0; j < num_group_; j++) {
      val1 = static_cast<double>(
          std::abs(arr1_[i][j][nbits_] - other.arr1_[i][j][nbits_]));
      if (val1 <= threshold) { //在这个组中没有heavyChanger
        continue;
      }
      FlowKey<key_len> fk{}; //建一个全为0的flowkey
      bool reject = false;
      for (int k = 0; k < nbits_; k++) {
        val1 = static_cast<double>(
            std::abs(arr1_[i][j][k] - other.arr1_[i][j][k]));
        val0 = static_cast<double>(
            std::abs(arr0_[i][j][k] - other.arr0_[i][j][k]));
        bool t1 = (val1 > threshold);
        bool t0 = (val0 > threshold);
        if (t1 == t0) {
          reject = true;
          break;
        }
        if (t1) {
          fk.setBit(k, true);
        }
      }
      if (reject) {
        continue;
      } else if (heavy_changers.find(fk) == heavy_changers.end()) {
        T esti_val = std::abs(query(fk) - other.query(fk));
        heavy_changers.emplace(fk, esti_val);
      }
    }
  }
  return heavy_changers;
}

template <typename T, typename hash_t, int32_t key_len>
size_t Deltoid<T, hash_t, key_len>::size() const {
  return sizeof(Deltoid<T, hash_t, key_len>) +
         (2 * num_group_ * num_hash_ * nbits_ + 1) * sizeof(T) +
         num_hash_ * sizeof(hash_t);
}

template <typename T, typename hash_t, int32_t key_len>
void Deltoid<T, hash_t, key_len>::clear() {
  sum_ = 0;
  std::fill(arr0_[0][0], arr0_[0][0] + num_hash_ * num_group_ * nbits_, 0);
  std::fill(arr1_[0][0], arr1_[0][0] + num_hash_ * num_group_ * (nbits_ + 1),
            0);
}

template <typename T, typename hash_t, int32_t key_len>
void swap(Deltoid<T, hash_t, key_len> &lhs, Deltoid<T, hash_t, key_len> &rhs) {
  lhs.swap(rhs);
}
} // namespace SketchLab

#endif