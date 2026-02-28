#!/usr/bin/env python3
"""
Parse Skeleton CSS theme files and generate C++ theme preset data.
Converts OKLCH colors to sRGB hex for Qt QColor compatibility.
Uses enum-based APIs (ThemePreset, ColorAccent) for type safety.
"""

import re
import math
import os
import glob

# ─── OKLCH → sRGB Conversion ───────────────────────────────────────────────

def oklch_to_oklab(L, C, H):
    """Convert OKLCH to OKLab. H is in degrees."""
    h_rad = math.radians(H)
    a = C * math.cos(h_rad)
    b = C * math.sin(h_rad)
    return L, a, b

def oklab_to_linear_srgb(L, a, b):
    """Convert OKLab to linear sRGB."""
    l_ = L + 0.3963377774 * a + 0.2158037573 * b
    m_ = L - 0.1055613458 * a - 0.0638541728 * b
    s_ = L - 0.0894841775 * a - 1.2914855480 * b

    l = l_ * l_ * l_
    m = m_ * m_ * m_
    s = s_ * s_ * s_

    r = +4.0767416621 * l - 3.3077115913 * m + 0.2309699292 * s
    g = -1.2684380046 * l + 2.6097574011 * m - 0.3413193965 * s
    b_out = -0.0041960863 * l - 0.7034186147 * m + 1.7076147010 * s
    return r, g, b_out

def linear_to_srgb(x):
    """Apply sRGB gamma."""
    if x <= 0.0031308:
        return 12.92 * x
    return 1.055 * (x ** (1.0 / 2.4)) - 0.055

def clamp(x, lo=0.0, hi=1.0):
    return max(lo, min(hi, x))

def oklch_to_hex(L, C, H, alpha=1.0):
    """Convert OKLCH to #RRGGBB or #RRGGBBAA hex string."""
    lab_L, lab_a, lab_b = oklch_to_oklab(L, C, H)
    lr, lg, lb = oklab_to_linear_srgb(lab_L, lab_a, lab_b)
    r = clamp(linear_to_srgb(lr))
    g = clamp(linear_to_srgb(lg))
    b = clamp(linear_to_srgb(lb))
    ri, gi, bi = int(round(r * 255)), int(round(g * 255)), int(round(b * 255))
    if alpha < 1.0:
        ai = int(round(alpha * 255))
        return f"#{ri:02x}{gi:02x}{bi:02x}{ai:02x}"
    return f"#{ri:02x}{gi:02x}{bi:02x}"


# ─── CSS Parsing ────────────────────────────────────────────────────────────

# Matches: oklch(0.92 0.04 257.51) or oklch(97.603% 0.01971 212.821) or oklch(1 0 0 / 1)
OKLCH_RE = re.compile(
    r'oklch\(\s*'
    r'([\d.]+)(%?)\s+'    # L (optionally with %)
    r'([\d.]+)\s+'         # C
    r'([\d.]+)\s*(?:deg)?' # H (optionally with deg)
    r'(?:\s*/\s*([\d.]+))?' # optional alpha
    r'\s*\)'
)

COLOR_FAMILIES = ['primary', 'secondary', 'tertiary', 'success', 'warning', 'error', 'surface']
SHADES = ['50', '100', '200', '300', '400', '500', '600', '700', '800', '900', '950']

# Map CSS shade/contrast names to C++ ColorAccent enum values
ACCENT_MAP = {}
for s in SHADES:
    ACCENT_MAP[f'shade{s}'] = f'Shade{s}'
ACCENT_MAP['contrastDark'] = 'ContrastDark'
ACCENT_MAP['contrastLight'] = 'ContrastLight'
for s in SHADES:
    ACCENT_MAP[f'contrast{s}'] = f'Contrast{s}'

# Map family names to ColorVariant enum values
VARIANT_MAP = {f: f.title() for f in COLOR_FAMILIES}

# ThemePreset enum name mapping
PRESET_NAMES = {
    'catppuccin': 'Catppuccin',
    'cerberus': 'Cerberus',
    'concord': 'Concord',
    'crimson': 'Crimson',
    'fennec': 'Fennec',
    'legacy': 'Legacy',
}

def parse_oklch_value(value_str):
    """Parse an oklch() CSS value and return hex color."""
    m = OKLCH_RE.search(value_str)
    if not m:
        return None
    L_val = float(m.group(1))
    if m.group(2) == '%':
        L_val /= 100.0
    C = float(m.group(3))
    H = float(m.group(4))
    alpha = float(m.group(5)) if m.group(5) else 1.0
    return oklch_to_hex(L_val, C, H, alpha)


def parse_css_theme(filepath):
    """Parse a Skeleton CSS theme file and extract structured data."""
    with open(filepath) as f:
        content = f.read()

    name_match = re.search(r"data-theme[='\"]+([\w-]+)", content)
    theme_name = name_match.group(1) if name_match else os.path.splitext(os.path.basename(filepath))[0]

    props = {}
    for m in re.finditer(r'--([\w-]+)\s*:\s*([^;]+);', content):
        props[m.group(1)] = m.group(2).strip()

    # ─── Colors ───
    colors = {}
    for family in COLOR_FAMILIES:
        palette = {}
        for shade in SHADES:
            key = f'color-{family}-{shade}'
            if key in props:
                hex_color = parse_oklch_value(props[key])
                if hex_color:
                    palette[f'shade{shade}'] = hex_color

        palette['contrastDark'] = palette.get('shade950', '#000000')
        palette['contrastLight'] = palette.get('shade50', '#ffffff')

        for shade in SHADES:
            contrast_key = f'color-{family}-contrast-{shade}'
            if contrast_key in props:
                val = props[contrast_key]
                if 'contrast-dark' in val:
                    palette[f'contrast{shade}'] = palette['contrastDark']
                elif 'contrast-light' in val:
                    palette[f'contrast{shade}'] = palette['contrastLight']
                else:
                    hex_color = parse_oklch_value(val)
                    if hex_color:
                        palette[f'contrast{shade}'] = hex_color

        colors[family] = palette

    # ─── Primitives ───
    primitives = {}
    spacing_str = props.get('spacing', '0.25rem')
    primitives['spacing'] = parse_rem(spacing_str)
    primitives['textScaling'] = float(props.get('text-scaling', '1.067'))
    primitives['radiusBase'] = parse_rem(props.get('radius-base', '0.25rem'))
    primitives['radiusContainer'] = parse_rem(props.get('radius-container', '0.25rem'))
    primitives['borderWidth'] = parse_px(props.get('default-border-width', '1px'))
    primitives['divideWidth'] = parse_px(props.get('default-divide-width', '1px'))
    primitives['ringWidth'] = parse_px(props.get('default-ring-width', '1px'))

    bg = props.get('body-background-color', '')
    bg_hex = parse_oklch_value(bg) if 'oklch' in bg else None
    if bg_hex:
        primitives['bodyBackgroundColor'] = bg_hex
    elif 'surface-50' in bg:
        primitives['bodyBackgroundColor'] = colors.get('surface', {}).get('shade50', '#ffffff')
    else:
        primitives['bodyBackgroundColor'] = '#ffffff'

    bg_dark = props.get('body-background-color-dark', '')
    if 'surface-950' in bg_dark:
        primitives['bodyBackgroundColorDark'] = colors.get('surface', {}).get('shade950', '#1a1a1a')
    else:
        bg_dark_hex = parse_oklch_value(bg_dark) if 'oklch' in bg_dark else None
        primitives['bodyBackgroundColorDark'] = bg_dark_hex or '#1a1a1a'

    # ─── Typography ───
    typography = {
        'base': {
            'fontFamily': extract_font_family(props.get('base-font-family', 'system-ui')),
            'fontWeight': props.get('base-font-weight', 'normal'),
            'fontStyle': props.get('base-font-style', 'normal'),
            'letterSpacing': parse_em(props.get('base-letter-spacing', '0em')),
        },
        'heading': {
            'fontFamily': extract_font_family(props.get('heading-font-family', 'inherit')),
            'fontWeight': props.get('heading-font-weight', 'bold'),
            'fontStyle': props.get('heading-font-style', 'normal'),
            'letterSpacing': parse_em(props.get('heading-letter-spacing', 'inherit')),
        },
        'anchor': {
            'fontFamily': extract_font_family(props.get('anchor-font-family', 'inherit')),
            'fontWeight': props.get('anchor-font-weight', 'inherit'),
            'fontStyle': props.get('anchor-font-style', 'normal'),
            'letterSpacing': parse_em(props.get('anchor-letter-spacing', 'inherit')),
        },
    }

    def resolve_font_color(val):
        if not val:
            return None
        hex_c = parse_oklch_value(val) if 'oklch' in val else None
        if hex_c:
            return hex_c
        for family in COLOR_FAMILIES:
            for shade in SHADES:
                if f'color-{family}-{shade}' in val:
                    return colors.get(family, {}).get(f'shade{shade}')
        return None

    typography['base']['fontColor'] = resolve_font_color(props.get('base-font-color')) or colors.get('surface', {}).get('shade950', '#1a1a1a')
    typography['base']['fontColorDark'] = resolve_font_color(props.get('base-font-color-dark')) or colors.get('surface', {}).get('shade50', '#fafafa')

    heading_color = resolve_font_color(props.get('heading-font-color'))
    typography['heading']['fontColor'] = heading_color or typography['base']['fontColor']
    heading_color_dark = resolve_font_color(props.get('heading-font-color-dark'))
    typography['heading']['fontColorDark'] = heading_color_dark or typography['base']['fontColorDark']

    anchor_color = resolve_font_color(props.get('anchor-font-color'))
    typography['anchor']['fontColor'] = anchor_color or colors.get('primary', {}).get('shade500', '#3b82f6')
    anchor_color_dark = resolve_font_color(props.get('anchor-font-color-dark'))
    typography['anchor']['fontColorDark'] = anchor_color_dark or colors.get('primary', {}).get('shade400', '#60a5fa')

    return {
        'name': theme_name,
        'colors': colors,
        'primitives': primitives,
        'typography': typography,
    }


def parse_rem(s):
    m = re.search(r'([\d.]+)\s*rem', s)
    return float(m.group(1)) * 16.0 if m else 4.0

def parse_px(s):
    m = re.search(r'([\d.]+)\s*px', s)
    return float(m.group(1)) if m else 1.0

def parse_em(s):
    if s == 'inherit' or s == '0em' or s == '0':
        return 0.0
    m = re.search(r'([\d.]+)\s*em', s)
    return float(m.group(1)) if m else 0.0

def extract_font_family(s):
    if s == 'inherit':
        return ''
    first = s.split(',')[0].strip().strip("'\"")
    return first


# ─── C++ Code Generation (enum-based) ───────────────────────────────────────

def font_weight_to_int(w):
    mapping = {'normal': 400, 'bold': 700, 'bolder': 800, 'lighter': 300, 'inherit': -1}
    if w in mapping:
        return mapping[w]
    try:
        return int(w)
    except ValueError:
        return 400


def generate_cpp(themes):
    """Generate the complete C++ preset header and source files using enum APIs."""

    # ─── Header ───
    header = '''//
// Auto-generated by scripts/generate_presets.py
// Do not edit manually.
//

#ifndef NANDINA_THEME_PRESETS_HPP
#define NANDINA_THEME_PRESETS_HPP

#include <QColor>
#include <QString>
#include <QStringList>

#include "color_schema.hpp"
#include "nandina_types.hpp"
#include "primitive_schema.hpp"

namespace Nandina::Core::Tokens {

    /// Apply a built-in theme preset to the given schemas.
    /// Returns false if the preset value is out of range.
    bool applyPreset(Nandina::Types::ThemePreset preset,
                     Nandina::Core::Color::ColorSchema *colors,
                     Nandina::Core::Primitives::PrimitiveSchema *primitives);

'''

    for theme in themes:
        enum_name = PRESET_NAMES.get(theme['name'], theme['name'].title())
        header += f'    void apply{enum_name}(Nandina::Core::Color::ColorSchema *colors,\n'
        header += f'        {" " * len(enum_name)}    Nandina::Core::Primitives::PrimitiveSchema *primitives);\n\n'

    header += '''} // namespace Nandina::Core::Tokens

#endif // NANDINA_THEME_PRESETS_HPP
'''

    # ─── Source ───
    source = '''//
// Auto-generated by scripts/generate_presets.py
// Do not edit manually.
//

#include "theme_presets.hpp"

using namespace Nandina::Core::Color;
using namespace Nandina::Core::Primitives;
using namespace Nandina::Types;

namespace Nandina::Core::Tokens {

bool applyPreset(ThemePreset preset, ColorSchema *colors, PrimitiveSchema *primitives) {
    switch (preset) {
'''

    for theme in themes:
        enum_name = PRESET_NAMES.get(theme['name'], theme['name'].title())
        source += f'        case ThemePreset::{enum_name}:\n'
        source += f'            apply{enum_name}(colors, primitives);\n'
        source += f'            return true;\n'

    source += '''    }
    return false;
}

'''

    # Generate each theme's apply function using setColor() for compact output
    for theme in themes:
        enum_name = PRESET_NAMES.get(theme['name'], theme['name'].title())
        source += f'void apply{enum_name}(ColorSchema *colors, PrimitiveSchema *primitives) {{\n'

        for family in COLOR_FAMILIES:
            palette = theme['colors'].get(family, {})
            variant_enum = VARIANT_MAP[family]
            source += f'    // {family}\n'
            source += f'    {{\n'
            source += f'        auto *p = colors->palette(ColorVariant::{variant_enum});\n'

            for prop_name, accent_enum in ACCENT_MAP.items():
                if prop_name in palette:
                    source += f'        p->setColor(ColorAccent::{accent_enum}, QColor("{palette[prop_name]}"));\n'

            source += f'    }}\n'

        # Primitives
        p = theme['primitives']
        source += f'\n    // Primitives\n'
        source += f'    primitives->setSpacing({p["spacing"]});\n'
        source += f'    primitives->setTextScaling({p["textScaling"]});\n'
        source += f'    primitives->setRadiusBase({p["radiusBase"]});\n'
        source += f'    primitives->setRadiusContainer({p["radiusContainer"]});\n'
        source += f'    primitives->setBorderWidth({p["borderWidth"]});\n'
        source += f'    primitives->setDivideWidth({p["divideWidth"]});\n'
        source += f'    primitives->setRingWidth({p["ringWidth"]});\n'
        source += f'    primitives->setBodyBackgroundColor(QColor("{p["bodyBackgroundColor"]}"));\n'
        source += f'    primitives->setBodyBackgroundColorDark(QColor("{p["bodyBackgroundColorDark"]}"));\n'

        # Typography
        typo = theme['typography']
        for group_name in ['base', 'heading', 'anchor']:
            t = typo[group_name]
            source += f'\n    // {group_name} typography\n'
            source += f'    {{\n'
            source += f'        auto *typo = primitives->{group_name}Font();\n'
            if t.get('fontFamily'):
                source += f'        typo->setFontFamily(QStringLiteral("{t["fontFamily"]}"));\n'
            weight = font_weight_to_int(t.get('fontWeight', 'normal'))
            source += f'        typo->setFontWeight({weight});\n'
            italic = 'true' if t.get('fontStyle') == 'italic' else 'false'
            source += f'        typo->setItalic({italic});\n'
            source += f'        typo->setLetterSpacing({t.get("letterSpacing", 0.0)});\n'
            if t.get('fontColor'):
                source += f'        typo->setFontColor(QColor("{t["fontColor"]}"));\n'
            if t.get('fontColorDark'):
                source += f'        typo->setFontColorDark(QColor("{t["fontColorDark"]}"));\n'
            source += f'    }}\n'

        source += '}\n\n'

    source += '} // namespace Nandina::Core::Tokens\n'

    return header, source


# ─── Main ────────────────────────────────────────────────────────────────────

def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.dirname(script_dir)
    css_dir = os.path.join(project_root, 'temp', 'global')

    css_files = sorted(glob.glob(os.path.join(css_dir, '*.css')))
    if not css_files:
        print("Error: No CSS files found in temp/global/")
        return

    themes = []
    for f in css_files:
        print(f"Parsing {os.path.basename(f)}...")
        theme = parse_css_theme(f)
        themes.append(theme)
        print(f"  -> {theme['name']}: {sum(len(p) for p in theme['colors'].values())} color values")

    header_code, source_code = generate_cpp(themes)

    output_dir = os.path.join(project_root, 'Nandina', 'Core', 'Tokens')
    os.makedirs(output_dir, exist_ok=True)

    header_path = os.path.join(output_dir, 'theme_presets.hpp')
    source_path = os.path.join(output_dir, 'theme_presets.cpp')

    with open(header_path, 'w') as f:
        f.write(header_code)
    print(f"\nGenerated: {header_path}")

    with open(source_path, 'w') as f:
        f.write(source_code)
    print(f"Generated: {source_path}")

    print("\n-- Color Sample (cerberus primary) --")
    cerberus = next((t for t in themes if t['name'] == 'cerberus'), themes[0])
    for shade in SHADES:
        key = f'shade{shade}'
        val = cerberus['colors']['primary'].get(key, '?')
        print(f"  primary-{shade}: {val}")


if __name__ == '__main__':
    main()
