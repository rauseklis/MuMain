#include "stdafx.h"
#include "doctest.h"
#include "MUHelper/MuHelper.h"
#include "Engine/Object/ZzzInterface.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Engine/Object/w_ObjectInfo.h"
#include "Engine/Object/w_CharacterInfo.h"
#include "Core/Utilities/Log/MuLogger.h"
#include "Scenes/SceneCore.h"
#include "Network/Server/WSclient.h"
#include "Render/Models/ZzzBMD.h"

#include <memory>

extern int Attacking;
extern MovementSkill g_MovementSkill;

namespace
{
class HelperScope
{
public:
    HelperScope()
        : previousHero(Hero), previousAttacking(Attacking), previousScene(SceneFlag), previousSocket(SocketClient),
          previousActionTarget(ActionTarget), previousMovementSkill(g_MovementSkill), previousModels(Models)
    {
        mu::log::Init();
        Hero = &character;
        SceneFlag = MAIN_SCENE;
        SocketClient = nullptr;
        Models = models.get();
        Models[MODEL_PLAYER].NumActions = PLAYER_DIE2 + 1;
        character.Object.Type = MODEL_PLAYER;
        character.Wing.Type = -1;
        character.Helper.Type = -1;
        character.Weapon[0].Type = -1;
        character.Weapon[1].Type = -1;
    }

    ~HelperScope()
    {
        helper.Stop();
        Hero = previousHero;
        Attacking = previousAttacking;
        SceneFlag = previousScene;
        SocketClient = previousSocket;
        ActionTarget = previousActionTarget;
        g_MovementSkill = previousMovementSkill;
        Models = previousModels;
    }

    CHARACTER character{};
    MUHelper::CMuHelper helper{};

private:
    CHARACTER* previousHero;
    int previousAttacking;
    EGameScene previousScene;
    decltype(SocketClient) previousSocket;
    int previousActionTarget;
    MovementSkill previousMovementSkill;
    BMD* previousModels;
    std::unique_ptr<BMD[]> models = std::make_unique<BMD[]>(MODEL_PLAYER + 1);
};
} // namespace

TEST_CASE("Stopping MU Helper cancels its queued repeating attack [helper]")
{
    HelperScope scope;
    scope.helper.Start();
    REQUIRE(scope.helper.IsActive());
    Attacking = 2;

    scope.helper.Stop();

    CHECK_FALSE(scope.helper.IsActive());
    CHECK(Attacking == -1);
}

TEST_CASE("An inactive MU Helper stop does not cancel manual attacks [helper]")
{
    HelperScope scope;
    Attacking = 2;

    scope.helper.Stop();

    CHECK(Attacking == 2);
}

TEST_CASE("A delayed helper start cannot start attacking while dead [helper]")
{
    HelperScope scope;
    scope.character.Dead = 1;

    scope.helper.Start();

    CHECK_FALSE(scope.helper.IsActive());
}

TEST_CASE("MU Helper cannot start in a safe zone [helper]")
{
    HelperScope scope;
    scope.character.SafeZone = true;

    scope.helper.Start();

    CHECK_FALSE(scope.helper.IsActive());
}

TEST_CASE("MU Helper stop takes effect without a server reply [helper]")
{
    HelperScope scope;
    scope.helper.Start();
    REQUIRE(scope.helper.IsActive());

    scope.helper.TriggerStop();

    CHECK_FALSE(scope.helper.IsActive());
    scope.helper.Start();
    CHECK_FALSE(scope.helper.IsActive());
}

TEST_CASE("Escape immediately cancels helper attacks and walking [helper]")
{
    HelperScope scope;
    scope.helper.Start();
    REQUIRE(scope.helper.IsActive());
    Attacking = 2;
    ActionTarget = 7;
    g_MovementSkill.m_bMagic = true;
    g_MovementSkill.m_iTarget = 7;
    scope.character.Movement = true;
    scope.character.Path.PathNum = 4;
    scope.character.Path.CurrentPath = 2;

    scope.helper.HandleCancelInput(true);

    CHECK_FALSE(scope.helper.IsActive());
    CHECK(Attacking == -1);
    CHECK(ActionTarget == -1);
    CHECK_FALSE(g_MovementSkill.m_bMagic);
    CHECK(g_MovementSkill.m_iTarget == -1);
    CHECK_FALSE(scope.character.Movement);
    CHECK(scope.character.Path.PathNum == 0);
    CHECK(scope.character.Path.CurrentPath == 0);
}

TEST_CASE("Escape while approaching a monster ends the movement animation [helper]")
{
    HelperScope scope;
    scope.helper.Start();
    REQUIRE(scope.helper.IsActive());
    int expectedIdle = PLAYER_STOP_MALE;
    SUBCASE("Running on the ground")
    {
        scope.character.Object.CurrentAction = PLAYER_RUN;
    }
    SUBCASE("Flying with wings")
    {
        scope.character.Wing.Type = MODEL_WINGS_OF_HEAVEN;
        scope.character.Object.CurrentAction = PLAYER_FLY;
        expectedIdle = PLAYER_STOP_FLY;
    }
    scope.character.Object.Position[0] = 4032.0f;
    scope.character.Object.Position[1] = 11822.0f;
    scope.character.Movement = true;
    scope.character.Run = 40.0f;
    scope.character.Path.PathNum = 2;
    scope.character.Path.CurrentPathFloat = 1;
    scope.character.Path.VisualCatchupActive = true;

    scope.helper.HandleCancelInput(true);

    CHECK_FALSE(scope.helper.IsActive());
    CHECK_FALSE(scope.character.Movement);
    CHECK(scope.character.Object.CurrentAction == expectedIdle);
    CHECK(scope.character.Run == 0.0f);
    CHECK(scope.character.Path.CurrentPathFloat == 0);
    CHECK_FALSE(scope.character.Path.VisualCatchupActive);
    CHECK(scope.character.Object.Position[0] == 4032.0f);
    CHECK(scope.character.Object.Position[1] == 11822.0f);
}

TEST_CASE("Stopping the helper preserves a death animation [helper]")
{
    HelperScope scope;
    scope.helper.Start();
    scope.character.Movement = true;
    scope.character.Dead = 1.0f;
    scope.character.Object.CurrentAction = PLAYER_DIE1;

    scope.helper.Stop();

    CHECK_FALSE(scope.character.Movement);
    CHECK(scope.character.Object.CurrentAction == PLAYER_DIE1);
}

TEST_CASE("Stopping an in-place helper attack lets its animation finish [helper]")
{
    HelperScope scope;
    scope.helper.Start();
    scope.character.Movement = false;
    scope.character.Object.CurrentAction = PLAYER_SKILL_HAND1;

    scope.helper.Stop();

    CHECK_FALSE(scope.helper.IsActive());
    CHECK(scope.character.Object.CurrentAction == PLAYER_SKILL_HAND1);
}

TEST_CASE("Other input leaves MU Helper running [helper]")
{
    HelperScope scope;
    scope.helper.Start();
    REQUIRE(scope.helper.IsActive());

    scope.helper.HandleCancelInput(false);

    CHECK(scope.helper.IsActive());
}

TEST_CASE("Escape without an active helper preserves manual actions [helper]")
{
    HelperScope scope;
    Attacking = 2;
    scope.character.Movement = true;

    scope.helper.HandleCancelInput(true);

    CHECK(Attacking == 2);
    CHECK(scope.character.Movement);
}
