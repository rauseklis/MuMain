# Jewel Bank — handover (2026-10-20, updated 2026-10-21)

Design: [`docs/superpowers/specs/2026-10-20-jewel-bank-design.md`](superpowers/specs/2026-10-20-jewel-bank-design.md)
(committed on `codex/auction-tabs-centered`, not on this branch). The window layout was redesigned after the spec (see below).

The MSPRO S6E19 backup was inspiration only; nothing of its schema is used.

## Where the work is

| Repo | Worktree | Branch | State |
| --- | --- | --- | --- |
| Server (OpenMU, C#) | `server/.worktrees/jewel-bank` | `feature/jewel-bank` (from `origin/master`) | committed, **not pushed** |
| Client (MuMain, C++) | `client/.worktrees/jewel-bank` | `feature/jewel-bank` (from `origin/main`) | committed, **not pushed** |

## What it does

- Per-account store of the 10 jewels (Chaos, Bless, Soul, Life, Creation, Guardian, Harmony, Gemstone, Lower and Higher
  Refining Stone) and the 5 stat fruits (one count per fruit type, the fruit's item level).
- Window on **Ctrl+J** (plain **J** still toggles the chat-command list). Two tabs, **Deposit** and **Withdraw**. Every tab lists
  all 15 kinds with a coloured square, **In bank** and **In bag**; the column the tab takes from is in gold.
  - Tick a row to move everything available of that kind, or click its amount cell and type a number (Enter accepts).
  - Any combination of rows moves with one button; **Select all** and **Clear** help. There is no "loading" text anywhere: the
    counts are asked for every 1.5 s while the window is open, so they follow pickups and trades.
  - The auto-deposit tick sits on the Deposit tab. The window closes with the frame's own round X (top right corner) or Esc.
- Auto-deposit: a picked-up plain jewel or fruit goes straight into the bank, for MU Helper (online and offline) and manual
  pickups alike, even with a full backpack. Default off, per account.
- Hand deposit/withdraw only in a safe zone with nothing else open (same rule as the Auction House). Auto-deposit works anywhere.
- Bundles (10/20/30 jewels) are deposited whole, so a request that is not a sum of the bundles carried moves a little less; the
  window then says "Deposited X of Y".

## Server

- Tables `data."JewelBankBalance" / "JewelBankSettings" / "JewelBankMovement" / "JewelBankOperation"`, created by the raw-SQL
  migration `20261020120000_AddJewelBank` (deliberately not in the EF model; the repository uses plain SQL).
- `Persistence/JewelBank/*` (kinds, planner, interface), `Persistence/EntityFramework/JewelBank/JewelBankRepository.cs`,
  `GameLogic/JewelBank/*` (service, pickup hook, inventory counter, options, rate limiter), `GameServer/MessageHandler/JewelBank/*`,
  `GameServer/RemoteView/JewelBank/JewelBankPickupPlugIn.cs`.
- Pickup hook: `PickupItemAction` → `JewelBankPickup.TryDepositAsync` → `DroppedItem.TryPickUpIntoBankAsync` (claims the drop and
  credits the bank under the drop's lock; a refused credit leaves the drop untouched).
- Packets: group `0xD6`. Requests are **encrypted C3** packets (00 open, 01 deposit, 02 withdraw, 03 set auto-deposit); deposit and
  withdraw carry an amount for each of the 15 kinds. Responses are C1: 80 state (counts of the bank **and** of the inventory),
  81 operation result, 82 pickup notification. See `docs/Packets/*D6*.md`.
- Safety: every mutation is one transaction that first claims `(AccountId, OperationId)`; balances are guarded (`>= 0`, per-kind
  cap `JewelBankOptions.MaximumPerKind` = 1,000,000); every movement is in the ledger; only items of durability exactly 1,
  without options or sockets, are banked; a multi-kind withdrawal is all-or-nothing.

## Client

- `UI/NewUI/JewelBank/` — `JewelBankModel` (pure logic, tested) and `NewUIJewelBankWindow`; registered as `INTERFACE_JEWEL_BANK`.
- The window borrows the Auction House frame textures and skins (they are loaded at startup by the Auction window).
- Packet wrappers under `src/source/Dotnet/` and `ClientLibrary/` are regenerated from `MUnique.OpenMU.Network.Packets` **0.9.18**.

## Building and testing

Server (needs a PostgreSQL copy; never the live database):

```
dotnet build src/Persistence/SourceGenerator/MUnique.OpenMU.Persistence.SourceGenerator.csproj
dotnet test tests/MUnique.OpenMU.Tests/MUnique.OpenMU.Tests.csproj --filter "FullyQualifiedName~JewelBank"
```
Set `AUCTION_TEST_CONNECTION_STRING` to a throwaway database that already has the Jewel Bank tables to include the PostgreSQL
tests (otherwise they are skipped).

Packet package (0.9.18 is packed into `C:\Users\rause\.local-nuget\auction-house`): rebuild with `-t:Rebuild`, then
`dotnet pack --no-build -p:Version=0.9.18 -p:PackageVersion=0.9.18` for `PlugIns`, `Network`, `Network.Packets`.

Client: configure with `-DOPENMU_PACKETS_VERSION=0.9.18`. If the C++ send wrappers lack the new signatures, touch the cached
`contentFiles/**/*.xml` under `.nuget/munique.openmu.network.packets/0.9.18` and run `dotnet build ClientLibrary/... -c Release
-p:Platform=x86` once; the generator only runs when its inputs are newer than its outputs.
Unit tests: configure with `-DBUILD_TESTING=ON`, build `jewel_bank_model_tests`, run the exe (ctest does not list it).

### Running a second test server next to the real one (pitfalls found the hard way)

1. Copy a database (`CREATE DATABASE x TEMPLATE y`), shift the ports in it (`ConnectServerDefinition.ClientListenerPort`,
   `GameServerEndpoint.NetworkPort`, `ChatServerEndpoint.NetworkPort`), and start the server with `ASPNETCORE_URLS` on another port.
2. **The client ignores `/p`**: it reads `ServerIP` and `ServerPort` from its own `config.ini` (default 44405). Point `ServerPort` at
   the new connect port, and use the **Open Source client** definition (`…06`, here 44506), not the GMO one (`…05`).
   Check with `Get-NetTCPConnection -OwningProcess <Main pid>` that the game connection goes to your test server's port.
3. The connect server drops idle clients after 60 s, so log in within a minute of the server list appearing.
4. The server console and log file show nothing about players, so "no log line" proves nothing; check effects in the database.

## Verified / not verified

Verified in the real game (client built from this branch, test server on a copy of the database, account `test0`):
the migration creates the tables at server start; Ctrl+J opens the window; counts for the bank and the bag show and follow
changes; Deposit of everything, Withdraw of a mixed selection with a typed amount, tab switching, the auto-deposit tick (stored
in the database), the frame's close X; the ledger equals the balances afterwards. Automated: server suite (58 Jewel Bank tests incl.
PostgreSQL ones, packet layout pins, whole suite earlier), client model tests (12 cases).

**Not verified live: the auto-deposit pickup.** The server refuses to let a player drop jewels ("You are not allowed to drop this
expensive item"), so a pickup could not be staged by hand. Its claim logic is unit tested (`JewelBankPickupTests`), but the path
through MU Helper / a real monster drop has not been seen working. First check: tick auto-deposit, let a helper pick up jewels,
watch the chat line "+1 Jewel of Bless -> Jewel Bank (n)".

## Known gaps / follow-ups

- Rows use coloured squares instead of item icons; no dedicated HD texture set yet.
- Texts are English literals (the client's I18N tables were not extended).
- The cap and the request interval are constants in `JewelBankOptions`, not an `.ini`.
- A change made on another game server process can take up to 30 s to apply to pickups (auto-deposit cache).
- Nothing is pushed or merged; both branches wait for review.

## Update 2026-10-21: bundles, pages and the pickup test

- **45 kinds.** Codes 1-10 jewels, 11-15 fruits, 16-45 bundles (10 jewels x sizes 10/20/30, in jewel order; item group 12, level 0/1/2). A kind is exactly one item definition at one item level; a bundle is banked and withdrawn as itself, never merged into or split from loose jewels. Server: `JewelBankKinds`, repository (deposit removes N items, withdraw creates N, all-or-nothing); withdrawal cap 256 items per request.
- **Wire.** Deposit/Withdraw requests are C3, `OperationId[16]` + `Amounts` binary (45 x u32 LE), length 200. The state response is a C2 packet (367 bytes): `Counts` and `Inventory` binary (45 x u32 LE) at 7 and 187. The client reads the 0xD6 sub-code at `[4]` for C2. Packet package 0.9.19 (`OpenMuPacketsVersion` in `ClientLibrary`).
- **Window.** Three pages of 15 equal rows: jewels and fruits; bundles of the first five jewels; bundles of the last five (each jewel's x10/x20/x30 together). `< >` pager under the list uses the storage window's HD arrow textures (`newui_vault_arrow_L/R`). Bundle icons use their jewel's fill so a jewel keeps its size. The Auction House and the Jewel Bank close each other (`NewUIHotKey.cpp`).
- **Tried and dropped:** a page rail on the right, a grid (row per jewel, column per form), a dual bank/inventory cell. The user wanted the plain list with In bank / In inventory / amount columns on every page.
- **Testing a pickup deposit.** There is no jewel drop event (the Golden Invasion drops Box of Luck and cannot be started by command). Use a GM character (status 32; on the test database `testgm` / `testgm2` characters) and type `/item 14 13 0` (Bless), `/item 12 15 0` (Chaos), `/item 13 15 3` (Strength fruit), `/item 12 30 2` (Bless Bundle x30) in Lorencia with the Deposit tab's auto-deposit tick on, then pick the item up. Not yet verified live.
