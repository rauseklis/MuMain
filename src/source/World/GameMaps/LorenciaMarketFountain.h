// LorenciaMarketFountain.h: the animated water of the Loren Market fountain shown in Lorencia.
//////////////////////////////////////////////////////////////////////
#pragma once

class OBJECT;
class BMD;

// The fountain and lamp models are the Loren Market's, loaded into free Lorencia model slots. The Market's own
// map code animates them, but it only runs in the Market, so Lorencia repeats that behaviour here.
namespace World::Lorencia::MarketFountain
{
// Slows the water surface, hides the invisible water emitters and lights the lamps.
void Move(OBJECT* o);

// Draws the base (stone, water surfaces and spray) and the lamps. Returns true when it drew the object.
bool RenderObjectMesh(OBJECT* o, BMD* b);

// Spawns the falling-water particles of the emitters and the lamp flares.
void RenderVisual(OBJECT* o);
} // namespace World::Lorencia::MarketFountain
