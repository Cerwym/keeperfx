$input v_texcoord0, v_shade

/*
 * Palette lookup fragment shader
 * Samples 8-bit texture, looks up color in palette, applies shading
 */

#include <bgfx_shader.sh>

SAMPLER2D(s_texColor, 0);  // 8-bit texture atlas
SAMPLER2D(s_palette, 1);   // 256x1 palette texture

void main()
{
    // Sample the 8-bit texture to get palette index
    float paletteIndex = texture2D(s_texColor, v_texcoord0).r;
    
    // Look up the color in the palette
    vec4 color = texture2D(s_palette, vec2(paletteIndex, 0.5));
    
    // Apply shading (multiply by shade value)
    color.rgb *= v_shade;
    
    gl_FragColor = color;
}
