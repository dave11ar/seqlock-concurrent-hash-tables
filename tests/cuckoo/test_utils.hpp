#pragma once

// Utilities for running stress tests and benchmarks
#include <array>
#include <atomic>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <mutex>
#include <random>

#include "cuckoo/cuckoohash_map.hpp"
#include "rh/rhhash_map.hpp"

std::mutex print_lock;
int main_return_value = EXIT_SUCCESS;
typedef std::lock_guard<std::mutex> mutex_guard;

// Parses boolean flags and flags with positive integer arguments
void parse_flags(int argc, char **argv, const char *description,
                 const char *args[], size_t *arg_vars[], const char *arg_help[],
                 size_t arg_num, const char *flags[], bool *flag_vars[],
                 const char *flag_help[], size_t flag_num) {

  errno = 0;
  for (int i = 0; i < argc; i++) {
    for (size_t j = 0; j < arg_num; j++) {
      if (strcmp(argv[i], args[j]) == 0) {
        if (i == argc - 1) {
          std::cerr << "You must provide a positive integer argument"
                    << " after the " << args[j] << " argument" << std::endl;
          exit(EXIT_FAILURE);
        } else {
          size_t argval = strtoull(argv[i + 1], NULL, 10);
          if (errno != 0) {
            std::cerr << "The argument to " << args[j]
                      << " must be a valid size_t" << std::endl;
            exit(EXIT_FAILURE);
          } else {
            *(arg_vars[j]) = argval;
          }
        }
      }
    }
    for (size_t j = 0; j < flag_num; j++) {
      if (strcmp(argv[i], flags[j]) == 0) {
        *(flag_vars[j]) = true;
      }
    }
    if (strcmp(argv[i], "--help") == 0) {
      std::cerr << description << std::endl;
      std::cerr << "Arguments:" << std::endl;
      for (size_t j = 0; j < arg_num; j++) {
        std::cerr << args[j] << " (default " << *arg_vars[j] << "):\t"
                  << arg_help[j] << std::endl;
      }
      for (size_t j = 0; j < flag_num; j++) {
        std::cerr << flags[j] << " (default "
                  << (*flag_vars[j] ? "true" : "false") << "):\t"
                  << flag_help[j] << std::endl;
      }
      exit(0);
    }
  }
}

// generateKey is a function from a number to another given type, used to
// generate keys for insertion.
template <class T> T generateKey(size_t i) { return (T)i; }
// This specialization returns a stringified representation of the given
// integer, where the number is copied to the end of a long string of 'a's, in
// order to make comparisons and hashing take time.
template <> std::string generateKey<std::string>(size_t n) {
  const size_t min_length = 100;
  const std::string num(std::to_string(n));
  if (num.size() >= min_length) {
    return num;
  }
  std::string ret(min_length, 'a');
  const size_t startret = min_length - num.size();
  for (size_t i = 0; i < num.size(); i++) {
    ret[i + startret] = num[i];
  }
  return ret;
}

// An overloaded class that does the inserts for different table types. Inserts
// with a value of 0.
template <class Table> class insert_thread {
public:
  typedef typename std::vector<typename Table::key_type>::iterator it_t;
  static void func(Table &table, it_t begin, it_t end) {
    for (; begin != end; begin++) {
      ASSERT_TRUE(table.insert(*begin, 0));
    }
  }
};

// An overloaded class that does the reads for different table types. It
// repeatedly searches for the keys in the given range until the time is up. All
// the keys we're searching for should either be in the table or not in the
// table, so we assert that.
template <class Table> class read_thread {
public:
  typedef typename std::vector<typename Table::key_type>::iterator it_t;
  static void func(Table &table, it_t begin, it_t end,
                   std::atomic<size_t> &counter, bool in_table,
                   std::atomic<bool> &finished) {
    typename Table::mapped_type v;
    // We keep track of our own local counter for reads, to avoid
    // over-burdening the shared atomic counter
    size_t reads = 0;
    while (!finished.load(std::memory_order_acquire)) {
      for (auto it = begin; it != end; it++) {
        if (finished.load(std::memory_order_acquire)) {
          counter.fetch_add(reads);
          return;
        }
        ASSERT_EQ(in_table, table.find(*it, v));
        reads++;
      }
    }
  }
};

// An overloaded class that does a mixture of reads and inserts for different
// table types. It repeatedly searches for the keys in the given range until
// everything has been inserted.
template <class Table> class read_insert_thread {
public:
  typedef typename std::vector<typename Table::key_type>::iterator it_t;
  static void func(Table &table, it_t begin, it_t end,
                   std::atomic<size_t> &counter, const double insert_prob,
                   const size_t start_seed) {
    typename Table::mapped_type v;
    std::mt19937_64 gen(start_seed);
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    auto inserter_it = begin;
    auto reader_it = begin;
    size_t ops = 0;
    while (inserter_it != end) {
      if (dist(gen) < insert_prob) {
        // Do an insert
        ASSERT_TRUE(table.insert(*inserter_it, 0));
        ++inserter_it;
      } else {
        // Do a read
        ASSERT_EQ(table.find(*reader_it, v), (reader_it < inserter_it));
        ++reader_it;
        if (reader_it == end) {
          reader_it = begin;
        }
      }
      ++ops;
    }
    counter.fetch_add(ops);
  }
};

template <size_t size>
class big_object {
  std::array<uint64_t, size> a;

public:
  big_object() : big_object(0) {}

  big_object(uint32_t value) {
    for (size_t i = 0; i < size; ++i) {
      a[i] = value + i;
    }
  }

  explicit operator uint32_t() {
    return a[0];
  }

  bool operator<=(const big_object& other) const {
    return a[0] < other.a[0];
  }

  big_object& operator+=(uint32_t v) {
    for (size_t i = 0; i < size; ++i) {
      a[i] += v;
    }
    return *this;
  }

  big_object& operator-=(uint32_t v) {
    for (size_t i = 0; i < size; ++i) {
      a[i] -= v;
    }
    return *this;
  }

  big_object& operator++() {
    for (size_t i = 0; i < size; ++i) {
      ++a[i];
    }
    return *this;
  }
  big_object operator++(int) {
    big_object result(*this);
    ++result;
    return result;
  }

  bool operator==(const big_object& other) const = default;
  bool operator!=(const big_object& other) const = default;

  template <typename Outstream>
  friend Outstream& operator<<(Outstream& out, const big_object& object) {
    for (size_t i = 0; i < size; ++i) {
      out << object.a[i] << " ";
    }
    return out;
  }
  
  friend std::hash<big_object<size>>;
};

namespace std {
template <size_t size>
struct hash<big_object<size>> {
  size_t operator()(const big_object<size>& object) const {
    return std::hash<uint64_t>()(object.a[0]);
  }
};
}  // namespace std

template <typename Key, typename Value>
class cuckoo_wrapper : public seqlock_lib::cuckoo::cuckoohash_map<Key, Value> {
  using base = seqlock_lib::cuckoo::cuckoohash_map<Key, Value>;
  public:
  using base::base;
};

template <typename Key, typename Value>
class robin_hood_wrapper : public seqlock_lib::rh::rhhash_map<Key, Value> {
  using base = seqlock_lib::rh::rhhash_map<Key, Value>;
  public:
  using base::base;
};