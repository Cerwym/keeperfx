/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file DisplacementEffect.cpp
 *     Displacement/warping lens effect implementation.
 * @par Purpose:
 *     Self-contained image warping/distortion effect.
 * @par Comment:
 *     Displacement effects warp the image by reading pixels from different
 *     locations, creating wave, ripple, or fisheye-like distortions.
 * @author   Peter Lockett, KeeperFX Team
 * @date     09 Feb 2026
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#include "../../pre_inc.h"
#include "DisplacementEffect.h"

#include <math.h>
#include "../../config_lenses.h"
#include "../../vidmode.h"
#include "../../lens_api.h"

#include "../../keeperfx.hpp"
#include "../../post_inc.h"

/**
 * Generate a displacement lookup map that distorts image coordinates.
 * 
 * @param lens_mem Output buffer for displacement map (width*height uint32_t values)
 * @param width Viewport width
 * @param height Viewport height
 * @param pitch Source buffer pitch (typically full screen width)
 * @param algorithm Displacement algorithm to use
 * @param magnitude Strength of the distortion effect
 * @param period Frequency/period parameter (meaning depends on algorithm)
 */
static void GenerateDisplacementMap(uint32_t *lens_mem, int width, int height, int pitch,
                                   int algorithm, int magnitude, int period)
{
    long w, h;
    long shift_w, shift_h;
    uint32_t *mem;
    double flwidth, flheight;
    double center_w, center_h;
    double flpos_w, flpos_h;
    double flmag;
    
    switch (algorithm)
    {
    case DisplaceAlgo_Linear:
        // Simple linear offset - not commonly used
        mem = lens_mem;
        for (h = 0; h < height; h++)
        {
            for (w = 0; w < width; w++)
            {
                *mem = ((h + (height >> 1)) / 2) * pitch + ((w + (width >> 1)) / 2);
                mem++;
            }
        }
        break;
        
    case DisplaceAlgo_Sinusoidal:
        // Sine wave distortion - creates ripple/wave effects
        {
            flmag = magnitude;
            double flperiod = period;
            flwidth = width;
            flheight = height;
            center_h = flheight * 0.5;
            center_w = flwidth * 0.5;
            flpos_h = -center_h;
            mem = lens_mem;
            
            for (h = 0; h < height; h++)
            {
                flpos_w = -center_w;
                for (w = 0; w < width; w++)
                {
                    shift_w = (long)(sin(flpos_h / flwidth  * flperiod) * flmag + flpos_w + center_w);
                    shift_h = (long)(sin(flpos_w / flheight * flperiod) * flmag + flpos_h + center_h);
                    
                    // Clamp to valid range
                    if (shift_w >= width)  shift_w = width - 1;
                    if (shift_w < 0)       shift_w = 0;
                    if (shift_h >= height) shift_h = height - 1;
                    if (shift_h < 0)       shift_h = 0;
                    
                    *mem = shift_w + shift_h * pitch;
                    flpos_w += 1.0;
                    mem++;
                }
                flpos_h += 1.0;
            }
        }
        break;
        
    case DisplaceAlgo_Radial:
        // Radial distortion - creates fisheye or pincushion effects
        {
            flmag = magnitude * (double)magnitude;
            flwidth = width;
            flheight = height;
            center_h = flheight * 0.5;
            center_w = flwidth * 0.5;
            double fldivs = sqrt(center_h * center_h + center_w * center_w + flmag);
            flpos_h = -center_h;
            mem = lens_mem;
            
            for (h = 0; h < height; h++)
            {
                flpos_w = -center_w;
                for (w = 0; w < width; w++)
                {
                    double fldist = sqrt(flpos_w * flpos_w + flpos_h * flpos_h + flmag) / fldivs;
                    shift_w = (long)(fldist * flpos_w + center_w);
                    shift_h = (long)(fldist * flpos_h + center_h);
                    
                    // Clamp to valid range
                    if (shift_w >= width)  shift_w = width - 1;
                    if ((shift_w < 0) || ((period & 1) == 0)) shift_w = 0;
                    if (shift_h >= height) shift_h = height - 1;
                    if ((shift_h < 0) || ((period & 2) == 0)) shift_h = 0;
                    
                    *mem = shift_w + shift_h * pitch;
                    flpos_w += 1.0;
                    mem++;
                }
                flpos_h += 1.0;
            }
        }
        break;
        
    default:
        ERRORLOG("Unknown displacement algorithm %d", algorithm);
        break;
    }
}

/**
 * Apply displacement effect using pre-generated lookup map.
 */
static void ApplyDisplacement(unsigned char *dstbuf, unsigned char *srcbuf,
                             uint32_t *lens_mem, int width, int height,
                             int dstpitch, int lens_pitch)
{
    SYNCDBG(16, "Applying displacement");
    unsigned char *dst = dstbuf;
    uint32_t *mem = lens_mem;
    
    for (int h = 0; h < height; h++)
    {
        for (int w = 0; w < width; w++)
        {
            long pos_map = *mem;
            dst[w] = srcbuf[pos_map];
            mem++;
        }
        dst += dstpitch;
        mem += (lens_pitch - width);
    }
}

DisplacementEffect::DisplacementEffect()
    : LensEffect(LensEffectType::Displacement, "Displacement")
    , m_current_lens(-1)
    , m_algorithm(DisplaceAlgo_Sinusoidal)
    , m_magnitude(0)
    , m_period(0)
{
}

DisplacementEffect::~DisplacementEffect()
{
    Cleanup();
}

TbBool DisplacementEffect::Setup(long lens_idx)
{
    SYNCDBG(8, "Setting up displacement effect for lens %ld", lens_idx);
    
    struct LensConfig* cfg = &lenses_conf.lenses[lens_idx];
    
    // Read displacement parameters from config
    m_algorithm = (DisplacementAlgorithm)cfg->displace_kind;
    m_magnitude = cfg->displace_magnitude;
    m_period = cfg->displace_period;
    
    // Generate displacement map
    int disp_pixel_size = lbDisplay.GraphicsScreenHeight / 200;
    GenerateDisplacementMap(eye_lens_memory,
                          MyScreenWidth / disp_pixel_size, MyScreenHeight / disp_pixel_size,
                          lbDisplay.GraphicsScreenWidth,
                          m_algorithm, m_magnitude, m_period);
    
    m_current_lens = lens_idx;
    SYNCDBG(7, "Displacement effect ready (algo=%d, mag=%d, period=%d)",
           m_algorithm, m_magnitude, m_period);
    return true;
}

void DisplacementEffect::Cleanup()
{
    if (m_current_lens >= 0)
    {
        m_current_lens = -1;
    }
}

TbBool DisplacementEffect::Draw(LensRenderContext* ctx)
{
    if (m_current_lens < 0)
    {
        return false;
    }
    
    // Displacement needs to account for viewport offset in the source buffer
    uint32_t* viewport_lens_mem = eye_lens_memory + ctx->viewport_x;
    
    ApplyDisplacement(ctx->dstbuf, ctx->srcbuf, viewport_lens_mem,
                     ctx->width, ctx->height, ctx->dstpitch, ctx->srcpitch);
    
    ctx->buffer_copied = true;  // Displacement writes to dstbuf
    return true;
}
