#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "physics2d/physics_world2d.hpp"
#include "scene/node2d.hpp"
#include "scene/scene_tree.hpp"
#include "widget/authoring.hpp"

using namespace nandina;

TEST_CASE("PhysicsWorld2D validates configuration and converts units", "[physics2d]") {
    REQUIRE_THROWS_AS(
        physics2d::PhysicsWorld2D(physics2d::PhysicsWorldConfig {.pixels_per_meter = 0.0F}),
        std::invalid_argument
    );

    physics2d::PhysicsWorld2D world(
        physics2d::PhysicsWorldConfig {
            .gravity = foundation::NanPoint::zero(),
            .pixels_per_meter = 50.0F,
        }
    );
    REQUIRE(world.pixels_to_meters(foundation::NanPoint(100.0F, 25.0F))
            == foundation::NanPoint(2.0F, 0.5F));
    REQUIRE(world.meters_to_pixels(foundation::NanPoint(2.0F, 0.5F))
            == foundation::NanPoint(100.0F, 25.0F));
}

TEST_CASE("authoring configures the concrete physics world", "[physics2d][authoring]") {
    int events = 0;
    auto world = widget::authoring::from(physics2d::PhysicsWorld2D::create(
                     physics2d::PhysicsWorldConfig {
                         .gravity = foundation::NanPoint::zero(),
                         .pixels_per_meter = 40.0F,
                     }
    ))
                     .configure([&](physics2d::PhysicsWorld2D& value) {
                         value.set_event_handler([&](const auto&) { ++events; });
                     })
                     .build();

    static_assert(
        std::same_as<decltype(world), std::shared_ptr<physics2d::PhysicsWorld2D>>
    );
    REQUIRE(world->config().pixels_per_meter == Catch::Approx(40.0F));
    REQUIRE(events == 0);
}

TEST_CASE("PhysicsWorld2D uses deterministic fixed steps and caps catch-up", "[physics2d]") {
    physics2d::PhysicsWorld2D world(physics2d::PhysicsWorldConfig {
        .gravity = foundation::NanPoint::zero(),
        .fixed_timestep = 0.1F,
        .substeps = 1,
        .max_steps_per_tick = 2,
    });

    REQUIRE(world.step(0.05F) == 0);
    REQUIRE(world.step(0.05F) == 1);
    REQUIRE(world.total_steps() == 1);
    REQUIRE(world.step(0.35F) == 2);
    REQUIRE(world.total_steps() == 3);
    REQUIRE(world.accumulator() < world.config().fixed_timestep);
}

TEST_CASE("dynamic bodies drive bound scene nodes after physics phase", "[physics2d][scene]") {
    auto root = std::make_shared<scene::NanNode2D>();
    root->set_rotation(0.25F);
    auto visual = std::make_shared<scene::NanNode2D>();
    auto world = physics2d::PhysicsWorld2D::create(physics2d::PhysicsWorldConfig {
        .gravity = foundation::NanPoint::zero(),
        .pixels_per_meter = 100.0F,
        .fixed_timestep = 0.1F,
        .substeps = 1,
    });
    root->add_child(visual);
    root->add_child(world);
    auto body = world->create_body(physics2d::PhysicsBodyDef {
        .type = physics2d::PhysicsBodyType::dynamic_body,
        .position = foundation::NanPoint(10.0F, 20.0F),
        .visual = visual,
    });
    body->set_position(foundation::NanPoint(110.0F, 20.0F));
    body->set_rotation(0.75F);
    scene::NanSceneTree tree;
    tree.set_root(root);

    {
        auto phase = tree.enter_phase(scene::FramePhase::physics);
        tree.physics_step(0.1F);
    }

    REQUIRE(body->position().get_x() == Catch::Approx(110.0F).margin(0.01F));
    REQUIRE(visual->global_position().get_x() == Catch::Approx(110.0F).margin(0.01F));
    REQUIRE(visual->global_position().get_y() == Catch::Approx(20.0F).margin(0.01F));
    REQUIRE(visual->global_rotation() == Catch::Approx(0.75F).margin(0.001F));
}

TEST_CASE("kinematic bodies take authoritative transforms from scene state", "[physics2d][scene]") {
    auto visual = std::make_shared<scene::NanNode2D>();
    visual->set_position(foundation::NanPoint(240.0F, 80.0F));
    auto world = physics2d::PhysicsWorld2D::create(physics2d::PhysicsWorldConfig {
        .gravity = foundation::NanPoint::zero(),
        .pixels_per_meter = 80.0F,
        .fixed_timestep = 0.1F,
        .substeps = 1,
    });
    auto body = world->create_body(physics2d::PhysicsBodyDef {
        .type = physics2d::PhysicsBodyType::kinematic_body,
        .visual = visual,
    });

    REQUIRE(world->step(0.1F) == 1);
    REQUIRE(body->position().get_x() == Catch::Approx(240.0F).margin(0.01F));
    REQUIRE(body->position().get_y() == Catch::Approx(80.0F).margin(0.01F));
}

TEST_CASE("body and world teardown invalidate external handles", "[physics2d][lifecycle]") {
    auto world = physics2d::PhysicsWorld2D::create(
        physics2d::PhysicsWorldConfig {.gravity = foundation::NanPoint::zero()}
    );
    auto first = world->create_body();
    auto second = world->create_body();
    REQUIRE(world->body_count() == 2);

    world->destroy_body(*first);
    REQUIRE_FALSE(first->valid());
    REQUIRE(world->body_count() == 1);

    world.reset();
    REQUIRE_FALSE(second->valid());
}

TEST_CASE("box and circle shapes expose filter and sensor contracts", "[physics2d][shape]") {
    auto world = physics2d::PhysicsWorld2D::create(
        physics2d::PhysicsWorldConfig {.gravity = foundation::NanPoint::zero()}
    );
    auto body = world->create_body({.type = physics2d::PhysicsBodyType::dynamic_body});
    auto box = body->create_box(foundation::NanSize(20.0F, 30.0F), {
        .density = 2.0F,
        .filter = {.category = 2, .mask = 4, .group = -1},
    });
    auto circle = body->create_circle(5.0F, {.sensor = true});

    REQUIRE(body->shape_count() == 2);
    REQUIRE(box->type() == physics2d::PhysicsShapeType::box);
    REQUIRE_FALSE(box->sensor());
    REQUIRE(circle->type() == physics2d::PhysicsShapeType::circle);
    REQUIRE(circle->sensor());
    REQUIRE(box->filter().category == 2);
    REQUIRE(box->filter().mask == 4);
    REQUIRE(box->filter().group == -1);

    body->destroy_shape(*box);
    REQUIRE_FALSE(box->valid());
    REQUIRE(body->shape_count() == 1);
}

TEST_CASE("sensor events are snapshotted and shape destruction is deferred", "[physics2d][events]") {
    auto world = physics2d::PhysicsWorld2D::create(physics2d::PhysicsWorldConfig {
        .gravity = foundation::NanPoint::zero(),
        .fixed_timestep = 1.0F / 60.0F,
        .substeps = 2,
    });
    auto sensor_body = world->create_body({.type = physics2d::PhysicsBodyType::static_body});
    auto visitor_body = world->create_body({
        .type = physics2d::PhysicsBodyType::dynamic_body,
        .position = foundation::NanPoint(5.0F, 0.0F),
    });
    auto sensor = sensor_body->create_circle(10.0F, {.sensor = true});
    auto visitor = visitor_body->create_circle(5.0F);
    std::size_t callbacks = 0;
    world->set_event_handler([&](const physics2d::PhysicsTouchEvent& event) {
        ++callbacks;
        if (event.kind == physics2d::PhysicsTouchKind::sensor_begin) {
            sensor_body->destroy_shape(*sensor);
        }
    });

    REQUIRE(world->step(1.0F / 60.0F) == 1);
    REQUIRE(callbacks >= 1);
    REQUIRE_FALSE(sensor->valid());
    REQUIRE_FALSE(world->events().empty());
    REQUIRE(world->events().front().first != nullptr);
    REQUIRE(world->events().front().second != nullptr);
    REQUIRE(visitor->valid());
}

TEST_CASE("dynamic shape bodies respond to gravity", "[physics2d][simulation]") {
    auto world = physics2d::PhysicsWorld2D::create(physics2d::PhysicsWorldConfig {
        .gravity = foundation::NanPoint(0.0F, 1000.0F),
        .fixed_timestep = 0.1F,
        .substeps = 4,
    });
    auto body = world->create_body({
        .type = physics2d::PhysicsBodyType::dynamic_body,
        .position = foundation::NanPoint(0.0F, 0.0F),
    });
    (void)body->create_circle(5.0F);
    REQUIRE(world->step(0.1F) == 1);
    REQUIRE(body->position().get_y() > 0.0F);
}
