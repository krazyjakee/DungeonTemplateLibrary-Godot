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

// We'll pass the final world-space height to the fragment shader
varying float v_world_height;

void vertex() {
    // Sample height from the texture
    float height_val = texture(height_texture, UV).r * amplitude;
    
    // If the sampled height is below sea_level, clamp it (so we get a flat ocean).
    if (height_val < sea_level) {
        height_val = sea_level;
    }
    
    // Move the vertex up according to the final height.
    VERTEX.y += height_val;
    
    // Pass this height into a varying so the fragment function can color properly.
    v_world_height = VERTEX.y;
}

void fragment() {
    // Compare our vertex height to thresholds
    // Note: The thresholds are "above" sea_level, so consider how you want to measure.
    // Here we measure relative to actual world height for simplicity.
    
    if (v_world_height <= sea_level + 0.001) {
        // Ocean
        ALBEDO = ocean_color;
    } 
    else if (v_world_height <= sea_level + beach_level) {
        // Beach
        ALBEDO = beach_color;
    } 
    else if (v_world_height <= sea_level + cliff_level) {
        // Grass
        ALBEDO = grass_color;
    }
    else if (v_world_height <= sea_level + snow_level) {
        // Cliffs
        ALBEDO = cliff_color;
    } 
    else {
        // Snow
        ALBEDO = snow_color;
    }
}
