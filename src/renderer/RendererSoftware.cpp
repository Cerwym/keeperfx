/******************************************************************************/
// Dungeon Keeper - Renderer Abstraction Layer
/******************************************************************************/
/** @file RendererSoftware.cpp
 *     Software passthrough renderer backend implementation.
 * @par Purpose:
 *     Wraps the original SDL2-based rendering path so it satisfies the
 *     IRenderer interface. Behaviour is byte-for-byte identical to the
 *     pre-abstraction code path.
 */
/******************************************************************************/
#include "pre_inc.h"
#include "renderer/RendererSoftware.h"

#include "bflib_video.h"
#include "bflib_vidsurface.h"

#include <SDL2/SDL.h>
#include "post_inc.h"

/******************************************************************************/

bool RendererSoftware::Init()
{
    // Obtain the window surface. This must happen here (not in LbScreenSetup) so
    // that SDL_GetWindowSurface and SDL_CreateRenderer are never both called on the
    // same window — mixing them causes a GXM crash on Vita.
    lbScreenSurface = SDL_GetWindowSurface(lbWindow);
    if (!lbScreenSurface) {
        ERRORLOG("RendererSoftware: SDL_GetWindowSurface failed: %s", SDL_GetError());
        return false;
    }

    // If the draw surface (8bpp game buffer) and window surface have different
    // BitsPerPixel, SDL_BlitScaled requires matching BPP. Create an intermediate
    // surface in the window's pixel format: convert format first, then scale.
    if (lbDrawSurface->format->BitsPerPixel != lbScreenSurface->format->BitsPerPixel)
    {
        lbScaleSurface = SDL_CreateRGBSurfaceWithFormat(0,
            lbDrawSurface->w, lbDrawSurface->h,
            lbScreenSurface->format->BitsPerPixel, lbScreenSurface->format->format);
        if (!lbScaleSurface) {
            WARNLOG("RendererSoftware: can't create scale surface: %s — direct blit will be attempted", SDL_GetError());
        }
    }

    return true;
}

void RendererSoftware::Shutdown()
{
    if (lbScaleSurface) {
        SDL_FreeSurface(lbScaleSurface);
        lbScaleSurface = NULL;
    }
    // lbScreenSurface is owned by SDL (window surface); do not free it.
    lbScreenSurface = NULL;
}

bool RendererSoftware::BeginFrame()
{
    return true;
}

void RendererSoftware::EndFrame()
{
    // Refresh the window surface pointer each frame (guards against window resize / alt-tab).
    lbScreenSurface = SDL_GetWindowSurface(lbWindow);
    SDL_Rect dst = { 0, 0, lbScreenSurface->w, lbScreenSurface->h };

    if (lbScaleSurface != NULL)
    {
        // Two-step: convert format (e.g. 8bpp palette → window BPP) at game resolution,
        // then scale to the physical window surface. SDL_BlitScaled requires matching BPP.
        if (SDL_BlitSurface(lbDrawSurface, NULL, lbScaleSurface, NULL) < 0)
        {
            ERRORLOG("RendererSoftware::EndFrame format-convert blit failed: %s", SDL_GetError());
            return;
        }
        if (SDL_BlitScaled(lbScaleSurface, NULL, lbScreenSurface, &dst) < 0)
        {
            ERRORLOG("RendererSoftware::EndFrame scale blit failed: %s", SDL_GetError());
            return;
        }
    }
    else if (lbDrawSurface->w != lbScreenSurface->w || lbDrawSurface->h != lbScreenSurface->h)
    {
        if (SDL_BlitScaled(lbDrawSurface, NULL, lbScreenSurface, &dst) < 0)
        {
            ERRORLOG("RendererSoftware::EndFrame blit failed: %s", SDL_GetError());
            return;
        }
    }
    else
    {
        if (SDL_BlitSurface(lbDrawSurface, NULL, lbScreenSurface, NULL) < 0)
        {
            ERRORLOG("RendererSoftware::EndFrame blit failed: %s", SDL_GetError());
            return;
        }
    }

    if (SDL_UpdateWindowSurface(lbWindow) < 0)
    {
        ERRORDBG(11, "RendererSoftware::EndFrame flip failed: %s", SDL_GetError());
    }
}

uint8_t* RendererSoftware::LockFramebuffer(int* out_pitch)
{
    if (SDL_LockSurface(lbDrawSurface) < 0)
        return nullptr;

    if (out_pitch)
        *out_pitch = lbDrawSurface->pitch;

    return static_cast<uint8_t*>(lbDrawSurface->pixels);
}

void RendererSoftware::UnlockFramebuffer()
{
    SDL_UnlockSurface(lbDrawSurface);
}

const char* RendererSoftware::GetName() const
{
    return "Software";
}

bool RendererSoftware::SupportsRuntimeSwitch() const
{
    return true;
}
