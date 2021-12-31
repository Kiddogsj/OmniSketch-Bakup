//
// Created by Ferric on 2020/12/29.
//

#ifndef SKETCHLAB_CPP_UTIL_H
#define SKETCHLAB_CPP_UTIL_H

namespace SketchLab {
namespace Util {

const int MANGLE_MAGIC = 2083697005;

template <typename T> T Mangle(T key) {
  size_t n = sizeof(T);
  char *s = (char *)&key;
  for (int i = 0; i < (n >> 1); ++i)
    std::swap(s[i], s[n - 1 - i]);

  return key * MANGLE_MAGIC;
}

bool IsPrime(int n) {
  for (int i = 2; i * i <= n; ++i)
    if (n % i == 0)
      return false;
  return true;
}

int NextPrime(int n) {
  while (!IsPrime(n))
    ++n;
  return n;
}

} // namespace Util
} // namespace SketchLab

#endif // SKETCHLAB_CPP_UTIL_H
