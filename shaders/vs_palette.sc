$input a_position, a_texcoord0, a_color0
$output v_texcoord0, v_shade

/*
 * Palette lookup vertex shader
 * Takes PolyPoint data (X, Y, U, V, S) and passes through texture coords and shade
 */

#include <bgfx_shader.sh>

void main()
{
    // Transform position to clip space
    gl_Position = mul(u_modelViewProj, vec4(a_position, 1.0));
    
    // Pass through texture coordinates (U, V)
    v_texcoord0 = a_texcoord0;
    
    // Pass through shade value (S) via color channel
    v_shade = a_color0.r;
}
