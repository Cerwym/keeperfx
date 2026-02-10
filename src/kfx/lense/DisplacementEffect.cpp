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
#include <string.h>
#include "../../config_lenses.h"
#include "../../vidmode.h"
#include "../../lens_api.h"

#include "../../keeperfx.hpp"
#include "../../post_inc.h"

/******************************************************************************/
// Compound eye (flyeye) effect - original implementation from lens_flyeye.cpp
/******************************************************************************/

#define CSCAN_STRIPS 26

struct CScan {
    long strips_num;
    unsigned short strip_len[CSCAN_STRIPS];
    short strip_w[CSCAN_STRIPS];
    short strip_h[CSCAN_STRIPS];
};

class CHex {
public:
    CHex(long width, long height, long scrCenterX, long scrCenterY, long scrWidth, long scrHeight, struct CScan* scanBuffer);
    void BlitHex();
    static void AddScan(struct CScan *scan, long strip_len, long len_limit, long nstrip_w, long nstrip_h, long scrWidth);
    static void BlitScan(struct CScan *scan, long dst_line, unsigned char* dstbuf, unsigned char* srcbuf, long dstpitch, long srcpitch, long scrWidth, int pixel_scale);
    
private:
    long arrA[6];
    long arrB[6];
    long source_strip_w;
    long source_strip_h;
    long m_scrCenterX;
    long m_scrCenterY;
    long m_scrWidth;
    long m_scrHeight;
    struct CScan* m_scanBuffer;
};

CHex::CHex(long width, long height, long scrCenterX, long scrCenterY, long scrWidth, long scrHeight, struct CScan* scanBuffer)
    : m_scrCenterX(scrCenterX)
    , m_scrCenterY(scrCenterY)
    , m_scrWidth(scrWidth)
    , m_scrHeight(scrHeight)
    , m_scanBuffer(scanBuffer)
{
    long maxsize;
    long mwidth;
    long mheight;
    long double ldpar1;
    long double ldpar2;
    long double varA;
    long double varB;
    long double len;
    long i;
    
    if (scrHeight > scrWidth)
        maxsize = scrHeight;
    else
        maxsize = scrWidth;
    mwidth = 50 * width;
    mheight = 60 * height;
    ldpar1 = (long double)maxsize * 0.0175L;
    ldpar2 = (long double)maxsize * 0.0025L;
    if ((width & 1) != 0)
        mheight += 30;
        
    this->arrA[0] = mwidth - 35;
    this->arrB[0] = mheight + 30;
    this->arrA[1] = mwidth - 15;
    this->arrB[1] = mheight;
    this->arrA[2] = mwidth + 15;
    this->arrB[2] = mheight;
    this->arrA[3] = mwidth + 35;
    this->arrA[4] = mwidth + 15;
    this->arrB[4] = mheight + 60;
    this->arrB[3] = mheight + 30;
    this->arrA[5] = mwidth - 15;
    this->arrB[5] = mheight + 60;
    
    for (i = 0; i < 6; i++)
    {
        varA = this->arrA[i];
        varB = this->arrB[i];
        len = sqrt(varA * varA + varB * varB) * 0.0025L + 1.0L;
        this->arrA[i] = (signed long long)((long double)scrCenterX + varA / len * ldpar2);
        this->arrB[i] = (signed long long)((long double)scrCenterY + varB / len * ldpar2);
    }
    this->source_strip_w = ((long double)-width * ldpar1);
    this->source_strip_h = (ldpar1 * (long double)-height);
}

void CHex::AddScan(struct CScan *scan, long strip_len, long len_limit, long nstrip_w, long nstrip_h, long scrWidth)
{
    long stlen;
    long clen;
    long i;
    long n;

    stlen = strip_len;
    if (strip_len < 0)
        stlen = 0;
    if ((stlen >= scrWidth) || (stlen >= len_limit))
        return;
    if (scan->strips_num >= CSCAN_STRIPS)
        return;
    n = 0;
    // Find index for new strip
    for (i = 0; ; i++)
    {
        if (i >= scan->strips_num)
        {
            // If it's last, just fill it and finish
            scan->strip_len[scan->strips_num] = stlen;
            scan->strip_w[scan->strips_num] = nstrip_w;
            scan->strip_h[scan->strips_num] = nstrip_h;
            scan->strips_num++;
            return;
        }
        clen = scan->strip_len[n];
        if (stlen == clen)
        {
            scan->strip_w[i] = nstrip_w;
            scan->strip_h[i] = nstrip_h;
            return;
        }
        if (clen > stlen)
            break;
        n++;
    }
    // Move elements to make space for new scan
    n = scan->strips_num;
    while (n > i)
    {
        scan->strip_len[n] = scan->strip_len[n-1];
        scan->strip_w[n] = scan->strip_w[n-1];
        scan->strip_h[n] = scan->strip_h[n-1];
        n--;
    }
    // Insert scan data
    scan->strip_len[i] = stlen;
    scan->strip_w[i] = nstrip_w;
    scan->strip_h[i] = nstrip_h;
    scan->strips_num++;
}

void CHex::BlitHex()
{
    int min_idx;
    int counter1;
    int counter2;
    int first_idx;
    int last_idx;
    int deltaV1;
    int deltaV2;
    int posV1;
    int posV2;
    long scan_num;
    int i;
    int n;
    
    min_idx = 0;
    for (i = 1; i < 6; i++)
    {
        if (this->arrB[i] < this->arrB[min_idx])
            min_idx = i;
    }
    scan_num = this->arrB[min_idx];
    first_idx = (min_idx + 1) % 6;
    last_idx = (min_idx + 5) % 6;
    deltaV1 = 0; deltaV2 = 0;
    posV1 = 0; posV2 = 0;
    counter2 = 0;
    counter1 = 0;
    
    while (1)
    {
        i = first_idx;
        while (counter1 == 0)
        {
            first_idx = (first_idx + 5) % 6;
            if (first_idx == i)
                return;
            posV1 = this->arrA[first_idx] << 16;
            n = (first_idx + 5) % 6;
            counter1 = this->arrB[n] - this->arrB[first_idx];
            if (counter1 > 0) {
                deltaV1 = ((this->arrA[n] << 16) - posV1) / counter1;
            }
        }
        i = last_idx;
        while (counter2 == 0)
        {
            last_idx = (last_idx + 1) % 6;
            if (last_idx == i)
                return;
            n = (last_idx + 1) % 6;
            counter2 = this->arrB[n] - this->arrB[last_idx];
            posV2 = this->arrA[last_idx] << 16;
            if (counter2 > 0) {
                deltaV2 = ((this->arrA[n] << 16) - posV2) / counter2;
            }
        }
        if ((counter1 <= 0) || (counter2 <= 0))
            return;
        if ((scan_num >= 0) && (scan_num < m_scrHeight))
        {
            SYNCDBG(19, "ScanBuffer %ld pos %d,%d from %d,%d", scan_num, posV1, posV2, (int)this->source_strip_w, (int)this->source_strip_h);
            AddScan(&m_scanBuffer[scan_num], posV1 >> 16, posV2 >> 16, this->source_strip_w, this->source_strip_h, m_scrWidth);
        }
        counter1--;
        counter2--;
        posV1 += deltaV1;
        posV2 += deltaV2;
        scan_num++;
    }
}

void CHex::BlitScan(struct CScan *scan, long dst_line, unsigned char* dstbuf, unsigned char* srcbuf, long dstpitch, long srcpitch, long scrWidth, int pixel_scale)
{
    unsigned char *dst;
    unsigned char *src;
    long shift_w;
    long shift_h;
    long w;
    long end_w;
    long i;
    
    SYNCDBG(18, "Rendering line %ld (scale=%d, strips=%ld)", dst_line, pixel_scale, scan->strips_num);
    
    for (i = 0; i < scan->strips_num; i++)
    {
        w = scan->strip_len[i];
        if (i == scan->strips_num - 1)
            end_w = scrWidth;
        else
            end_w = scan->strip_len[i+1];
        
        // Scanline coordinates are in scaled-down space, scale them up for full resolution buffer
        // The strip_w/strip_h offsets tell us which part of the source to read from
        shift_w = ((long)scan->strip_w[i] + w) * pixel_scale;
        shift_h = ((long)scan->strip_h[i] + dst_line / pixel_scale) * pixel_scale;
        
        // Destination coordinates scaled to full resolution
        long dst_w = w * pixel_scale;
        long dst_end_w = end_w * pixel_scale;
        
        dst = &dstbuf[dst_line * dstpitch + dst_w];
        src = &srcbuf[shift_h * srcpitch + shift_w];
        
        long copy_len = dst_end_w - dst_w;
        if (copy_len > 0 && shift_w >= 0 && shift_h >= 0 && shift_w < srcpitch && shift_h * srcpitch < srcpitch * 2000) {
            memcpy(dst, src, copy_len);
        }
    }
}

/******************************************************************************/
// Standard displacement map generation
/******************************************************************************/

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
        
    case DisplaceAlgo_Compound:
        // Compound eye / hexagonal tiling effect (for fly creature)
        // Uses original scanline-based rendering with CHex class
        // NOTE: This case sets up CScan structures in lens_mem, not a displacement map
        {
            long scrCenterX = width >> 1;
            long scrCenterY = height >> 1;
            struct CScan* scanBuffer = (struct CScan*)lens_mem;
            
            // Initialize scan buffer
            for (long i = 0; i < height; i++)
            {
                scanBuffer[i].strips_num = 0;
            }
            
            // Generate hexagonal pattern
            for (long y = -12; y <= 12; y++) {
                for (long x = -12; x <= 12; x++) {
                    CHex chx(x, y, scrCenterX, scrCenterY, width, height, scanBuffer);
                    chx.BlitHex();
                }
            }
            
            SYNCDBG(7, "Compound eye scanline buffer initialized");
        }
        break;
        
    default:
        ERRORLOG("Unknown displacement algorithm %d - initializing to identity map", algorithm);
        // Initialize to identity map (no distortion) as fallback
        mem = lens_mem;
        for (h = 0; h < height; h++)
        {
            for (w = 0; w < width; w++)
            {
                *mem = w + h * pitch;
                mem++;
            }
        }
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
    
    // Calculate maximum valid source buffer offset for bounds checking
    // Assuming source buffer is at least as large as screen buffer
    long max_src_offset = lbDisplay.GraphicsScreenWidth * lbDisplay.GraphicsScreenHeight;
    
    for (int h = 0; h < height; h++)
    {
        for (int w = 0; w < width; w++)
        {
            long pos_map = *mem;
            
            // Bounds check to prevent crashes from invalid displacement maps
            if (pos_map >= 0 && pos_map < max_src_offset) {
                dst[w] = srcbuf[pos_map];
            } else {
                // Invalid offset - use unmodified pixel (identity mapping)
                dst[w] = srcbuf[w + h * lens_pitch];
            }
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
    , m_uses_compound_eye(false)
    , m_setup_width(0)
    , m_setup_height(0)
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
    
    // Check if using compound eye (which needs scanline rendering, not displacement map)
    m_uses_compound_eye = (m_algorithm == DisplaceAlgo_Compound);
    
    // Generate displacement map or scanline buffer
    int disp_pixel_size = lbDisplay.GraphicsScreenHeight / 200;
    m_setup_width = MyScreenWidth / disp_pixel_size;
    m_setup_height = MyScreenHeight / disp_pixel_size;
    
    GenerateDisplacementMap(eye_lens_memory,
                          m_setup_width, m_setup_height,
                          lbDisplay.GraphicsScreenWidth,
                          m_algorithm, m_magnitude, m_period);
    
    m_current_lens = lens_idx;
    SYNCDBG(7, "Displacement effect ready (algo=%d, mag=%d, period=%d, compound_eye=%d, dims=%dx%d)",
           m_algorithm, m_magnitude, m_period, m_uses_compound_eye, m_setup_width, m_setup_height);
    return true;
}

void DisplacementEffect::Cleanup()
{
    if (m_current_lens >= 0)
    {
        m_current_lens = -1;
        m_uses_compound_eye = false;
        m_setup_width = 0;
        m_setup_height = 0;
    }
}

TbBool DisplacementEffect::Draw(LensRenderContext* ctx)
{
    if (m_current_lens < 0)
    {
        return false;
    }
    
    // Compound eye uses scanline rendering, not displacement map
    if (m_uses_compound_eye)
    {
        struct CScan* scanBuffer = (struct CScan*)eye_lens_memory;
        
        // The scanBuffer was built for m_setup_width x m_setup_height dimensions (scaled down)
        // But ctx dimensions are the full resolution. Calculate scaling factor.
        int disp_pixel_size = lbDisplay.GraphicsScreenHeight / 200;
        if (disp_pixel_size < 1) disp_pixel_size = 1;
        
        // Source buffer needs viewport offset applied (2D addressing)
        // Calculate once outside loop for efficiency
        unsigned char* viewport_src = ctx->srcbuf + (ctx->viewport_y * ctx->srcpitch) + ctx->viewport_x;
        
        // Render each scanline using the pre-computed scan buffer
        // Each scanline in the buffer needs to be scaled up by pixel_scale
        for (long scan_y = 0; scan_y < m_setup_height; scan_y++)
        {
            // Render this scanline pixel_scale times (vertically)
            for (int scale_y = 0; scale_y < disp_pixel_size; scale_y++)
            {
                long dst_y = scan_y * disp_pixel_size + scale_y;
                if (dst_y >= ctx->height) break;
                
                // BlitScan renders scanline to destination line dst_y
                // Pass viewport width as srcpitch (not full screen pitch)
                CHex::BlitScan(&scanBuffer[scan_y], dst_y, ctx->dstbuf, viewport_src, 
                              ctx->dstpitch, ctx->width, ctx->width, disp_pixel_size);
            }
        }
    }
    else
    {
        // Standard displacement using lookup map
        // Source buffer needs viewport offset applied (2D addressing)
        unsigned char* viewport_src = ctx->srcbuf + (ctx->viewport_y * ctx->srcpitch) + ctx->viewport_x;
        
        ApplyDisplacement(ctx->dstbuf, viewport_src, eye_lens_memory,
                         ctx->width, ctx->height, ctx->dstpitch, ctx->srcpitch);
    }
    
    ctx->buffer_copied = true;  // Displacement writes to dstbuf
    return true;
}
