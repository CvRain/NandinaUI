// Direct unit tests for the pure column-track solver shared by static Table.
#include <nandina/widget/internal/track_solver.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>

#include <cmath>
#include <initializer_list>
#include <limits>
#include <optional>
#include <stdexcept>
#include <vector>

namespace internal = nandina::widget::internal;

namespace
{
    auto make_tracks(const std::initializer_list<internal::TrackSpec> tracks)
        -> std::vector<internal::TrackSpec> {
        return {tracks.begin(), tracks.end()};
    }

    auto solve(
        const std::vector<internal::TrackSpec>& tracks,
        const std::optional<float> available,
        const std::vector<float>& intrinsic,
        const float gap
    ) -> internal::TrackSolution {
        return internal::solve_tracks(tracks, available, intrinsic, gap);
    }
} // namespace

TEST_CASE(
    "track solver keeps fixed columns and shares finite width by flex",
    "[table][track-solver]"
) {
    const auto tracks = make_tracks({{.width = 120.0F}, {}, {}});
    const auto solved = solve(tracks, 400.0F, {}, 10.0F);

    REQUIRE(solved.widths.size() == 3);
    CHECK(solved.widths[0] == Catch::Approx(120.0F));
    CHECK(solved.widths[1] == Catch::Approx(130.0F));
    CHECK(solved.widths[2] == Catch::Approx(130.0F));
    CHECK(solved.total_width == Catch::Approx(400.0F));
}

TEST_CASE("track solver splits the remainder by flex weight", "[table][track-solver]") {
    const auto tracks = make_tracks({{.flex = 1.0F}, {.flex = 2.0F}});
    const auto solved = solve(tracks, 300.0F, {}, 0.0F);

    CHECK(solved.widths[0] == Catch::Approx(100.0F));
    CHECK(solved.widths[1] == Catch::Approx(200.0F));
    CHECK(solved.total_width == Catch::Approx(300.0F));
}

TEST_CASE(
    "track solver clamps flex to zero when fixed tracks exceed the width",
    "[table][track-solver]"
) {
    const auto tracks = make_tracks({{.width = 200.0F}, {}});
    const auto solved = solve(tracks, 100.0F, {}, 20.0F);

    CHECK(solved.widths[0] == Catch::Approx(200.0F));
    CHECK(solved.widths[1] == Catch::Approx(0.0F));
    CHECK(solved.total_width == Catch::Approx(220.0F));
}

TEST_CASE(
    "track solver uses intrinsic widths only for flex tracks when unbounded",
    "[table][track-solver]"
) {
    const auto tracks = make_tracks({{.width = 80.0F}, {}, {.flex = 2.0F}});
    const std::vector<float> intrinsic {0.0F, 40.0F, 120.0F};
    const auto solved = solve(tracks, std::nullopt, intrinsic, 5.0F);

    CHECK(solved.widths[0] == Catch::Approx(80.0F));
    CHECK(solved.widths[1] == Catch::Approx(40.0F));
    CHECK(solved.widths[2] == Catch::Approx(120.0F));
    CHECK(solved.total_width == Catch::Approx(250.0F));
}

TEST_CASE(
    "track solver ignores intrinsic widths when the width is bounded",
    "[table][track-solver]"
) {
    const auto tracks = make_tracks({{.flex = 1.0F}, {.flex = 1.0F}});
    const auto solved = solve(tracks, 200.0F, {}, 0.0F);

    CHECK(solved.widths[0] == Catch::Approx(100.0F));
    CHECK(solved.widths[1] == Catch::Approx(100.0F));
}

TEST_CASE("track solver includes column gaps in the total width", "[table][track-solver]") {
    const auto tracks = make_tracks({{.width = 10.0F}, {.width = 10.0F}, {.width = 10.0F}});
    const auto solved = solve(tracks, 500.0F, {}, 5.0F);

    CHECK(solved.widths[0] == Catch::Approx(10.0F));
    CHECK(solved.widths[1] == Catch::Approx(10.0F));
    CHECK(solved.widths[2] == Catch::Approx(10.0F));
    CHECK(solved.total_width == Catch::Approx(40.0F));
}

TEST_CASE(
    "track solver rejects invalid track definitions and gaps",
    "[table][track-solver][validation]"
) {
    CHECK_THROWS_AS(
        internal::validate_tracks(std::span<const internal::TrackSpec> {}, 0.0F),
        std::invalid_argument
    );

    const auto negative_width = make_tracks({{.width = -1.0F}});
    CHECK_THROWS_AS(internal::validate_tracks(negative_width, 0.0F), std::invalid_argument);

    const auto infinite_width = make_tracks({{.width = std::numeric_limits<float>::infinity()}});
    CHECK_THROWS_AS(internal::validate_tracks(infinite_width, 0.0F), std::invalid_argument);

    const auto zero_flex = make_tracks({{.flex = 0.0F}});
    CHECK_THROWS_AS(internal::validate_tracks(zero_flex, 0.0F), std::invalid_argument);

    const auto negative_flex = make_tracks({{.flex = -1.0F}});
    CHECK_THROWS_AS(internal::validate_tracks(negative_flex, 0.0F), std::invalid_argument);

    const auto infinite_flex = make_tracks({{.flex = std::numeric_limits<float>::infinity()}});
    CHECK_THROWS_AS(internal::validate_tracks(infinite_flex, 0.0F), std::invalid_argument);

    const auto valid = make_tracks({{.width = 10.0F}});
    CHECK_THROWS_AS(internal::validate_tracks(valid, -1.0F), std::invalid_argument);
    CHECK_THROWS_AS(
        internal::validate_tracks(valid, std::numeric_limits<float>::infinity()),
        std::invalid_argument
    );
}

TEST_CASE(
    "track solver rejects fixed sums beyond finite layout",
    "[table][track-solver][validation]"
) {
    const auto huge = std::numeric_limits<float>::max();
    const auto huge_fixed = make_tracks({{.width = huge}, {.width = huge}});
    CHECK_THROWS_AS(internal::validate_tracks(huge_fixed, 0.0F), std::invalid_argument);

    const auto valid = make_tracks({{.width = 10.0F}, {}, {}});
    CHECK_THROWS_AS(internal::validate_tracks(valid, huge), std::invalid_argument);
}

TEST_CASE("track solver validates its own inputs", "[table][track-solver][validation]") {
    CHECK_THROWS_AS(solve({}, 100.0F, {}, 0.0F), std::invalid_argument);

    const auto invalid = make_tracks({{.flex = 0.0F}});
    CHECK_THROWS_AS(solve(invalid, 100.0F, {}, 0.0F), std::invalid_argument);

    const auto valid = make_tracks({{.width = 10.0F}});
    CHECK_THROWS_AS(
        solve(valid, std::optional<float>(std::numeric_limits<float>::infinity()), {}, 0.0F),
        std::invalid_argument
    );
}

TEST_CASE("track solver checks unbounded intrinsic widths", "[table][track-solver][validation]") {
    const auto tracks = make_tracks({{.width = 10.0F}, {}});
    CHECK_THROWS_AS(
        solve(tracks, std::nullopt, std::vector<float> {0.0F}, 0.0F),
        std::invalid_argument
    );

    const std::vector<float> infinite {0.0F, std::numeric_limits<float>::infinity()};
    CHECK_THROWS_WITH(
        solve(tracks, std::nullopt, infinite, 0.0F),
        "Table content extent must fit finite layout coordinates"
    );

    // 旧实现用 max(0, measured) 初始化列宽：负 intrinsic 静默夹取为 0。
    const std::vector<float> negative {0.0F, -1.0F};
    const auto clamped = solve(tracks, std::nullopt, negative, 0.0F);
    CHECK(clamped.widths[0] == Catch::Approx(10.0F));
    CHECK(clamped.widths[1] == Catch::Approx(0.0F));

    // 固定列的 intrinsic 不参与解算：即使给异常值也不应影响结果。
    const std::vector<float> fixed_ignored {-5.0F, 30.0F};
    const auto solved = solve(tracks, std::nullopt, fixed_ignored, 0.0F);
    CHECK(solved.widths[0] == Catch::Approx(10.0F));
    CHECK(solved.widths[1] == Catch::Approx(30.0F));
}

TEST_CASE(
    "track solver rejects unbounded totals beyond finite layout",
    "[table][track-solver][validation]"
) {
    const auto huge = std::numeric_limits<float>::max();
    const auto tracks = make_tracks({{.flex = 1.0F}, {.flex = 1.0F}});
    const std::vector<float> intrinsic {huge, huge};
    CHECK_THROWS_AS(solve(tracks, std::nullopt, intrinsic, 0.0F), std::invalid_argument);
}
