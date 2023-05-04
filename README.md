Hash tables with seqlock-based synchronization
=========
## Overview
This repository contains open-addressed concurrent hash tables with seqlock-based synchronization, tests for it, and mini benchmark framework for creation custom benchmarks for concurrent hash tables.

## Hash tables
By now, seqlock was integrated only in libcuckoo hash table, libcuckoo with seqlock much better scalability 
for read load than original alogithm, without overhead on write load. Algorithms works for x86_64 and aarch64 architectures.\
At the moment, work is underway to implement the robin hood hash table with seqlock synchronization.

## Install dependencies
Using conda:
```bash
conda env create -f environment.yml
conda activate benchmarks
```
## Build & Run
```bash
scripts/init_cmake.sh

# run benchmarks
scripts/run_bench.sh

# run seqlock-based cuckoohash_map tests
scripts/cuckoo/run_unit.sh
scripts/cuckoo/run_stress_checked.sh
scripts/cuckoo/run_stress_unchecked.sh
```

## Plot results
Check benchmark results with draw_bench.ipynb
