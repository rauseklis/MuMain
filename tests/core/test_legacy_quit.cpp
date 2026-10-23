#include "stdafx.h"
#include "doctest.h"
#include "App/Platform/Windows/Winmain.h"
#include "App/Platform/Windows/LegacyWindowMessage.h"
#include "Network/Reconnect/ReconnectManager.h"

#ifdef _WIN32
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

TEST_CASE("Offlevel close request exits without automatic login [window][offlevel]")
{
    auto& reconnect = ReconnectManager::Instance();
    reconnect.CacheServer(L"127.0.0.1", 1);
    reconnect.CacheCredentials(L"fixture", L"fixture");
    reconnect.CacheCharacter(L"fixture");
    REQUIRE(reconnect.HasSession());
    const bool previousDestroy = Destroy;
    Destroy = false;

    CHECK(App::Platform::ShouldForwardLegacyWindowMessage(WM_DESTROY, WM_USER + 1000));
    SUBCASE("connection has not started reconnecting") {}
    SUBCASE("connection is already waiting to reconnect")
    {
        reconnect.RequestBegin();
        REQUIRE(reconnect.IsActive());
    }

    WndProc(nullptr, WM_DESTROY, 0, 0);

    CHECK(Destroy);
    CHECK_FALSE(reconnect.HasSession());
    reconnect.RequestBegin();
    CHECK_FALSE(reconnect.IsActive());

    Destroy = previousDestroy;
    reconnect.ClearSession();
}
#endif
