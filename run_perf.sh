cmake --build build --config Release --target bench
build/bench --benchmark_out=bench_out.json --benchmark_out_format=json
