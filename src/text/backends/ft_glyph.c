// FreeType glyph bitmap accessor — single function returns all glyph data.
// Zig's @cImport can't access FT_FaceRec_ fields, so this C wrapper provides
// a simple interface.
#include <ft2build.h>
#include FT_FREETYPE_H>

// Result structure for nandina_ft_render_glyph.
typedef struct {
    int width;
    int rows;
    int pitch;
    unsigned char* buffer;
    int left;
    int top;
    int advance_x;
} NandinaFTGlyphResult;

// Render a glyph and return its bitmap data + advance.
NandinaFTGlyphResult nandina_ft_render_glyph(FT_Face face, unsigned int glyph_index, int pixel_size) {
    NandinaFTGlyphResult result = {0, 0, 0, NULL, 0, 0, 0};
    FT_Set_Pixel_Sizes(face, 0, pixel_size);
    if (FT_Load_Glyph(face, glyph_index, FT_LOAD_RENDER)) return result;
    FT_GlyphSlot slot = face->glyph;
    result.width = slot->bitmap.width;
    result.rows = slot->bitmap.rows;
    result.pitch = slot->bitmap.pitch;
    result.buffer = slot->bitmap.buffer;
    result.left = slot->bitmap_left;
    result.top = slot->bitmap_top;
    result.advance_x = (int)(slot->advance.x >> 6); // Convert from 26.6 fixed point to pixels
    return result;
}

// Get face metrics.
typedef struct {
    int units_per_EM;
    int ascender;
    int descender;
    int height;
} NandinaFTFaceMetrics;

NandinaFTFaceMetrics nandina_ft_face_metrics(FT_Face face) {
    NandinaFTFaceMetrics m;
    m.units_per_EM = face->units_per_EM;
    m.ascender = face->ascender;
    m.descender = face->descender;
    m.height = face->height;
    return m;
}
