// Generated code, do not modify this file!
#pragma once
#include <mbgl/shaders/shader_source.hpp>

namespace mbgl {
namespace shaders {

template <>
struct ShaderSource<BuiltIn::FillExtrusionShader, gfx::Backend::Type::OpenGL> {
    static constexpr const char* name = "FillExtrusionShader";
    static constexpr const char* vertex = R"(layout (location = 0) in vec2 a_pos;
layout (location = 1) in vec4 a_normal_ed;
layout (location = 2) in float a_face_width;
layout (location = 3) in vec2 a_centroid;
out vec4 v_color;
out highp vec2 v_wall_uv;
flat out highp float v_height_m;
flat out lowp float v_is_side;
flat out highp float v_ed_flat;
flat out highp float v_face_width;
flat out mediump vec3 v_wall_normal;
flat out highp float v_body_hash;
out float v_directional;
flat out float v_shadow_opacity;

layout (std140) uniform FillExtrusionDrawableUBO {
    highp mat4 u_matrix;
    highp vec2 u_pixel_coord_upper;
    highp vec2 u_pixel_coord_lower;
    highp float u_height_factor;
    highp float u_tile_ratio;
    // Interpolations
    highp float u_base_t;
    highp float u_height_t;
    highp float u_color_t;
    highp float u_pattern_from_t;
    highp float u_pattern_to_t;
    highp float u_centroid_scale;
    highp vec2 u_tile_id;
    highp float u_is_shadow;
    highp float u_meters_to_tile;
};

layout (std140) uniform FillExtrusionTilePropsUBO {
    highp vec4 u_pattern_from;
    highp vec4 u_pattern_to;
    highp vec2 u_texsize;
    lowp float tileprops_pad1;
    lowp float tileprops_pad2;
};

layout (std140) uniform FillExtrusionPropsUBO {
    highp vec4 u_color;
    highp vec3 u_lightcolor;
    lowp float props_pad1;
    highp vec3 u_lightpos;
    highp float u_base;
    highp float u_height;
    highp float u_lightintensity;
    highp float u_vertical_gradient;
    highp float u_opacity;
    highp float u_fade;
    highp float u_from_scale;
    highp float u_to_scale;
    lowp float props_pad2;
    lowp vec3 u_camera_dir;
    lowp float props_pad3;
};

#ifndef HAS_UNIFORM_u_base
layout (location = 4) in highp vec2 a_base;
#endif
#ifndef HAS_UNIFORM_u_height
layout (location = 5) in highp vec2 a_height;
#endif
#ifndef HAS_UNIFORM_u_color
layout (location = 6) in highp vec4 a_color;
#endif

void main() {
    #ifndef HAS_UNIFORM_u_base
highp float base = unpack_mix_vec2(a_base, u_base_t);
#else
highp float base = u_base;
#endif
    #ifndef HAS_UNIFORM_u_height
highp float height = unpack_mix_vec2(a_height, u_height_t);
#else
highp float height = u_height;
#endif
    #ifndef HAS_UNIFORM_u_color
highp vec4 color = unpack_mix_color(a_color, u_color_t);
#else
highp vec4 color = u_color;
#endif

    vec3 normal = a_normal_ed.xyz;
    float edgedistance = a_normal_ed.w;

    base = max(0.0, base);
    height = max(0.0, height);

    float t = mod(normal.x, 2.0);
    float elevation = t > 0.0 ? height : base;

    // --- Shadow pass: project geometry onto ground plane ---
    v_shadow_opacity = u_is_shadow;
    if (u_is_shadow > 0.001) {
        vec2 light_xy = u_lightpos.xy;
        float light_xy_len = length(light_xy);
        float light_z = max(u_lightpos.z, 0.05);
        vec2 light_dir = light_xy_len > 0.0 ? -light_xy / light_xy_len : vec2(0.0, 0.0);
        float shadow_angle_factor = clamp(light_xy_len / light_z, 0.0, 6.0);

        float shadow_height_m = max(height - base, 0.0);
        float shadow_len_m = shadow_height_m * shadow_angle_factor;
        vec2 shadow_offset_tile = light_dir * shadow_len_m * u_meters_to_tile;

        float shadow_mix = t > 0.0 ? 1.0 : 0.0;
        vec2 shadow_xy = a_pos + shadow_offset_tile * shadow_mix;

        gl_Position = u_matrix * vec4(shadow_xy, 0.0, 1.0);

        // Set varyings needed by fragment shader
        v_is_side = (normal.y != 0.0) ? 1.0 : 0.0;
        v_height_m = shadow_height_m;
        float height_range_s = max(height - base, 0.001);
        v_wall_uv = vec2(edgedistance, (elevation - base) / height_range_s);
        v_ed_flat = 0.0;
        v_face_width = 0.0;
        v_wall_normal = vec3(0.0);
        v_body_hash = 0.0;
        // Encode whether this side face is on the outer shadow perimeter
        // (wall normal faces in shadow cast direction) for per-face edge fade.
        // >0 = outer perimeter (smooth), <0 = overlaps roof shadow (keep sharp).
        v_directional = (normal.y != 0.0)
            ? dot(vec2(normal.x, normal.y), light_dir)
            : 0.0;
        v_color = vec4(0.0);
        return;
    }

    gl_Position = u_matrix * vec4(a_pos, elevation, 1);

    // --- Procedural window data ---
    v_is_side = (normal.y != 0.0) ? 1.0 : 0.0;
    v_height_m = max(0.0, height - base);
    float height_range = max(height - base, 0.001);
    v_wall_uv = vec2(edgedistance, (elevation - base) / height_range);
    v_ed_flat = edgedistance;
    v_face_width = a_face_width;
    v_wall_normal = normal.y != 0.0 ? normalize(vec3(normal.x, normal.y, 0.0)) : vec3(0.0);
    vec2 world_centroid = u_tile_id + (a_centroid / 8192.0) * u_centroid_scale;
    // Hash world_centroid directly — no grid snapping needed.
    // world_centroid is bit-exact across zoom levels (all ops are power-of-2
    // divisions), so the chaotic sin() hash produces stable colors per building.
    // Two-round hash with large-magnitude constants for decorrelation.
    float h = fract(sin(dot(world_centroid, vec2(127.1, 311.7))) * 43758.5453);
    h = fract(sin(h * 78.233 + dot(world_centroid, vec2(269.5, 183.3))) * 24634.6345);
    v_body_hash = fract(h + height * 0.0197);

    // Relative luminance (how dark/bright is the surface color?)
    float colorvalue = color.r * 0.2126 + color.g * 0.7152 + color.b * 0.0722;

    v_color = vec4(0.0, 0.0, 0.0, 1.0);

    // Add slight ambient lighting so no extrusions are totally black
    vec4 ambientlight = vec4(0.03, 0.03, 0.03, 1.0);
    color += ambientlight;

    // Calculate cos(theta), where theta is the angle between surface normal and diffuse light ray
    float directional = clamp(dot(normal / 16384.0, u_lightpos), 0.0, 1.0);

    // Adjust directional so that
    // the range of values for highlight/shading is narrower
    // with lower light intensity
    // and with lighter/brighter surface colors
    directional = mix((1.0 - u_lightintensity), max((1.0 - colorvalue + u_lightintensity), 1.0), directional);

    // Add gradient along z axis of side surfaces
    if (normal.y != 0.0) {
        directional *= (
            (1.0 - u_vertical_gradient) +
            (u_vertical_gradient * clamp((t + base) * pow(height / 150.0, 0.5), mix(0.7, 0.98, 1.0 - u_lightintensity), 1.0)));
    }

    // Pass directional factor to fragment shader for procedural body color lighting
    v_directional = directional;

    // Assign final color based on surface + ambient light color, diffuse light directional, and light color
    v_color.r += clamp(color.r * directional * u_lightcolor.r, mix(0.0, 0.3, 1.0 - u_lightcolor.r), 1.0);
    v_color.g += clamp(color.g * directional * u_lightcolor.g, mix(0.0, 0.3, 1.0 - u_lightcolor.g), 1.0);
    v_color.b += clamp(color.b * directional * u_lightcolor.b, mix(0.0, 0.3, 1.0 - u_lightcolor.b), 1.0);
    v_color *= u_opacity;
}
)";
    static constexpr const char* fragment = R"(in vec4 v_color;
in highp vec2 v_wall_uv;
flat in highp float v_height_m;
flat in lowp float v_is_side;
flat in highp float v_ed_flat;
flat in highp float v_face_width;
flat in mediump vec3 v_wall_normal;
flat in highp float v_body_hash;
in float v_directional;
flat in float v_shadow_opacity;

layout (std140) uniform FillExtrusionPropsUBO {
    highp vec4 u_color;
    highp vec3 u_lightcolor;
    lowp float props_pad1;
    highp vec3 u_lightpos;
    highp float u_base;
    highp float u_height;
    highp float u_lightintensity;
    highp float u_vertical_gradient;
    highp float u_opacity;
    highp float u_fade;
    highp float u_from_scale;
    highp float u_to_scale;
    lowp float props_pad2;
    lowp vec3 u_camera_dir;
    lowp float props_pad3;
};

void main() {
    // --- Shadow pass early return ---
    if (v_shadow_opacity > 0.001) {
        float alpha = v_shadow_opacity;
        // Soft-fade the outer tip of side-face shadows (away from the
        // building) so the shadow silhouette is feathered.  The base
        // edge (under the building) stays sharp — it meets the roof.
        // v_directional > 0 identifies outer-perimeter side faces (wall
        // normal aligns with shadow cast direction); inner faces that
        // overlap the roof shadow are left at full alpha.
        if (v_is_side > 0.5 && v_directional > 0.0) {
            alpha *= smoothstep(1.0, 0.85, v_wall_uv.y);
        }
        fragColor = vec4(0.0, 0.0, 0.0, alpha);
        return;
    }

    fragColor = v_color;

    // --- Per-building body color variation (computed in vertex shader from centroid+height) ---
    float body_hash = v_body_hash;
    // 8-color palette indexed by body_hash [0,1)
    vec3 pal[8];
    pal[0] = vec3(0.965, 0.933, 0.875); // #F6EEDF
    pal[1] = vec3(0.957, 0.941, 0.918); // #F4F0EA
    pal[2] = vec3(0.976, 0.957, 0.918); // #F9F4EA
    pal[3] = vec3(0.961, 0.929, 0.886); // #F5EDE2
    pal[4] = vec3(0.937, 0.902, 0.867); // #EFE6DD
    pal[5] = vec3(0.961, 0.957, 0.941); // #F5F4F0
    pal[6] = vec3(0.910, 0.867, 0.816); // #E8DDD0
    pal[7] = vec3(0.957, 0.922, 0.886); // #F4EBE2
    vec3 body_color = pal[clamp(int(floor(body_hash * 8.0)), 0, 7)];
    fragColor.rgb = body_color * v_directional;
    fragColor.a = v_color.a;

    // --- Procedural windows on side faces ---
    if (v_is_side > 0.5 && v_height_m >= 3.1) {
        float num_floors = max(1.0, floor(v_height_m / 3.0));
        float floor_v = fract(v_wall_uv.y * num_floors);

        // Floor band margins (60% window fill — visible floor slabs)
        float band_b = 0.18;
        float band_t = 0.78;
        float fw_v = fwidth(v_wall_uv.y * num_floors);
        float floor_mask = smoothstep(band_b - fw_v, band_b + fw_v, floor_v)
                         * smoothstep(band_t + fw_v, band_t - fw_v, floor_v);

        // Vertical window columns — face-width-relative positioning
        // (identical constants to web shader, coordinate scales are 1:1)
        float window_width = 360.0;
        float window_gap = 8.0;
        float window_spacing = window_width + window_gap;
        float outer_pad_l = 60.0;
        float outer_pad_r = 60.0;

        float col_mask = 0.0;
        float raw_u = 0.0;
        float cell_u = 0.0;
        float face_u = clamp(v_wall_uv.x - v_ed_flat, 0.0, max(v_face_width, 0.0));

        if (v_face_width > outer_pad_l + outer_pad_r) {
            float content_max = v_face_width - outer_pad_r;
            float fw_face = fwidth(face_u);
            float within_content = smoothstep(outer_pad_l - fw_face, outer_pad_l + fw_face, face_u)
                                 * smoothstep(content_max + fw_face, content_max - fw_face, face_u);

            raw_u = (face_u - outer_pad_l) / window_spacing;
            cell_u = fract(raw_u);
            float fw_u = fwidth(raw_u);
            float win_r = window_width / window_spacing;
            col_mask = within_content
                     * smoothstep(0.0 - fw_u, 0.0 + fw_u, cell_u)
                     * smoothstep(win_r + fw_u, win_r - fw_u, cell_u);
        }

        float win_mask = floor_mask * col_mask;

        // Grazing-angle detail gate: fade out windows when face is nearly
        // edge-on (UV frequency exceeds Nyquist → aliasing).
        // Native thresholds are ~3x smaller than web (0.4/0.8) because
        // high-DPI mobile screens (~440 DPI vs ~96 DPI) produce ~3x
        // smaller fwidth() values at the same zoom level.
        float detail = 1.0 - smoothstep(0.13, 0.27, fwidth(raw_u));
        float floor_detail = 1.0 - smoothstep(0.13, 0.27, fw_v);
        win_mask *= min(detail, floor_detail);

        // Top-of-building parapet — same thickness as inter-floor slab
        float slab_uv = (1.0 - band_t + band_b) / num_floors;
        float fw_top = fwidth(v_wall_uv.y);
        win_mask *= smoothstep(1.0 - slab_uv + fw_top, 1.0 - slab_uv - fw_top, v_wall_uv.y);

        if (win_mask > 0.01) {
            vec2 grid_id = floor(vec2(raw_u, v_wall_uv.y * num_floors));
            float face_seed = v_ed_flat * 0.0073 + v_face_width * 0.0129 + v_height_m * 0.0197;
            float hash = fract(sin(dot(grid_id, vec2(12.9898, 78.233)) + face_seed) * 43758.5453);
            float hash2 = fract(sin(dot(grid_id + 19.37, vec2(39.3468, 11.1351)) + face_seed) * 24634.6345);

            vec3 palette_a = vec3(0.57, 0.82, 0.95);
            vec3 palette_b = vec3(0.70, 0.87, 0.98);
            vec3 palette_c = vec3(0.63, 0.79, 0.94);
            vec3 palette_d = vec3(0.80, 0.92, 0.99);
            vec3 palette_base = mix(
                mix(palette_a, palette_b, step(0.25, hash)),
                mix(palette_c, palette_d, step(0.75, hash)),
                step(0.50, hash)
            );
            vec3 window_color = clamp(palette_base + (hash2 - 0.5) * vec3(0.055, 0.034, 0.038), 0.0, 1.0);

            float local_u = clamp((cell_u * window_spacing) / window_width, 0.0, 1.0);
            float local_v = clamp((floor_v - band_b) / (band_t - band_b), 0.0, 1.0);

            // Diagonal reflection streak
            float streak_axis = (1.0 - local_u) * 0.72 + local_v * 0.92;
            float streak_center = 0.77 + (hash2 - 0.5) * 0.12;
            float streak_width = 0.26;
            float streak_dist = abs(streak_axis - streak_center);
            float fw_streak = fwidth(streak_axis);
            float streak = 1.0 - smoothstep(streak_width - fw_streak, streak_width + 0.22 + fw_streak, streak_dist);
            float streak_gradient = 0.58 * local_v + 0.42 * (1.0 - local_u);
            window_color *= 0.90 + streak_gradient * 0.14;
            window_color = mix(window_color, vec3(0.97, 0.985, 1.0), streak * (0.11 + hash * 0.04));

            // Camera-based Fresnel specular
            float facing = clamp(abs(dot(normalize(v_wall_normal), normalize(u_camera_dir))), 0.0, 1.0);
            float grazing = pow(1.0 - facing, 1.65);
            float specular = min(grazing * (0.32 + hash2 * 0.12), 0.52);
            window_color = mix(window_color, vec3(0.985, 0.995, 1.0), specular);

            float luminance = dot(v_color.rgb, vec3(0.299, 0.587, 0.114));
            vec3 lit_window = window_color * max(luminance * 1.18, 0.70);
            lit_window *= 1.0 + 0.24 * grazing;
            fragColor.rgb = mix(fragColor.rgb, lit_window, 0.84 * win_mask);
        }
    }

    // --- Soft edge AO (side faces only) ---
    if (v_is_side > 0.5) {
        float base_ao = smoothstep(0.0, 0.10, v_wall_uv.y);
        fragColor.rgb *= mix(0.86, 1.0, base_ao);

        float top_glow = smoothstep(0.92, 1.0, v_wall_uv.y);
        fragColor.rgb *= mix(1.0, 1.06, top_glow);
    }

#ifdef OVERDRAW_INSPECTOR
    fragColor = vec4(1.0);
#endif
}
)";
};

} // namespace shaders
} // namespace mbgl
