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
uniform vec3 beach_color  : source_color = vec3(0.9, 0.8, 0.4);
uniform vec3 grass_color  : source_color = vec3(0.2, 0.6, 0.1);
uniform vec3 cliff_color  : source_color = vec3(0.5, 0.5, 0.4);
uniform vec3 snow_color   : source_color = vec3(1.0, 1.0, 1.0);

// Blend width for smooth transitions between terrain zones
uniform float blend_width : hint_range(0.0, 5.0) = 0.3;

void vertex() {
    float height_val = texture(height_texture, UV).r * amplitude;
    if (height_val < sea_level) {
        height_val = sea_level;
    }
    VERTEX.y += height_val;
}

void fragment() {
    // Sample height per-pixel for accurate color boundaries
    float raw_height = texture(height_texture, UV).r * amplitude;
    float h = raw_height - sea_level;

    float beach_t = smoothstep(-blend_width * 0.5, blend_width * 0.5, h);
    float grass_t = smoothstep(beach_level - blend_width * 0.5, beach_level + blend_width * 0.5, h);
    float cliff_t = smoothstep(cliff_level - blend_width * 0.5, cliff_level + blend_width * 0.5, h);
    float snow_t  = smoothstep(snow_level - blend_width * 0.5, snow_level + blend_width * 0.5, h);

    vec3 color = ocean_color;
    color = mix(color, beach_color, beach_t);
    color = mix(color, grass_color, grass_t);
    color = mix(color, cliff_color, cliff_t);
    color = mix(color, snow_color, snow_t);

    ALBEDO = color;
}
