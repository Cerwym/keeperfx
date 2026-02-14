/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file FlyeyeEffect.cpp
 *     Flyeye/compound eye lens effect implementation.
 * @par Purpose:
 *     Self-contained fish eye/hexagonal tiling effect.
 * @par Comment:
 *     Creates a compound eye view by rendering hexagonal tiles with radial distortion.
 *     Resolution-independent via pre-computed lookup tables.
 * @author   Tomasz Lis, KeeperFX Team
 * @date     11 Mar 2010 - 09 Feb 2026
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#include "../../pre_inc.h"
#include "FlyeyeEffect.h"

#include <cmath>
#include <stdlib.h>
#include "../../config_lenses.h"
#include "../../lens_api.h"

#include "../../keeperfx.hpp"
#include "../../post_inc.h"

/******************************************************************************/

// Reference dimensions for resolution-independent scaling
static const int REF_WIDTH = 640;
static const int REF_HEIGHT = 480;
static const int REF_MAXSIZE = 640;

// Hexagonal grid parameters (in reference 640x480 space)
static const double HEX_SPACING_X = 50.0;
static const double HEX_SPACING_Y = 60.0;
static const double HEX_ODD_OFFSET_Y = 30.0;

// Distortion parameters from original algorithm
static const double LDPAR1 = REF_MAXSIZE * 0.0175;
static const double LDPAR2 = REF_MAXSIZE * 0.0025;

/******************************************************************************/

FlyeyeEffect::FlyeyeEffect()
    : LensEffect(LensEffectType::Flyeye, "Flyeye")
    , m_current_lens(-1)
    , m_lookup_table(nullptr)
    , m_table_width(0)
    , m_table_height(0)
{
}

FlyeyeEffect::~FlyeyeEffect()
{
    Cleanup();
}

void FlyeyeEffect::FreeLookupTable()
{
    if (m_lookup_table != nullptr)
    {
        free(m_lookup_table);
        m_lookup_table = nullptr;
    }
    m_table_width = 0;
    m_table_height = 0;
}

/**
 * Build pre-computed lookup table for current resolution.
 * Computes flyeye effect in virtual 640x480 space, maps to actual coords.
 */
void FlyeyeEffect::BuildLookupTable(long width, long height)
{
    FreeLookupTable();
    
    size_t table_size = width * height * sizeof(FlyeyeLookupEntry);
    m_lookup_table = (FlyeyeLookupEntry*)malloc(table_size);
    if (m_lookup_table == nullptr)
    {
        ERRORLOG("Failed to allocate flyeye lookup table (%" PRIuSIZE " bytes)", SZCAST(table_size));
        return;
    }
    
    m_table_width = width;
    m_table_height = height;
    
    // Scale factors
    const unsigned int scale_x = (REF_WIDTH << 16) / width;
    const unsigned int scale_y = (REF_HEIGHT << 16) / height;
    const unsigned int inv_scale_x = (width << 16) / REF_WIDTH;
    const unsigned int inv_scale_y = (height << 16) / REF_HEIGHT;
    
    const double ref_center_x = REF_WIDTH * 0.5;
    const double ref_center_y = REF_HEIGHT * 0.5;
    
    FlyeyeLookupEntry* entry = m_lookup_table;
    
    for (long y = 0; y < height; y++)
    {
        int virtual_y = (y * scale_y) >> 16;
        
        for (long x = 0; x < width; x++)
        {
            int virtual_x = (x * scale_x) >> 16;
            
            // Determine hex cell
            double rel_x = virtual_x - ref_center_x;
            double rel_y = virtual_y - ref_center_y;
            
            int hex_x = (int)floor(rel_x / HEX_SPACING_X + 0.5);
            double y_offset = ((hex_x & 1) != 0) ? HEX_ODD_OFFSET_Y : 0.0;
            int hex_y = (int)floor((rel_y - y_offset) / HEX_SPACING_Y + 0.5);
            
            // Clamp hex coordinates
            if (hex_x < -12) hex_x = -12;
            if (hex_x > 12) hex_x = 12;
            if (hex_y < -12) hex_y = -12;
            if (hex_y > 12) hex_y = 12;
            
            // Calculate source offset for this hex cell
            double source_offset_x = -hex_x * LDPAR1;
            double source_offset_y = -hex_y * LDPAR1;
            
            // Apply radial distortion
            double len = sqrt(rel_x * rel_x + rel_y * rel_y) * 0.0025 + 1.0;
            double distorted_x = ref_center_x + (rel_x / len) * LDPAR2;
            double distorted_y = ref_center_y + (rel_y / len) * LDPAR2;
            
            // Calculate source position
            double src_ref_x = distorted_x + source_offset_x;
            double src_ref_y = distorted_y + source_offset_y;
            
            // Clamp to reference bounds
            if (src_ref_x < 0) src_ref_x = 0;
            if (src_ref_x >= REF_WIDTH) src_ref_x = REF_WIDTH - 1;
            if (src_ref_y < 0) src_ref_y = 0;
            if (src_ref_y >= REF_HEIGHT) src_ref_y = REF_HEIGHT - 1;
            
            // Map to actual resolution
            long actual_src_x = ((long)(src_ref_x) * inv_scale_x) >> 16;
            long actual_src_y = ((long)(src_ref_y) * inv_scale_y) >> 16;
            
            if (actual_src_x >= width) actual_src_x = width - 1;
            if (actual_src_y >= height) actual_src_y = height - 1;
            
            entry->src_x = (short)actual_src_x;
            entry->src_y = (short)actual_src_y;
            entry++;
        }
    }
    
    SYNCDBG(7, "Built flyeye lookup table %ldx%ld", width, height);
}

TbBool FlyeyeEffect::Setup(long lens_idx)
{
    SYNCDBG(8, "Setting up flyeye effect for lens %ld", lens_idx);
    
    FreeLookupTable();
    m_current_lens = lens_idx;
    
    SYNCDBG(7, "Flyeye effect ready");
    return true;
}

void FlyeyeEffect::Cleanup()
{
    FreeLookupTable();
    m_current_lens = -1;
}

TbBool FlyeyeEffect::Draw(LensRenderContext* ctx)
{
    if (m_current_lens < 0)
    {
        return false;
    }
    
    // Build lookup table if needed
    if (m_lookup_table == nullptr ||
        m_table_width != ctx->width ||
        m_table_height != ctx->height)
    {
        BuildLookupTable(ctx->width, ctx->height);
        if (m_lookup_table == nullptr)
        {
            return false;
        }
    }
    
    // Fast lookup-based rendering
    unsigned char* viewport_src = ctx->srcbuf + ctx->viewport_x;
    unsigned char* dst = ctx->dstbuf;
    FlyeyeLookupEntry* entry = m_lookup_table;
    
    for (long y = 0; y < ctx->height; y++)
    {
        for (long x = 0; x < ctx->width; x++)
        {
            dst[x] = viewport_src[entry->src_y * ctx->srcpitch + entry->src_x];
            entry++;
        }
        dst += ctx->dstpitch;
    }
    
    ctx->buffer_copied = true;
    return true;
}