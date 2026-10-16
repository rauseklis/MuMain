#include "stdafx.h"
#include "Network/Server/BuffDetailsProtocol.h"

#include "Dotnet/Connection.h"
#include "Network/Server/WSclient.h"
#include "GameLogic/Buffs/BuffDetails.h"

namespace Network::Buffs
{
    namespace
    {
        constexpr std::uint64_t RefreshInterval = 1000;
        std::uint64_t lastRequest = 0;
    }

    void RequestDetails()
    {
        const auto now = GameLogic::Buffs::BuffDetailsClock();
        if (!SocketClient || !SocketClient->IsConnected() || (lastRequest != 0 && now - lastRequest < RefreshInterval))
        {
            return;
        }
        const BYTE request[] = {0xC1, 4, GameLogic::Buffs::BuffDetails::PacketCode, GameLogic::Buffs::BuffDetails::ProtocolVersion};
        SocketClient->Send(request, sizeof(request));
        lastRequest = now;
    }

    void ResetDetails()
    {
        GameLogic::Buffs::ActiveBuffDetails().Clear();
        lastRequest = 0;
    }
}
