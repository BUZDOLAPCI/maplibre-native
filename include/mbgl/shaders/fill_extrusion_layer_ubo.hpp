#pragma once

#include <mbgl/shaders/layer_ubo.hpp>

#include <array>

namespace mbgl {
namespace shaders {

struct alignas(16) FillExtrusionDrawableUBO {
    /*   0 */ std::array<float, 4 * 4> matrix;
    /*  64 */ std::array<float, 2> pixel_coord_upper;
    /*  72 */ std::array<float, 2> pixel_coord_lower;
    /*  80 */ float height_factor;
    /*  84 */ float tile_ratio;

    // Interpolations
    /*  88 */ float base_t;
    /*  92 */ float height_t;
    /*  96 */ float color_t;
    /* 100 */ float pattern_from_t;
    /* 104 */ float pattern_to_t;
    /* 108 */ float centroid_scale;
    /* 112 */ std::array<float, 2> tile_id;
    /* 120 */ float is_shadow;
    /* 124 */ float meters_to_tile;
    /* 128 */
};
static_assert(sizeof(FillExtrusionDrawableUBO) == 8 * 16);

struct alignas(16) FillExtrusionTilePropsUBO {
    /*  0 */ std::array<float, 4> pattern_from;
    /* 16 */ std::array<float, 4> pattern_to;
    /* 32 */ std::array<float, 2> texsize;
    /* 40 */ float pad1;
    /* 44 */ float pad2;
    /* 48 */
};
static_assert(sizeof(FillExtrusionTilePropsUBO) == 3 * 16);

/// Evaluated properties that do not depend on the tile
struct alignas(16) FillExtrusionPropsUBO {
    /*  0 */ Color color;
    /* 16 */ std::array<float, 3> light_color;
    /* 28 */ float pad1;
    /* 32 */ std::array<float, 3> light_position;
    /* 44 */ float base;
    /* 48 */ float height;
    /* 52 */ float light_intensity;
    /* 56 */ float vertical_gradient;
    /* 60 */ float opacity;
    /* 64 */ float fade;
    /* 68 */ float from_scale;
    /* 72 */ float to_scale;
    /* 76 */ float pad2;
    /* 80 */ std::array<float, 3> camera_dir;
    /* 92 */ float pad3;
    /* 96 */
};
static_assert(sizeof(FillExtrusionPropsUBO) == 6 * 16);

} // namespace shaders
} // namespace mbgl
