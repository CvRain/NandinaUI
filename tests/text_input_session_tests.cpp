//
// text_input_session_tests — D1 phase-2 text session protocol (headless).
//
// Uses a mock ITextInputService and fake editing targets: session identity, composition
// routing, commit-once, stale-event filtering, focus/window lifecycle, read-only targets,
// legacy committed-text fallback and caret-area updates.
//

#include <catch2/catch_test_macros.hpp>

#include <nandina/scene/control.hpp>
#include <nandina/scene/scene_tree.hpp>
#include <nandina/scene/text_input_service.hpp>

#include <memory>
#include <string>
#include <vector>

using namespace nandina;

namespace
{
    class FakeTextInputService final: public scene::ITextInputService {
    public:
        [[nodiscard]] auto capabilities() const -> scene::TextInputCapabilities override {
            return scene::TextInputCapabilities::composition
                | (input_area ? scene::TextInputCapabilities::input_area
                              : scene::TextInputCapabilities::none);
        }

        auto begin_session(const scene::TextInputBeginRequest& request)
            -> scene::TextInputRequestResult override {
            ++begin_count;
            last_request = request;
            active = request.session;
            return begin_result;
        }

        auto
        update_caret_area(const scene::TextInputSession session, const scene::TextInputArea& area)
            -> scene::TextInputRequestResult override {
            ++area_updates;
            last_area_session = session;
            last_area = area;
            return scene::TextInputRequestResult::accepted;
        }

        auto
        request_keyboard_visibility(const scene::TextInputSession /*session*/, const bool visible)
            -> scene::TextInputRequestResult override {
            ++visibility_requests;
            last_visibility = visible;
            return scene::TextInputRequestResult::unsupported;
        }

        void end_session(
            const scene::TextInputSession session,
            const scene::TextInputEndReason reason
        ) override {
            ++end_count;
            last_end_reason = reason;
            if (active == session) {
                active = {};
            }
        }

        void set_sink(scene::ITextInputSessionSink* next) noexcept override {
            sink = next;
        }

        void dispatch(const scene::TextInputPlatformEvent& event) override {
            if (sink == nullptr || !active) {
                return;
            }
            switch (event.type) {
                case scene::TextInputEventType::composition_updated:
                    sink->on_composition_updated(
                        active,
                        event.text,
                        event.selection_start,
                        event.selection_end
                    );
                    break;
                case scene::TextInputEventType::composition_committed:
                    sink->on_text_committed(active, event.text);
                    break;
                case scene::TextInputEventType::composition_cancelled:
                    sink->on_composition_cancelled(active);
                    break;
                case scene::TextInputEventType::keyboard_visibility_changed:
                    sink->on_keyboard_visibility_changed(active, event.visibility);
                    break;
            }
        }

        scene::ITextInputSessionSink* sink = nullptr;
        scene::TextInputSession active {};
        scene::TextInputSession last_area_session {};
        scene::TextInputArea last_area {};
        scene::TextInputBeginRequest last_request {};
        scene::TextInputRequestResult begin_result = scene::TextInputRequestResult::accepted;
        scene::TextInputEndReason last_end_reason = scene::TextInputEndReason::focus_lost;
        bool input_area = false;
        int begin_count = 0;
        int end_count = 0;
        int area_updates = 0;
        int visibility_requests = 0;
        bool last_visibility = false;
    };

    class FakeTarget final: public scene::NanNode2D, public scene::ITextInputTarget {
    public:
        FakeTarget(std::string label): label_(std::move(label)) {}

        [[nodiscard]] auto is_focusable() const -> bool override {
            return true;
        }

        [[nodiscard]] auto text_input_target() -> scene::ITextInputTarget* override {
            return this;
        }

        [[nodiscard]] auto text_input_purpose() const -> scene::TextInputPurpose override {
            return purpose;
        }
        [[nodiscard]] auto text_input_multiline() const -> bool override {
            return multiline;
        }
        [[nodiscard]] auto text_input_read_only() const -> bool override {
            return read_only;
        }
        [[nodiscard]] auto text_input_caret_area() const -> foundation::NanRect override {
            return foundation::NanRect::from_xywh(12.0F, 6.0F, 1.0F, 18.0F);
        }

        void text_input_set_composition(
            const std::string_view text,
            const std::size_t start,
            const std::size_t end
        ) override {
            composition = std::string(text);
            selection_start = start;
            selection_end = end;
        }
        void text_input_commit(const std::string_view text) override {
            commits.emplace_back(text);
        }
        void text_input_cancel_composition() override {
            ++cancels;
            composition.clear();
        }

        auto on_input(scene::InputEvent& event) -> bool override {
            if (event.type() == scene::EventType::text_input) {
                legacy_texts.emplace_back(
                    std::string(static_cast<scene::TextInputEvent&>(event).text())
                );
            }
            return false;
        }

        scene::TextInputPurpose purpose = scene::TextInputPurpose::text;
        bool multiline = false;
        bool read_only = false;
        std::string composition;
        std::size_t selection_start = 0;
        std::size_t selection_end = 0;
        std::vector<std::string> commits;
        std::vector<std::string> legacy_texts;
        int cancels = 0;

    private:
        std::string label_;
    };

    struct Fixture {
        // The service must outlive the tree; the tree destructor clears the session without
        // calling a destroyed service.
        FakeTextInputService service;
        scene::NanSceneTree tree;
        std::shared_ptr<scene::NanControl> root = std::make_shared<scene::NanControl>();
        std::shared_ptr<FakeTarget> editable = std::make_shared<FakeTarget>("editable");
        std::shared_ptr<FakeTarget> button = std::make_shared<FakeTarget>("button");

        Fixture() {
            button->read_only = true; // focusable but not an editing target
            root->add_child(editable);
            root->add_child(button);
            tree.set_root(root);
            tree.set_window_active(true);
            tree.set_text_input_service(&service);
        }
    };

    auto composition_update(const std::string& text, std::size_t start, std::size_t end)
        -> scene::TextInputPlatformEvent {
        return scene::TextInputPlatformEvent {
            .type = scene::TextInputEventType::composition_updated,
            .text = text,
            .selection_start = start,
            .selection_end = end,
        };
    }

    auto committed(const std::string& text) -> scene::TextInputPlatformEvent {
        return scene::TextInputPlatformEvent {
            .type = scene::TextInputEventType::composition_committed,
            .text = text,
        };
    }
} // namespace

TEST_CASE("editable focus opens a session and leaving closes it", "[input][text-session]") {
    Fixture fixture;
    auto& tree = fixture.tree;
    auto& service = fixture.service;

    REQUIRE_FALSE(tree.text_input_session_active());
    tree.set_focus(fixture.editable.get());
    REQUIRE(service.begin_count == 1);
    REQUIRE(tree.text_input_session_active());
    REQUIRE(service.last_request.purpose == scene::TextInputPurpose::text);
    REQUIRE_FALSE(service.last_request.multiline);
    REQUIRE(service.active == tree.active_text_input_session());

    tree.set_focus(fixture.button.get());
    REQUIRE(service.end_count == 1);
    REQUIRE(service.last_end_reason == scene::TextInputEndReason::focus_lost);
    REQUIRE_FALSE(tree.text_input_session_active());
    REQUIRE(fixture.editable->cancels >= 1);
}

TEST_CASE(
    "composition routes to the active target and commit happens once",
    "[input][text-session]"
) {
    Fixture fixture;
    auto& tree = fixture.tree;
    tree.set_focus(fixture.editable.get());

    tree.apply_text_input_event(composition_update("にほ", 2, 2));
    REQUIRE(fixture.editable->composition == "にほ");
    REQUIRE(fixture.editable->selection_start == 2);
    REQUIRE(fixture.editable->selection_end == 2);

    tree.apply_text_input_event(committed("日本"));
    REQUIRE(fixture.editable->commits == std::vector<std::string> {"日本"});
    // Session path owns committed text: no duplicate legacy TextInputEvent dispatch.
    REQUIRE(fixture.editable->legacy_texts.empty());
}

TEST_CASE("empty composition update cancels the preedit", "[input][text-session]") {
    Fixture fixture;
    auto& tree = fixture.tree;
    tree.set_focus(fixture.editable.get());

    tree.apply_text_input_event(composition_update("にほ", 1, 2));
    REQUIRE_FALSE(fixture.editable->composition.empty());
    const auto cancels = fixture.editable->cancels;

    tree.apply_text_input_event(composition_update("", 0, 0));
    REQUIRE(fixture.editable->composition.empty());
    REQUIRE(fixture.editable->cancels == cancels + 1);
}

TEST_CASE("stale session events never reach the target", "[input][text-session]") {
    Fixture fixture;
    auto& tree = fixture.tree;
    auto& service = fixture.service;
    tree.set_focus(fixture.editable.get());
    const auto stale = tree.active_text_input_session();
    REQUIRE(stale);

    tree.set_focus(fixture.button.get());
    tree.set_focus(fixture.editable.get());
    REQUIRE(tree.active_text_input_session() != stale);

    // A late callback from the old session is filtered by session identity.
    service.sink->on_text_committed(stale, "stale");
    REQUIRE(fixture.editable->commits.empty());
    service.sink->on_composition_updated(stale, "旧", 1, 1);
    REQUIRE(fixture.editable->composition.empty());
}

TEST_CASE(
    "window deactivation pauses the session and reactivation opens a new one",
    "[input][text-session]"
) {
    Fixture fixture;
    auto& tree = fixture.tree;
    auto& service = fixture.service;
    tree.set_focus(fixture.editable.get());
    const auto first = tree.active_text_input_session();
    REQUIRE(first);

    tree.set_window_active(false);
    REQUIRE(service.end_count == 1);
    REQUIRE(service.last_end_reason == scene::TextInputEndReason::window_inactive);
    REQUIRE_FALSE(tree.text_input_session_active());
    REQUIRE(tree.focused_node() == fixture.editable.get());

    tree.set_window_active(true);
    REQUIRE(tree.text_input_session_active());
    REQUIRE(tree.active_text_input_session() != first);
}

TEST_CASE("read-only targets do not open an editing session", "[input][text-session]") {
    Fixture fixture;
    fixture.editable->read_only = true;
    fixture.tree.set_focus(fixture.editable.get());
    REQUIRE(fixture.service.begin_count == 0);
    REQUIRE_FALSE(fixture.tree.text_input_session_active());
    REQUIRE(fixture.tree.focused_node() == fixture.editable.get());
}

TEST_CASE("purpose and multiline reach the begin request", "[input][text-session]") {
    Fixture fixture;
    fixture.editable->purpose = scene::TextInputPurpose::password;
    fixture.editable->multiline = true;
    fixture.tree.set_focus(fixture.editable.get());
    REQUIRE(fixture.service.last_request.purpose == scene::TextInputPurpose::password);
    REQUIRE(fixture.service.last_request.multiline);
}

TEST_CASE(
    "committed text falls back to legacy dispatch without a service",
    "[input][text-session]"
) {
    scene::NanSceneTree tree;
    auto root = std::make_shared<scene::NanControl>();
    auto target = std::make_shared<FakeTarget>("edit");
    root->add_child(target);
    tree.set_root(root);
    tree.set_window_active(true);
    tree.set_focus(target.get());

    tree.apply_text_input_event(composition_update("にほ", 1, 1));
    REQUIRE(target->composition.empty());
    tree.apply_text_input_event(committed("日本"));
    REQUIRE(target->commits.empty());
    REQUIRE(target->legacy_texts == std::vector<std::string> {"日本"});
}

TEST_CASE(
    "detaching the service ends the session without touching focus",
    "[input][text-session]"
) {
    Fixture fixture;
    auto& tree = fixture.tree;
    auto& service = fixture.service;
    tree.set_focus(fixture.editable.get());
    REQUIRE(tree.text_input_session_active());

    tree.set_text_input_service(nullptr);
    REQUIRE_FALSE(tree.text_input_session_active());
    REQUIRE(tree.focused_node() == fixture.editable.get());
    REQUIRE(service.sink == nullptr);
    REQUIRE(service.end_count == 1);
    REQUIRE(service.last_end_reason == scene::TextInputEndReason::service_detached);
}

TEST_CASE(
    "caret area updates only when the service advertises the capability",
    "[input][text-session]"
) {
    Fixture fixture;
    auto& tree = fixture.tree;
    auto& service = fixture.service;
    tree.set_focus(fixture.editable.get());

    tree.update_text_input_area();
    REQUIRE(service.area_updates == 0);

    service.input_area = true;
    tree.update_text_input_area();
    REQUIRE(service.area_updates == 1);
    REQUIRE(service.last_area_session == tree.active_text_input_session());
    REQUIRE(service.last_area.caret.get_left() == 12.0F);
    REQUIRE(service.last_area.caret.get_top() == 6.0F);
}
