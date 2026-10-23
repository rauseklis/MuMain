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

extern int Attacking;
extern MovementSkill g_MovementSkill;

namespace
{
class HelperScope
{
public:
    HelperScope()
        : previousHero(Hero), previousAttacking(Attacking), previousScene(SceneFlag), previousSocket(SocketClient),
          previousActionTarget(ActionTarget), previousMovementSkill(g_MovementSkill)
    {
        mu::log::Init();
        Hero = &character;
        SceneFlag = MAIN_SCENE;
        SocketClient = nullptr;
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
