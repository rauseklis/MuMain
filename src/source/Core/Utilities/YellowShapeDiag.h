#pragma once

// TEMPORARY diagnostic for the "yellow shape" bug (a yellow mass on weapons and wings while attacking). Ctrl+Shift+F1 to
// F6 switch one drawing path off each, to find out which one draws the shape. Nothing is saved; every switch starts on.
// Remove this file and its few call sites when the bug is fixed.
namespace Diag::YellowShape
{
    // Polls the keys, and says in the system log what is switched off. Called once per frame.
    void Update();

    bool WeaponBlurCreationOff();  // Ctrl+Shift+F1: no new weapon trails
    bool BlurRenderOff();          // Ctrl+Shift+F2: weapon trails are not drawn
    bool EffectsOff();             // Ctrl+Shift+F3: effect models (RenderEffects)
    bool JointsOff();              // Ctrl+Shift+F4: joints/ribbons (RenderJoints)
    bool GpuSkinningOff();         // Ctrl+Shift+F5: skin models on the CPU instead of the GPU
    bool SpritesAndParticlesOff(); // Ctrl+Shift+F6: sprites and particles
}
