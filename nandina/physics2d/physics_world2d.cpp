#include "physics_world2d.hpp"

#include "../scene/node2d.hpp"

#include <box2d/box2d.h>

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace nandina::physics2d
{

    namespace
    {
        [[nodiscard]] auto pack_world_id(const b2WorldId id) noexcept -> std::uint32_t {
            return b2StoreWorldId(id);
        }

        [[nodiscard]] auto unpack_world_id(const std::uint32_t raw) noexcept -> b2WorldId {
            return b2LoadWorldId(raw);
        }

        [[nodiscard]] auto pack_body_id(const b2BodyId id) noexcept -> std::uint64_t {
            return b2StoreBodyId(id);
        }

        [[nodiscard]] auto unpack_body_id(const std::uint64_t raw) noexcept -> b2BodyId {
            return b2LoadBodyId(raw);
        }

        [[nodiscard]] auto pack_shape_id(const b2ShapeId id) noexcept -> std::uint64_t {
            return b2StoreShapeId(id);
        }

        [[nodiscard]] auto unpack_shape_id(const std::uint64_t raw) noexcept -> b2ShapeId {
            return b2LoadShapeId(raw);
        }

        [[nodiscard]] auto to_b2_vec(const PhysicsWorld2D& world, const foundation::NanPoint value)
            -> b2Vec2 {
            const auto meters = world.pixels_to_meters(value);
            return {.x = meters.get_x(), .y = meters.get_y()};
        }

        [[nodiscard]] auto from_b2_vec(const PhysicsWorld2D& world, const b2Vec2 value)
            -> foundation::NanPoint {
            return world.meters_to_pixels(foundation::NanPoint(value.x, value.y));
        }

        [[nodiscard]] auto to_b2_type(const PhysicsBodyType type) -> b2BodyType {
            switch (type) {
                case PhysicsBodyType::static_body:
                    return b2_staticBody;
                case PhysicsBodyType::kinematic_body:
                    return b2_kinematicBody;
                case PhysicsBodyType::dynamic_body:
                    return b2_dynamicBody;
            }
            return b2_staticBody;
        }
    } // namespace

    PhysicsBody2D::PhysicsBody2D(
        PhysicsWorld2D& world,
        const std::uint64_t raw_id,
        const PhysicsBodyType type
    ):
        world_(&world),
        raw_id_(raw_id),
        type_(type) {}

    auto PhysicsBody2D::valid() const -> bool {
        const auto id = unpack_body_id(raw_id_);
        return world_ != nullptr && B2_IS_NON_NULL(id) && b2Body_IsValid(id);
    }

    auto PhysicsBody2D::type() const -> PhysicsBodyType {
        return type_;
    }

    auto PhysicsBody2D::position() const -> foundation::NanPoint {
        if (!valid()) {
            throw std::logic_error("PhysicsBody2D is not valid");
        }
        return from_b2_vec(*world_, b2Body_GetPosition(unpack_body_id(raw_id_)));
    }

    void PhysicsBody2D::set_position(const foundation::NanPoint position) {
        if (!valid()) {
            throw std::logic_error("PhysicsBody2D is not valid");
        }
        const auto id = unpack_body_id(raw_id_);
        b2Body_SetTransform(id, to_b2_vec(*world_, position), b2Body_GetRotation(id));
    }

    auto PhysicsBody2D::rotation() const -> float {
        if (!valid()) {
            throw std::logic_error("PhysicsBody2D is not valid");
        }
        return b2Rot_GetAngle(b2Body_GetRotation(unpack_body_id(raw_id_)));
    }

    void PhysicsBody2D::set_rotation(const float radians) {
        if (!valid()) {
            throw std::logic_error("PhysicsBody2D is not valid");
        }
        const auto id = unpack_body_id(raw_id_);
        b2Body_SetTransform(id, b2Body_GetPosition(id), b2MakeRot(radians));
    }

    auto PhysicsBody2D::linear_velocity() const -> foundation::NanPoint {
        if (!valid()) {
            throw std::logic_error("PhysicsBody2D is not valid");
        }
        return from_b2_vec(*world_, b2Body_GetLinearVelocity(unpack_body_id(raw_id_)));
    }

    void PhysicsBody2D::set_linear_velocity(const foundation::NanPoint velocity) {
        if (!valid()) {
            throw std::logic_error("PhysicsBody2D is not valid");
        }
        b2Body_SetLinearVelocity(unpack_body_id(raw_id_), to_b2_vec(*world_, velocity));
    }

    auto PhysicsBody2D::visual() const -> scene::NanNode2D* {
        return visual_.lock().get();
    }
    void PhysicsBody2D::bind_visual(const std::shared_ptr<scene::NanNode2D>& visual) {
        visual_ = visual;
    }

    auto PhysicsBody2D::shape_count() const -> std::size_t {
        return shapes_.size();
    }

    auto PhysicsBody2D::create_box(const foundation::NanSize size, PhysicsShapeDef def)
        -> std::shared_ptr<PhysicsShape2D> {
        if (!valid() || !std::isfinite(size.get_width()) || !std::isfinite(size.get_height())
            || size.get_width() <= 0.0F || size.get_height() <= 0.0F || !std::isfinite(def.density)
            || def.density < 0.0F || !std::isfinite(def.friction) || def.friction < 0.0F
            || !std::isfinite(def.restitution) || def.restitution < 0.0F)
        {
            throw std::invalid_argument("invalid physics box");
        }
        auto shape_def = b2DefaultShapeDef();
        shape_def.density = def.density;
        shape_def.material.friction = def.friction;
        shape_def.material.restitution = def.restitution;
        shape_def.filter = {def.filter.category, def.filter.mask, def.filter.group};
        shape_def.isSensor = def.sensor;
        shape_def.enableSensorEvents = def.enable_events;
        shape_def.enableContactEvents = def.enable_events;
        const auto polygon = b2MakeBox(
            size.get_width() / (2.0F * world_->config().pixels_per_meter),
            size.get_height() / (2.0F * world_->config().pixels_per_meter)
        );
        const auto shape_id = b2CreatePolygonShape(unpack_body_id(raw_id_), &shape_def, &polygon);
        if (B2_IS_NULL(shape_id)) {
            throw std::runtime_error("failed to create Box2D box shape");
        }
        auto shape = std::shared_ptr<PhysicsShape2D>(new PhysicsShape2D(
            *world_,
            *this,
            pack_shape_id(shape_id),
            PhysicsShapeType::box,
            def.sensor
        ));
        shapes_.push_back(shape);
        return shape;
    }

    auto PhysicsBody2D::create_circle(const float radius, PhysicsShapeDef def)
        -> std::shared_ptr<PhysicsShape2D> {
        if (!valid() || !std::isfinite(radius) || radius <= 0.0F || !std::isfinite(def.density)
            || def.density < 0.0F || !std::isfinite(def.friction) || def.friction < 0.0F
            || !std::isfinite(def.restitution) || def.restitution < 0.0F)
        {
            throw std::invalid_argument("invalid physics circle");
        }
        auto shape_def = b2DefaultShapeDef();
        shape_def.density = def.density;
        shape_def.material.friction = def.friction;
        shape_def.material.restitution = def.restitution;
        shape_def.filter = {def.filter.category, def.filter.mask, def.filter.group};
        shape_def.isSensor = def.sensor;
        shape_def.enableSensorEvents = def.enable_events;
        shape_def.enableContactEvents = def.enable_events;
        const b2Circle circle {
            .center = {0.0F, 0.0F},
            .radius = radius / world_->config().pixels_per_meter,
        };
        const auto shape_id = b2CreateCircleShape(unpack_body_id(raw_id_), &shape_def, &circle);
        if (B2_IS_NULL(shape_id)) {
            throw std::runtime_error("failed to create Box2D circle shape");
        }
        auto shape = std::shared_ptr<PhysicsShape2D>(new PhysicsShape2D(
            *world_,
            *this,
            pack_shape_id(shape_id),
            PhysicsShapeType::circle,
            def.sensor
        ));
        shapes_.push_back(shape);
        return shape;
    }

    void PhysicsBody2D::destroy_shape(PhysicsShape2D& shape) {
        if (world_ != nullptr && shape.body_ == this) {
            world_->destroy_shape(shape);
        }
    }

    PhysicsShape2D::PhysicsShape2D(
        PhysicsWorld2D& world,
        PhysicsBody2D& body,
        const std::uint64_t raw_id,
        const PhysicsShapeType type,
        const bool sensor
    ):
        world_(&world),
        body_(&body),
        raw_id_(raw_id),
        type_(type),
        sensor_(sensor) {}

    auto PhysicsShape2D::valid() const -> bool {
        const auto id = unpack_shape_id(raw_id_);
        return world_ != nullptr && B2_IS_NON_NULL(id) && b2Shape_IsValid(id);
    }
    auto PhysicsShape2D::type() const -> PhysicsShapeType {
        return type_;
    }
    auto PhysicsShape2D::sensor() const -> bool {
        return sensor_;
    }
    auto PhysicsShape2D::body() const -> PhysicsBody2D* {
        return body_;
    }
    auto PhysicsShape2D::filter() const -> PhysicsFilter {
        if (!valid()) {
            throw std::logic_error("PhysicsShape2D is not valid");
        }
        const auto value = b2Shape_GetFilter(unpack_shape_id(raw_id_));
        return {.category = value.categoryBits, .mask = value.maskBits, .group = value.groupIndex};
    }
    void PhysicsShape2D::set_filter(const PhysicsFilter filter) {
        if (!valid()) {
            throw std::logic_error("PhysicsShape2D is not valid");
        }
        b2Shape_SetFilter(unpack_shape_id(raw_id_), {filter.category, filter.mask, filter.group});
    }

    PhysicsWorld2D::PhysicsWorld2D(PhysicsWorldConfig config): config_(config) {
        if (!std::isfinite(config_.pixels_per_meter) || config_.pixels_per_meter <= 0.0F) {
            throw std::invalid_argument("pixels_per_meter must be finite and positive");
        }
        if (!std::isfinite(config_.fixed_timestep) || config_.fixed_timestep <= 0.0F) {
            throw std::invalid_argument("fixed_timestep must be finite and positive");
        }
        if (config_.substeps <= 0 || config_.max_steps_per_tick <= 0) {
            throw std::invalid_argument("physics step counts must be positive");
        }
        auto definition = b2DefaultWorldDef();
        definition.gravity = to_b2_vec(*this, config_.gravity);
        const auto world_id = b2CreateWorld(&definition);
        if (B2_IS_NULL(world_id)) {
            throw std::runtime_error("failed to create Box2D world");
        }
        raw_world_id_ = pack_world_id(world_id);
    }

    PhysicsWorld2D::~PhysicsWorld2D() {
        destroy_world();
    }

    auto PhysicsWorld2D::create(PhysicsWorldConfig config) -> std::shared_ptr<PhysicsWorld2D> {
        return std::make_shared<PhysicsWorld2D>(config);
    }

    auto PhysicsWorld2D::config() const -> const PhysicsWorldConfig& {
        return config_;
    }
    auto PhysicsWorld2D::body_count() const -> std::size_t {
        return bodies_.size();
    }
    auto PhysicsWorld2D::accumulator() const -> float {
        return accumulator_;
    }
    auto PhysicsWorld2D::total_steps() const -> std::size_t {
        return total_steps_;
    }
    auto PhysicsWorld2D::events() const -> const std::vector<PhysicsTouchEvent>& {
        return events_;
    }
    void PhysicsWorld2D::set_event_handler(std::function<void(const PhysicsTouchEvent&)> handler) {
        event_handler_ = std::move(handler);
    }

    auto PhysicsWorld2D::pixels_to_meters(const foundation::NanPoint value) const
        -> foundation::NanPoint {
        return value * (1.0F / config_.pixels_per_meter);
    }

    auto PhysicsWorld2D::meters_to_pixels(const foundation::NanPoint value) const
        -> foundation::NanPoint {
        return value * config_.pixels_per_meter;
    }

    auto PhysicsWorld2D::find_shape(const std::uint64_t raw_shape_id) const
        -> std::shared_ptr<PhysicsShape2D> {
        for (const auto& body: bodies_) {
            for (const auto& shape: body->shapes_) {
                if (shape->raw_id_ == raw_shape_id) {
                    return shape;
                }
            }
        }
        return nullptr;
    }

    void PhysicsWorld2D::collect_events() {
        const auto world_id = unpack_world_id(raw_world_id_);
        const auto contacts = b2World_GetContactEvents(world_id);
        for (int i = 0; i < contacts.beginCount; ++i) {
            auto first = find_shape(pack_shape_id(contacts.beginEvents[i].shapeIdA));
            auto second = find_shape(pack_shape_id(contacts.beginEvents[i].shapeIdB));
            if (first != nullptr && second != nullptr) {
                events_.push_back(
                    {PhysicsTouchKind::contact_begin, std::move(first), std::move(second)}
                );
            }
        }
        for (int i = 0; i < contacts.endCount; ++i) {
            auto first = find_shape(pack_shape_id(contacts.endEvents[i].shapeIdA));
            auto second = find_shape(pack_shape_id(contacts.endEvents[i].shapeIdB));
            if (first != nullptr && second != nullptr) {
                events_.push_back(
                    {PhysicsTouchKind::contact_end, std::move(first), std::move(second)}
                );
            }
        }

        const auto sensors = b2World_GetSensorEvents(world_id);
        for (int i = 0; i < sensors.beginCount; ++i) {
            auto sensor = find_shape(pack_shape_id(sensors.beginEvents[i].sensorShapeId));
            auto visitor = find_shape(pack_shape_id(sensors.beginEvents[i].visitorShapeId));
            if (sensor != nullptr && visitor != nullptr) {
                events_.push_back(
                    {PhysicsTouchKind::sensor_begin, std::move(sensor), std::move(visitor)}
                );
            }
        }
        for (int i = 0; i < sensors.endCount; ++i) {
            auto sensor = find_shape(pack_shape_id(sensors.endEvents[i].sensorShapeId));
            auto visitor = find_shape(pack_shape_id(sensors.endEvents[i].visitorShapeId));
            if (sensor != nullptr && visitor != nullptr) {
                events_.push_back(
                    {PhysicsTouchKind::sensor_end, std::move(sensor), std::move(visitor)}
                );
            }
        }
    }

    void PhysicsWorld2D::flush_pending_destroys() {
        auto shapes = std::move(pending_shape_destroys_);
        pending_shape_destroys_.clear();
        for (auto* shape: shapes) {
            if (shape != nullptr) {
                shape->destroy_pending_ = false;
                destroy_shape(*shape);
            }
        }
        auto bodies = std::move(pending_body_destroys_);
        pending_body_destroys_.clear();
        for (auto* body: bodies) {
            if (body != nullptr) {
                destroy_body(*body);
            }
        }
    }

    auto PhysicsWorld2D::create_body(PhysicsBodyDef def) -> std::shared_ptr<PhysicsBody2D> {
        const auto world_id = unpack_world_id(raw_world_id_);
        if (!b2World_IsValid(world_id)) {
            throw std::logic_error("PhysicsWorld2D is not valid");
        }
        auto definition = b2DefaultBodyDef();
        definition.type = to_b2_type(def.type);
        definition.position = to_b2_vec(*this, def.position);
        definition.rotation = b2MakeRot(def.rotation);
        definition.linearVelocity = to_b2_vec(*this, def.linear_velocity);
        definition.angularVelocity = def.angular_velocity;
        definition.fixedRotation = def.fixed_rotation;
        definition.isBullet = def.bullet;
        const auto id = b2CreateBody(world_id, &definition);
        if (B2_IS_NULL(id)) {
            throw std::runtime_error("failed to create Box2D body");
        }
        auto body =
            std::shared_ptr<PhysicsBody2D>(new PhysicsBody2D(*this, pack_body_id(id), def.type));
        body->bind_visual(def.visual);
        bodies_.push_back(body);
        return body;
    }

    void PhysicsWorld2D::destroy_body(PhysicsBody2D& body) {
        if (body.world_ != this) {
            return;
        }
        if (stepping_) {
            if (std::ranges::find(pending_body_destroys_, &body) == pending_body_destroys_.end()) {
                pending_body_destroys_.push_back(&body);
            }
            return;
        }
        for (const auto& shape: body.shapes_) {
            shape->raw_id_ = 0;
            shape->world_ = nullptr;
            shape->body_ = nullptr;
        }
        body.shapes_.clear();
        if (body.valid()) {
            b2DestroyBody(unpack_body_id(body.raw_id_));
        }
        body.raw_id_ = 0;
        body.world_ = nullptr;
        body.visual_.reset();
        std::erase_if(bodies_, [&body](const auto& owned) { return owned.get() == &body; });
    }

    void PhysicsWorld2D::destroy_shape(PhysicsShape2D& shape) {
        if (shape.world_ != this || shape.destroy_pending_) {
            return;
        }
        if (stepping_) {
            shape.destroy_pending_ = true;
            pending_shape_destroys_.push_back(&shape);
            return;
        }
        auto* owner = shape.body_;
        if (shape.valid()) {
            b2DestroyShape(unpack_shape_id(shape.raw_id_), true);
        }
        shape.raw_id_ = 0;
        shape.world_ = nullptr;
        shape.body_ = nullptr;
        shape.destroy_pending_ = false;
        if (owner != nullptr) {
            std::erase_if(owner->shapes_, [&shape](const auto& owned) {
                return owned.get() == &shape;
            });
        }
    }

    void PhysicsWorld2D::sync_scene_to_physics() {
        for (const auto& body: bodies_) {
            auto visual = body->visual_.lock();
            if (!body->valid() || visual == nullptr || body->type_ == PhysicsBodyType::dynamic_body)
            {
                continue;
            }
            b2Body_SetTransform(
                unpack_body_id(body->raw_id_),
                to_b2_vec(*this, visual->global_position()),
                b2MakeRot(visual->global_rotation())
            );
        }
    }

    void PhysicsWorld2D::sync_physics_to_scene() {
        for (const auto& body: bodies_) {
            auto visual = body->visual_.lock();
            if (!body->valid() || visual == nullptr || body->type_ != PhysicsBodyType::dynamic_body)
            {
                continue;
            }
            const auto id = unpack_body_id(body->raw_id_);
            visual->set_global_position(from_b2_vec(*this, b2Body_GetPosition(id)));
            const auto parent_rotation =
                visual->parent() != nullptr && visual->parent()->as_node2d() != nullptr
                ? visual->parent()->as_node2d()->global_rotation()
                : 0.0F;
            visual->set_rotation(b2Rot_GetAngle(b2Body_GetRotation(id)) - parent_rotation);
        }
    }

    auto PhysicsWorld2D::step(const float dt) -> int {
        const auto world_id = unpack_world_id(raw_world_id_);
        if (!std::isfinite(dt) || dt < 0.0F || !b2World_IsValid(world_id)) {
            return 0;
        }
        accumulator_ += dt;
        events_.clear();
        int steps = 0;
        stepping_ = true;
        while (accumulator_ >= config_.fixed_timestep && steps < config_.max_steps_per_tick) {
            sync_scene_to_physics();
            b2World_Step(world_id, config_.fixed_timestep, config_.substeps);
            const auto event_begin = events_.size();
            collect_events();
            if (event_handler_) {
                for (auto index = event_begin; index < events_.size(); ++index) {
                    event_handler_(events_[index]);
                }
            }
            sync_physics_to_scene();
            accumulator_ -= config_.fixed_timestep;
            ++steps;
            ++total_steps_;
        }
        stepping_ = false;
        flush_pending_destroys();
        if (steps == config_.max_steps_per_tick && accumulator_ >= config_.fixed_timestep) {
            accumulator_ = std::fmod(accumulator_, config_.fixed_timestep);
        }
        return steps;
    }

    void PhysicsWorld2D::physics_step(const float dt) {
        (void)step(dt);
    }

    void PhysicsWorld2D::destroy_world() {
        for (const auto& body: bodies_) {
            for (const auto& shape: body->shapes_) {
                shape->raw_id_ = 0;
                shape->world_ = nullptr;
                shape->body_ = nullptr;
                shape->destroy_pending_ = false;
            }
            body->shapes_.clear();
            body->raw_id_ = 0;
            body->world_ = nullptr;
            body->visual_.reset();
        }
        bodies_.clear();
        events_.clear();
        pending_body_destroys_.clear();
        pending_shape_destroys_.clear();
        const auto world_id = unpack_world_id(raw_world_id_);
        if (B2_IS_NON_NULL(world_id) && b2World_IsValid(world_id)) {
            b2DestroyWorld(world_id);
        }
        raw_world_id_ = 0;
        accumulator_ = 0.0F;
    }

} // namespace nandina::physics2d
