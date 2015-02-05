//
//  util.h
//  moka
//
//  Created by Firas Abuzaid on 1/25/15.
//  Copyright (c) 2015 Hazy Research. All rights reserved.
//

#ifndef moka_util_h
#define moka_util_h

#include <iostream>
#include <math.h>
#include <random>
#include <float.h>
#include <limits>

enum InitializerType {
  CONSTANT = 0, // for now, CONSTANT assumes that it's always initialized to zero
  BERNOULLI = 1,
  XAVIER = 2,
  GAUSSIAN = 3
};

using std::max;
using std::min;
using std::random_device;
using std::mt19937;
using std::uniform_real_distribution;
using std::bernoulli_distribution;
using std::normal_distribution;
using std::string;

typedef float  DataType_SFFloat; /*< Single-precision Floating Point. */
typedef short  DataType_FPFloat; /*< 16-bit Fixed Point. */
typedef int    DataType_Int; /*< 32-bit integer. */
typedef string DataType_String; /*< String-type data only for deubgging/unit testing. */

#define NOT_IMPLEMENTED std::cerr << "ERROR: Using a bridge with unsupported Layout or DataType." << std::endl; assert(false)

class Util {
  public:

    // memcpy doesn't inline with clang++/g++, so we use this instead
    // (Shamelessly stolen from https://software.intel.com/en-us/articles/memcpy-performance)
    static inline void * _our_memcpy(void *b, const void *a, size_t n) {
      char *s1 = (char*) b;
      const char *s2 = (const char*)a;
      for(; 0<n; --n)*s1++ = *s2++;
      return b;
    }

    // Same as above: memset doesn't inline with g++, so we use this instead
    static inline void * _our_memset(void *b, const int value, size_t n) {
#ifdef _DO_ASSERT
      assert(value >= -1); // memset will not work correctly if the value is less than -1
#endif
      char * s1 = (char *) b;
      const unsigned char val = (const unsigned char) value;
      for(; 0<n; --n)*s1++ = val;
      return b;
    }

    // Same as above: memset doesn't inline with g++, so we use this instead
    //static inline void * _our_memset(void *b, const float value, size_t n) {
    //}

    template <typename T>
    static inline void xavier_initialize(T * const arr, const size_t n_arr_elements, const size_t n_batch) {
      mt19937 gen(rd());

      const size_t fan_in = n_arr_elements / n_batch;
      const float scale = sqrt(T(3) / fan_in);
      uniform_real_distribution<T> uni(-scale, scale);
      for (size_t i = 0; i < n_arr_elements; ++i) {
        arr[i] = uni(gen);
      }
    }

    template <typename T>
    static inline void bernoulli_initialize(unsigned int * const arr, const size_t n_arr_elements, const T p) {
      mt19937 gen(rd());

      bernoulli_distribution bern(p);
      for (size_t i = 0; i < n_arr_elements; ++i) {
        arr[i] = bern(gen);
      }
    }

    template <typename T>
    static inline void gaussian_initialize(unsigned int * const arr, const size_t n_arr_elements, const T mean, const T std_dev) {
      mt19937 gen(rd());

      normal_distribution<T> gaussian(mean, std_dev);
      for (size_t i = 0; i < n_arr_elements; ++i) {
        arr[i] = gaussian(gen);
      }
    }

    // Note: this is only used for shorts and floats, since _our_memset will only work for ints
    template <typename T>
    static inline void constant_initialize(T * const arr, const T value, const size_t n_arr_elements) {
      for(size_t i = 0; i < n_arr_elements; ++i)
        arr[i] = value;
    }

  private:
    static random_device rd;
};

#endif
