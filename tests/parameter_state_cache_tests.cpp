#include <nandina/render/detail/parameter_state_cache.hpp>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstdint>
#include <stdexcept>
#include <vector>

using namespace nandina;

namespace
{
    using Cache = render::detail::ParameterStateCache<int, 2>;
    using Key = Cache::Key;

    auto key(std::uint32_t first, std::uint32_t second) -> Key {
        return {first, second};
    }
} // namespace

TEST_CASE("parameter state cache creates on miss and hits on repeat", "[render][sdf][cache]") {
    std::vector<int> destroyed;
    int next = 0;
    Cache cache(
        4,
        [&](const Key&) { return next++; },
        [&](int handle) { destroyed.push_back(handle); }
    );

    const auto first = cache.acquire(key(1, 2));
    REQUIRE(first.created);
    REQUIRE(first.handle == 0);
    REQUIRE(cache.misses() == 1);
    REQUIRE(cache.hits() == 0);

    const auto repeat = cache.acquire(key(1, 2));
    REQUIRE_FALSE(repeat.created);
    REQUIRE(repeat.handle == first.handle);
    REQUIRE(cache.hits() == 1);
    REQUIRE(cache.misses() == 1);
    REQUIRE(destroyed.empty());
}

TEST_CASE("parameter state cache compares keys bitwise", "[render][sdf][cache]") {
    Cache cache(4, [](const Key&) { return 0; }, [](int) {});
    // 缓存层的键是按位比较：调用方负责把浮点装箱；这里直接验证不同的位模式是不同键。
    REQUIRE(cache.acquire(key(0x00000000U, 7)).created);
    // 位模式不同（浮点 +0.0/-0.0 或参数差异）就是不同条目。
    REQUIRE(cache.acquire(key(0x80000000U, 7)).created);
    REQUIRE(cache.size() == 2);
}

TEST_CASE("parameter state cache evicts the least recently used entry", "[render][sdf][cache]") {
    std::vector<int> destroyed;
    int next = 0;
    Cache cache(
        2,
        [&](const Key&) { return next++; },
        [&](int handle) { destroyed.push_back(handle); }
    );

    const auto a = cache.acquire(key(1, 0));
    const auto b = cache.acquire(key(2, 0));
    REQUIRE(cache.size() == 2);

    // 触碰 a，使其成为最近使用；b 成为最旧。
    REQUIRE_FALSE(cache.acquire(key(1, 0)).created);

    const auto c = cache.acquire(key(3, 0));
    REQUIRE(c.created);
    REQUIRE(cache.size() == 2);
    REQUIRE(destroyed.size() == 1);
    REQUIRE(destroyed.front() == b.handle);
    REQUIRE(destroyed.front() != a.handle);
}

TEST_CASE("parameter state cache clears and destroys everything", "[render][sdf][cache]") {
    std::vector<int> destroyed;
    int next = 0;
    {
        Cache cache(
            8,
            [&](const Key&) { return next++; },
            [&](int handle) { destroyed.push_back(handle); }
        );
        cache.acquire(key(1, 0));
        cache.acquire(key(2, 0));
        REQUIRE(destroyed.empty());
        cache.clear();
        REQUIRE(cache.size() == 0);
        REQUIRE(destroyed.size() == 2);
        // 清空后再次获取应重新创建，且计数继续累加。
        const auto again = cache.acquire(key(1, 0));
        REQUIRE(again.created);
        REQUIRE(cache.misses() == 3);
    }
    // 析构会销毁剩余条目（上面又创建了一条）。
    REQUIRE(destroyed.size() == 3);
}

TEST_CASE("parameter state cache tolerates zero capacity", "[render][sdf][cache]") {
    Cache cache(0, [](const Key&) { return 7; }, [](int) {});
    REQUIRE(cache.capacity() == 1);
    REQUIRE(cache.acquire(key(1, 1)).handle == 7);
    REQUIRE(cache.size() == 1);
    REQUIRE(cache.acquire(key(2, 2)).handle == 7);
    REQUIRE(cache.size() == 1);
}

TEST_CASE("failed state creation preserves cached handles", "[render][sdf][cache]") {
    std::vector<int> destroyed;
    Cache cache(
        1,
        [](const Key& value) {
            if (value[0] == 2) {
                throw std::runtime_error("injected state creation failure");
            }
            return 7;
        },
        [&](int handle) { destroyed.push_back(handle); }
    );
    cache.acquire(key(1, 0));
    REQUIRE_THROWS_AS(cache.acquire(key(2, 0)), std::runtime_error);
    REQUIRE(cache.size() == 1);
    REQUIRE(destroyed.empty());
    REQUIRE_FALSE(cache.acquire(key(1, 0)).created);
}
