//
// Created by Lingma on 2025/10/28.
//

#include "baseColors.hpp"

namespace Nandina {
    BaseColors::BaseColors(QObject *parent) : QObject(parent) {}

    // 拷贝构造函数：不复制父对象，避免 Qt 对象树混乱
    // 新对象的父对象设置为 nullptr，由调用者决定其归属
    BaseColors::BaseColors(const BaseColors &other) : QObject(nullptr) { // 重要：使用 nullptr 而不是 other.parent()
        rosewater = other.rosewater;
        flamingo = other.flamingo;
        pink = other.pink;
        mauve = other.mauve;
        red = other.red;
        maroon = other.maroon;
        peach = other.peach;
        yellow = other.yellow;
        green = other.green;
        teal = other.teal;
        sky = other.sky;
        sapphire = other.sapphire;
        blue = other.blue;
        lavender = other.lavender;
        text = other.text;
        subtext1 = other.subtext1;
        subtext0 = other.subtext0;
        overlay2 = other.overlay2;
        overlay1 = other.overlay1;
        overlay0 = other.overlay0;
        surface2 = other.surface2;
        surface1 = other.surface1;
        surface0 = other.surface0;
        base = other.base;
        mantle = other.mantle;
        crust = other.crust;
    }

    // 赋值运算符：只复制数据成员，不修改对象树关系
    BaseColors &BaseColors::operator=(const BaseColors &other) {
        if (this != &other) {
            // 注意：不复制父对象，保持现有的对象树关系
            rosewater = other.rosewater;
            flamingo = other.flamingo;
            pink = other.pink;
            mauve = other.mauve;
            red = other.red;
            maroon = other.maroon;
            peach = other.peach;
            yellow = other.yellow;
            green = other.green;
            teal = other.teal;
            sky = other.sky;
            sapphire = other.sapphire;
            blue = other.blue;
            lavender = other.lavender;
            text = other.text;
            subtext1 = other.subtext1;
            subtext0 = other.subtext0;
            overlay2 = other.overlay2;
            overlay1 = other.overlay1;
            overlay0 = other.overlay0;
            surface2 = other.surface2;
            surface1 = other.surface1;
            surface0 = other.surface0;
            base = other.base;
            mantle = other.mantle;
            crust = other.crust;
        }
        return *this;
    }
} // namespace Nandina
