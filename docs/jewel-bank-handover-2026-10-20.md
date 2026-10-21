# Jewel Bank — handover (final state, 2026-10-21)

Start here if you pick this feature up. Design (as built):
[`docs/superpowers/specs/2026-10-20-jewel-bank-design.md`](superpowers/specs/2026-10-20-jewel-bank-design.md). The related
scheduled drop event is documented in `docs/superpowers/specs/2026-10-21-jewel-rain-event-design.md`.

## State

Finished, merged and used in the real game. Nothing is pending.

| Repo | Merged as |
| --- | --- |
| Server (`rauseklis/OpenMU`, base `master`) | #6 Jewel Bank, #7 Jewel Rain event |
| Client (`rauseklis/MuMain`, base `main`) | #15 window, #16 spec, #17 Jewel Rain docs, #18 full-inventory pickup |

The MSPRO S6E19 backup that inspired it was inspiration only; nothing of its schema is used.

## What it does

- A per-account bank with **45 kinds**, each its own count: the 10 jewels (Chaos, Bless, Soul, Life, Creation, Guardian,
  Harmony, Gemstone, Lower and Higher Refining Stone), the 5 stat fruits, and the bundles (packs) of 10, 20 and 30 of each of
  the 10 jewels. A bundle is banked and withdrawn as a bundle; it never turns into loose jewels.
- Window on **Ctrl+J** (plain **J** still toggles the chat-command list): **Deposit** and **Withdraw** tabs, three pages of 15
  rows (jewels and fruits; bundles of the first five jewels; bundles of the last five) with a `< >` pager, each row showing the
  3D item model, what the bank holds, what you carry, and an amount (click to select all, click the amount to type).
- **Auto-deposit** (round tick on the Deposit tab, per account, default off): a picked-up plain jewel, fruit or bundle goes
  straight into the bank, manual pickup and MU Helper alike, even with a full inventory. The chat shows the normal blue line,
  "<item> Obtained and moved to the Jewel Bank".
- Cap **25,000 per kind** (`JewelBankOptions.MaximumPerKind`, a constant), at most 256 items per withdrawal, hand deposit and
  withdraw only in a safe zone, only plain items (durability 1, no options, no sockets).

## Where the code is

Server:
- `Persistence/JewelBank/` (kinds catalogue, repository interface), `Persistence/EntityFramework/JewelBank/JewelBankRepository.cs`
  (plain SQL), migration `Persistence/EntityFramework/Migrations/20261020120000_AddJewelBank.cs` (four raw-SQL tables, not in the EF
  model, created at server start).
- `GameLogic/JewelBank/` (service, pickup hook, inventory counter, options, rate limiter), `GameServer/MessageHandler/JewelBank/`,
  `GameServer/RemoteView/JewelBank/`. The pickup hook is `PickupItemAction` → `JewelBankPickup.TryDepositAsync`.
- Packet definitions in `src/Network/Packets/**/*Packets.xml` (head `0xD6`); generated docs in `docs/Packets/*D6*.md`.
- Tests in `tests/MUnique.OpenMU.Tests/JewelBank/` and `tests/MUnique.OpenMU.Network.Packets.Tests/JewelBankPacketDefinitionTests.cs`.

Client: `src/source/UI/NewUI/JewelBank/` (`JewelBankModel` is pure logic with tests in `tests/ui/test_jewel_bank_model.cpp`;
`NewUIJewelBankWindow` is the window), the hotkey in `NewUIHotKey.cpp`, the `0xD6` dispatch and the world-entry state request in
`Network/Server/WSclient.cpp`, the pickup check in `Engine/Object/ZzzInterface.cpp` (`MOVEMENT_GET`). The window borrows the Auction House
frame textures and the storage window's HD arrow buttons (both are loaded at startup).

## Protocol (what a change must keep)

- Requests are SimpleModulus-encrypted: **open (00) and set auto-deposit (03) are C3; deposit (01) and withdraw (02) are C4**
  (201 bytes: `OperationId[16]` at 5, `Amounts` at 21 = 45 little-endian 32-bit counts by kind code). Encryption turns 8 bytes into
  11, so a C3 packet longer than about 180 bytes encrypts past 255 bytes, the server rejects it ("unexpected content size") and
  drops the connection. A test pins that every C3 request fits; use C4 for anything bigger.
- Responses: 80 state (**C2**, 367 bytes: result, auto-deposit, 45 bank counts at 7, 45 inventory counts at 187), 81 operation
  result (C1), 82 pickup notification (C1). A C2/C4 packet has its sub-code at byte 4, a C1/C3 at byte 3.
- Kind codes: 1-10 jewels, 11-15 fruits, 16-45 bundles (sizes 10, 20, 30, each in jewel order). Codes are never reused.

## Building and testing

The client takes its packets from the NuGet package `MUnique.OpenMU.Network.Packets` (now **0.9.20**,
`OpenMuPacketsVersion` in `ClientLibrary/MUnique.Client.Library.csproj`), resolved from the local feed in `nuget.config`
(`C:\Users\<user>\.local-nuget\auction-house`). The server's project files say 0.9.10, so pack with an override, from the server
repo (`-t:Rebuild` first so the XML changes are included):

```
dotnet build src/PlugIns/MUnique.OpenMU.PlugIns.csproj -c Release -p:Version=0.9.20 -p:PackageVersion=0.9.20 -t:Rebuild
dotnet pack  src/PlugIns/MUnique.OpenMU.PlugIns.csproj -c Release --no-build -p:Version=0.9.20 -p:PackageVersion=0.9.20 -o <feed folder>
```
(the same two commands for `src/Network/MUnique.OpenMU.Network.csproj` and `src/Network/Packets/MUnique.OpenMU.Network.Packets.csproj`).
When the packet XML changes: bump the version, pack, set `OpenMuPacketsVersion` and `-DOPENMU_PACKETS_VERSION`, and run
`dotnet build ClientLibrary/MUnique.Client.Library.csproj -c Release -p:Platform=x86` once so the C++ wrappers regenerate (the
generator only runs when its inputs are newer than its outputs; a stale `.nuget` copy of the old version can hide changes).

Server tests:
```
dotnet test tests/MUnique.OpenMU.Tests/MUnique.OpenMU.Tests.csproj --filter "FullyQualifiedName~JewelBank"
dotnet test tests/MUnique.OpenMU.Network.Packets.Tests --filter "FullyQualifiedName~JewelBank"
```
Set `AUCTION_TEST_CONNECTION_STRING` to a **throwaway** PostgreSQL database that already has the Jewel Bank tables to include the
repository tests (otherwise they are skipped). Never point tests at a live database.

Client: build with `-DBUILD_TESTING=OFF` for the game (with testing on, the compiler can run out of heap on the big files). For the
model tests configure with `-DBUILD_TESTING=ON`, build the target `jewel_bank_model_tests` and run the exe directly (`ctest` does not
list it).

## Trying it in the game

- Use a throwaway copy of the database and a second server next to the real one: copy a database (`CREATE DATABASE x TEMPLATE y`),
  shift the ports in it (`ConnectServerDefinition.ClientListenerPort`, `GameServerEndpoint.NetworkPort`,
  `ChatServerEndpoint.NetworkPort`), and start the server with `ASPNETCORE_URLS` on another port. The migration creates the tables at start.
- **The client ignores `/p`**: it reads `ServerIP` and `ServerPort` from its own `config.ini`. Point `ServerPort` at the new connect
  port, and use the **Open Source client** definition (`…06`), not the GMO one (`…05`); the connect server drops idle clients after 60 s.
- The server console shows nothing about players; check effects in the database (`data."JewelBankBalance"`, `"JewelBankMovement"`).
- Staging a pickup: log in with a GM character (character status 32) and type `/item 14 13 0` (Jewel of Bless), `/item 12 15 0` (Chaos),
  `/item 13 15 3` (Fruit of Strength; the level is the fruit type 0-4), `/item 12 30 2` (Bless Bundle x30; level 0/1/2 = size 10/20/30)
  next to your character, then pick the item up with the Deposit tab's tick on. The Jewel Rain event (`/jewelrain`, see its spec) drops
  jewels over a town and exercises the same path.
- Players cannot drop jewels themselves (the server refuses), which is why the GM command is used.

## Verified

In the real game: Ctrl+J, both tabs, deposit and withdraw of jewels, fruits and bundles (also several kinds at once), the three pages
and pager, the auto-deposit tick, several auto-deposited pickups in a row (Jewel Rain), a pickup with a full inventory, the blue
pickup line, and the Auction House / Jewel Bank closing each other. Automated: 47 server Jewel Bank tests (incl. PostgreSQL), 16 packet
definition tests, 16 client model test cases.

## Known gaps

- Texts are English literals (the client's I18N tables were not extended).
- The cap and the request interval are constants, not settings.
- A change made on another game server process can take up to 30 s to apply to pickups (the auto-deposit setting is cached).
- Auto-deposit for the offline helper has not been verified.
- The client guesses whether a pickup would be banked (same rules, 25,000 cap mirrored in `JewelBankModel`); if the guess is wrong the
  server's answer wins and the player just sees the normal inventory-full behaviour.

## Pitfalls found along the way

- Large shell heredocs with quotes broke in the build environment; write files with an editor tool and patch with scripts that keep
  the file's existing line endings (the repo mixes LF and CRLF).
- A banked pickup is answered by the 0xD6/82 notification, not the usual item reply (0x22); the client clears its "pickup in
  flight" flag there, otherwise no second item could be picked up.
- Window layouts that were tried and dropped after review: a page rail on the right, a grid (row per jewel, column per form), and
  a cell with both numbers. The plain list with In bank / In inventory / amount on equal pages is what was approved.
