#include "stdafx.h"
#include "doctest.h"
#include "MUHelper/MuHelper.h"
#include "Engine/Object/ZzzInterface.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Core/Utilities/Log/MuLogger.h"
#include "Scenes/SceneCore.h"
#include "Network/Server/WSclient.h"

extern int Attacking;

namespace
{
class HelperScope
{
public:
    HelperScope() : previousHero(Hero), previousAttacking(Attacking), previousScene(SceneFlag), previousSocket(SocketClient)
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
    }

    CHARACTER character{};
    MUHelper::CMuHelper helper{};

private:
    CHARACTER* previousHero;
    int previousAttacking;
    EGameScene previousScene;
    decltype(SocketClient) previousSocket;
};
}

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
