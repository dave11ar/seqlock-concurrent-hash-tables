// Tests concurrent inserts, deletes, updates, and finds. The test makes sure
// that multiple operations are not run on the same key, so that the accuracy of
// the operations can be verified.

#include <string>
#include <string_view>
#include <gtest/gtest.h>

#include "test_utils.hpp"

using KeyType = uint32_t; 
using KeyType2 = big_object<5>;
using ValType = uint32_t;
using ValType2 = big_object<10>;

// The number of keys to size the table with, expressed as a power of
// 2. This can be set with the command line flag --power
size_t g_power = 25;
size_t g_numkeys; // Holds 2^power
// The number of threads spawned for each type of operation. This can
// be set with the command line flag --thread-num
size_t g_thread_num = 4;
// Whether to disable inserts or not. This can be set with the command
// line flag --disable-inserts
bool g_disable_inserts = false;
// Whether to disable deletes or not. This can be set with the command
// line flag --disable-deletes
bool g_disable_deletes = false;
// Whether to disable updates or not. This can be set with the command
// line flag --disable-updates
bool g_disable_updates = false;
// Whether to disable finds or not. This can be set with the command
// line flag --disable-finds
bool g_disable_finds = false;
// How many seconds to run the test for. This can be set with the
// command line flag --time
size_t g_test_len = 10;
// The seed for the random number generator. If this isn't set to a
// nonzero value with the --seed flag, the current time is used
size_t g_seed = 0;
// Whether to use big_objects as the value and key
bool g_use_big_objects = false;

std::atomic<size_t> num_inserts;
std::atomic<size_t> num_deletes;
std::atomic<size_t> num_updates;
std::atomic<size_t> num_finds;

template <class KType, template <typename, typename> typename Map>
class AllEnvironment {
public:
  AllEnvironment()
      : table(64), table2(64), keys(g_numkeys), vals(g_numkeys),
        vals2(g_numkeys), in_table(new bool[g_numkeys]),
        in_use(new std::atomic_flag[g_numkeys]),
        val_dist(std::numeric_limits<uint32_t>::min(),
                 std::numeric_limits<uint32_t>::max()),
        val_dist2(std::numeric_limits<uint32_t>::min(),
                  std::numeric_limits<uint32_t>::max()),
        ind_dist(0, g_numkeys - 1), finished(false) {
    //table.max_num_worker_threads(16);
    // Sets up the random number generator
    if (g_seed == 0) {
      g_seed = std::chrono::system_clock::now().time_since_epoch().count();
    }
    std::cout << "seed = " << g_seed << std::endl;
    gen_seed = g_seed;
    // Fills in all the vectors except vals, which will be filled
    // in by the insertion threads.
    for (size_t i = 0; i < g_numkeys; i++) {
      keys[i] = generateKey<KType>(i);
      in_table[i] = false;
      in_use[i].clear();
    }
  }

  Map<KType, ValType> table;
  Map<KType, ValType2> table2;
  std::vector<KType> keys;
  std::vector<ValType> vals;
  std::vector<ValType2> vals2;
  std::unique_ptr<bool[]> in_table;
  std::unique_ptr<std::atomic_flag[]> in_use;
  std::uniform_int_distribution<uint32_t> val_dist;
  std::uniform_int_distribution<uint32_t> val_dist2;
  std::uniform_int_distribution<size_t> ind_dist;
  size_t gen_seed;
  // When set to true, it signals to the threads to stop running
  std::atomic<bool> finished;
};

template <class KType, template <typename, typename> typename Map>
void stress_insert_thread(AllEnvironment<KType, Map> *env) {
  std::mt19937 gen(env->gen_seed);
  while (!env->finished.load()) {
    // Pick a random number between 0 and g_numkeys. If that slot is
    // not in use, lock the slot. Insert a random value into both
    // tables. The inserts should only be successful if the key
    // wasn't in the table. If the inserts succeeded, check that
    // the insertion were actually successful with another find
    // operation, and then store the values in their arrays and
    // set in_table to true and clear in_use
    size_t ind = env->ind_dist(gen);
    if (!env->in_use[ind].test_and_set()) {
      KType k = env->keys[ind];
      ValType v = env->val_dist(gen);
      ValType2 v2 = env->val_dist2(gen);
      bool res = env->table.insert(k, v);
      bool res2 = env->table2.insert(k, v2);

      ASSERT_NE(res, env->in_table[ind]);
      ASSERT_NE(res2, env->in_table[ind]);
      if (res) {
        ASSERT_EQ(v, env->table.find(k));
        ASSERT_EQ(v2, env->table2.find(k));
        env->vals[ind] = v;
        env->vals2[ind] = v2;
        env->in_table[ind] = true;
        num_inserts.fetch_add(2, std::memory_order_relaxed);
      }
      env->in_use[ind].clear();
    }
  }
}

template <class KType, template <typename, typename> typename Map> 
void delete_thread(AllEnvironment<KType, Map> *env) {
  std::mt19937 gen(env->gen_seed);
  while (!env->finished.load()) {
    // Run deletes on a random key, check that the deletes
    // succeeded only if the keys were in the table. If the
    // deletes succeeded, check that the keys are indeed not in
    // the tables anymore, and then set in_table to false
    size_t ind = env->ind_dist(gen);
    if (!env->in_use[ind].test_and_set()) {
      KType k = env->keys[ind];
      bool res = env->table.erase(k);
      bool res2 = env->table2.erase(k);
      ASSERT_EQ(res, env->in_table[ind]);
      ASSERT_EQ(res2, env->in_table[ind]);
      if (res) {
        ValType find_v = 0;
        ValType2 find_v2 = 0;
        ASSERT_FALSE(env->table.find(k, find_v));
        ASSERT_FALSE(env->table2.find(k, find_v2));
        env->in_table[ind] = false;
        num_deletes.fetch_add(2, std::memory_order_relaxed);
      }
      env->in_use[ind].clear();
    }
  }
}

template <class KType, template <typename, typename> typename Map>
void update_thread(AllEnvironment<KType, Map> *env) {
  std::mt19937 gen(env->gen_seed);
  std::uniform_int_distribution<size_t> third(0, 2);
  auto updatefn = [](ValType &v) { v += 3; };
  auto updatefn2 = [](ValType2 &v) { v += 10; };
  while (!env->finished.load()) {
    // Run updates, update_fns, or upserts on a random key, check
    // that the operations succeeded only if the keys were in the
    // table (or that they succeeded regardless if it's an
    // upsert). If successful, check that the keys are indeed in
    // the table with the new value, and then set in_table to true
    size_t ind = env->ind_dist(gen);
    if (!env->in_use[ind].test_and_set()) {
      KType k = env->keys[ind];
      ValType v;
      ValType2 v2;
      bool res, res2;
      switch (third(gen)) {
      case 0:
        // update
        v = env->val_dist(gen);
        v2 = env->val_dist2(gen);
        res = env->table.update(k, v);
        res2 = env->table2.update(k, v2);
        ASSERT_EQ(res, env->in_table[ind]);
        ASSERT_EQ(res2, env->in_table[ind]);
        break;
      case 1:
        // update_fn
        v = env->vals[ind];
        v2 = env->vals2[ind];
        updatefn(v);
        updatefn2(v2);
        res = env->table.update_fn(k, updatefn);
        res2 = env->table2.update_fn(k, updatefn2);
        ASSERT_EQ(res, env->in_table[ind]);
        ASSERT_EQ(res2, env->in_table[ind]);
        break;
      case 2:
        // upsert
        if (env->in_table[ind]) {
          // Then it should run updatefn
          v = env->vals[ind];
          v2 = env->vals2[ind];
          updatefn(v);
          updatefn2(v2);
        } else {
          // Then it should run an insert
          v = env->val_dist(gen);
          v2 = env->val_dist2(gen);
        }
        // These upserts should always succeed, so set res and res2 to
        // true.
        env->table.upsert(k, updatefn, v);
        env->table2.upsert(k, updatefn2, v2);
        res = res2 = true;
        env->in_table[ind] = true;
        break;
      default:
        throw std::logic_error("Impossible");
      }
      if (res) {
        ASSERT_EQ(v, env->table.find(k));
        ASSERT_EQ(v2, env->table2.find(k));
        env->vals[ind] = v;
        env->vals2[ind] = v2;
        num_updates.fetch_add(2, std::memory_order_relaxed);
      }
      env->in_use[ind].clear();
    }
  }
}

template <class KType, template <typename, typename> typename Map>
void find_thread(AllEnvironment<KType, Map> *env) {
  std::mt19937 gen(env->gen_seed);
  while (!env->finished.load()) {
    // Run finds on a random key and check that the presence of
    // the keys matches in_table
    size_t ind = env->ind_dist(gen);
    if (!env->in_use[ind].test_and_set()) {
      KType k = env->keys[ind];
      try {
        ASSERT_EQ(env->vals[ind], env->table.find(k));
        ASSERT_TRUE(env->in_table[ind]);
      } catch (const std::out_of_range &) {
        ASSERT_FALSE(env->in_table[ind]);
      }
      try {
        ASSERT_EQ(env->vals2[ind], env->table2.find(k));
        ASSERT_TRUE(env->in_table[ind]);
      } catch (const std::out_of_range &) {
        ASSERT_FALSE(env->in_table[ind]);
      }
      num_finds.fetch_add(2, std::memory_order_relaxed);
      env->in_use[ind].clear();
    }
  }
}

// Spawns g_thread_num insert, delete, update, and find threads
template <class KType, template <typename, typename> typename Map>
void StressTest(AllEnvironment<KType, Map> *env) {
  std::vector<std::thread> threads;
  for (size_t i = 0; i < g_thread_num; i++) {
    if (!g_disable_inserts) {
      threads.emplace_back(stress_insert_thread<KType, Map>, env);
    }
    if (!g_disable_deletes) {
      threads.emplace_back(delete_thread<KType, Map>, env);
    }
    if (!g_disable_updates) {
      threads.emplace_back(update_thread<KType, Map>, env);
    }
    if (!g_disable_finds) {
      threads.emplace_back(find_thread<KType, Map>, env);
    }
  }
  // Sleeps before ending the threads
  std::this_thread::sleep_for(std::chrono::seconds(g_test_len));
  env->finished.store(true);
  for (size_t i = 0; i < threads.size(); i++) {
    threads[i].join();
  }
  // Finds the number of slots that are filled
  size_t numfilled = 0;
  for (size_t i = 0; i < g_numkeys; i++) {
    if (env->in_table[i]) {
      numfilled++;
    }
  }
  ASSERT_EQ(numfilled, env->table.size());
  std::cout << "----------Results----------" << std::endl;
  std::cout << "Number of inserts:\t" << num_inserts.load() << std::endl;
  std::cout << "Number of deletes:\t" << num_deletes.load() << std::endl;
  std::cout << "Number of updates:\t" << num_updates.load() << std::endl;
  std::cout << "Number of finds:\t" << num_finds.load() << std::endl;
}

template <template <typename, typename> typename Map>
void RunStressTests(std::string_view sw) {
  std::cout << sw << std::endl;

  num_inserts.store(0);
  num_deletes.store(0);
  num_updates.store(0);
  num_finds.store(0);

  if (g_use_big_objects) {
    auto *env = new AllEnvironment<KeyType2, Map>;
    StressTest(env);
    delete env;
  } else {
    auto *env = new AllEnvironment<KeyType, Map>;
    StressTest(env);
    delete env;
  }
}

int main(int argc, char **argv) {
  const char *args[] = {"--power", "--thread-num", "--time", "--seed"};
  size_t *arg_vars[] = {&g_power, &g_thread_num, &g_test_len, &g_seed};
  const char *arg_help[] = {
      "The number of keys to size the table with, expressed as a power of 2",
      "The number of threads to spawn for each type of operation",
      "The number of seconds to run the test for",
      "The seed for the random number generator"};
  const char *flags[] = {"--disable-inserts", "--disable-deletes",
                         "--disable-updates", "--disable-finds",
                         "--use-big-objects"};
  bool *flag_vars[] = {&g_disable_inserts, &g_disable_deletes,
                       &g_disable_updates, &g_disable_finds, &g_use_big_objects};
  const char *flag_help[] = {
      "If set, no inserts will be run", "If set, no deletes will be run",
      "If set, no updates will be run", "If set, no finds will be run",
      "If set, the key and value type will be big_object"};
  parse_flags(argc, argv, "Runs a stress test on inserts, deletes, and finds",
              args, arg_vars, arg_help, sizeof(args) / sizeof(const char *),
              flags, flag_vars, flag_help,
              sizeof(flags) / sizeof(const char *));
  g_numkeys = 1U << g_power;

  RunStressTests<cuckoo_wrapper>("Testing cuckoohash_map");
  RunStressTests<robin_hood_wrapper>("Testing rhhash_map");
  
  return main_return_value;
}
