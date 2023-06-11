Hash tables with seqlock-based synchronization
=========
## Overview
This repository contains open-addressed concurrent hash tables with seqlock-based synchronization, tests for it, and mini benchmark framework for creation of custom benchmarks for concurrent hash tables.

## Hash tables
Seqlock has been integrated into the libcuckoo hash table. New cuckoo hash map algorithm provides much better scalability for reads than original algorithm, without overhead on writes.\
Robin hood concurrent hash table with seqlock was implemented from scratch, so it has poor interface.\
Correctes idea was inspired by ["Can Seqlocks Get Along With Programming Language Memory Models?"](https://www.hpl.hp.com/techreports/2012/HPL-2012-68.pdf) paper

## Install dependencies
Using conda:
```bash
conda env create -f environment.yml
conda activate seqlock_hash_map
```
## Build & Run
```bash
scripts/init_cmake.sh

# run benchmarks
scripts/run_bench.sh

# run tests
scripts/cuckoo/run_unit.sh
scripts/cuckoo/run_stress_checked.sh
scripts/cuckoo/run_stress_unchecked.sh
```

## Plot results
Check benchmark results with draw_bench.ipynb
