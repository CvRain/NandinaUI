//
// render/detail/parameter_state_cache — 有界 LRU：把参数块映射到设备句柄。
//
// 生产 SDF 路径的提交策略（docs/references/render_device_path_contract.md §9）：
// 参数块是不可变的，同一块只创建/播种一次，之后按"连续段"绑定。这里只做缓存与淘汰，
// 句柄的创建/销毁由使用方注入，因此本头不依赖任何图形 API，可以直接单测。
//

#ifndef NANDINA_RENDER_DETAIL_PARAMETER_STATE_CACHE_HPP
#define NANDINA_RENDER_DETAIL_PARAMETER_STATE_CACHE_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <list>
#include <unordered_map>
#include <utility>

namespace nandina::render::detail
{
    /// key 必须可按位比较（浮点参数用 bit_cast 装箱，`-0.0` 与 `+0.0` 不是同一个块）。
    template<typename Handle, std::size_t KeyWords>
    class ParameterStateCache {
    public:
        using Key = std::array<std::uint32_t, KeyWords>;
        using Create = std::function<Handle(const Key&)>;
        using Destroy = std::function<void(Handle)>;

        struct Acquired {
            Handle handle {};
            bool created = false;
        };

        ParameterStateCache(std::size_t capacity, Create create, Destroy destroy):
            capacity_(capacity == 0 ? 1 : capacity),
            create_(std::move(create)),
            destroy_(std::move(destroy)) {}

        ParameterStateCache(const ParameterStateCache&) = delete;
        auto operator=(const ParameterStateCache&) -> ParameterStateCache& = delete;
        ParameterStateCache(ParameterStateCache&&) = delete;
        auto operator=(ParameterStateCache&&) -> ParameterStateCache& = delete;

        ~ParameterStateCache() {
            clear();
        }

        /// 命中则移到表头并返回 false 的 created；未命中则创建、淘汰最旧条目并返回 true。
        auto acquire(const Key& key) -> Acquired {
            const auto found = index_.find(key);
            if (found != index_.end()) {
                entries_.splice(entries_.begin(), entries_, found->second);
                ++hits_;
                return {.handle = found->second->second, .created = false};
            }
            ++misses_;
            if (entries_.size() >= capacity_) {
                const auto& oldest = entries_.back();
                if (destroy_) {
                    destroy_(oldest.second);
                }
                index_.erase(oldest.first);
                entries_.pop_back();
            }
            auto handle = create_(key);
            entries_.emplace_front(key, handle);
            index_.emplace(key, entries_.begin());
            return {.handle = handle, .created = true};
        }

        void clear() {
            if (destroy_) {
                for (const auto& entry: entries_) {
                    destroy_(entry.second);
                }
            }
            entries_.clear();
            index_.clear();
        }

        [[nodiscard]] auto hits() const -> std::uint64_t {
            return hits_;
        }

        [[nodiscard]] auto misses() const -> std::uint64_t {
            return misses_;
        }

        [[nodiscard]] auto size() const -> std::size_t {
            return entries_.size();
        }

        [[nodiscard]] auto capacity() const -> std::size_t {
            return capacity_;
        }

    private:
        struct KeyHash {
            auto operator()(const Key& key) const noexcept -> std::size_t {
                // FNV-1a over the bit pattern: keys are compared bitwise, so must the hash.
                std::uint64_t hash = 0xcbf29ce484222325ULL;
                for (const auto word: key) {
                    hash ^= word;
                    hash *= 0x100000001b3ULL;
                }
                return static_cast<std::size_t>(hash);
            }
        };

        using Entry = std::pair<Key, Handle>;
        using Iterator = typename std::list<Entry>::iterator;

        std::size_t capacity_;
        Create create_;
        Destroy destroy_;
        std::list<Entry> entries_;
        std::unordered_map<Key, Iterator, KeyHash> index_;
        std::uint64_t hits_ = 0;
        std::uint64_t misses_ = 0;
    };
} // namespace nandina::render::detail

#endif // NANDINA_RENDER_DETAIL_PARAMETER_STATE_CACHE_HPP
