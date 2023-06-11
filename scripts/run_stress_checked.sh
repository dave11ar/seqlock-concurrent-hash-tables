cmake --build build/debug --config Debug --target stress_checked
build/debug/tests/cuckoo/stress_tests/stress_checked --power 16 --thread-num 16 --time 5 --use-big-objects
