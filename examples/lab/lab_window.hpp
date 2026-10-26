#ifndef NANDINAUI_LAB_WINDOW_HPP
#define NANDINAUI_LAB_WINDOW_HPP

#include <nandina/app/nan_window.hpp>

namespace nandina::lab
{
    class LabWindow final: public app::NanWindow {
    public:
        LabWindow(app::NanApplication& application, const app::WindowConfig& config);

    protected:
        void on_setup() override;
        void on_frame(float dt) override;

    private:
        float smoke_seconds_ = 0.0F;
        float smoke_elapsed_ = 0.0F;
    };
} // namespace nandina::lab

#endif // NANDINAUI_LAB_WINDOW_HPP
