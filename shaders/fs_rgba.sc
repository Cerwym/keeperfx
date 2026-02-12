$input v_texcoord0, v_shade

/*
 * RGBA fragment shader (pre-expanded texture mode)
 * Samples RGBA texture directly and applies shading
 */

#include <bgfx_shader.sh>

SAMPLER2D(s_texColor, 0);  // RGBA texture atlas

void main()
{
    // Sample the RGBA texture
    vec4 color = texture2D(s_texColor, v_texcoord0);
    
    // Apply shading (multiply by shade value)
    color.rgb *= v_shade;
    
    gl_FragColor = color;
}
