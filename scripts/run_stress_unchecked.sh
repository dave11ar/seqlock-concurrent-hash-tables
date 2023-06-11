cmake --build build/release --config Release --target stress_unchecked
build/release/tests/cuckoo/stress_tests/stress_unchecked --power 26 --thread-num 16 --time 15 --use-big-objects
