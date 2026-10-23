// LorenciaMarketFountain.h: the animated water of the Loren Market fountain shown in Lorencia.
//////////////////////////////////////////////////////////////////////
#pragma once

class OBJECT;
class BMD;

// The fountain's models are the Loren Market's, loaded into free Lorencia model slots. The Market's own
// map code animates them, but it only runs in the Market, so Lorencia repeats that behaviour here.
namespace World::Lorencia::MarketFountain
{
// Hides the invisible water emitters and slows the water surface.
void Move(OBJECT* o);

// Draws the base: its stone, its water surfaces and the spray. Returns true for the base.
bool RenderObjectMesh(OBJECT* o, BMD* b);

// Spawns the falling-water particles of the two emitter types.
void RenderVisual(OBJECT* o);
} // namespace World::Lorencia::MarketFountain
