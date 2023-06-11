#pragma once

#include <cstdint>
#include <vector>
#include <iostream>

#include "atomic_buffer.hpp"
#include "rhhash_config.hpp"
#include "rh_bucket_container.hpp"
#include "seqlock.hpp"
#include "utils.hpp"

namespace seqlock_lib::rh {

template <class Key, class T, class Hash = std::hash<Key>,
          class KeyEqual = std::equal_to<Key>,
          class Allocator = std::allocator<std::pair<const Key, T>>,
          std::size_t SLOT_PER_BUCKET = DEFAULT_SLOT_PER_BUCKET>
class rhhash_map {
  static_assert(std::is_trivially_copyable_v<Key> && std::is_trivially_copyable_v<T>, 
    "Key and value should be trivially copyable due to seqlock requirements");
private:
  // The type of the buckets container
  using buckets_t = rh_bucket_container<Key, T, Allocator, SLOT_PER_BUCKET>;

public:
  // The type of the bucket
  using bucket_t = typename buckets_t::typed_bucket;

  /** @name Type Declarations */
  /**@{*/
  using key_type = typename buckets_t::key_type;
  using mapped_type = typename buckets_t::mapped_type;

  /**
   * This type is defined as an @c std::pair. Note that table behavior is
   * undefined if a user-defined specialization of @c std::pair<Key, T> or @c
   * std::pair<const Key, T> exists.
   */
  using value_type = typename buckets_t::value_type;
  using size_type = typename buckets_t::size_type;
  using difference_type = std::ptrdiff_t;
  using hasher = Hash;
  using key_equal = KeyEqual;
  using allocator_type = typename buckets_t::allocator_type;
  using reference = typename buckets_t::reference;
  using const_reference = typename buckets_t::const_reference;
  using pointer = typename buckets_t::pointer;
  using const_pointer = typename buckets_t::const_pointer;

private:
  enum class table_mode : bool {
    LOCKED,
    UNLOCKED
  };  
  
  enum class collect_epochs_status : uint16_t {
    RETRY,
    FOUND,
    NOT_FOUND
  };

  enum class cycle_status: uint16_t {
    OUT_OF_WINDOW,
    NOT_OCCUPIED,
    LESS_DIST,
    EQUAL
  };

  struct cycle_checks {
    bool out_of_window = false;
    bool not_occupied = false;
    bool less_dist = false;
    bool equal = false;
  };

  using lock_manager_t = std::unique_ptr<bucket_t, LockDeleter<bucket_t>>;
  using lock_list_t = std::vector<lock_manager_t>;

  using epoch_array_t = std::vector<seqlock_epoch_t>;

  struct AllUnlocker {
    void operator()(rhhash_map* map) const {
      for (bucket_t& bucket : map->buckets_) {
        bucket.unlock();
      }
    }
  };
  using AllLocksManager = std::unique_ptr<rhhash_map, AllUnlocker>;

  using bucket_iterator = typename buckets_t::iterator;

  struct rh_data {
    bucket_iterator it;

    const size_type original_index;

    const size_t hv;
    const int16_t hp;

    const uint16_t window_size;
    uint16_t dist;
    uint16_t slot;

    bool out_of_window() const {
      return dist > window_size;
    }
  };

  struct rh_path_data {
    bucket_iterator it;

    uint16_t current_dist;
    uint16_t dist;
    uint16_t slot;
  };
public:
  rhhash_map(size_type n = DEFAULT_SIZE, const Hash &hf = Hash(),
             const KeyEqual &equal = KeyEqual(),
             const Allocator &alloc = Allocator())
      : hash_fn_(hf), eq_fn_(equal), 
        buckets_(reserve_calc_for_slots<SLOT_PER_BUCKET>(n + MAX_WINDOW_SIZE + 1), alloc){}

  size_type size() const {
    counter_type s = 0;
    for (bucket_t& bucket : buckets_) {
      s += bucket.elem_counter();
    }
    assert(s >= 0);
    return static_cast<size_type>(s);
  }

  template <typename K, typename... Args> bool insert(K&& key, Args&& ...val)  {
    return insert_fn([](mapped_type&) {}, std::forward<K>(key), std::forward<Args>(val)...);
  }

  template <typename K, typename Val> bool insert_or_assign(K&& key, Val&& val)  {
    return insert_fn([&val](mapped_type& value) {
      mapped_type v(std::forward<K>(val));
      atomic_store_memcpy(&value, v);
    }, std::forward<K>(key), std::forward<K>(val));
  }
  template <typename K> bool insert_or_assign(K&& key, const mapped_type& val)  {
    return insert_fn([&val](mapped_type& value) {
      atomic_store_memcpy(&value, val);
    }, std::forward<K>(key), val);
  }

  template <operation_mode OPERATION_MODE = operation_mode::SAFE, typename K, typename F, typename... Args>
  bool upsert(K&& key, F fn, Args&& ...val) {
    return insert_fn([&fn](mapped_type& value) {
      update_safely<OPERATION_MODE>(value, fn);
    }, std::forward<K>(key), std::forward<Args>(val)...);
  }

  template <operation_mode OPERATION_MODE = operation_mode::SAFE, typename K, typename F> 
  bool update_fn(const K &key, F fn) {
    static constexpr cycle_checks update_fn_checks {
      .out_of_window = true,
      .not_occupied = true,
      .less_dist = true,
      .equal = true
    };
    
    while (true) {
      rh_data data = get_rh_data(key);

      lock_list_t locks;
      locks.reserve(data.window_size);
      if (!lock_first(data, locks)) {
        continue;
      }

      switch (abstract_cycle<table_mode::UNLOCKED, update_fn_checks>(key, data, locks)) {
        case cycle_status::OUT_OF_WINDOW:
        case cycle_status::NOT_OCCUPIED:
        case cycle_status::LESS_DIST:
          return false;
        case cycle_status::EQUAL:
          update_safely<OPERATION_MODE>(data.it->mapped(data.slot), fn);
          return true;
      }
    }
  }

  template <typename K, typename V> bool update(const K& key, V&& val) {
    return update_fn<operation_mode::SAFE>(key, [&val](mapped_type &v) { v = std::forward<V>(val); });
  }

  template <typename K> bool update(const K &key, const mapped_type& val) {
    return update_fn<operation_mode::UNSAFE>(key, [&val](mapped_type &v) { atomic_store_memcpy(&v, val); });
  }

  template <typename K> bool find(const K& key, mapped_type& val) const {
    while (true) {
      rh_data data = get_rh_data(key);

      epoch_array_t epochs;
      epochs.reserve(data.window_size);
      collect_epochs_status status = collect_epochs(key, data, epochs);

      if (status == collect_epochs_status::RETRY) {
        continue;
      }

      typename bucket_t::mapped_raw_type storage;
      if (status == collect_epochs_status::FOUND) {
        atomic_load_memcpy(reinterpret_cast<mapped_type*>(&storage), data.it->mapped(data.slot));
      }
      std::atomic_thread_fence(std::memory_order_acquire);

      if (!check_epochs(data, epochs)) {
        continue;
      }

      if (status == collect_epochs_status::FOUND) {
        val = *reinterpret_cast<mapped_type*>(&storage);
      }

      return status == collect_epochs_status::FOUND;
    }
  }

  template <typename K> mapped_type find(const K &key) const {
    mapped_type value;
    if (find(key, value)) {
      return value;
    } 
    
    throw std::out_of_range("key not found in table");
  }

  template <typename K> bool erase(const K& key) {
    static constexpr cycle_checks erase_checks {
      .out_of_window = true,
      .not_occupied = true,
      .less_dist = true,
      .equal = true
    };

    while (true) {
      rh_data data = get_rh_data(key);

      lock_list_t locks;
      locks.reserve(data.window_size);
      if (!lock_first(data, locks)) {
        continue;
      }

      switch (abstract_cycle<table_mode::UNLOCKED, erase_checks>(key, data, locks)) {
        case cycle_status::OUT_OF_WINDOW:
        case cycle_status::NOT_OCCUPIED:
        case cycle_status::LESS_DIST:
          return false;
        case cycle_status::EQUAL:
          del_from_bucket(*data.it, data.slot);
          data.slot = (data.original_index + data.dist) & BUCKET_MASK;
          ++data.dist;
          ++data.slot;

          while (true) {
            for (; data.slot < SLOT_PER_BUCKET; ++data.slot) {
              if (!data.it->occupied(data.slot) ||
                  data.it->dist(data.slot) == 0) {
                return true;
              }
              
              set_on_prev(data.it, data.slot);
            }

            before_next_bucket<table_mode::UNLOCKED>(data, locks);
          }
          return true;
      }
    }
  }

private:
  template <typename K> rh_data get_rh_data(const K& key) const {
    const int16_t hp = hashpower();
    const size_t hv = hash_fn_(key);
    const size_type original_index = get_original_index(hp, hv);
    return {
      .it = buckets_.get_iterator(get_bucket_index(original_index)),
      .original_index = original_index,
      .hv = hv,
      .hp = hp,
      .window_size = calc_window_size(hp),
      .dist = 0,
      .slot = get_slot(original_index)
    };
  }

  rh_path_data get_rh_path_data(const rh_data& data) const {
    return {
      .it = data.it,
      .current_dist = static_cast<uint16_t>(data.it->dist(data.slot) + 1),
      .dist = static_cast<uint16_t>(data.dist + 1),
      .slot = static_cast<uint16_t>(data.slot + 1),
    };
  }

  template <table_mode TABLE_MODE, cycle_checks Checks, typename K, typename Data>
  cycle_status abstract_cycle(const K& key, Data& data, lock_list_t& lock_array) {
    while (true) {
      for (; data.slot < SLOT_PER_BUCKET; ++data.slot, ++data.dist) {
        if constexpr (Checks.out_of_window) {
          if (data.out_of_window()) {
            return cycle_status::OUT_OF_WINDOW;
          }
        }

        if constexpr (Checks.not_occupied) {
          if (!data.it->occupied(data.slot)) {
            return cycle_status::NOT_OCCUPIED;
          }
        }

        if constexpr (Checks.less_dist) {
          if (data.it->dist(data.slot) < data.dist) {
            return cycle_status::LESS_DIST;
          }
        }

        if constexpr (Checks.equal) {
          if (eq_fn_(data.it->key(data.slot), key)) {
            return cycle_status::EQUAL;
          }
        }
      }

      if constexpr (Checks.out_of_window) {
        if (data.out_of_window()) {
          return cycle_status::OUT_OF_WINDOW;
        }
      }

      before_next_bucket<TABLE_MODE>(data, lock_array);
    }
  }

  bool lock_first(rh_data& data, lock_list_t& locks) {
    data.it->lock();
    if (hashpower_changed(data.hp, *data.it)) {
      return false;
    }
    locks.emplace_back(&(*data.it));
    std::atomic_thread_fence(std::memory_order_release);

    return true;
  }

  size_type hashsize() const {
    return buckets_.size() * SLOT_PER_BUCKET_POW;
  }

  int16_t hashpower() const {
    return buckets_.hashpower() + SLOT_PER_BUCKET_POW;
  }

  inline static uint16_t calc_window_size(uint16_t hp) {
    return hp + 1;
  }

  size_type get_original_index(const size_type hp, const size_type hv) const {
    const size_type size_mask = (size_type(1) << hp) - 1;
    const size_type index = hv & size_mask;
    const size_type border = size_mask - MAX_WINDOW_SIZE;
    return index <= border ? index : index - border; 
  }

  static constexpr size_type get_bucket_index(size_type original_index) {
    return original_index >> SLOT_PER_BUCKET_POW;
  }

  static constexpr uint16_t get_slot(size_type index) {
    return index & BUCKET_MASK;
  }

  static bool no_further_data(const rh_data& data, bool occupied, uint16_t dist) {
    return data.out_of_window() ||
           !occupied ||
           dist + 1 < data.dist;
  }

  template <table_mode TABLE_MODE>
  static void before_next_bucket(rh_data& data, lock_list_t& lock_list) {
    ++data.it;
    data.slot = 0;
    if constexpr (TABLE_MODE == table_mode::UNLOCKED) {
      data.it->lock();
      lock_list.emplace_back(&(*data.it));
    }
  }

  bool hashpower_changed(int32_t hp, bucket_t& bucket) const {
    if (hashpower() != hp) {
      bucket.unlock();
      return true;
    }
    return false;
  }

  template <typename K>
  collect_epochs_status collect_epochs(const K& key, rh_data& data, epoch_array_t& epochs) const {
    epochs.push_back(data.it->get_epoch());
    if (seqlock_t::is_locked(epochs.back()) || data.hp != hashpower()) {
      return collect_epochs_status::RETRY;
    }

    while (true) {
      for (; data.slot < SLOT_PER_BUCKET; ++data.slot, ++data.dist) {
        bool cur_occupied;
        uint16_t cur_dist;

        atomic_load_memcpy(&cur_occupied, data.it->occupied(data.slot));
        atomic_load_memcpy(&cur_dist, data.it->dist(data.slot));
        
        if (no_further_data(data, cur_occupied, cur_dist)) {
          return collect_epochs_status::NOT_FOUND;
        }

        typename bucket_t::key_raw_type cur_key;
        atomic_load_memcpy(reinterpret_cast<key_type*>(&cur_key), data.it->key(data.slot));
        if (eq_fn_(*reinterpret_cast<key_type*>(&cur_key), key)) {
          return collect_epochs_status::FOUND;
        }
      }

      if (data.out_of_window()) {
        return collect_epochs_status::NOT_FOUND;
      }

      ++data.it;
      epochs.push_back(data.it->get_epoch());
      if (seqlock_t::is_locked(epochs.back())) {
        return collect_epochs_status::RETRY;
      }
      data.slot = 0;
    }
  }

  bool check_epochs(rh_data& data, const epoch_array_t& epochs) const {
    for (uint16_t i = epochs.size(); i > 0; --i) {
      if (epochs[i - 1] != data.it->get_epoch(std::memory_order_relaxed)) {
        return false;
      }

      if (i != 1) {
        --data.it;
      }
    }

    return true;
  }

  void set_on_prev(bucket_iterator& it, size_type slot) {
    if (slot == 0) {
      bucket_iterator prev(it);
      --prev;

      add_to_bucket(*prev, SLOT_PER_BUCKET - 1, it->dist(slot) - 1, 
          it->key(slot), it->mapped(slot));
      --it->elem_counter();
    } else {
      buckets_.setKV(*it, slot - 1, it->dist(slot) - 1, it->key(slot), it->mapped(slot));
    }
    buckets_.deoccupy(*it, slot);
  }

  template <typename K, typename... Args>
  void add_to_bucket(bucket_t& bucket, const size_type slot,
                     const uint16_t dist, K&& key, Args&& ...val) const {
    buckets_.setKV(bucket, slot, dist, std::forward<K>(key), std::forward<Args>(val)...);
    ++bucket.elem_counter();
  }
  
  void del_from_bucket(bucket_t& bucket, const size_type slot) const {
    buckets_.deoccupy(bucket, slot);
    --bucket.elem_counter();
  }

  template <typename F, typename K, typename... Args> 
  bool rehash_required(const rh_data& data, lock_list_t& lock_array, F fn, K&& key, Args&& ...val)  {
    lock_array.clear();
    rh_fast_double(data.hp);
    return insert_fn(fn, std::forward<K>(key), std::forward<Args>(val)...);
  }

  template <table_mode TABLE_MODE = table_mode::UNLOCKED, typename F, typename K, typename... Args> 
  bool insert_fn(F fn, K&& key, Args&& ...val)  {
    static constexpr cycle_checks insert_fn_checks {
      .out_of_window = true,
      .not_occupied = true,
      .less_dist = true,
      .equal = true
    };

    while (true) {
      rh_data data = get_rh_data(key);

      lock_list_t locks;
      locks.reserve(data.window_size);
      if constexpr (TABLE_MODE == table_mode::UNLOCKED) {
        if (!lock_first(data, locks)) {
          continue;
        }
      }

      switch (abstract_cycle<TABLE_MODE, insert_fn_checks>(key, data, locks)) {
        case cycle_status::OUT_OF_WINDOW:
          return rehash_required(data, locks, fn, std::forward<K>(key), std::forward<Args>(val)...);
        case cycle_status::NOT_OCCUPIED:
          add_to_bucket(*data.it, data.slot, 
              data.dist, std::forward<K>(key), std::forward<Args>(val)...);
          return true;
        case cycle_status::LESS_DIST:
          if constexpr (TABLE_MODE == table_mode::UNLOCKED) {
            if (!path_exists(data, locks)) {
              return rehash_required(data, locks, fn, std::forward<K>(key), std::forward<Args>(val)...);
            }
          }
          
          move_path(data, std::forward<K>(key), std::forward<Args>(val)...);
          return true;
        case cycle_status::EQUAL:
          fn(data.it->mapped(data.slot));
          return false;
      }
    }
  }

  template <bool LOCKED = false>
  bool path_exists(const rh_data& data, lock_list_t& lock_list) {
    rh_path_data path_data = get_rh_path_data(data);
    while (true) {
      for (; path_data.slot < SLOT_PER_BUCKET; ++path_data.current_dist, ++path_data.dist, ++path_data.slot) {
        if (path_data.dist > data.window_size) {
          return false;
        }

        if (!path_data.it->occupied(path_data.slot)) {
          return true;
        } else if (path_data.it->dist(path_data.slot) < path_data.current_dist) {
          path_data.current_dist = path_data.it->dist(path_data.slot);
        }
      }

      if (path_data.dist > data.window_size) {
        return false;
      }

      ++path_data.it;
      path_data.slot = 0;
      if constexpr (!LOCKED) {
        path_data.it->lock();
        lock_list.emplace_back(&(*path_data.it));
      }
    }
  }

  template <typename K, typename... Args> 
  void move_path(rh_data& data, K&& key, Args&& ...val) {
    typename bucket_t::storage_value_type storage(data.it->storage_kvpair(data.slot));
    uint16_t tmp_dist = data.it->dist(data.slot);
    buckets_.deoccupy(*data.it, data.slot);
    buckets_.setKV(*data.it, data.slot, data.dist, std::forward<K>(key), std::forward<Args>(val)...);

    ++data.slot;
    data.dist = tmp_dist + 1;
    while (true) {
      for (; data.slot < SLOT_PER_BUCKET; ++data.slot, ++data.dist) {
        if (!data.it->occupied(data.slot)) {
          add_to_bucket(*data.it, data.slot, data.dist, 
            storage.first, storage.second);
          return;
        } else if (data.it->dist(data.slot) < data.dist) {
          typename bucket_t::storage_value_type tmp = storage;
          storage = data.it->storage_kvpair(data.slot);

          tmp_dist = data.it->dist(data.slot);
          buckets_.deoccupy(*data.it, data.slot);
          buckets_.setKV(*data.it, data.slot, data.dist, tmp.first, tmp.second);
          data.dist = tmp_dist;
        }
      }

      ++data.it;
      data.slot = 0;
    }
  }

  AllLocksManager lock_all() {
    bucket_iterator it = buckets_.begin();
    it->lock();
    ++it;
    for (; it != buckets_.end(); ++it) {
      it->lock();
    }
    return AllLocksManager(this, AllUnlocker());
  }

  AllLocksManager rh_fast_double(int16_t current_hp) {
    const int16_t new_hp = current_hp + 1;
    auto all_locks_manager = lock_all();

    if (hashpower() != current_hp) {
      return nullptr;
    }

    buckets_.double_size(true/*locked*/);
    std::atomic_thread_fence(std::memory_order_release);

    bucket_iterator it = buckets_.begin();
    uint16_t free_behind = 0;
    for (size_type index = 0; index < (size_type(1) << current_hp); ++it) {
      for (uint16_t slot = 0; slot < SLOT_PER_BUCKET; ++slot, ++index) {
        if (!it->occupied(slot)) {
          ++free_behind;
          continue;
        }
        
        size_type original_index = get_original_index(new_hp, hash_fn_(it->key(slot)));

        // change location
        if (original_index > index) {
          insert_fn<table_mode::LOCKED>([](mapped_type&) {}, it->key(slot), it->mapped(slot));
          del_from_bucket(*it, slot);
          ++free_behind;
        } else if (it->dist(slot) != 0 && free_behind > 0) {
          size_type best_index = std::max(original_index, index - free_behind);
          size_type new_slot = get_slot(best_index);

          add_to_bucket(buckets_[get_bucket_index(best_index)], new_slot, 
              best_index - original_index, it->key(slot), it->mapped(slot));
          del_from_bucket(*it, slot);
          
          free_behind = index - best_index;
        } else {
          free_behind = 0;
        }
      }
    }

    return all_locks_manager;
  }
  
private:
  static constexpr uint16_t SLOT_PER_BUCKET_POW = std::countr_zero(SLOT_PER_BUCKET);
  static_assert((size_t(1) << SLOT_PER_BUCKET_POW) == SLOT_PER_BUCKET);
  static constexpr uint16_t BUCKET_MASK = SLOT_PER_BUCKET - 1;
  
  static constexpr uint16_t MAX_WINDOW_SIZE = 64;

  hasher hash_fn_;
  key_equal eq_fn_;
  mutable buckets_t buckets_;
};

} // seqlock_lib::rh
