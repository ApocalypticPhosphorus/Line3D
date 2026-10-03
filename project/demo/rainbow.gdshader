shader_type spatial;
render_mode blend_mix, depth_draw_opaque, cull_back, diffuse_lambert, specular_schlick_ggx;

uniform float speed : hint_range(0.0, 10.0) = 1.0;
uniform float frequency : hint_range(0.1, 10.0) = 1.0;

// Function to convert Hue to RGB using cosine waves
vec3 hue_to_rgb(float hue) {
    float r = cos((hue - 0.0 / 3.0) * 2.0 * 3.14159) * 0.5 + 0.5;
    float g = cos((hue - 1.0 / 3.0) * 2.0 * 3.14159) * 0.5 + 0.5;
    float b = cos((hue - 2.0 / 3.0) * 2.0 * 3.14159) * 0.5 + 0.5;
    return vec3(r, g, b);
}

void fragment() {
    // Offset the V coordinate over time to animate it, multiplied by frequency
    float rainbow_hue = (UV.y * frequency) - (TIME * speed);
    
    // Keep hue between 0.0 and 1.0 using fract
    rainbow_hue = fract(rainbow_hue);
    
    // Assign the rainbow color to the material Albedo
    ALBEDO = hue_to_rgb(rainbow_hue);
}
