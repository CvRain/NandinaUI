//
// UI dispatcher and owner-bound asynchronous work tests.
//

#include "app/async_scope.hpp"
#include "app/ui_dispatcher.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>

#include <atomic>
#include <chrono>
#include <exception>
#include <future>
#include <stdexcept>
#include <thread>

namespace
{
    using namespace std::chrono_literals;

    auto wait_for_pending(nandina::app::UiDispatcher& dispatcher) -> bool {
        const auto deadline = std::chrono::steady_clock::now() + 2s;
        while (dispatcher.pending_count() == 0 && std::chrono::steady_clock::now() < deadline) {
            std::this_thread::sleep_for(1ms);
        }
        return dispatcher.pending_count() != 0;
    }
} // namespace

TEST_CASE("UI dispatcher drains a stable task snapshot", "[app][dispatcher]") {
    nandina::app::UiDispatcher dispatcher;
    int value = 0;

    REQUIRE(dispatcher.post([&] {
        value = 1;
        REQUIRE(dispatcher.post([&] { value = 2; }));
    }));

    REQUIRE(dispatcher.drain() == 1);
    REQUIRE(value == 1);
    REQUIRE(dispatcher.drain() == 1);
    REQUIRE(value == 2);
}

TEST_CASE("UI dispatcher rejects draining from a background thread", "[app][dispatcher]") {
    nandina::app::UiDispatcher dispatcher;
    std::exception_ptr error;
    std::thread background([&] {
        try {
            (void)dispatcher.drain();
        }
        catch (...) {
            error = std::current_exception();
        }
    });
    background.join();

    REQUIRE(error != nullptr);
    REQUIRE_THROWS_AS(std::rethrow_exception(error), std::logic_error);
}

TEST_CASE("async completion returns to the UI dispatcher", "[app][async]") {
    nandina::app::UiDispatcher dispatcher;
    nandina::app::BackgroundExecutor executor {1};
    nandina::app::AsyncScope scope {dispatcher, executor};
    int result = 0;
    auto completion_thread = std::thread::id {};

    scope.run(
        [](nandina::app::CancellationToken) { return 42; },
        [&](std::expected<int, std::exception_ptr> outcome) {
            REQUIRE(outcome.has_value());
            result = *outcome;
            completion_thread = std::this_thread::get_id();
        }
    );

    REQUIRE(wait_for_pending(dispatcher));
    REQUIRE(dispatcher.drain() == 1);
    REQUIRE(result == 42);
    REQUIRE(completion_thread == std::this_thread::get_id());
    REQUIRE_FALSE(scope.active());
}

TEST_CASE("new async generation cancels and suppresses the previous result", "[app][async]") {
    nandina::app::UiDispatcher dispatcher;
    nandina::app::BackgroundExecutor executor {1};
    nandina::app::AsyncScope scope {dispatcher, executor};
    std::promise<void> first_started;
    auto started = first_started.get_future();
    int completion_count = 0;
    int result = 0;

    scope.run(
        [&](nandina::app::CancellationToken token) {
            first_started.set_value();
            while (!token.stop_requested()) {
                std::this_thread::yield();
            }
            return 1;
        },
        [&](std::expected<int, std::exception_ptr>) { ++completion_count; }
    );
    REQUIRE(started.wait_for(2s) == std::future_status::ready);

    scope.run(
        [](nandina::app::CancellationToken) { return 2; },
        [&](std::expected<int, std::exception_ptr> outcome) {
            REQUIRE(outcome.has_value());
            ++completion_count;
            result = *outcome;
        }
    );

    REQUIRE(wait_for_pending(dispatcher));
    (void)dispatcher.drain();
    REQUIRE(completion_count == 1);
    REQUIRE(result == 2);
}

TEST_CASE("cleared async scope discards an already queued completion", "[app][async]") {
    nandina::app::UiDispatcher dispatcher;
    nandina::app::BackgroundExecutor executor {1};
    nandina::app::AsyncScope scope {dispatcher, executor};
    bool completed = false;

    scope.run(
        [](nandina::app::CancellationToken) {},
        [&](std::expected<void, std::exception_ptr>) { completed = true; }
    );
    REQUIRE(wait_for_pending(dispatcher));
    scope.clear();
    (void)dispatcher.drain();

    REQUIRE_FALSE(completed);
    REQUIRE_FALSE(scope.active());
}

TEST_CASE("async work reports exceptions on the UI thread", "[app][async]") {
    nandina::app::UiDispatcher dispatcher;
    nandina::app::BackgroundExecutor executor {1};
    nandina::app::AsyncScope scope {dispatcher, executor};
    bool received_error = false;

    scope.run(
        [](nandina::app::CancellationToken) -> int { throw std::runtime_error("failed"); },
        [&](std::expected<int, std::exception_ptr> outcome) {
            REQUIRE_FALSE(outcome.has_value());
            REQUIRE_THROWS_WITH(std::rethrow_exception(outcome.error()), "failed");
            received_error = true;
        }
    );

    REQUIRE(wait_for_pending(dispatcher));
    (void)dispatcher.drain();
    REQUIRE(received_error);
}
