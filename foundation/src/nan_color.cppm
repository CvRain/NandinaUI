//
// Created by cvrain on 2026/4/19.
//
module;

#include <concepts>
#include <functional>
#include <type_traits>
#include <utility>

export module nandina.foundation.color;

export import nandina.foundation.nan_color_schema;
export import nandina.foundation.nan_color_converter;

/**
 * nandina.foundation.color
 *
 * 面向使用方的主入口模块。
 * `NanColor` 内部统一存储为 `NanOklab`，并通过 traits 获取任意颜色空间输入 / 输出。
 *
 * Example:
 *   auto color = nandina::NanColor::from(nandina::NanRgb{"#232634"});
 *   auto oklch = color.to<nandina::NanOklch>();
 */
export namespace nandina {
    /**
     * 通用颜色容器。
     *
     * 它适合在组件、主题系统和绘制接口之间传递颜色，
     * 避免在业务层直接持有某一种具体颜色空间。
     */
    class NanColor {
        template<color::ColorSpace T>
        class SetProxy {
        public:
            explicit SetProxy(NanColor &owner) noexcept :
                owner_(owner) {
            }

            auto operator=(const T &color) -> SetProxy & {
                owner_.oklab_color_ = color::ColorSpaceTraits<T>::to_oklab(color);
                return *this;
            }

        private:
            NanColor &owner_;
        };

    public:
        constexpr NanColor() noexcept : oklab_color_() {}

        explicit constexpr NanColor(const NanOklab &color) noexcept :
            oklab_color_(color) {
        }

        /// 从任意已注册颜色值对象构造统一颜色容器。
        template<color::ColorSpace T>
        [[nodiscard]] static auto from(const T &color) -> NanColor {
            return NanColor{color::ColorSpaceTraits<T>::to_oklab(color)};
        }

        /// 将当前颜色投影到目标颜色空间。
        template<color::ColorSpace T>
        [[nodiscard]] auto to() const -> T {
            return color::ColorSpaceTraits<T>::from_oklab(oklab_color_);
        }

        /// 以指定颜色空间重写当前颜色值。
        template<color::ColorSpace T>
        auto assign(const T &color) -> NanColor & {
            oklab_color_ = color::ColorSpaceTraits<T>::to_oklab(color);
            return *this;
        }

        /**
         * 按指定颜色空间变换当前颜色，并写回容器。
         *
         * Example:
         *   color.transform<nandina::NanRgb>([](auto rgb) {
         *       return nandina::NanRgb{rgb.red(), rgb.green(), 255u, rgb.alpha()};
         *   });
         */
        template<color::ColorSpace T, typename Fn>
            requires std::invocable<Fn &, T> &&
                     std::same_as<std::invoke_result_t<Fn &, T>, T>
        auto transform(Fn &&fn) -> NanColor & {
            return assign(std::invoke(std::forward<Fn>(fn), to<T>()));
        }

        /// 与 `transform()` 相同，但返回变换后的副本，不修改当前对象。
        template<color::ColorSpace T, typename Fn>
            requires std::invocable<Fn &, T> &&
                     std::same_as<std::invoke_result_t<Fn &, T>, T>
        [[nodiscard]] auto transformed(Fn &&fn) const -> NanColor {
            return NanColor::from(std::invoke(std::forward<Fn>(fn), to<T>()));
        }

        /**
         * 提供接近属性赋值的写法。
         *
         * Example:
         *   auto color = nandina::NanColor{};
         *   color.set<nandina::NanRgb>() = nandina::NanRgb{170, 173, 111, 200};
         */
        template<color::ColorSpace T>
        [[nodiscard]] auto set() -> SetProxy<T> {
            return SetProxy<T>{*this};
        }

        /// 直接访问内部 canonical color space，适合底层调试与精确控制。
        [[nodiscard]] constexpr auto raw_oklab() const noexcept -> const NanOklab & {
            return oklab_color_;
        }

    private:
        NanOklab oklab_color_;
    };
}
