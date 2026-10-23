#include "stdafx.h"
#include "doctest.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Core/Time/FrameTiming.h"
#include "Engine/Object/w_ObjectInfo.h"
#include "Engine/Object/w_CharacterInfo.h"

extern void FallingMonster(CHARACTER* character, OBJECT* object);

namespace
{
class FrameFactorScope
{
public:
    explicit FrameFactorScope(float factor) : previous(FPS_ANIMATION_FACTOR)
    {
        FPS_ANIMATION_FACTOR = factor;
    }
    ~FrameFactorScope()
    {
        FPS_ANIMATION_FACTOR = previous;
    }

private:
    float previous;
};
} // namespace

TEST_CASE("Falling monsters accelerate and turn at the same rate across FPS [physics]")
{
    constexpr float referenceFps = 25.0f;
    for (const int fps : {25, 60, 144, 240})
    {
        INFO("FPS: ", fps);
        FrameFactorScope timing(referenceFps / static_cast<float>(fps));
        OBJECT object{};
        object.Direction[0] = 1.0f;
        object.Angle[2] = 37.0f;
        for (int frame = 0; frame < fps; ++frame)
        {
            FallingMonster(nullptr, &object);
        }
        CHECK(object.Gravity == doctest::Approx(62.5f));
        CHECK(object.Direction[1] == doctest::Approx(25.0f));
        CHECK(object.Angle[0] == doctest::Approx(-90.0f));
        CHECK(object.Angle[2] == doctest::Approx(37.0f));
        // Discrete acceleration retains the original 25 Hz integration. Smaller
        // render steps converge within one reference step of the same fall.
        CHECK(object.Position[2] == doctest::Approx(-812.5f).epsilon(0.05));
    }
}

TEST_CASE("A falling monster preserves its heading before gravity becomes positive [physics]")
{
    FrameFactorScope timing(0.1f);
    OBJECT object{};
    object.Gravity = -5.0f;
    object.Angle[2] = 37.0f;
    FallingMonster(nullptr, &object);
    CHECK(object.Angle[2] == doctest::Approx(37.0f));
}
