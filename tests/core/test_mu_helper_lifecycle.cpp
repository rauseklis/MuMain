#include "stdafx.h"
#include "doctest.h"
#include "MUHelper/MuHelper.h"
#include "Engine/Object/ZzzInterface.h"
#include "Core/Utilities/Log/MuLogger.h"

extern int Attacking;

namespace
{
class HelperScope
{
public:
    HelperScope() : previousHero(Hero), previousAttacking(Attacking)
    {
        mu::log::Init();
        Hero = &character;
    }

    ~HelperScope()
    {
        helper.Stop();
        Hero = previousHero;
        Attacking = previousAttacking;
    }

    CHARACTER character{};
    MUHelper::CMuHelper helper{};

private:
    CHARACTER* previousHero;
    int previousAttacking;
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
