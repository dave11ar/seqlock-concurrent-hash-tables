cmake --build build/release --config Release --target bench
build/release/bench/bench --benchmark_out=bench_out.json --benchmark_out_format=json --benchmark_threads_step=2 --benchmark_threads_count=16
