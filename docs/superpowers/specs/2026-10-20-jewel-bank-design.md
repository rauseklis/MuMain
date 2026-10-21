# Jewel Bank — design

Status: implemented and merged (server rauseklis/OpenMU#6, client rauseklis/MuMain#15). This document describes the
feature as built; `docs/jewel-bank-handover-2026-10-20.md` has the build, test and pitfall notes.

## Intent

Players farm jewels and stat fruits constantly. They fill the backpack, force trips to town, and get mixed or traded. The
Jewel Bank is a per-account store for those items: they are kept as counts instead of inventory slots, can be taken back
out as real items at any time, and can be sent there automatically when picked up.

The idea comes from a Jewel Bank in the MSPRO S6 repack. That repack was inspiration only; we do not copy its schema,
item codes or column layout. We build our own on OpenMU patterns.

## Goals

- Per-account bank, shared by all characters on the account.
- **45 kinds**, each stored as its own count: the 10 jewels and refining stones, the 5 stat fruits, and the bundles
  (packs) of 10, 20 and 30 of each of the 10 jewels.
- Deposit and withdraw from a client window opened with **Ctrl+J**, any combination of kinds with an amount each.
- **Auto-deposit**: a per-account tick in the window (default off). When on, a bankable item picked up by the player,
  manually or by the MU Helper, goes straight into the bank instead of the inventory.
- Safe against duplication, loss, overflow, double submits and concurrent use; every movement is recorded.
- Looks and behaves like the existing HD UI (Auction House frame and widgets).

## Non-goals

- No NPC; the bank is opened only by hotkey. Plain `J` keeps toggling the server command list.
- No Zen, no other item types, no trading or selling from the bank, no fees.
- No import of any MSPRO data.

## Rules

- **A kind is exactly one item definition at one item level**: jewels at level 0, fruits at their type's level (0 to 4),
  bundles at their size's level (0 = 10, 1 = 20, 2 = 30; item group 12). A deposit removes that many items, a withdrawal
  creates that many. A bundle is banked and withdrawn as itself; it is never merged into or split into loose jewels.
- Only plain items qualify: durability exactly 1, no item options, no sockets. Anything else stays in the inventory.
- **Cap: 25,000 per kind** (`JewelBankOptions.MaximumPerKind`, a code constant). A deposit moves only what fits; at the
  cap a hand deposit answers "The bank is full for that item." and an auto-deposit pickup behaves like no auto-deposit.
- Hand deposit and withdraw are allowed in safe zones only (and under the Auction House's mutation checks: not trading,
  not in a shop, not dead). The window opens anywhere. Auto-deposit works anywhere.
- Withdraw is all-or-nothing against free inventory space (size-aware placement), at most 256 items per request.
- Rate limited per account.

## Server

### Data (PostgreSQL, raw-SQL migration `20261020120000_AddJewelBank`, not in the EF model)

- `data."JewelBankBalance"`: `AccountId`, `Kind` (the kind code), `Count` (bigint, check >= 0); primary key (AccountId, Kind).
- `data."JewelBankSettings"`: `AccountId`, `AutoDeposit`.
- `data."JewelBankMovement"`: append-only ledger (account, character, kind, signed delta, resulting balance, reason,
  operation id, UTC time).
- `data."JewelBankOperation"`: unique (AccountId, OperationId) with the stored result: a retried request returns the
  original result and applies nothing twice.
- `ReconcileAsync` checks the invariants (balance equals the sum of the movements, no negatives).
- The kinds are a code catalogue (`JewelBankKinds`, codes 1 to 10 jewels, 11 to 15 fruits, 16 to 45 bundles by size, each
  size in jewel order); codes are never reused.

### Flow of a hand operation

1. The handler checks eligibility and the rate limit, then runs under `player.RunPersistenceExclusiveAsync` after
   `SaveProgressAsync()`.
2. One database transaction: claim the operation id, validate, delete or create the item rows, change the balances with
   guarded statements, write the ledger and operation rows, commit.
3. After the commit the live inventory is refreshed in place (`PlayerInventoryRefresh`); no disconnect on success. If the
   refresh itself fails the player is disconnected as a last resort so the committed state is reloaded.

### Auto-deposit on pickup

All pickups of the online player go through `PickupItemAction`. If the account's tick is on and the dropped item is
bankable, the drop is claimed under its lock and credited to the bank inside that lock; if the credit fails the drop is
released and the pickup continues as normal. The client is told with a pickup notification; the inventory being full does
not matter on the server.

### Packets (group 0xD6, package `MUnique.OpenMU.Network.Packets` 0.9.20)

Requests are encrypted: Open (00, C3, 4 bytes) and SetAutoDeposit (03, C3, 5 bytes); Deposit (01) and Withdraw (02) are
**C4**, 201 bytes: `OperationId[16]` at 5 and `Amounts` at 21, 45 little-endian 32-bit counts, one per kind code. (As C3
they would encrypt to about 275 bytes, which a C3 length byte cannot carry; the server drops such a connection. A test
pins that every C3 request fits.) Responses: State (80, **C2**, 367 bytes: result, auto-deposit, 45 counts and 45
inventory counts as 32-bit values), Operation (81, C1) and PickupNotification (82, C1).

## Client

- Window `INTERFACE_JEWEL_BANK`, opened with Ctrl+J (handled before the plain `J` branch), closing the Auction House if it
  is open and the other way round. Frame, tabs, buttons and background are the Auction House's textures; the round X is
  the frame's own close button.
- Two tabs, **Deposit** and **Withdraw**, over one list: every kind always shown with its real 3D item model, what the
  bank holds, what the player carries, and an amount. A row is clicked to select all of it, or an amount is typed.
  Select all, Clear and the action button move any combination; the round tick sets auto-deposit.
- **Three equal pages** of 15 rows: jewels and fruits, then the bundles of five jewels each with their 10, 20 and 30
  together; a `< >` pager under the list (the storage window's HD arrow buttons). Selections carry across pages.
- A banked pickup shows the normal blue line "<item> Obtained and moved to the Jewel Bank" with the item's pickup
  sound, and ends the outstanding pickup request.
- The client never writes items or counts; it renders what the server sends. Pure logic (kinds, packet parsing, amounts,
  pages) is unit tested in `tests/ui/test_jewel_bank_model.cpp`.

## Deployment

- Rebuild the server from `master` and the client from `main`. Packets 0.9.20 come from the local NuGet feed
  (`nuget.config`, `C:\Users\<user>\.local-nuget\auction-house`), like the Auction House's. Server and client must be
  deployed together: the 0xD6 protocol is new, and an older client simply has no Jewel Bank window.
- The four tables are created by the startup migration; it is additive and needs no data step.
- The window uses the Auction House's textures, so the client's `Data` folder must be the one that has them.

## Known gaps

- The client refuses a pickup with "inventory is full" when no inventory slot is free, even if auto-deposit would bank
  the item (the client does not know the setting while the window is closed).
- Texts are English only. The cap is a constant, not a setting.
- Auto-deposit for the offline helper has not been verified.
