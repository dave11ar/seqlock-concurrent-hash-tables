rm -rf external/
rm perf/benchmark_custom_candidates/concurrent_hash_map.hpp

wget https://github.com/boostorg/sync/archive/master.tar.gz -P external/boost/sync
wget https://raw.githubusercontent.com/BlazingPhoenix/concurrent-hash-map/master/concurrent_hash_map/concurrent_hash_map.hpp -P perf/benchmark_custom_candidates/cuckoo

tar -xzvf external/boost/sync/master.tar.gz --strip-components 1 -C external/boost/sync/
rm external/boost/sync/master.tar.gz
