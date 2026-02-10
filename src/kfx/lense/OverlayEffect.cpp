/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file OverlayEffect.cpp
 *     Overlay lens effect implementation.
 * @par Purpose:
 *     Self-contained overlay compositing effect with alpha blending.
 * @par Comment:
 *     Loads and draws overlay sprites onto the viewport.
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
#include "OverlayEffect.h"

#include <string.h>
#include "../../bflib_basics.h"
#include "../../bflib_fileio.h"
#include "../../bflib_dernc.h"
#include "../../globals.h"
#include "../../config_lenses.h"
#include "../../custom_sprites.h"
#include "../../vidmode.h"

#include "../../keeperfx.hpp"
#include "../../post_inc.h"

/******************************************************************************/
// INTERNAL OVERLAY RENDERER CLASS
/*****************************************************************************/

/**
 * COverlayRenderer - Internal overlay compositing system.
 * Manages overlay image loading and alpha blending.
 */
class COverlayRenderer {
public:
    COverlayRenderer(OverlayEffect* parent);
    ~COverlayRenderer();
    
    TbBool LoadOverlay(long lens_idx);
    void Render(unsigned char *dstbuf, long dstpitch, unsigned char *srcbuf, long srcpitch, 
                long width, long height);
    
private:
    OverlayEffect* m_parent;         // Parent effect for asset loading
    unsigned char* m_overlay_data;   // Overlay image data (from registry)
    int m_width;                     // Overlay width
    int m_height;                    // Overlay height
    short m_alpha;                   // Alpha blending level (0-256)
    TbBool m_loaded;                 // Whether overlay is loaded
    TbBool m_owns_data;              // True if we allocated m_overlay_data and must free it
};

COverlayRenderer::COverlayRenderer(OverlayEffect* parent)
    : m_parent(parent)
    , m_overlay_data(NULL)
    , m_width(0)
    , m_height(0)
    , m_alpha(128)
    , m_loaded(false)
    , m_owns_data(false)
{
}

COverlayRenderer::~COverlayRenderer()
{
    // Only free if we allocated the data (file fallback path)
    if (m_owns_data && m_overlay_data != NULL)
    {
        free(m_overlay_data);
    }
    m_overlay_data = NULL;
}

TbBool COverlayRenderer::LoadOverlay(long lens_idx)
{
    // Get lens configuration
    struct LensConfig* cfg = get_lens_config(lens_idx);
    if (cfg == NULL)
    {
        WARNLOG("Failed to get lens config for index %ld", lens_idx);
        return false;
    }
    
    // Check if this lens has an overlay effect
    if ((cfg->flags & LCF_HasOverlay) == 0)
    {
        SYNCDBG(8, "Lens %ld does not have overlay effect", lens_idx);
        return false;
    }
    
    // Try to get overlay data from asset registry (ZIP files with lenses.json)
    const struct LensOverlayData* overlay = get_lens_overlay_data(cfg->overlay_file);
    
    if (overlay != NULL && overlay->data != NULL && 
        overlay->width > 0 && overlay->height > 0)
    {
        // Use registry data (we don't own it, no need to free)
        m_overlay_data = overlay->data;
        m_width = overlay->width;
        m_height = overlay->height;
        m_owns_data = false;  // Registry owns this data
        
        SYNCDBG(7, "Loaded overlay '%s' (%dx%d) from asset registry", 
                cfg->overlay_file, m_width, m_height);
    }
    else
    {
        // Registry lookup failed - try loading raw files from mods/data directories
        // This allows simple file-based mods without requiring ZIP/JSON
        // For overlays without registry, assume 256x256 (standard size)
        const int default_size = 256;
        m_overlay_data = (unsigned char*)malloc(default_size * default_size);
        
        if (m_overlay_data == NULL)
        {
            ERRORLOG("Failed to allocate memory for overlay (lens %ld)", lens_idx);
            return false;
        }
        
        m_owns_data = true;  // We allocated this, must free in destructor
        
        const char* loaded_from = NULL;
        if (!m_parent->LoadAssetWithFallback(cfg->overlay_file, m_overlay_data, 
                                             default_size * default_size, &loaded_from))
        {
            WARNLOG("Failed to load overlay '%s' from registry or files for lens %ld", 
                    cfg->overlay_file, lens_idx);
            free(m_overlay_data);
            m_overlay_data = NULL;
            m_owns_data = false;
            return false;
        }
        
        m_width = default_size;
        m_height = default_size;
        
        if (loaded_from != NULL) {
            SYNCDBG(7, "Loaded overlay '%s' (%dx%d) from mod '%s'", 
                    cfg->overlay_file, m_width, m_height, loaded_from);
        } else {
            SYNCDBG(7, "Loaded overlay '%s' (%dx%d) from base game files", 
                    cfg->overlay_file, m_width, m_height);
        }
    }
    
    // Store alpha value from config
    m_alpha = cfg->overlay_alpha;
    m_loaded = true;
    
    return true;
}

void COverlayRenderer::Render(unsigned char *dstbuf, long dstpitch, unsigned char *srcbuf, long srcpitch,
                              long width, long height)
{
    if (!m_loaded || m_overlay_data == NULL)
    {
        return;
    }
    
    const unsigned char* overlay_src = m_overlay_data;
    const short alpha = m_alpha;
    
    // Overlay dimensions
    const int overlay_w = m_width;
    const int overlay_h = m_height;
    
    // Get ghost table for proper color blending in palette space
    const unsigned char* ghost_table = pixmap.ghost;
    
    // Composite overlay onto destination buffer with stretching to fill viewport
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            // Get source pixel (the 3D view)
            unsigned char src_pixel = srcbuf[y * srcpitch + x];
            
            // Calculate overlay coordinates with scaling to match viewport
            // Map viewport coordinates (0..width-1, 0..height-1) to overlay (0..overlay_w-1, 0..overlay_h-1)
            int overlay_x = (x * overlay_w) / width;
            int overlay_y = (y * overlay_h) / height;
            
            // Clamp to ensure we don't read out of bounds
            if (overlay_x >= overlay_w) overlay_x = overlay_w - 1;
            if (overlay_y >= overlay_h) overlay_y = overlay_h - 1;
            
            // Get overlay pixel
            unsigned char overlay_pixel = overlay_src[overlay_y * overlay_w + overlay_x];
            
            // Check for transparency (palette index 255 from PNG alpha < 128)
            if (overlay_pixel == 255)
            {
                // Transparent pixel - pass through source unchanged
                dstbuf[y * dstpitch + x] = src_pixel;
                continue;
            }
            
            // Alpha blending strategy:
            // High alpha (>= 200): Show overlay directly (opaque, for knight helmet frame)
            // Medium alpha (100-199): Blend using ghost table
            // Low alpha (< 100): Light blend, favor source
            unsigned char result;
            
            if (alpha >= 200)
            {
                // Very high alpha: overlay is opaque, use directly
                // This ensures white helmet frame shows solid, not transparent
                result = overlay_pixel;
            }
            else if (alpha >= 100)
            {
                // Medium alpha: balanced ghost table blend (1:2 ratio)
                result = ghost_table[src_pixel * 256 + overlay_pixel];
            }
            else
            {
                // Low alpha: mostly source, light overlay effect
                // Double ghost lookup for lighter blend
                result = ghost_table[src_pixel * 256 + overlay_pixel];
                result = ghost_table[src_pixel * 256 + result];
            }
            
            // Write to destination
            dstbuf[y * dstpitch + x] = result;
        }
    }
}

/******************************************************************************/
// OVERLAYEFFECT PUBLIC INTERFACE
/******************************************************************************/

OverlayEffect::OverlayEffect()
    : LensEffect(LensEffectType::Overlay, "Overlay")
    , m_current_lens(-1)
{
}

OverlayEffect::~OverlayEffect()
{
    Cleanup();
}

TbBool OverlayEffect::Setup(long lens_idx)
{
    SYNCDBG(8, "Setting up overlay effect for lens %ld", lens_idx);
    
    struct LensConfig* cfg = get_lens_config(lens_idx);
    if (cfg == NULL)
    {
        WARNLOG("Failed to get lens config for index %ld", lens_idx);
        return true;  // Continue without overlay
    }
    
    // Check if this lens has an overlay effect configured
    if ((cfg->flags & LCF_HasOverlay) == 0)
    {
        SYNCDBG(8, "Lens %ld does not have overlay effect configured", lens_idx);
        return true;  // Not an error - effect just not configured
    }
    
    // Create and setup renderer
    COverlayRenderer* renderer = new COverlayRenderer(this);
    
    if (!renderer->LoadOverlay(lens_idx))
    {
        WARNLOG("Failed to load overlay for lens %ld - effect will be skipped", lens_idx);
        delete renderer;
        return true;  // Continue without overlay effect (graceful degradation)
    }
    
    m_user_data = renderer;
    m_current_lens = lens_idx;
    
    SYNCDBG(7, "Overlay effect ready");
    return true;
}

void OverlayEffect::Cleanup()
{
    if (m_current_lens >= 0)
    {
        if (m_user_data != NULL)
        {
            delete static_cast<COverlayRenderer*>(m_user_data);
            m_user_data = NULL;
        }
        m_current_lens = -1;
        SYNCDBG(9, "Overlay effect cleaned up");
    }
}

TbBool OverlayEffect::Draw(LensRenderContext* ctx)
{
    if (m_current_lens < 0 || m_user_data == NULL)
    {
        return false;
    }
    
    SYNCDBG(16, "Drawing overlay effect");
    
    COverlayRenderer* renderer = static_cast<COverlayRenderer*>(m_user_data);
    
    // Overlay reads from source (3D view) and composites with overlay sprite to destination
    // srcbuf: NOT pre-offset, calculate viewport position
    unsigned char* viewport_src = ctx->srcbuf + (ctx->viewport_y * ctx->srcpitch) + ctx->viewport_x;
    // dstbuf: ALREADY pre-offset per buffer contract, use directly
    
    renderer->Render(ctx->dstbuf, ctx->dstpitch, viewport_src, ctx->srcpitch,
                    ctx->width, ctx->height);
    
    ctx->buffer_copied = true;  // We wrote the full frame to dstbuf
    return true;
}

/******************************************************************************/
