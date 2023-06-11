#pragma once

#include <cstddef>

//! The default maximum number of keys per bucket
constexpr size_t DEFAULT_SLOT_PER_BUCKET = 4;

//! The default number of elements in an empty hash table
constexpr size_t DEFAULT_SIZE =
    (1U << 16) * DEFAULT_SLOT_PER_BUCKET;