/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <unordered_map>

namespace reaction {

/**
 * @brief Unified base class for cache implementations with common functionality.
 *
 * This template class provides common caching patterns including:
 * - Version-based cache invalidation
 * - LRU-based cleanup with TTL
 * - Thread-safe operations
 * - Statistics tracking
 *
 * @tparam Key Cache key type
 * @tparam Value Cache value type
 * @tparam Hash Hash function for key type
 * @tparam KeyEqual Equality comparison for key type
 */
template <
    typename Key,
    typename Value,
    typename Hash = std::hash<Key>,
    typename KeyEqual = std::equal_to<Key>>
class CacheBase {
public:
    /**
     * @brief Cache statistics structure.
     */
    struct Stats {
        size_t totalEntries = 0;
        uint64_t currentVersion = 0;
        size_t hitCount = 0;
        size_t missCount = 0;
        double hitRatio = 0.0;

        Stats(size_t entries, uint64_t version, size_t hits, size_t misses)
            : totalEntries(entries), currentVersion(version), hitCount(hits), missCount(misses), hitRatio(hits + misses > 0 ? static_cast<double>(hits) / (hits + misses) : 0.0) {}
    };

    /**
     * @brief Cache entry wrapper with metadata.
     */
    struct CacheEntry {
        Value value;
        uint64_t version;
        mutable std::chrono::steady_clock::time_point lastAccess;

        template <typename V>
        CacheEntry(V &&val, uint64_t ver)
            : value(std::forward<V>(val)), version(ver), lastAccess(std::chrono::steady_clock::now()) {}

        CacheEntry(const CacheEntry &other)
            : value(other.value), version(other.version), lastAccess(other.lastAccess) {}

        CacheEntry(CacheEntry &&other) noexcept
            : value(std::move(other.value)), version(other.version), lastAccess(other.lastAccess) {}

        CacheEntry &operator=(const CacheEntry &other) {
            if (this != &other) {
                value = other.value;
                version = other.version;
                lastAccess = other.lastAccess;
            }
            return *this;
        }

        CacheEntry &operator=(CacheEntry &&other) noexcept {
            if (this != &other) {
                value = std::move(other.value);
                version = other.version;
                lastAccess = other.lastAccess;
            }
            return *this;
        }
    };

protected:
    /**
     * @brief Constructor with cache configuration.
     *
     * @param maxSize Maximum number of entries before cleanup
     * @param ttl Time-to-live for cache entries
     */
    explicit CacheBase(size_t maxSize, std::chrono::minutes ttl)
        : m_maxCacheSize(maxSize), m_cacheTTL(ttl) {}

    /**
     * @brief Try to get cached value for a key.
     *
     * @param key The key to look up
     * @return Pointer to cached value if valid, nullptr otherwise
     */
    const Value *getCachedValue(const Key &key) const noexcept {
        auto it = m_cacheEntries.find(key);

        if (it != m_cacheEntries.end() &&
            it->second.version == m_currentVersion) {
            // Update last access time (mutable)
            it->second.lastAccess = std::chrono::steady_clock::now();
            ++m_hitCount;
            return &it->second.value;
        }

        ++m_missCount;
        return nullptr;
    }

    /**
     * @brief Cache a value for a key.
     *
     * @param key The key to cache under
     * @param value The value to cache
     */
    template <typename V>
    void cacheValue(const Key &key, V &&value) noexcept {

        uint64_t currentVersion = m_currentVersion;
        m_cacheEntries.insert_or_assign(key, CacheEntry{std::forward<V>(value), currentVersion});

        // Cleanup if cache is getting too large
        if (m_cacheEntries.size() > m_maxCacheSize) {
            cleanupOldEntriesInternal();
        }
    }

    /**
     * @brief Invalidate all cache entries due to structural change.
     */
    void invalidateAllEntries() noexcept {
        // Simply increment version - old entries will be automatically ignored
        ++m_currentVersion;

        // Optionally clear cache entries to free memory immediately
        m_cacheEntries.clear();
    }

    /**
     * @brief Get current cache version.
     */
    uint64_t getCurrentVersion() const noexcept {
        return m_currentVersion;
    }

    /**
     * @brief Get cache statistics.
     */
    Stats getStatsInternal() const noexcept {
        return Stats{
            m_cacheEntries.size(),
            m_currentVersion,
            m_hitCount,
            m_missCount};
    }

    /**
     * @brief Trigger cleanup of expired cache entries.
     */
    void triggerCleanupInternal() noexcept {
        cleanupOldEntriesInternal();
    }

private:
    /**
     * @brief Remove old cache entries based on LRU and TTL.
     * Must be called with m_cacheMutex held in exclusive mode.
     */
    void cleanupOldEntriesInternal() noexcept {
        auto now = std::chrono::steady_clock::now();
        auto cutoffTime = now - m_cacheTTL;

        // Remove entries older than TTL
        for (auto it = m_cacheEntries.begin(); it != m_cacheEntries.end();) {
            if (it->second.lastAccess < cutoffTime) {
                it = m_cacheEntries.erase(it);
            } else {
                ++it;
            }
        }

        // If still too many entries, remove least recently used
        if (m_cacheEntries.size() > m_maxCacheSize) {
            auto oldestIt = std::min_element(m_cacheEntries.begin(), m_cacheEntries.end(),
                [](const auto &a, const auto &b) {
                    return a.second.lastAccess < b.second.lastAccess;
                });

            if (oldestIt != m_cacheEntries.end()) {
                m_cacheEntries.erase(oldestIt);
            }
        }
    }

    const size_t m_maxCacheSize;
    const std::chrono::minutes m_cacheTTL;

    mutable std::unordered_map<Key, CacheEntry, Hash, KeyEqual> m_cacheEntries;
    uint64_t m_currentVersion{1};

    // Statistics tracking
    mutable size_t m_hitCount{0};
    mutable size_t m_missCount{0};
};

/**
 * @brief Specialized cache base for pointer key types with custom hash.
 */
template <typename PtrType, typename Value>
class PtrCacheBase : public CacheBase<PtrType, Value, std::hash<PtrType>, std::equal_to<PtrType>> {
public:
    using Base = CacheBase<PtrType, Value, std::hash<PtrType>, std::equal_to<PtrType>>;

protected:
    explicit PtrCacheBase(size_t maxSize, std::chrono::minutes ttl)
        : Base(maxSize, ttl) {}
};

/**
 * @brief Specialized cache base for pair key types with custom hash.
 */
template <typename First, typename Second, typename Value>
class PairCacheBase : public CacheBase<
                          std::pair<First, Second>,
                          Value,
                          std::hash<std::pair<First, Second>>,
                          std::equal_to<std::pair<First, Second>>> {
public:
    using KeyType = std::pair<First, Second>;
    using Base = CacheBase<KeyType, Value, std::hash<KeyType>, std::equal_to<KeyType>>;

protected:
    explicit PairCacheBase(size_t maxSize, std::chrono::minutes ttl)
        : Base(maxSize, ttl) {}
};

} // namespace reaction

// Specialize std::hash for std::pair to support PairCacheBase
namespace std {
template <typename T1, typename T2>
struct hash<std::pair<T1, T2>> {
    size_t operator()(const std::pair<T1, T2> &p) const noexcept {
        auto h1 = std::hash<T1>{}(p.first);
        auto h2 = std::hash<T2>{}(p.second);
        return h1 ^ (h2 << 1);
    }
};
} // namespace std