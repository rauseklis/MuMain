# Jewel Bank — handover (2026-10-20)

Design: [`docs/superpowers/specs/2026-10-20-jewel-bank-design.md`](superpowers/specs/2026-10-20-jewel-bank-design.md)
(committed on `codex/auction-tabs-centered`, not on this branch).

The MSPRO S6E19 backup was inspiration only; nothing of its schema is used.

## Where the work is

| Repo | Worktree | Branch | State |
| --- | --- | --- | --- |
| Server (OpenMU, C#) | `server/.worktrees/jewel-bank` | `feature/jewel-bank` (from `origin/master`) | committed, **not pushed** |
| Client (MuMain, C++) | `client/.worktrees/jewel-bank` | `feature/jewel-bank` (from `origin/main`) | committed, **not pushed** |

## What it does

- Per-account store of the 10 jewels (Chaos, Bless, Soul, Life, Creation, Guardian, Harmony, Gemstone, Lower and Higher
  Refining Stone) and the 5 stat fruits (one count per fruit type, the fruit's item level).
- Window on **Ctrl+J** (plain **J** still toggles the chat-command list). Pick a row, type an amount, **Withdraw**; **Deposit**
  one kind or **Deposit All**; **Max** fills the stored count. A tick at the bottom switches **auto-deposit**.
- Auto-deposit: a picked-up plain jewel or fruit goes straight into the bank, for MU Helper (online and offline) and manual
  pickups alike, even with a full backpack. Default off, per account.
- Hand deposit/withdraw only in a safe zone with nothing else open (same rule as the Auction House). Auto-deposit works anywhere.

## Server

- Tables `data."JewelBankBalance" / "JewelBankSettings" / "JewelBankMovement" / "JewelBankOperation"`, created by the raw-SQL
  migration `20261020120000_AddJewelBank` (deliberately not in the EF model; the repository uses plain SQL).
- `Persistence/JewelBank/*` (kinds, interface), `Persistence/EntityFramework/JewelBank/JewelBankRepository.cs`,
  `GameLogic/JewelBank/*` (service, pickup hook, options, rate limiter), `GameServer/MessageHandler/JewelBank/*`,
  `GameServer/RemoteView/JewelBank/JewelBankPickupPlugIn.cs`.
- Pickup hook: `PickupItemAction` → `JewelBankPickup.TryDepositAsync` → `DroppedItem.TryPickUpIntoBankAsync` (claims the drop and
  credits the bank under the drop's lock; a refused credit leaves the drop untouched).
- Packets: group `0xD6`, sub-codes 00–03 client→server, 80–82 server→client (see `docs/Packets/C1-D6-*.md`).
- Safety: every mutation is one transaction that first claims `(AccountId, OperationId)`; balances are guarded (`>= 0`, per-kind
  cap `JewelBankOptions.MaximumPerKind` = 1,000,000); every movement is in the ledger; only items of durability exactly 1,
  without options or sockets, are banked.

## Client

- `UI/NewUI/JewelBank/` — `JewelBankModel` (pure logic, tested) and `NewUIJewelBankWindow`; registered as `INTERFACE_JEWEL_BANK`.
- The window borrows the Auction House frame textures and skins (they are loaded at startup by the Auction window).
- Packet wrappers under `src/source/Dotnet/` and `ClientLibrary/` are regenerated from `MUnique.OpenMU.Network.Packets` **0.9.16**.

## Building and testing

Server (needs a PostgreSQL copy; never the live database):

```
dotnet build src/Persistence/SourceGenerator/MUnique.OpenMU.Persistence.SourceGenerator.csproj
dotnet test tests/MUnique.OpenMU.Tests/MUnique.OpenMU.Tests.csproj --filter "FullyQualifiedName~JewelBank"
```
Set `AUCTION_TEST_CONNECTION_STRING` to a throwaway database that already has the Jewel Bank tables to include the 24
PostgreSQL tests (otherwise they are skipped).

Packet package (0.9.16 was packed into `C:\Users\rause\.local-nuget\auction-house`): rebuild with `-t:Rebuild`, then
`dotnet pack --no-build -p:Version=0.9.16 -p:PackageVersion=0.9.16` for `PlugIns`, `Network`, `Network.Packets`.

Client: configure with `-DOPENMU_PACKETS_VERSION=0.9.16`. If the C++ send wrappers lack `SendJewelBank*`, touch the cached
`contentFiles/**/*.xml` under `.nuget/munique.openmu.network.packets/0.9.16` and run `dotnet build ClientLibrary/... -c Release
-p:Platform=x86` once; the generator only runs when its inputs are newer than its outputs.
Unit tests: configure with `-DBUILD_TESTING=ON`, build `jewel_bank_model_tests`, run the exe (ctest does not list it).

## Verified / not verified

Verified: server unit and PostgreSQL tests (40 + 3 + 48 packet tests), whole server suite, client builds in Release, client model tests.
**Not verified: the real game.** Nobody has started the server with the migration and logged in with this client. First checks:
1. Start the server on a copy of the database; accept the schema update (applies `AddJewelBank`).
2. Log in, press Ctrl+J in Lorencia: the window should open and list 15 zero counts.
3. Drop jewels, **Deposit All**, check counts and the inventory; **Withdraw** a few; try with a full inventory.
4. Tick auto-deposit, switch the MU Helper to pick jewels, and watch the chat line and the counts.

## Known gaps / follow-ups

- Rows use a coloured marker instead of item icons; no dedicated HD texture set yet.
- Texts are English literals (the client's I18N tables were not extended).
- The cap and the request interval are constants in `JewelBankOptions`, not an `.ini`.
- `JewelBankOptions.MaximumPerKind` and the 30 s auto-deposit cache mean a toggle made on another game server process can take
  up to 30 s to apply.
- Nothing is pushed or merged; both branches wait for review.
