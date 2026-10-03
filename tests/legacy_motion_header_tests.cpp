#include <nandina/animation/motion.hpp>

#include <catch2/catch_test_macros.hpp>

#include <concepts>

using namespace nandina;

static_assert(std::same_as<animation::motion::TweenSpec, motion::TweenSpec>);
static_assert(std::same_as<decltype(animation::motion::spring()), motion::SpringSpec>);

TEST_CASE("legacy motion header remains standalone", "[motion][compat]") {
    REQUIRE(animation::motion::tween(0.3F).duration() == 0.3F);
    REQUIRE(animation::motion::spring(300.0F, 18.0F).damping() == 18.0F);
}
