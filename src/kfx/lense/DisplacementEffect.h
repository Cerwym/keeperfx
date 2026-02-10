/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file DisplacementEffect.h
 *     Displacement/warping lens effect.
 * @par Purpose:
 *     Image warping/distortion effect implementation.
 * @par Comment:
 *     Uses displacement maps to create warping effects.
 * @author   Peter Lockett, KeeperFX Team
 * @date     09 Feb 2026
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#ifndef KFX_DISPLACEMENTEFFECT_H
#define KFX_DISPLACEMENTEFFECT_H

#include "LensEffect.h"

/******************************************************************************/

enum DisplacementAlgorithm {
    DisplaceAlgo_Linear = 0,
    DisplaceAlgo_Sinusoidal = 1,
    DisplaceAlgo_Radial = 2,
    DisplaceAlgo_Compound = 3  // Compound eye/hexagonal tiling
};

class DisplacementEffect : public LensEffect {
public:
    DisplacementEffect();
    virtual ~DisplacementEffect();
    
    virtual TbBool Setup(long lens_idx) override;
    virtual void Cleanup() override;
    virtual TbBool Draw(LensRenderContext* ctx) override;
    
private:
    long m_current_lens;
    DisplacementAlgorithm m_algorithm;
    int m_magnitude;
    int m_period;
};

/******************************************************************************/
#endif
