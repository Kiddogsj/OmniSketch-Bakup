//
// Created by Ferric on 2020/12/29.
//

#ifndef SKETCHLAB_CPP_HASH_H
#define SKETCHLAB_CPP_HASH_H

#include "FlowKey.h"
#include "util.h"

#include <cstdint>
#include <cstdlib>
#include <ctime>

namespace SketchLab {
namespace Hash {

// class BaseHash {
// public:
//   virtual uint64_t operator()(const unsigned char *data, int n) const = 0;
// };

class AwareHash {
  uint64_t init;
  uint64_t scale;
  uint64_t hardener;

public:
  static void random_seed() { srand((unsigned)time(NULL)); }
  AwareHash() {
    static const int GEN_INIT_MAGIC = 388650253;
    static const int GEN_SCALE_MAGIC = 388650319;
    static const int GEN_HARDENER_MAGIC = 1176845762;
    static int index = 0;
    static uint64_t seed = 0;
    seed = rand();
    static AwareHash gen_hash(GEN_INIT_MAGIC, GEN_SCALE_MAGIC,
                              GEN_HARDENER_MAGIC);

    uint64_t mangled;
    mangled = Util::Mangle(seed + (index++));
    init = gen_hash((unsigned char *)&mangled, sizeof(uint64_t));
    mangled = Util::Mangle(seed + (index++));
    scale = gen_hash((unsigned char *)&mangled, sizeof(uint64_t));
    mangled = Util::Mangle(seed + (index++));
    hardener = gen_hash((unsigned char *)&mangled, sizeof(uint64_t));
  }

  AwareHash(uint64_t init, uint64_t scale, uint64_t hardener)
      : init(init), scale(scale), hardener(hardener) {}

  uint64_t operator()(const uint8_t *data, int n) const {
    uint64_t result = init;
    while (n--) {
      result *= scale;
      result += *data++;
    }
    return result ^ hardener;
  }
  template <int32_t key_len>
  uint64_t operator()(const FlowKey<key_len> &flowkey) const {
    return this->operator()(flowkey.cKey(), key_len);
  }
  uint64_t operator()(const uint32_t val) {
    uint64_t result = init;
    for (int i = 0; i < 4; ++i) {
      result *= scale;
      result += (val >> (8 * i)) & 0xFF;
    }
    return result ^ hardener;
  }
};

/*
 * MurmurHash2, 64-bit versions
 * modified from MurmurHash2_64.cpp, https://sites.google.com/site/murmurhash/
 *
 * Caveats:
 * 1. alignment
 * 2. little-endian-ness
 *
 */
class MurmurHash {
  const uint64_t mul_magic;
  const uint64_t scramble_magic;
  const uint64_t seed_magic;

public:
  static void random_seed() { srand((unsigned)time(NULL)); }
  MurmurHash()
      : mul_magic(0xc6a4a7935bd1e995UL), scramble_magic(47),
        seed_magic(rand()) {}

  uint64_t operator()(const uint8_t *key, const int len) const {
    uint64_t hash_val = seed_magic ^ (len * mul_magic);

    const uint64_t *data = (const uint64_t *)key;
    const uint64_t *end = data + (len / 8);

    // every 8 bytes
    // only on little-endian platform
    while (data != end) {
      uint64_t tmp = *data++;

      tmp *= mul_magic;
      tmp ^= tmp >> scramble_magic;
      tmp *= mul_magic;

      hash_val ^= tmp;
      hash_val *= mul_magic;
    }

    // trailing bytes
    const uint8_t *data2 = (const uint8_t *)data;

    switch (len & 7) {
    case 7:
      hash_val ^= ((uint64_t)data2[6]) << 48;
    case 6:
      hash_val ^= ((uint64_t)data2[5]) << 40;
    case 5:
      hash_val ^= ((uint64_t)data2[4]) << 32;
    case 4:
      hash_val ^= ((uint64_t)data2[3]) << 24;
    case 3:
      hash_val ^= ((uint64_t)data2[2]) << 16;
    case 2:
      hash_val ^= ((uint64_t)data2[1]) << 8;
    case 1:
      hash_val ^= ((uint64_t)data2[0]);
    default:
      break;
    }

    hash_val ^= hash_val >> scramble_magic;
    hash_val *= mul_magic;
    hash_val ^= hash_val >> scramble_magic;

    return hash_val;
  }

  template <int32_t key_len>
  uint64_t operator()(const FlowKey<key_len> &flowkey) const {
    return this->operator()(flowkey.cKey(), key_len);
  }

  uint64_t operator()(const uint32_t val) const {
    return this->operator()((const uint8_t *)&val, 4);
  }
};

class BOBHash32 {
private:
    static const int MAX_BIG_PRIME32 = 50;
    static const int MAX_PRIME32 = 1229;
    uint32_t big_prime3232[MAX_BIG_PRIME32] = {
    20177, 20183, 20201, 20219, 20231, 20233, 20249, 20261, 20269, 20287,
    20297, 20323, 20327, 20333, 20341, 20347, 20353, 20357, 20359, 20369,
    20389, 20393, 20399, 20407, 20411, 20431, 20441, 20443, 20477, 20479,
    20483, 20507, 20509, 20521, 20533, 20543, 20549, 20551, 20563, 20593,
    20599, 20611, 20627, 20639, 20641, 20663, 20681, 20693, 20707, 20717};
    uint32_t prime32[MAX_PRIME32] = {
    2,    3,    5,    7,    11,   13,   17,   19,   23,   29,   31,   37,
    41,   43,   47,   53,   59,   61,   67,   71,   73,   79,   83,   89,
    97,   101,  103,  107,  109,  113,  127,  131,  137,  139,  149,  151,
    157,  163,  167,  173,  179,  181,  191,  193,  197,  199,  211,  223,
    227,  229,  233,  239,  241,  251,  257,  263,  269,  271,  277,  281,
    283,  293,  307,  311,  313,  317,  331,  337,  347,  349,  353,  359,
    367,  373,  379,  383,  389,  397,  401,  409,  419,  421,  431,  433,
    439,  443,  449,  457,  461,  463,  467,  479,  487,  491,  499,  503,
    509,  521,  523,  541,  547,  557,  563,  569,  571,  577,  587,  593,
    599,  601,  607,  613,  617,  619,  631,  641,  643,  647,  653,  659,
    661,  673,  677,  683,  691,  701,  709,  719,  727,  733,  739,  743,
    751,  757,  761,  769,  773,  787,  797,  809,  811,  821,  823,  827,
    829,  839,  853,  857,  859,  863,  877,  881,  883,  887,  907,  911,
    919,  929,  937,  941,  947,  953,  967,  971,  977,  983,  991,  997,
    1009, 1013, 1019, 1021, 1031, 1033, 1039, 1049, 1051, 1061, 1063, 1069,
    1087, 1091, 1093, 1097, 1103, 1109, 1117, 1123, 1129, 1151, 1153, 1163,
    1171, 1181, 1187, 1193, 1201, 1213, 1217, 1223, 1229, 1231, 1237, 1249,
    1259, 1277, 1279, 1283, 1289, 1291, 1297, 1301, 1303, 1307, 1319, 1321,
    1327, 1361, 1367, 1373, 1381, 1399, 1409, 1423, 1427, 1429, 1433, 1439,
    1447, 1451, 1453, 1459, 1471, 1481, 1483, 1487, 1489, 1493, 1499, 1511,
    1523, 1531, 1543, 1549, 1553, 1559, 1567, 1571, 1579, 1583, 1597, 1601,
    1607, 1609, 1613, 1619, 1621, 1627, 1637, 1657, 1663, 1667, 1669, 1693,
    1697, 1699, 1709, 1721, 1723, 1733, 1741, 1747, 1753, 1759, 1777, 1783,
    1787, 1789, 1801, 1811, 1823, 1831, 1847, 1861, 1867, 1871, 1873, 1877,
    1879, 1889, 1901, 1907, 1913, 1931, 1933, 1949, 1951, 1973, 1979, 1987,
    1993, 1997, 1999, 2003, 2011, 2017, 2027, 2029, 2039, 2053, 2063, 2069,
    2081, 2083, 2087, 2089, 2099, 2111, 2113, 2129, 2131, 2137, 2141, 2143,
    2153, 2161, 2179, 2203, 2207, 2213, 2221, 2237, 2239, 2243, 2251, 2267,
    2269, 2273, 2281, 2287, 2293, 2297, 2309, 2311, 2333, 2339, 2341, 2347,
    2351, 2357, 2371, 2377, 2381, 2383, 2389, 2393, 2399, 2411, 2417, 2423,
    2437, 2441, 2447, 2459, 2467, 2473, 2477, 2503, 2521, 2531, 2539, 2543,
    2549, 2551, 2557, 2579, 2591, 2593, 2609, 2617, 2621, 2633, 2647, 2657,
    2659, 2663, 2671, 2677, 2683, 2687, 2689, 2693, 2699, 2707, 2711, 2713,
    2719, 2729, 2731, 2741, 2749, 2753, 2767, 2777, 2789, 2791, 2797, 2801,
    2803, 2819, 2833, 2837, 2843, 2851, 2857, 2861, 2879, 2887, 2897, 2903,
    2909, 2917, 2927, 2939, 2953, 2957, 2963, 2969, 2971, 2999, 3001, 3011,
    3019, 3023, 3037, 3041, 3049, 3061, 3067, 3079, 3083, 3089, 3109, 3119,
    3121, 3137, 3163, 3167, 3169, 3181, 3187, 3191, 3203, 3209, 3217, 3221,
    3229, 3251, 3253, 3257, 3259, 3271, 3299, 3301, 3307, 3313, 3319, 3323,
    3329, 3331, 3343, 3347, 3359, 3361, 3371, 3373, 3389, 3391, 3407, 3413,
    3433, 3449, 3457, 3461, 3463, 3467, 3469, 3491, 3499, 3511, 3517, 3527,
    3529, 3533, 3539, 3541, 3547, 3557, 3559, 3571, 3581, 3583, 3593, 3607,
    3613, 3617, 3623, 3631, 3637, 3643, 3659, 3671, 3673, 3677, 3691, 3697,
    3701, 3709, 3719, 3727, 3733, 3739, 3761, 3767, 3769, 3779, 3793, 3797,
    3803, 3821, 3823, 3833, 3847, 3851, 3853, 3863, 3877, 3881, 3889, 3907,
    3911, 3917, 3919, 3923, 3929, 3931, 3943, 3947, 3967, 3989, 4001, 4003,
    4007, 4013, 4019, 4021, 4027, 4049, 4051, 4057, 4073, 4079, 4091, 4093,
    4099, 4111, 4127, 4129, 4133, 4139, 4153, 4157, 4159, 4177, 4201, 4211,
    4217, 4219, 4229, 4231, 4241, 4243, 4253, 4259, 4261, 4271, 4273, 4283,
    4289, 4297, 4327, 4337, 4339, 4349, 4357, 4363, 4373, 4391, 4397, 4409,
    4421, 4423, 4441, 4447, 4451, 4457, 4463, 4481, 4483, 4493, 4507, 4513,
    4517, 4519, 4523, 4547, 4549, 4561, 4567, 4583, 4591, 4597, 4603, 4621,
    4637, 4639, 4643, 4649, 4651, 4657, 4663, 4673, 4679, 4691, 4703, 4721,
    4723, 4729, 4733, 4751, 4759, 4783, 4787, 4789, 4793, 4799, 4801, 4813,
    4817, 4831, 4861, 4871, 4877, 4889, 4903, 4909, 4919, 4931, 4933, 4937,
    4943, 4951, 4957, 4967, 4969, 4973, 4987, 4993, 4999, 5003, 5009, 5011,
    5021, 5023, 5039, 5051, 5059, 5077, 5081, 5087, 5099, 5101, 5107, 5113,
    5119, 5147, 5153, 5167, 5171, 5179, 5189, 5197, 5209, 5227, 5231, 5233,
    5237, 5261, 5273, 5279, 5281, 5297, 5303, 5309, 5323, 5333, 5347, 5351,
    5381, 5387, 5393, 5399, 5407, 5413, 5417, 5419, 5431, 5437, 5441, 5443,
    5449, 5471, 5477, 5479, 5483, 5501, 5503, 5507, 5519, 5521, 5527, 5531,
    5557, 5563, 5569, 5573, 5581, 5591, 5623, 5639, 5641, 5647, 5651, 5653,
    5657, 5659, 5669, 5683, 5689, 5693, 5701, 5711, 5717, 5737, 5741, 5743,
    5749, 5779, 5783, 5791, 5801, 5807, 5813, 5821, 5827, 5839, 5843, 5849,
    5851, 5857, 5861, 5867, 5869, 5879, 5881, 5897, 5903, 5923, 5927, 5939,
    5953, 5981, 5987, 6007, 6011, 6029, 6037, 6043, 6047, 6053, 6067, 6073,
    6079, 6089, 6091, 6101, 6113, 6121, 6131, 6133, 6143, 6151, 6163, 6173,
    6197, 6199, 6203, 6211, 6217, 6221, 6229, 6247, 6257, 6263, 6269, 6271,
    6277, 6287, 6299, 6301, 6311, 6317, 6323, 6329, 6337, 6343, 6353, 6359,
    6361, 6367, 6373, 6379, 6389, 6397, 6421, 6427, 6449, 6451, 6469, 6473,
    6481, 6491, 6521, 6529, 6547, 6551, 6553, 6563, 6569, 6571, 6577, 6581,
    6599, 6607, 6619, 6637, 6653, 6659, 6661, 6673, 6679, 6689, 6691, 6701,
    6703, 6709, 6719, 6733, 6737, 6761, 6763, 6779, 6781, 6791, 6793, 6803,
    6823, 6827, 6829, 6833, 6841, 6857, 6863, 6869, 6871, 6883, 6899, 6907,
    6911, 6917, 6947, 6949, 6959, 6961, 6967, 6971, 6977, 6983, 6991, 6997,
    7001, 7013, 7019, 7027, 7039, 7043, 7057, 7069, 7079, 7103, 7109, 7121,
    7127, 7129, 7151, 7159, 7177, 7187, 7193, 7207, 7211, 7213, 7219, 7229,
    7237, 7243, 7247, 7253, 7283, 7297, 7307, 7309, 7321, 7331, 7333, 7349,
    7351, 7369, 7393, 7411, 7417, 7433, 7451, 7457, 7459, 7477, 7481, 7487,
    7489, 7499, 7507, 7517, 7523, 7529, 7537, 7541, 7547, 7549, 7559, 7561,
    7573, 7577, 7583, 7589, 7591, 7603, 7607, 7621, 7639, 7643, 7649, 7669,
    7673, 7681, 7687, 7691, 7699, 7703, 7717, 7723, 7727, 7741, 7753, 7757,
    7759, 7789, 7793, 7817, 7823, 7829, 7841, 7853, 7867, 7873, 7877, 7879,
    7883, 7901, 7907, 7919, 7927, 7933, 7937, 7949, 7951, 7963, 7993, 8009,
    8011, 8017, 8039, 8053, 8059, 8069, 8081, 8087, 8089, 8093, 8101, 8111,
    8117, 8123, 8147, 8161, 8167, 8171, 8179, 8191, 8209, 8219, 8221, 8231,
    8233, 8237, 8243, 8263, 8269, 8273, 8287, 8291, 8293, 8297, 8311, 8317,
    8329, 8353, 8363, 8369, 8377, 8387, 8389, 8419, 8423, 8429, 8431, 8443,
    8447, 8461, 8467, 8501, 8513, 8521, 8527, 8537, 8539, 8543, 8563, 8573,
    8581, 8597, 8599, 8609, 8623, 8627, 8629, 8641, 8647, 8663, 8669, 8677,
    8681, 8689, 8693, 8699, 8707, 8713, 8719, 8731, 8737, 8741, 8747, 8753,
    8761, 8779, 8783, 8803, 8807, 8819, 8821, 8831, 8837, 8839, 8849, 8861,
    8863, 8867, 8887, 8893, 8923, 8929, 8933, 8941, 8951, 8963, 8969, 8971,
    8999, 9001, 9007, 9011, 9013, 9029, 9041, 9043, 9049, 9059, 9067, 9091,
    9103, 9109, 9127, 9133, 9137, 9151, 9157, 9161, 9173, 9181, 9187, 9199,
    9203, 9209, 9221, 9227, 9239, 9241, 9257, 9277, 9281, 9283, 9293, 9311,
    9319, 9323, 9337, 9341, 9343, 9349, 9371, 9377, 9391, 9397, 9403, 9413,
    9419, 9421, 9431, 9433, 9437, 9439, 9461, 9463, 9467, 9473, 9479, 9491,
    9497, 9511, 9521, 9533, 9539, 9547, 9551, 9587, 9601, 9613, 9619, 9623,
    9629, 9631, 9643, 9649, 9661, 9677, 9679, 9689, 9697, 9719, 9721, 9733,
    9739, 9743, 9749, 9767, 9769, 9781, 9787, 9791, 9803, 9811, 9817, 9829,
    9833, 9839, 9851, 9857, 9859, 9871, 9883, 9887, 9901, 9907, 9923, 9929,
    9931, 9941, 9949, 9967, 9973};
    uint32_t prime32Num_;
    inline uint32_t mix(uint32_t a, uint32_t b, uint32_t c) const {
      a -= b;
      a -= c;
      a ^= (c >> 13);
      b -= c;
      b -= a;
      b ^= (a << 8);
      c -= a;
      c -= b;
      c ^= (b >> 13);
      a -= b;
      a -= c;
      a ^= (c >> 12);
      b -= c;
      b -= a;
      b ^= (a << 16);
      c -= a;
      c -= b;
      c ^= (b >> 5);
      a -= b;
      a -= c;
      a ^= (c >> 3);
      b -= c;
      b -= a;
      b ^= (a << 10);
      c -= a;
      c -= b;
      c ^= (b >> 15);
      return c;
    }

public:
  static void random_seed() { srand((unsigned)time(NULL)); };
  BOBHash32() { prime32Num_ = rand() % MAX_PRIME32; }
  ~BOBHash32() {};
  uint32_t operator()(const uint8_t *data, int n) const {
    // register ub4 a,b,c,len;
    uint32_t a, b, c;
    //	uint32_t initval = 0;
    /* Set up the internal state */
    // len = length;
    a = b = 0x9e3779b9;       /* the golden ratio; an arbitrary value */
    c = prime32[prime32Num_]; /* the previous hash value */

    /*---------------------------------------- handle most of the key */
    while (n >= 12) {
      a += (data[0] + ((uint32_t)data[1] << 8) + ((uint32_t)data[2] << 16) +
          ((uint32_t)data[3] << 24));
      b += (data[4] + ((uint32_t)data[5] << 8) + ((uint32_t)data[6] << 16) +
          ((uint32_t)data[7] << 24));
      c += (data[8] + ((uint32_t)data[9] << 8) + ((uint32_t)data[10] << 16) +
          ((uint32_t)data[11] << 24));
      c = mix(a, b, c);
      data += 12;
      n -= 12;
    }

    /*------------------------------------- handle the last 11 bytes */
    c += n;
    switch (n) /* all the case statements fall through */
    {
    case 11:
      c += ((uint32_t)data[10] << 24);
      // fall through
    case 10:
      c += ((uint32_t)data[9] << 16);
      // fall through
    case 9:
      c += ((uint32_t)data[8] << 8);
      /* the first byte of c is reserved for the length */
      // fall through
    case 8:
      b += ((uint32_t)data[7] << 24);
      // fall through
    case 7:
      b += ((uint32_t)data[6] << 16);
      // fall through
    case 6:
      b += ((uint32_t)data[5] << 8);
      // fall through
    case 5:
      b += data[4];
      // fall through
    case 4:
      a += ((uint32_t)data[3] << 24);
      // fall through
    case 3:
      a += ((uint32_t)data[2] << 16);
      // fall through
    case 2:
      a += ((uint32_t)data[1] << 8);
      // fall through
    case 1:
      a += data[0];
      /* case 0: nothing left to add */
    }
    c = mix(a, b, c);
    /*-------------------------------------------- report the result */
    return c;
  }
  template <int32_t key_len>
  uint32_t operator()(const FlowKey<key_len> &flowkey) const {
    return this->operator()(flowkey.cKey(), key_len);
  }

  uint64_t operator()(const uint32_t val) const {
    return this->operator()((const uint8_t *)&val, 4);
  }
};

class DJBHash {
  
public:
  DJBHash() {}
  ~DJBHash() {}
  uint64_t operator()(const uint8_t *key, const int len) const {
    uint64_t hash = 5381;
    int c = 0;
    int pos = 0;
    while(pos < len) {
      c = *(key+pos);
      hash = ((hash << 5) + hash) + c;
    }
    return hash;
  }

  template <int32_t key_len>
  uint64_t operator()(const FlowKey<key_len> &flowkey) const {
    return this->operator()(flowkey.cKey(), key_len);
  }

  uint64_t operator()(const uint32_t val) const {
    return this->operator()((const uint8_t *)&val, 4);
  }
};

class CRCHash {
  static const uint16_t CRC16_INITIAL_VALUE = 0xFFFF;
  static const uint16_t P_16 = 0xA001;
  static const uint32_t TOPBIT = (((uint32_t)1) << 31);
  uint64_t seed = 0;
  const uint16_t crc16_ansi_tab[256] = {        
    0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
    0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
    0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
    0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
    0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
    0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
    0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
    0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
    0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
    0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
    0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
    0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
    0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
    0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
    0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
    0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
    0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
    0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
    0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
    0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
    0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
    0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
    0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
    0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
    0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
    0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
    0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
    0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
    0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
    0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
    0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
    0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040,};
  const uint16_t crc16_ccitt_tab[256]= {
    0x0000,0x1021,0x2042,0x3063,0x4084,0x50a5,0x60c6,0x70e7,
    0x8108,0x9129,0xa14a,0xb16b,0xc18c,0xd1ad,0xe1ce,0xf1ef,
    0x1231,0x0210,0x3273,0x2252,0x52b5,0x4294,0x72f7,0x62d6,
    0x9339,0x8318,0xb37b,0xa35a,0xd3bd,0xc39c,0xf3ff,0xe3de,
    0x2462,0x3443,0x0420,0x1401,0x64e6,0x74c7,0x44a4,0x5485,
    0xa56a,0xb54b,0x8528,0x9509,0xe5ee,0xf5cf,0xc5ac,0xd58d,
    0x3653,0x2672,0x1611,0x0630,0x76d7,0x66f6,0x5695,0x46b4,
    0xb75b,0xa77a,0x9719,0x8738,0xf7df,0xe7fe,0xd79d,0xc7bc,
    0x48c4,0x58e5,0x6886,0x78a7,0x0840,0x1861,0x2802,0x3823,
    0xc9cc,0xd9ed,0xe98e,0xf9af,0x8948,0x9969,0xa90a,0xb92b,
    0x5af5,0x4ad4,0x7ab7,0x6a96,0x1a71,0x0a50,0x3a33,0x2a12,
    0xdbfd,0xcbdc,0xfbbf,0xeb9e,0x9b79,0x8b58,0xbb3b,0xab1a,
    0x6ca6,0x7c87,0x4ce4,0x5cc5,0x2c22,0x3c03,0x0c60,0x1c41,
    0xedae,0xfd8f,0xcdec,0xddcd,0xad2a,0xbd0b,0x8d68,0x9d49,
    0x7e97,0x6eb6,0x5ed5,0x4ef4,0x3e13,0x2e32,0x1e51,0x0e70,
    0xff9f,0xefbe,0xdfdd,0xcffc,0xbf1b,0xaf3a,0x9f59,0x8f78,
    0x9188,0x81a9,0xb1ca,0xa1eb,0xd10c,0xc12d,0xf14e,0xe16f,
    0x1080,0x00a1,0x30c2,0x20e3,0x5004,0x4025,0x7046,0x6067,
    0x83b9,0x9398,0xa3fb,0xb3da,0xc33d,0xd31c,0xe37f,0xf35e,
    0x02b1,0x1290,0x22f3,0x32d2,0x4235,0x5214,0x6277,0x7256,
    0xb5ea,0xa5cb,0x95a8,0x8589,0xf56e,0xe54f,0xd52c,0xc50d,
    0x34e2,0x24c3,0x14a0,0x0481,0x7466,0x6447,0x5424,0x4405,
    0xa7db,0xb7fa,0x8799,0x97b8,0xe75f,0xf77e,0xc71d,0xd73c,
    0x26d3,0x36f2,0x0691,0x16b0,0x6657,0x7676,0x4615,0x5634,
    0xd94c,0xc96d,0xf90e,0xe92f,0x99c8,0x89e9,0xb98a,0xa9ab,
    0x5844,0x4865,0x7806,0x6827,0x18c0,0x08e1,0x3882,0x28a3,
    0xcb7d,0xdb5c,0xeb3f,0xfb1e,0x8bf9,0x9bd8,0xabbb,0xbb9a,
    0x4a75,0x5a54,0x6a37,0x7a16,0x0af1,0x1ad0,0x2ab3,0x3a92,
    0xfd2e,0xed0f,0xdd6c,0xcd4d,0xbdaa,0xad8b,0x9de8,0x8dc9,
    0x7c26,0x6c07,0x5c64,0x4c45,0x3ca2,0x2c83,0x1ce0,0x0cc1,
    0xef1f,0xff3e,0xcf5d,0xdf7c,0xaf9b,0xbfba,0x8fd9,0x9ff8,
    0x6e17,0x7e36,0x4e55,0x5e74,0x2e93,0x3eb2,0x0ed1,0x1ef0};
  uint16_t crc16_ansi(const uint8_t *ptr, int length) const {
    uint16_t  crc16 = CRC16_INITIAL_VALUE;
    for (int i = 0; i < length; i++) {
	  crc16 = (crc16 >> 8) ^ crc16_ansi_tab[(crc16 ^ *ptr++) & 0x00ff];
    }
    return crc16;
  }
  uint16_t crc16_ccitt(const uint8_t *buf, int len) const {
    uint16_t crc = 0;
    for (int counter = 0; counter < len; counter++)
            crc = (crc<<8) ^ crc16_ccitt_tab[((crc>>8) ^ *buf++)&0x00FF];
    return crc;
  }
  uint16_t hash1(const uint8_t *buf, int len) const {
	  return crc16_ccitt(buf, len);
  }
  uint16_t hash2(const uint8_t *buf, int len) const {
	  return crc16_ansi(buf, len);
  }
  void mangle(const unsigned char* key, unsigned char* ret_key, int nbytes) const {
	  for (int i=0; i<nbytes; ++i) {
		  ret_key[i] = key[nbytes-i-1];
	  }

    if (nbytes == 13) {
	    ret_key[0] = key[5];
	    ret_key[1] = key[11];
		  ret_key[2] = key[7];
	    ret_key[3] = key[6];
	    ret_key[4] = key[1];
		  ret_key[5] = key[9];
		  ret_key[6] = key[10];
	    ret_key[7] = key[4];
	    ret_key[8] = key[2];
	    ret_key[9] = key[8];
		  ret_key[10] = key[12];
		  ret_key[11] = key[0];
	    ret_key[12] = key[3];
    }
  }
  void unmangle(const unsigned char* key, unsigned char* ret_key, int nbytes) const {
	  for (int i=0; i<nbytes; ++i) {
		  ret_key[i] = key[nbytes-i-1];
	  }

    if (nbytes == 13) {
		  ret_key[0] = key[11];
		  ret_key[1] = key[4];
		  ret_key[2] = key[8];
		  ret_key[3] = key[12];
		  ret_key[4] = key[7];
		  ret_key[5] = key[0];
		  ret_key[6] = key[3];
		  ret_key[7] = key[2];
		  ret_key[8] = key[9];
		  ret_key[9] = key[5];
		  ret_key[10] = key[6];
		  ret_key[11] = key[1];
		  ret_key[12] = key[10];
    }
  }
  uint64_t AwareHash(unsigned char* data, uint64_t n,
        uint64_t hash, uint64_t scale, uint64_t hardener) const {

	  while (n) {
		  hash *= scale;
		  hash += *data++;
		  n--;
	  }
	  return hash ^ hardener;
  }
  uint64_t AwareHash_debug(unsigned char* data, uint64_t n,
        uint64_t hash, uint64_t scale, uint64_t hardener) const {

	  while (n) {
      fprintf(stderr, "    %lu %lu %lu %u\n", n, hash, scale, *data);
		  hash *= scale;
		  hash += *data++;
		  n--;
      fprintf(stderr, "        internal %lu\n", hash);
	  }
	  return hash ^ hardener;
  }
  int is_prime(int num) const {
    int i;
    for (i=2; i<num; i++) {
        if ((num % i) == 0) {
            break;
        }
    }
    if (i == num) {
        return 1;
    }
    return 0;
  }
  uint64_t GenHashSeed(int index) {
    if (seed == 0) {
        seed = rand();
    }
    uint64_t x, y = seed + index;
    mangle((const unsigned char*)&y, (unsigned char*)&x, 8);
    return AwareHash((uint8_t*)&y, 8, 388650253, 388650319, 1176845762);
  }
  int calc_next_prime(int num) const {
    while (!is_prime(num)) {
        num++;
    }
    return num;
  }
  uint32_t reflect(uint32_t VF_dato, uint8_t VF_nBits) const {
    uint32_t VP_reflection = 0;

    for (uint8_t VP_Pos_bit = 0; VP_Pos_bit < VF_nBits; VP_Pos_bit++) {
        if ((VF_dato & 1) == 1) {
            VP_reflection |= (((uint32_t)1) << ((VF_nBits - 1) - VP_Pos_bit));
        }
        VF_dato = (VF_dato >> 1);
    }
    return (VP_reflection);
  }
  inline uint8_t reflect_DATA(uint32_t _DATO) const { return ((uint8_t)(reflect((_DATO), 8)&0xFF)); }
  inline uint32_t reflect_CRCTableValue(uint32_t _CRCTableValue) const { return ((uint32_t) reflect((_CRCTableValue), 32)); }
  uint32_t crc_ObtenValorDeTabla_Reversed(uint8_t VP_Pos_Tabla, uint32_t polynomial) const {
    uint32_t VP_CRCTableValue = 0;

    VP_CRCTableValue = ((uint32_t) reflect_DATA(VP_Pos_Tabla)) << 24;

    for (uint8_t VP_Pos_bit = 0; VP_Pos_bit < 8; VP_Pos_bit++) {
        if (VP_CRCTableValue & TOPBIT) {
            VP_CRCTableValue = (VP_CRCTableValue << 1) ^ polynomial;
        } else {
            VP_CRCTableValue = (VP_CRCTableValue << 1);
        }
    }
    return (reflect_CRCTableValue(VP_CRCTableValue));
  }
  uint32_t crc_body_reversed_true(const uint8_t* data, uint16_t len, uint32_t code, uint32_t init, uint32_t final_xor) const {
    for (int16_t VP_bytes = 0; VP_bytes < len; VP_bytes++) {
        init = (init >> 8) ^ crc_ObtenValorDeTabla_Reversed(((uint8_t)(init & 0xFF)) ^ data[VP_bytes], code);
    }

    return (init ^ final_xor);
  }
  // uint32_t crc_body_reversed_true(const uint8_t* data, uint16_t len, uint32_t code, uint32_t init, uint32_t final_xor) const {
  //   for (int16_t VP_bytes = 0; VP_bytes < len; VP_bytes++) {
  //       init = (init >> 8) ^ crc_ObtenValorDeTabla_Reversed(((uint8_t)(init & 0xFF)) ^ data[VP_bytes], code);
  //   }

  //   return (init ^ final_xor);
  // }
  uint32_t crc32(const uint8_t* data, uint16_t len) const {
    return crc_body_reversed_true(data, len, 0x04C11DB7, 0xFFFFFFFF, 0xFFFFFFFF);
  }
  uint32_t crc_32c(const uint8_t* data, uint16_t len) const {
    return crc_body_reversed_true(data, len, 0x1EDC6F41, 0xFFFFFFFF, 0xFFFFFFFF);
  }
  uint32_t crc_32d(const uint8_t* data, uint16_t len) const {
    return crc_body_reversed_true(data, len, 0xA833982B, 0xFFFFFFFF, 0xFFFFFFFF);
  }
  uint32_t jamcrc(const uint8_t* data, uint16_t len) const {
    return crc_body_reversed_true(data, len, 0x04C11DB7, 0xFFFFFFFF, 0);
  }
  uint32_t crc_ObtenValorDeTabla(const uint8_t VP_Pos_Tabla, uint32_t polynomial) const {
    uint32_t VP_CRCTableValue = ((uint32_t) VP_Pos_Tabla) << 24;

    for (uint8_t VP_Pos_bit = 0; VP_Pos_bit < 8; VP_Pos_bit++) {
        if (VP_CRCTableValue & TOPBIT) {
            VP_CRCTableValue = (VP_CRCTableValue << 1) ^ polynomial;
        } else {
            VP_CRCTableValue = (VP_CRCTableValue << 1);
        }
    }
    return (VP_CRCTableValue);
  }

  uint32_t crc_body_reversed_false(const uint8_t* data, uint16_t len, uint32_t code, uint32_t init, uint32_t final_xor) const {
    for (int16_t VP_bytes = 0; VP_bytes < len; VP_bytes++) {
        init = (init << 8) ^ crc_ObtenValorDeTabla(((uint8_t)((init >> 24) & 0xFF)) ^ data[VP_bytes], code);
    }

    return (init ^ final_xor); 
  }
  uint32_t xfer(const uint8_t* data, uint16_t len) const {
    return crc_body_reversed_false(data, len, 0xAF, 0, 0);
  }
  uint32_t posix(const uint8_t* data, uint16_t len) const {
    return crc_body_reversed_false(data, len, 0x04C11DB7, 0, 0xFFFFFFFF);
  }
  uint32_t crc_32_bzip2(const uint8_t* data, uint16_t len) const {
    return crc_body_reversed_false(data, len, 0x04C11DB7, 0xFFFFFFFF, 0xFFFFFFFF);
  }
  uint32_t crc_32_mpeg(const uint8_t* data, uint16_t len) const {
    return crc_body_reversed_false(data, len, 0x04C11DB7, 0xFFFFFFFF, 0);
  }
  uint32_t crc_32q(const uint8_t* data, uint16_t len) const {
    return crc_body_reversed_false(data, len, 0x814141AB, 0, 0);
  }

public:
  CRCHash() {}
  ~CRCHash() {}
  uint32_t operator()(int hashid, const uint8_t* data, uint16_t len) const {
    switch (hashid) {
        case 0: return crc32(data, len);
        case 1: return crc_32c(data, len);
        case 2: return crc_32d(data, len);
        case 3: return crc_32q(data, len);
        case 4: return crc_32_bzip2(data, len);
        case 5: return crc_32_mpeg(data, len);
        case 6: return posix(data, len);
        case 7: return xfer(data, len);
        case 8: return jamcrc(data, len);
        default: fprintf(stderr, "Unsupport hash id: %d.\n", hashid);
    }
  }
  template <int32_t key_len>
  uint32_t operator()(int hashid, const FlowKey<key_len> &flowkey) const {
    return this->operator()(hashid, flowkey.cKey(), key_len);
  }

  uint32_t operator()(int hashid, const uint32_t val) const {
    return this->operator()(hashid, (const uint8_t *)&val, 4);
  }
};

} // namespace Hash
} // namespace SketchLab

#endif // SKETCHLAB_CPP_HASH_H
