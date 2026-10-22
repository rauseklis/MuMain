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

    // Ctrl+Shift+Z, X, C, V: GPU skinning off only for plain textured (0), flat colour (1), bright/additive (2) or
    // environment-mapped (3) meshes.
    bool GpuSkinningOffForMaterial(int material);

    // Ctrl+Shift+B: steps through the models drawn with GPU skinning and switches the GPU skinning off for one model at
    // a time (the system log names it), to find the model which is drawn wrong.
    void NoteGpuSkinnedModel(const void* model, const char* name, int modelIndex);
    bool GpuSkinningOffForModel(const void* model);
}
