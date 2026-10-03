#include <nandina/foundation/motion/spec.hpp>

#include <catch2/catch_test_macros.hpp>

#include <concepts>

using namespace nandina;

static_assert(std::same_as<decltype(motion::tween(0.1F)), motion::TweenSpec>);
static_assert(std::same_as<decltype(motion::spring()), motion::SpringSpec>);

TEST_CASE("canonical motion specs can be included alone", "[motion][header]") {
    const auto tween = motion::tween(0.24F).easing(motion::ease_out);
    REQUIRE(tween.duration() == 0.24F);
    REQUIRE(tween.easing_curve() == motion::Easing::ease_out);

    const auto spring = motion::spring(200.0F, 12.0F);
    REQUIRE(spring.stiffness() == 200.0F);
    REQUIRE(spring.damping() == 12.0F);
}
