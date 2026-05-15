#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <thorvg-1/thorvg.h>

import nandina.app.authoring;
import nandina.foundation.nan_size;
import nandina.runtime.nan_widget;

namespace {

class ProbeWidget final : public nandina::runtime::NanWidget {
public:
    [[nodiscard]] auto preferred_size() const noexcept -> nandina::geometry::NanSize override {
        return {24.0f, 12.0f};
    }
};

class CountingProbeWidget final : public nandina::runtime::NanWidget {
public:
    explicit CountingProbeWidget(int* measure_count)
        : measure_count_(measure_count) {
    }

    auto measure(const nandina::geometry::NanConstraints& constraints) -> void override {
        if (measure_count_) {
            ++(*measure_count_);
        }
        nandina::runtime::NanWidget::measure(constraints);
    }

    [[nodiscard]] auto preferred_size() const noexcept -> nandina::geometry::NanSize override {
        return {24.0f, 12.0f};
    }

private:
    int* measure_count_{nullptr};
};

class TaggedComponent final : public nandina::app::NanComponent {
public:
    TaggedComponent(std::string tag, const nandina::geometry::NanSize preferred)
        : tag_(std::move(tag)), preferred_(preferred) {
    }

    [[nodiscard]] auto tag() const noexcept -> std::string_view {
        return tag_;
    }

    [[nodiscard]] auto preferred_size() const noexcept -> nandina::geometry::NanSize override {
        return preferred_;
    }

private:
    std::string                 tag_;
    nandina::geometry::NanSize  preferred_;
};

class TestPage final : public nandina::app::NanPage {
public:
    TestPage(std::string route_key,
             std::string title,
             std::string component_tag,
             const nandina::geometry::NanSize preferred,
             int* build_count = nullptr)
        : route_key_(std::move(route_key)),
          title_(std::move(title)),
          component_tag_(std::move(component_tag)),
          preferred_(preferred),
          build_count_(build_count) {
    }

    [[nodiscard]] auto route_key() const noexcept -> std::string_view override {
        return route_key_;
    }

    [[nodiscard]] auto title() const noexcept -> std::string_view override {
        return title_;
    }

    [[nodiscard]] auto build() -> nandina::app::NanComponent::Ptr override {
        if (build_count_) {
            ++(*build_count_);
        }
        return std::make_unique<TaggedComponent>(component_tag_, preferred_);
    }

private:
    std::string                 route_key_;
    std::string                 title_;
    std::string                 component_tag_;
    nandina::geometry::NanSize  preferred_;
    int*                        build_count_;
};

class MountedTestPage final : public nandina::app::NanPage {
public:
    [[nodiscard]] auto route_key() const noexcept -> std::string_view override {
        return "mounted";
    }

    [[nodiscard]] auto title() const noexcept -> std::string_view override {
        return "Mounted";
    }

    [[nodiscard]] auto build() -> nandina::app::NanComponent::Ptr override {
        return nandina::app::mount(nandina::app::adopt(std::make_unique<ProbeWidget>()));
    }
};

class CountingMountedPage final : public nandina::app::NanPage {
public:
    explicit CountingMountedPage(int* measure_count)
        : measure_count_(measure_count) {
    }

    [[nodiscard]] auto route_key() const noexcept -> std::string_view override {
        return "counting-mounted";
    }

    [[nodiscard]] auto title() const noexcept -> std::string_view override {
        return "CountingMounted";
    }

    [[nodiscard]] auto build() -> nandina::app::NanComponent::Ptr override {
        return nandina::app::mount(
            nandina::app::adopt(std::make_unique<CountingProbeWidget>(measure_count_)));
    }

private:
    int* measure_count_{nullptr};
};

class ThorvgCanvasScope {
public:
    ThorvgCanvasScope(const std::uint32_t width, const std::uint32_t height)
        : pixels_(width * height, 0u) {
        if (tvg::Initializer::init(0u) != tvg::Result::Success) {
            throw std::runtime_error("tvg::Initializer::init failed");
        }
        canvas_.reset(tvg::SwCanvas::gen());
        if (!canvas_) {
            tvg::Initializer::term();
            throw std::runtime_error("tvg::SwCanvas::gen() returned nullptr");
        }
        if (canvas_->target(
                pixels_.data(),
                width,
                width,
                height,
                tvg::ColorSpace::ARGB8888) != tvg::Result::Success) {
            canvas_.reset();
            tvg::Initializer::term();
            throw std::runtime_error("tvg::SwCanvas::target failed");
        }
    }

    ~ThorvgCanvasScope() {
        canvas_.reset();
        tvg::Initializer::term();
    }

    auto canvas() const noexcept -> tvg::SwCanvas& {
        return *canvas_;
    }

private:
    std::vector<std::uint32_t>      pixels_;
    std::unique_ptr<tvg::SwCanvas>  canvas_;
};

} // namespace

TEST(AppNavigationTest, RouterUsesFirstRegisteredPageAsInitialRoute) {
    auto router = nandina::app::NanRouter::create();

    int first_build_count = 0;
    int second_build_count = 0;

    router->register_page(std::make_unique<TestPage>(
        "overview", "Overview", "overview-root", nandina::geometry::NanSize{120.0f, 60.0f}, &first_build_count));
    router->register_page(std::make_unique<TestPage>(
        "widgets", "Widgets", "widgets-root", nandina::geometry::NanSize{80.0f, 40.0f}, &second_build_count));

    EXPECT_EQ(router->current_key(), "overview");
    ASSERT_EQ(router->pages().size(), 2u);
    EXPECT_EQ(router->pages()[0]->title(), "Overview");
    EXPECT_EQ(router->pages()[1]->title(), "Widgets");

    auto current = router->build_current();
    ASSERT_NE(current, nullptr);
    auto* tagged = dynamic_cast<TaggedComponent*>(current.get());
    ASSERT_NE(tagged, nullptr);
    EXPECT_EQ(tagged->tag(), "overview-root");
    EXPECT_EQ(first_build_count, 1);
    EXPECT_EQ(second_build_count, 0);
}

TEST(AppNavigationTest, RouterIgnoresUnknownRouteAndDoesNotDuplicateCallbacks) {
    auto router = nandina::app::NanRouter::create();
    router->register_page(std::make_unique<TestPage>(
        "overview", "Overview", "overview-root", nandina::geometry::NanSize{120.0f, 60.0f}));
    router->register_page(std::make_unique<TestPage>(
        "widgets", "Widgets", "widgets-root", nandina::geometry::NanSize{80.0f, 40.0f}));

    std::vector<std::string> navigations;
    router->on_navigate([&](std::string_view key) {
        navigations.emplace_back(key);
    });

    EXPECT_FALSE(router->navigate_to("missing"));
    EXPECT_EQ(router->current_key(), "overview");
    EXPECT_TRUE(navigations.empty());

    EXPECT_TRUE(router->navigate_to("widgets"));
    ASSERT_EQ(navigations.size(), 1u);
    EXPECT_EQ(navigations.front(), "widgets");

    EXPECT_TRUE(router->navigate_to("widgets"));
    EXPECT_EQ(navigations.size(), 1u);
}

TEST(AppNavigationTest, PageHostDefersPageSwapUntilDrawAndPropagatesBounds) {
    auto router = nandina::app::NanRouter::create();

    int first_build_count = 0;
    int second_build_count = 0;

    router->register_page(std::make_unique<TestPage>(
        "overview", "Overview", "overview-root", nandina::geometry::NanSize{120.0f, 60.0f}, &first_build_count));
    router->register_page(std::make_unique<TestPage>(
        "widgets", "Widgets", "widgets-root", nandina::geometry::NanSize{80.0f, 40.0f}, &second_build_count));

    auto host = std::make_unique<nandina::app::NanPageHost>(router);
    ASSERT_EQ(first_build_count, 1);
    EXPECT_EQ(second_build_count, 0);
    ASSERT_EQ(host->child_count(), 1u);

    host->set_bounds(10.0f, 20.0f, 300.0f, 180.0f);

    auto* initial = dynamic_cast<TaggedComponent*>(host->children().front().get());
    ASSERT_NE(initial, nullptr);
    EXPECT_EQ(initial->tag(), "overview-root");

    EXPECT_TRUE(router->navigate_to("widgets"));
    EXPECT_EQ(second_build_count, 0);
    ASSERT_EQ(host->child_count(), 1u);

    auto* before_draw = dynamic_cast<TaggedComponent*>(host->children().front().get());
    ASSERT_NE(before_draw, nullptr);
    EXPECT_EQ(before_draw->tag(), "overview-root");

    ThorvgCanvasScope canvas_scope(400, 300);
    host->draw(canvas_scope.canvas());

    EXPECT_EQ(second_build_count, 1);
    ASSERT_EQ(host->child_count(), 1u);

    auto* swapped = dynamic_cast<TaggedComponent*>(host->children().front().get());
    ASSERT_NE(swapped, nullptr);
    EXPECT_EQ(swapped->tag(), "widgets-root");

    const auto bounds = swapped->bounds();
    EXPECT_FLOAT_EQ(bounds.x(), 10.0f);
    EXPECT_FLOAT_EQ(bounds.y(), 20.0f);
    EXPECT_FLOAT_EQ(bounds.width(), 300.0f);
    EXPECT_FLOAT_EQ(bounds.height(), 180.0f);
}

TEST(AppNavigationTest, PageHostPropagatesBoundsIntoMountedPageRoot) {
    auto router = nandina::app::NanRouter::create();
    router->register_page(std::make_unique<MountedTestPage>());

    auto host = std::make_unique<nandina::app::NanPageHost>(router);
    ASSERT_EQ(host->child_count(), 1u);

    host->set_bounds(15.0f, 25.0f, 320.0f, 200.0f);

    auto* mounted_component = host->children().front().get();
    ASSERT_NE(mounted_component, nullptr);
    ASSERT_EQ(mounted_component->child_count(), 1u);

    const auto root_bounds = mounted_component->children().front()->bounds();
    EXPECT_FLOAT_EQ(root_bounds.x(), 15.0f);
    EXPECT_FLOAT_EQ(root_bounds.y(), 25.0f);
    EXPECT_FLOAT_EQ(root_bounds.width(), 320.0f);
    EXPECT_FLOAT_EQ(root_bounds.height(), 200.0f);
}

TEST(AppNavigationTest, PageHostLooseMeasureDoesNotEagerlyMeasureMountedPage) {
    auto router = nandina::app::NanRouter::create();
    int measure_count = 0;
    router->register_page(std::make_unique<CountingMountedPage>(&measure_count));

    auto host = std::make_unique<nandina::app::NanPageHost>(router);
    ASSERT_EQ(host->child_count(), 1u);

    host->measure(nandina::geometry::NanConstraints{0.0f, 600.0f, 0.0f, 400.0f});
    EXPECT_EQ(measure_count, 0);

    host->set_bounds(0.0f, 0.0f, 320.0f, 200.0f);

    ThorvgCanvasScope canvas_scope(400, 300);
    host->draw(canvas_scope.canvas());

    EXPECT_GE(measure_count, 1);
}