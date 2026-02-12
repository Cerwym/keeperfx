$input a_position, a_texcoord0, a_color0
$output v_texcoord0, v_shade

/*
 * RGBA vertex shader (pre-expanded texture mode)
 * Takes PolyPoint data and passes through texture coords and shade
 */

#include <bgfx_shader.sh>

void main()
{
    // Transform position to clip space
    gl_Position = mul(u_modelViewProj, vec4(a_position, 1.0));
    
    // Pass through texture coordinates
    v_texcoord0 = a_texcoord0;
    
    // Pass through shade value via color channel
    v_shade = a_color0.r;
}
