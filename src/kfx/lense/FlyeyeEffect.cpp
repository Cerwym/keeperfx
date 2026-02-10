/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file FlyeyeEffect.cpp
 *     Flyeye/compound eye lens effect implementation.
 * @par Purpose:
 *     Self-contained fish eye/hexagonal tiling effect.
 * @par Comment:
 *     Creates a compound eye view by rendering hexagonal tiles with radial distortion.
 * @author   Tomasz Lis, KeeperFX Team
 * @date     11 Mar 2010 - 09 Feb 2026
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#include "../../pre_inc.h"
#include "FlyeyeEffect.h"

#include <cmath>
#include <string.h>
#include "../../config_lenses.h"
#include "../../lens_api.h"

#include "../../keeperfx.hpp"
#include "../../post_inc.h"

/******************************************************************************/
// INTERNAL FLYEYE RENDERER CLASSES
/******************************************************************************/

#define CSCAN_STRIPS 26

/**
 * Scan line strip data structure.
 * Each scanline can contain up to CSCAN_STRIPS strips of different source offsets.
 */
struct CScan {
    long strips_num;
    unsigned short strip_len[CSCAN_STRIPS];
    short strip_w[CSCAN_STRIPS];
    short strip_h[CSCAN_STRIPS];
};

/**
 * CFlyeyeRenderer - Internal flyeye rendering system.
 * Manages hexagonal tile generation and scanline-based blitting.
 */
class CFlyeyeRenderer {
public:
    CFlyeyeRenderer();
    ~CFlyeyeRenderer();
    
    void Setup(long width, long height, uint32_t* buffer);
    void Render(unsigned char *srcbuf, long srcpitch, unsigned char *dstbuf, 
               long dstpitch, long start_h, long end_h);
    
private:
    class CHex {
    public:
        CHex(CFlyeyeRenderer* renderer, long x, long y);
        void BlitHex();
        void AddScan(struct CScan *scan, long strip_len, long len_limit, 
                    long nstrip_w, long nstrip_h);
        void BlitScan(struct CScan *scan, long h);
        
    private:
        CFlyeyeRenderer* m_renderer;
        long arrA[6];
        long arrB[6];
        long source_strip_w;
        long source_strip_h;
    };
    
    struct CScan *m_scan_buffer;
    long m_width;
    long m_height;
    long m_center_x;
    long m_center_y;
    
    // Rendering state (set during Render())
    unsigned char *m_source;
    long m_source_pitch;
    unsigned char *m_screen;
    long m_screen_pitch;
    
    friend class CHex;
};

CFlyeyeRenderer::CFlyeyeRenderer()
    : m_scan_buffer(NULL)
    , m_width(0)
    , m_height(0)
    , m_center_x(0)
    , m_center_y(0)
    , m_source(NULL)
    , m_source_pitch(0)
    , m_screen(NULL)
    , m_screen_pitch(0)
{
}

CFlyeyeRenderer::~CFlyeyeRenderer()
{
}

void CFlyeyeRenderer::Setup(long width, long height, uint32_t* buffer)
{
    m_width = width;
    m_height = height;
    m_center_x = width >> 1;
    m_center_y = height >> 1;
    m_scan_buffer = (struct CScan*)buffer;
    
    // Initialize scan buffer
    for (long i = 0; i < m_height; i++)
    {
        m_scan_buffer[i].strips_num = 0;
    }
    
    // Generate hexagonal tiles
    for (long y = -12; y <= 12; y++)
    {
        for (long x = -12; x <= 12; x++)
        {
            CHex hex(this, x, y);
            hex.BlitHex();
        }
    }
    
    SYNCDBG(8, "Flyeye setup complete for %ldx%ld", width, height);
}

void CFlyeyeRenderer::Render(unsigned char *srcbuf, long srcpitch, unsigned char *dstbuf,
                             long dstpitch, long start_h, long end_h)
{
    m_source = srcbuf;
    m_source_pitch = srcpitch;
    m_screen = dstbuf;
    m_screen_pitch = dstpitch;
    
    // Draw scanlines
    for (long h = start_h; h < end_h; h++)
    {
        CHex hex(this, 0, 0);  // Temp instance for BlitScan (static-like method)
        hex.BlitScan(&m_scan_buffer[h], h);
    }
}

// CHex implementation
CFlyeyeRenderer::CHex::CHex(CFlyeyeRenderer* renderer, long x, long y)
    : m_renderer(renderer)
{
    long maxsize;
    long mwidth;
    long mheight;
    long double ldpar1;
    long double ldpar2;
    long double varA;
    long double varB;
    long double len;
    
    if (m_renderer->m_height > m_renderer->m_width)
        maxsize = m_renderer->m_height;
    else
        maxsize = m_renderer->m_width;
        
    mwidth = 50 * x;
    mheight = 60 * y;
    ldpar1 = (long double)maxsize * 0.0175L;
    ldpar2 = (long double)maxsize * 0.0025L;
    
    if ((x & 1) != 0)
        mheight += 30;
        
    this->arrA[0] = mwidth - 35;
    this->arrB[0] = mheight + 30;
    this->arrA[1] = mwidth - 15;
    this->arrB[1] = mheight;
    this->arrA[2] = mwidth + 15;
    this->arrB[2] = mheight;
    this->arrA[3] = mwidth + 35;
    this->arrB[3] = mheight + 30;
    this->arrA[4] = mwidth + 15;
    this->arrB[4] = mheight + 60;
    this->arrA[5] = mwidth - 15;
    this->arrB[5] = mheight + 60;
    
    for (long i = 0; i < 6; i++)
    {
        varA = this->arrA[i];
        varB = this->arrB[i];
        len = sqrt(varA * varA + varB * varB) * 0.0025L + 1.0L;
        this->arrA[i] = (signed long long)((long double)m_renderer->m_center_x + varA / len * ldpar2);
        this->arrB[i] = (signed long long)((long double)m_renderer->m_center_y + varB / len * ldpar2);
    }
    
    this->source_strip_w = ((long double)-x * ldpar1);
    this->source_strip_h = (ldpar1 * (long double)-y);
}

void CFlyeyeRenderer::CHex::AddScan(struct CScan *scan, long strip_len, long len_limit,
                                    long nstrip_w, long nstrip_h)
{
    long stlen = strip_len;
    if (strip_len < 0)
        stlen = 0;
    if ((stlen >= m_renderer->m_width) || (stlen >= len_limit))
        return;
    if (scan->strips_num >= CSCAN_STRIPS)
        return;
        
    long n = 0;
    long i = 0;
    // Find index for new strip
    for (i = 0; ; i++)
    {
        if (i >= scan->strips_num)
        {
            // Last element - just fill it
            scan->strip_len[scan->strips_num] = stlen;
            scan->strip_w[scan->strips_num] = nstrip_w;
            scan->strip_h[scan->strips_num] = nstrip_h;
            scan->strips_num++;
            return;
        }
        long clen = scan->strip_len[n];
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
    
    // Move elements to make space
    n = scan->strips_num;
    while (n > (long)i)
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

void CFlyeyeRenderer::CHex::BlitHex()
{
    int min_idx = 0;
    int counter1, counter2;
    int first_idx, last_idx;
    int deltaV1, deltaV2;
    int posV1, posV2;
    long scan_num;
    
    for (int i = 1; i < 6; i++)
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
        int i = first_idx;
        while (counter1 == 0)
        {
            first_idx = (first_idx + 5) % 6;
            if (first_idx == i)
                return;
            posV1 = this->arrA[first_idx] << 16;
            int n = (first_idx + 5) % 6;
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
            int n = (last_idx + 1) % 6;
            counter2 = this->arrB[n] - this->arrB[last_idx];
            posV2 = this->arrA[last_idx] << 16;
            if (counter2 > 0) {
                deltaV2 = ((this->arrA[n] << 16) - posV2) / counter2;
            }
        }
        
        if ((counter1 <= 0) || (counter2 <= 0))
            return;
            
        if ((scan_num >= 0) && (scan_num < m_renderer->m_height))
        {
            SYNCDBG(19, "ScanBuffer %ld pos %d,%d from %d,%d", scan_num, posV1, posV2,
                   (int)this->source_strip_w, (int)this->source_strip_h);
            AddScan(&m_renderer->m_scan_buffer[scan_num], posV1 >> 16, posV2 >> 16,
                   this->source_strip_w, this->source_strip_h);
        }
        counter1--;
        counter2--;
        posV1 += deltaV1;
        posV2 += deltaV2;
        scan_num++;
    }
}

void CFlyeyeRenderer::CHex::BlitScan(struct CScan *scan, long h)
{
    unsigned char *dst;
    unsigned char *src;
    long shift_w, shift_h;
    long w, end_w;
    
    SYNCDBG(16, "Starting line %ld", h);
    for (long i = 0; i < scan->strips_num; i++)
    {
        w = scan->strip_len[i];
        if (i == scan->strips_num - 1)
            end_w = m_renderer->m_width;
        else
            end_w = scan->strip_len[i+1];
            
        shift_w = (long)scan->strip_w[i] + w;
        shift_h = (long)scan->strip_h[i] + h;
        dst = &m_renderer->m_screen[h * m_renderer->m_screen_pitch + w];
        src = &m_renderer->m_source[shift_h * m_renderer->m_source_pitch + shift_w];
        memcpy(dst, src, end_w - w);
    }
}

FlyeyeEffect::FlyeyeEffect()
    : LensEffect(LensEffectType::Flyeye, "Flyeye")
    , m_current_lens(-1)
{
}

FlyeyeEffect::~FlyeyeEffect()
{
    Cleanup();
}

TbBool FlyeyeEffect::Setup(long lens_idx)
{
    SYNCDBG(8, "Setting up flyeye effect for lens %ld", lens_idx);
    
    // Create and setup renderer
    CFlyeyeRenderer* renderer = new CFlyeyeRenderer();
    renderer->Setup(eye_lens_width, eye_lens_height, eye_lens_memory);
    
    m_user_data = renderer;
    m_current_lens = lens_idx;
    
    SYNCDBG(7, "Flyeye effect ready");
    return true;
}

void FlyeyeEffect::Cleanup()
{
    if (m_current_lens >= 0)
    {
        if (m_user_data != NULL)
        {
            delete static_cast<CFlyeyeRenderer*>(m_user_data);
            m_user_data = NULL;
        }
        m_current_lens = -1;
        SYNCDBG(9, "Flyeye effect cleaned up");
    }
}

TbBool FlyeyeEffect::Draw(LensRenderContext* ctx)
{
    if (m_current_lens < 0 || m_user_data == NULL)
    {
        return false;
    }
    
    SYNCDBG(16, "Drawing flyeye effect");
    
    CFlyeyeRenderer* renderer = static_cast<CFlyeyeRenderer*>(m_user_data);
    
    // Flyeye reads from viewport-aligned source
    unsigned char* viewport_src = ctx->srcbuf + ctx->viewport_x;
    
    renderer->Render(viewport_src, ctx->srcpitch, ctx->dstbuf, ctx->dstpitch,
                    1, ctx->height);
    
    ctx->buffer_copied = true;
    return true;
}