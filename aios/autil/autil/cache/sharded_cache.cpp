//  Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.
//
// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include <stdio.h>
#include <string>
#include <atomic>
#include <cstdint>

#include "autil/cache/sharded_cache.h"
#include "cache_hash.h"
#include "autil/Lock.h"
#include "autil/cache/cache.h"
#include "autil/cache/cache_allocator.h"
#include "autil/Span.h"

namespace autil {

ShardedCache::ShardedCache(size_t capacity, int num_shard_bits, bool strict_capacity_limit)
    : num_shard_bits_(num_shard_bits)
    , capacity_(capacity)
    , strict_capacity_limit_(strict_capacity_limit)
    , last_id_(1)
{
}

void ShardedCache::SetCapacity(size_t capacity)
{
    int num_shards = 1 << num_shard_bits_;
    const size_t per_shard = (capacity + (num_shards - 1)) / num_shards;
    ScopedLock l(capacity_mutex_);
    for (int s = 0; s < num_shards; s++) {
        GetShard(s)->SetCapacity(per_shard);
    }
    capacity_ = capacity;
}

void ShardedCache::SetStrictCapacityLimit(bool strict_capacity_limit)
{
    int num_shards = 1 << num_shard_bits_;
    ScopedLock l(capacity_mutex_);
    for (int s = 0; s < num_shards; s++) {
        GetShard(s)->SetStrictCapacityLimit(strict_capacity_limit);
    }
    strict_capacity_limit_ = strict_capacity_limit;
}

static uint32_t HashSlice(const autil::StringView& s) {
    return CacheHash::Hash(s.data(), s.size(), 0);
}

bool ShardedCache::Insert(const StringView& key, void* value, size_t charge,
                            void (*deleter)(const StringView& key, void* value, const CacheAllocatorPtr& allocator),
                            Handle** handle, Priority priority)
{
    uint32_t hash = HashSlice(key);
    return GetShard(Shard(hash))->Insert(key, hash, value, charge, deleter, handle, priority);
}

CacheBase::Handle* ShardedCache::Lookup(const StringView& key)
{
    uint32_t hash = HashSlice(key);
    return GetShard(Shard(hash))->Lookup(key, hash);
}

bool ShardedCache::Ref(Handle* handle)
{
    uint32_t hash = GetHash(handle);
    return GetShard(Shard(hash))->Ref(handle);
}

void ShardedCache::Release(Handle* handle)
{
    uint32_t hash = GetHash(handle);
    GetShard(Shard(hash))->Release(handle);
}

void ShardedCache::Erase(const StringView& key)
{
    uint32_t hash = HashSlice(key);
    GetShard(Shard(hash))->Erase(key, hash);
}

uint64_t ShardedCache::NewId() { return last_id_.fetch_add(1, std::memory_order_relaxed); }

size_t ShardedCache::GetCapacity() const
{
    ScopedLock l(capacity_mutex_);
    return capacity_;
}

bool ShardedCache::HasStrictCapacityLimit() const
{
    ScopedLock l(capacity_mutex_);
    return strict_capacity_limit_;
}

size_t ShardedCache::GetUsage() const
{
    // We will not lock the cache when getting the usage from shards.
    int num_shards = 1 << num_shard_bits_;
    size_t usage = 0;
    for (int s = 0; s < num_shards; s++) {
        usage += GetShard(s)->GetUsage();
    }
    return usage;
}

size_t ShardedCache::GetUsage(Handle* handle) const { return GetCharge(handle); }

size_t ShardedCache::GetPinnedUsage() const
{
    // We will not lock the cache when getting the usage from shards.
    int num_shards = 1 << num_shard_bits_;
    size_t usage = 0;
    for (int s = 0; s < num_shards; s++) {
        usage += GetShard(s)->GetPinnedUsage();
    }
    return usage;
}

void ShardedCache::ApplyToAllCacheEntries(void (*callback)(void*, size_t), bool thread_safe)
{
    int num_shards = 1 << num_shard_bits_;
    for (int s = 0; s < num_shards; s++) {
        GetShard(s)->ApplyToAllCacheEntries(callback, thread_safe);
    }
}

void ShardedCache::EraseUnRefEntries()
{
    int num_shards = 1 << num_shard_bits_;
    for (int s = 0; s < num_shards; s++) {
        GetShard(s)->EraseUnRefEntries();
    }
}

std::string ShardedCache::GetPrintableOptions() const
{
    std::string ret;
    ret.reserve(20000);
    const int kBufferSize = 200;
    char buffer[kBufferSize];
    {
        ScopedLock l(capacity_mutex_);
        snprintf(buffer, kBufferSize, "    capacity : %zu \n", capacity_);
        ret.append(buffer);
        snprintf(buffer, kBufferSize, "    num_shard_bits : %d\n", num_shard_bits_);
        ret.append(buffer);
        snprintf(buffer, kBufferSize, "    strict_capacity_limit : %d\n", strict_capacity_limit_);
        ret.append(buffer);
    }
    ret.append(GetShard(0)->GetPrintableOptions());
    return ret;
}

}
