shader_type spatial;

render_mode diffuse_burley, specular_schlick_ggx, cull_back;

// Textures and scaling
uniform sampler2D height_texture;
uniform float amplitude : hint_range(0, 100) = 50.0;

// Sea level
uniform float sea_level : hint_range(0, 100) = 10.0;

// Thresholds for different terrain zones (above sea_level)
uniform float beach_level : hint_range(0, 100) = 1.0;
uniform float cliff_level : hint_range(0, 100) = 20.0;
uniform float snow_level  : hint_range(0, 100) = 45.0;

// Colors for different zones
uniform vec3 ocean_color  : source_color = vec3(0.0, 0.3, 0.6);
uniform vec3 ocean_deep_color : source_color = vec3(0.0, 0.15, 0.35);
uniform vec3 beach_color  : source_color = vec3(0.9, 0.8, 0.4);
uniform vec3 grass_color  : source_color = vec3(0.2, 0.6, 0.1);
uniform vec3 cliff_color  : source_color = vec3(0.5, 0.5, 0.4);
uniform vec3 snow_color   : source_color = vec3(1.0, 1.0, 1.0);

// Blend width for smooth transitions between terrain zones
uniform float blend_width : hint_range(0.0, 5.0) = 0.3;

// Slope-based cliff blending
uniform float slope_cliff_threshold : hint_range(0.0, 1.0) = 0.6;
uniform float slope_cliff_blend : hint_range(0.0, 0.5) = 0.15;

// Color variation noise
uniform float noise_strength : hint_range(0.0, 0.2) = 0.06;
uniform float noise_scale : hint_range(1.0, 200.0) = 60.0;

// Water settings
uniform float water_specular : hint_range(0.0, 1.0) = 0.8;
uniform float water_metallic : hint_range(0.0, 1.0) = 0.1;
uniform float water_roughness : hint_range(0.0, 1.0) = 0.05;
uniform float water_fresnel_power : hint_range(0.5, 8.0) = 3.0;
uniform vec3 water_fresnel_color : source_color = vec3(0.5, 0.7, 0.9);

// Heightmap texel size (set from code or estimate)
uniform vec2 texel_size = vec2(0.002, 0.002);

// Simple hash-based noise (no texture needed)
float hash(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

float value_noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f); // smoothstep interpolation

    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));

    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

float fbm(vec2 p) {
    float val = 0.0;
    float amp = 0.5;
    for (int i = 0; i < 3; i++) {
        val += amp * value_noise(p);
        p *= 2.1;
        amp *= 0.5;
    }
    return val;
}

void vertex() {
    float height_val = texture(height_texture, UV).r * amplitude;
    if (height_val < sea_level) {
        height_val = sea_level;
    }
    VERTEX.y += height_val;
}

void fragment() {
    // --- Per-pixel height sampling ---
    float raw_height = texture(height_texture, UV).r * amplitude;
    float h = raw_height - sea_level;
    bool is_water = h <= 0.001;

    // --- Recalculate normals from heightmap ---
    float h_r = texture(height_texture, UV + vec2(texel_size.x, 0.0)).r * amplitude;
    float h_l = texture(height_texture, UV - vec2(texel_size.x, 0.0)).r * amplitude;
    float h_u = texture(height_texture, UV + vec2(0.0, texel_size.y)).r * amplitude;
    float h_d = texture(height_texture, UV - vec2(0.0, texel_size.y)).r * amplitude;

    // Tangent-space normal from heightmap gradients
    vec3 heightmap_normal = normalize(vec3(h_l - h_r, 2.0, h_d - h_u));

    // --- Slope calculation ---
    float slope = 1.0 - heightmap_normal.y; // 0 = flat, 1 = vertical

    // --- Height-based color blending ---
    float beach_t = smoothstep(-blend_width * 0.5, blend_width * 0.5, h);
    float grass_t = smoothstep(beach_level - blend_width * 0.5, beach_level + blend_width * 0.5, h);
    float cliff_t = smoothstep(cliff_level - blend_width * 0.5, cliff_level + blend_width * 0.5, h);
    float snow_t  = smoothstep(snow_level - blend_width * 0.5, snow_level + blend_width * 0.5, h);

    vec3 color = ocean_color;
    color = mix(color, beach_color, beach_t);
    color = mix(color, grass_color, grass_t);
    color = mix(color, cliff_color, cliff_t);
    color = mix(color, snow_color, snow_t);

    // --- Slope-based cliff override ---
    float slope_factor = smoothstep(
        slope_cliff_threshold - slope_cliff_blend,
        slope_cliff_threshold + slope_cliff_blend,
        slope
    );
    if (!is_water) {
        color = mix(color, cliff_color, slope_factor);
    }

    // --- Color variation noise ---
    if (!is_water) {
        float n = fbm(UV * noise_scale);
        color += n * noise_strength;
    }

    // --- Apply normal map ---
    if (!is_water) {
        NORMAL_MAP = heightmap_normal * 0.5 + 0.5;
    }

    // --- Water shading ---
    if (is_water) {
        // Depth-based color (deeper = darker)
        float depth = clamp(-h / 10.0, 0.0, 1.0);
        color = mix(ocean_color, ocean_deep_color, depth);

        // Fresnel effect
        float fresnel = pow(1.0 - clamp(dot(NORMAL, VIEW), 0.0, 1.0), water_fresnel_power);
        color = mix(color, water_fresnel_color, fresnel * 0.4);

        SPECULAR = water_specular;
        METALLIC = water_metallic;
        ROUGHNESS = water_roughness;
    } else {
        ROUGHNESS = mix(0.85, 0.95, slope_factor);
        METALLIC = 0.0;
        SPECULAR = 0.2;
    }

    ALBEDO = color;
}
