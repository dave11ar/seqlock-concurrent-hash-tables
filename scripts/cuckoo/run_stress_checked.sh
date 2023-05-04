cmake --build build/release --config Release --target stress_checked
build/release/tests/cuckoo/stress_tests/stress_checked --power 26 --thread-num 16 --time 15
