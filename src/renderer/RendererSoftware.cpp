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
    // SDL2 and the draw surface are already set up by LbScreenSetup().
    // Nothing extra to initialise for the software backend.
    return true;
}

void RendererSoftware::Shutdown()
{
    // SDL cleanup is handled by LbScreenReset() / SDL_Quit().
}

bool RendererSoftware::BeginFrame()
{
    return true;
}

void RendererSoftware::EndFrame()
{
    // Replicate the blit+present part of the original LbScreenSwap():
    // 1. If we have a secondary draw surface, blit it onto the window surface.
    // 2. Update the window surface to display it on screen.
    if (lbHasSecondSurface)
    {
        // Refresh the window surface pointer each frame (guards against alt-tab).
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
        else
        {
            if (SDL_BlitScaled(lbDrawSurface, NULL, lbScreenSurface, &dst) < 0)
            {
                ERRORLOG("RendererSoftware::EndFrame blit failed: %s", SDL_GetError());
                return;
            }
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
