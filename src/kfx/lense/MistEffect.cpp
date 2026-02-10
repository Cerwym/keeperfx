/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file MistEffect.cpp
 *     Mist/fog lens effect implementation.
 * @par Purpose:
 *     Self-contained fog overlay effect with animation.
 * @par Comment:
 *     Mist is an effect of bitmap image moving over original 3D view.
 *     When moving, mist can change directions, but can never go upward.
 *     Mist looks like there was a layer of dirt just behind the eye.
 * @author   Tomasz Lis, KeeperFX Team
 * @date     05 Jan 2009 - 09 Feb 2026
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#include "../../pre_inc.h"
#include "MistEffect.h"

#include <string.h>
#include "../../config_lenses.h"
#include "../../lens_api.h"
#include "../../custom_sprites.h"
#include "../../globals.h"
#include "../../vidmode.h"

#include "../../keeperfx.hpp"
#include "../../post_inc.h"

/******************************************************************************/
// INTERNAL MIST RENDERER CLASS
/******************************************************************************/

/**
 * CMistFade - Internal mist animation and rendering class.
 * Handles the animated layered fog effect with scrolling mist textures.
 */
class CMistFade {
public:
    CMistFade();
    ~CMistFade();
    
    void Setup(unsigned char *lens_mem, unsigned char *fade, unsigned char *ghost,
               unsigned char pos_x_step, unsigned char pos_y_step,
               unsigned char sec_x_step, unsigned char sec_y_step);
    void SetAnimation(long counter, long speed);
    void Render(unsigned char *dstbuf, long dstpitch, 
               unsigned char *srcbuf, long srcpitch,
               long width, long height);
    void Animate();
    
private:
    /** Mist data width and height are the same and equal to this dimension */
    unsigned int lens_dim;
    unsigned char *lens_data;
    unsigned char *fade_data;
    unsigned char *ghost_data;
    unsigned char position_offset_x;
    unsigned char position_offset_y;
    unsigned char secondary_offset_x;
    unsigned char secondary_offset_y;
    long animation_counter;
    long animation_speed;
    unsigned char position_x_step;
    unsigned char position_y_step;
    unsigned char secondary_x_step;
    unsigned char secondary_y_step;
};

CMistFade::CMistFade()
{
    Setup(NULL, NULL, NULL, 2, 1, 253, 3);
}

CMistFade::~CMistFade()
{
}

void CMistFade::Setup(unsigned char *lens_mem, unsigned char *fade, unsigned char *ghost,
                     unsigned char pos_x_step, unsigned char pos_y_step,
                     unsigned char sec_x_step, unsigned char sec_y_step)
{
    this->lens_data = lens_mem;
    this->fade_data = fade;
    this->ghost_data = ghost;
    this->lens_dim = 256;
    this->position_offset_x = 0;
    this->position_offset_y = 0;
    this->secondary_offset_x = 50;
    this->secondary_offset_y = 128;
    this->animation_speed = 1024;
    this->animation_counter = 0;
    this->position_x_step = pos_x_step;
    this->position_y_step = pos_y_step;
    this->secondary_x_step = sec_x_step;
    this->secondary_y_step = sec_y_step;
}

void CMistFade::SetAnimation(long a1, long a2)
{
    this->animation_counter = a1;
    this->animation_speed = a2;
}

void CMistFade::Animate()
{
    this->position_offset_x += this->position_x_step;
    this->position_offset_y += this->position_y_step;
    this->secondary_offset_x -= this->secondary_x_step;
    this->animation_counter += this->animation_speed;
    this->secondary_offset_y += this->secondary_y_step;
}

void CMistFade::Render(unsigned char *dstbuf, long dstpitch,
                      unsigned char *srcbuf, long srcpitch,
                      long width, long height)
{
    if ((lens_data == NULL) || (fade_data == NULL))
    {
        ERRORLOG("Can't draw Mist as it's not initialized!");
        return;
    }
    
    unsigned char *src = srcbuf;
    unsigned char *dst = dstbuf;
    unsigned long primary_offset_x = this->position_offset_x;
    unsigned long primary_offset_y = this->position_offset_y;
    unsigned long local_secondary_offset_x = this->secondary_offset_x;
    unsigned long local_secondary_offset_y = this->secondary_offset_y;
    unsigned long lens_div = width / (2 * lens_dim);
    if (lens_div < 1) lens_div = 1;
    
    for (long h = height; h > 0; h--)
    {
        for (long w = width; w > 0; w--)
        {
            long i = lens_data[(local_secondary_offset_y * lens_dim) + local_secondary_offset_x];
            long k = lens_data[(primary_offset_y * lens_dim) + primary_offset_x];
            long n = (k + i) >> 3;
            if (n > 32)
                n = 32;
            else if (n < 0)
                n = 0;
            *dst = this->fade_data[(n << 8) + *src];
            src++;
            dst++;
            
            if ((w % lens_div) == 0)
            {
                local_secondary_offset_y--;
                local_secondary_offset_y %= lens_dim;
                primary_offset_x++;
                primary_offset_x %= lens_dim;
            }
        }
        
        // Move buffers to end of this line
        dst += (dstpitch - width);
        src += (srcpitch - width);
        
        // Update other counters
        if ((h % lens_div) == 0)
        {
            local_secondary_offset_y += width;
            local_secondary_offset_y %= lens_dim;
            primary_offset_x -= width;
            primary_offset_x %= lens_dim;
            primary_offset_y++;
            primary_offset_y %= lens_dim;
            local_secondary_offset_x--;
            local_secondary_offset_x %= lens_dim;
        }
    }
}

/******************************************************************************/
// MISTEFFECT PUBLIC INTERFACE
/******************************************************************************/

MistEffect::MistEffect()
    : LensEffect(LensEffectType::Mist, "Mist")
    , m_current_lens(-1)
{
}

MistEffect::~MistEffect()
{
    Cleanup();
}

TbBool MistEffect::Setup(long lens_idx)
{
    SYNCDBG(8, "Setting up mist effect for lens %ld", lens_idx);
    
    struct LensConfig* cfg = &lenses_conf.lenses[lens_idx];
    
    // Check if this lens has a mist effect configured
    if ((cfg->flags & LCF_HasMist) == 0)
    {
        SYNCDBG(8, "Lens %ld does not have mist effect configured", lens_idx);
        return true;  // Not an error - effect just not configured
    }
    
    // Load mist texture using base class fallback loader
    if (!LoadMistTexture(cfg->mist_file))
    {
        WARNLOG("Failed to load mist texture '%s' for lens %ld - effect will be skipped", 
                cfg->mist_file, lens_idx);
        return true;  // Continue without mist effect (graceful degradation)
    }
    
    // Setup the mist renderer
    CMistFade* renderer = new CMistFade();
    renderer->Setup((unsigned char*)eye_lens_memory,
                   &pixmap.fade_tables[(cfg->mist_lightness) * 256],
                   &pixmap.ghost[(cfg->mist_ghost) * 256],
                   (unsigned char)cfg->mist_pos_x_step,
                   (unsigned char)cfg->mist_pos_y_step,
                   (unsigned char)cfg->mist_sec_x_step,
                   (unsigned char)cfg->mist_sec_y_step);
    renderer->SetAnimation(0, 1024);
    
    // Store renderer in user data (we'll manage it through the base class)
    m_user_data = renderer;
    m_current_lens = lens_idx;
    
    SYNCDBG(7, "Mist effect ready");
    return true;
}

void MistEffect::Cleanup()
{
    if (m_current_lens >= 0)
    {
        // Free the mist renderer
        if (m_user_data != NULL)
        {
            delete static_cast<CMistFade*>(m_user_data);
            m_user_data = NULL;
        }
        m_current_lens = -1;
        SYNCDBG(9, "Mist effect cleaned up");
    }
}

TbBool MistEffect::Draw(LensRenderContext* ctx)
{
    if (m_current_lens < 0 || m_user_data == NULL)
    {
        return false;
    }
    
    SYNCDBG(16, "Drawing mist effect");
    
    // Get the mist renderer
    CMistFade* renderer = static_cast<CMistFade*>(m_user_data);
    
    // Mist reads from viewport-aligned source
    unsigned char* viewport_src = ctx->srcbuf + ctx->viewport_x;
    
    // Render mist effect
    renderer->Render(ctx->dstbuf, ctx->dstpitch, viewport_src, ctx->srcpitch,
                    ctx->width, ctx->height);
    renderer->Animate();
    
    ctx->buffer_copied = true;  // Mist writes to dstbuf
    return true;
}

TbBool MistEffect::LoadMistTexture(const char* filename)
{
    // Try to load from asset registry first (ZIP files with mists.json)
    const struct LensMistData* mist = get_lens_mist_data(filename);
    
    if (mist != NULL && mist->data != NULL)
    {
        // Copy from registry (mists are always 256x256)
        memcpy(eye_lens_memory, mist->data, 256 * 256);
        SYNCDBG(7, "Loaded mist '%s' from asset registry", filename);
        return true;
    }
    
    // Fall back to loading raw .raw files from mods/data directories
    // This allows simple file-based mods without requiring ZIP/JSON
    const char* loaded_from = NULL;
    if (LoadAssetWithFallback(filename, (unsigned char*)eye_lens_memory,
                              256 * 256, &loaded_from))
    {
        if (loaded_from != NULL) {
            SYNCDBG(7, "Loaded mist '%s' from mod '%s'", filename, loaded_from);
        } else {
            SYNCDBG(7, "Loaded mist '%s' from base game files", filename);
        }
        return true;
    }
    
    // Neither registry nor file loading worked
    WARNLOG("Failed to load mist '%s' from registry or files", filename);
    return false;
}

/******************************************************************************/
