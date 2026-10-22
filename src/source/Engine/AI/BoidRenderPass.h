#pragma once

namespace Engine::AI
{
// Extra effects and shadows consume the model state prepared by drawBody.
// A faded-out body can decline drawing before that state exists.
template <typename DrawBody, typename DrawExtras>
void RenderBoidPass(DrawBody&& drawBody, DrawExtras&& drawExtras)
{
    if (!drawBody())
        return;

    drawExtras();
}
} // namespace Engine::AI
