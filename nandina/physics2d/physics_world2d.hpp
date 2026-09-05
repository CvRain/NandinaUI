#ifndef NANDINA_EXPERIMENT_PHYSICS_WORLD2D_HPP
#define NANDINA_EXPERIMENT_PHYSICS_WORLD2D_HPP

#include "../foundation/geometry.hpp"
#include "../scene/node2d.hpp"

#include <box2d/box2d.h>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

namespace nandina::scene
{
    class NanNode2D;
}

namespace nandina::physics2d
{

    enum class PhysicsBodyType {
        static_body,
        kinematic_body,
        dynamic_body,
    };

    struct PhysicsWorldConfig {
        foundation::NanPoint gravity {0.0F, 980.0F};
        float pixels_per_meter = 100.0F;
        float fixed_timestep = 1.0F / 60.0F;
        int substeps = 4;
        int max_steps_per_tick = 8;
    };

    struct PhysicsBodyDef {
        PhysicsBodyType type = PhysicsBodyType::static_body;
        foundation::NanPoint position {};
        float rotation = 0.0F;
        foundation::NanPoint linear_velocity {};
        float angular_velocity = 0.0F;
        bool fixed_rotation = false;
        bool bullet = false;
        std::shared_ptr<scene::NanNode2D> visual;
    };

    struct PhysicsFilter {
        std::uint64_t category = 1;
        std::uint64_t mask = ~std::uint64_t {0};
        int group = 0;
    };

    struct PhysicsShapeDef {
        float density = 1.0F;
        float friction = 0.6F;
        float restitution = 0.0F;
        PhysicsFilter filter {};
        bool sensor = false;
        bool enable_events = true;
    };

    enum class PhysicsShapeType {
        box,
        circle,
    };

    enum class PhysicsTouchKind {
        contact_begin,
        contact_end,
        sensor_begin,
        sensor_end,
    };

    class PhysicsWorld2D;
    class PhysicsShape2D;

    struct PhysicsTouchEvent {
        PhysicsTouchKind kind;
        std::shared_ptr<PhysicsShape2D> first;
        std::shared_ptr<PhysicsShape2D> second;
    };

    class PhysicsBody2D final {
        friend class PhysicsWorld2D;

    public:
        ~PhysicsBody2D() = default;
        PhysicsBody2D(const PhysicsBody2D&) = delete;
        auto operator=(const PhysicsBody2D&) -> PhysicsBody2D& = delete;

        [[nodiscard]] auto valid() const -> bool;
        [[nodiscard]] auto type() const -> PhysicsBodyType;
        [[nodiscard]] auto position() const -> foundation::NanPoint;
        void set_position(foundation::NanPoint position);
        [[nodiscard]] auto rotation() const -> float;
        void set_rotation(float radians);
        [[nodiscard]] auto linear_velocity() const -> foundation::NanPoint;
        void set_linear_velocity(foundation::NanPoint velocity);
        [[nodiscard]] auto visual() const -> scene::NanNode2D*;
        void bind_visual(const std::shared_ptr<scene::NanNode2D>& visual);
        [[nodiscard]] auto shape_count() const -> std::size_t;
        auto create_box(foundation::NanSize size, PhysicsShapeDef def = {})
            -> std::shared_ptr<PhysicsShape2D>;
        auto create_circle(float radius, PhysicsShapeDef def = {})
            -> std::shared_ptr<PhysicsShape2D>;
        void destroy_shape(PhysicsShape2D& shape);

    private:
        PhysicsBody2D(PhysicsWorld2D& world, b2BodyId id, PhysicsBodyType type);

        PhysicsWorld2D* world_;
        b2BodyId id_ = b2_nullBodyId;
        PhysicsBodyType type_;
        std::weak_ptr<scene::NanNode2D> visual_;
        std::vector<std::shared_ptr<PhysicsShape2D>> shapes_;
    };

    class PhysicsShape2D final {
        friend class PhysicsWorld2D;
        friend class PhysicsBody2D;

    public:
        ~PhysicsShape2D() = default;
        PhysicsShape2D(const PhysicsShape2D&) = delete;
        auto operator=(const PhysicsShape2D&) -> PhysicsShape2D& = delete;

        [[nodiscard]] auto valid() const -> bool;
        [[nodiscard]] auto type() const -> PhysicsShapeType;
        [[nodiscard]] auto sensor() const -> bool;
        [[nodiscard]] auto body() const -> PhysicsBody2D*;
        [[nodiscard]] auto filter() const -> PhysicsFilter;
        void set_filter(PhysicsFilter filter);

    private:
        PhysicsShape2D(
            PhysicsWorld2D& world,
            PhysicsBody2D& body,
            b2ShapeId id,
            PhysicsShapeType type,
            bool sensor
        );

        PhysicsWorld2D* world_;
        PhysicsBody2D* body_;
        b2ShapeId id_ = b2_nullShapeId;
        PhysicsShapeType type_;
        bool sensor_;
        bool destroy_pending_ = false;
    };

    class PhysicsWorld2D final: public scene::NanNode2D {
    public:
        explicit PhysicsWorld2D(PhysicsWorldConfig config = {});
        ~PhysicsWorld2D() override;

        [[nodiscard]] static auto create(PhysicsWorldConfig config = {})
            -> std::shared_ptr<PhysicsWorld2D>;

        [[nodiscard]] auto config() const -> const PhysicsWorldConfig&;
        [[nodiscard]] auto body_count() const -> std::size_t;
        [[nodiscard]] auto accumulator() const -> float;
        [[nodiscard]] auto total_steps() const -> std::size_t;
        [[nodiscard]] auto events() const -> const std::vector<PhysicsTouchEvent>&;
        void set_event_handler(std::function<void(const PhysicsTouchEvent&)> handler);

        [[nodiscard]] auto pixels_to_meters(foundation::NanPoint value) const
            -> foundation::NanPoint;
        [[nodiscard]] auto meters_to_pixels(foundation::NanPoint value) const
            -> foundation::NanPoint;

        auto create_body(PhysicsBodyDef def = {}) -> std::shared_ptr<PhysicsBody2D>;
        void destroy_body(PhysicsBody2D& body);
        void destroy_shape(PhysicsShape2D& shape);
        auto step(float dt) -> int;
        void physics_step(float dt) override;

    private:
        friend class PhysicsBody2D;

        [[nodiscard]] auto to_b2(foundation::NanPoint value) const -> b2Vec2;
        [[nodiscard]] auto from_b2(b2Vec2 value) const -> foundation::NanPoint;
        [[nodiscard]] auto find_shape(b2ShapeId id) const -> std::shared_ptr<PhysicsShape2D>;
        void collect_events();
        void flush_pending_destroys();
        void sync_scene_to_physics();
        void sync_physics_to_scene();
        void destroy_world();

        PhysicsWorldConfig config_;
        b2WorldId world_id_ = b2_nullWorldId;
        std::vector<std::shared_ptr<PhysicsBody2D>> bodies_;
        float accumulator_ = 0.0F;
        std::size_t total_steps_ = 0;
        bool stepping_ = false;
        std::vector<PhysicsTouchEvent> events_;
        std::function<void(const PhysicsTouchEvent&)> event_handler_;
        std::vector<PhysicsBody2D*> pending_body_destroys_;
        std::vector<PhysicsShape2D*> pending_shape_destroys_;
    };

} // namespace nandina::physics2d

#endif // NANDINA_EXPERIMENT_PHYSICS_WORLD2D_HPP
