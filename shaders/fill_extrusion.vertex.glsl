layout (location = 0) in vec2 a_pos;
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

#pragma mapbox: define highp float base
#pragma mapbox: define highp float height
#pragma mapbox: define highp vec4 color

void main() {
    #pragma mapbox: initialize highp float base
    #pragma mapbox: initialize highp float height
    #pragma mapbox: initialize highp vec4 color

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
