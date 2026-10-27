# xShop (in-game cash shop) — design draft

Status: DRAFT, waiting for the owner's yes. No code written.

## Goal (owner's words, 2026-10-27)
Players press **X** in a **safe zone**, the xShop opens, they can **buy items**; staff can **give players currency**.
Items: the official Webzen Season 6 Episode 3 shop list, all of it. Prices do not matter for now; we only need to see it working.

## What exists
- Client: the shop UI is complete and enabled (`src/source/GameShop/`, X key in `NewUIHotKey.cpp`). It needs the server to send
  `D2 0C` (script version) before it opens, then reads `Data/InGameShopScript/<zone.year.id>/IBS{Category,Package,Product}.txt`.
  It tries to download a missing script from `image.webzen.com` (dead) so our script folder must exist locally.
- The shipped script `512.2012.084` is Webzen's own list: 8+8+... categories in 4 currency tabs (W Coin C, W Coin P, Goblin Point + a
  special tab), **211 packages / 2719 product rows** (Buff 71, Special 57, Equipment 32, Ticket 27, Pet 10, Potion 6, Accessory 4, Box 5).
- Server `master`: packets defined, **no handlers, no balances, no items**. Open PR #24 (balances + open/balance/empty-storage, buying refused) fits as the base.

## Proposed design
1. **Wallet** = PR #24 (WC, WP, GP per account+server). Merge it first (after its own tests pass); I add to it, not beside it.
2. **Giving currency**: a GM chat command `/cash <account> <wc> <wp> <gp>` using PR #24's retry-safe credit operation, logged.
3. **Catalogue**: server reads the same three script files (one copy shipped with the server, one with the client; a test checks they match).
   Each package is mapped to what the buyer receives. This mapping is the real work: the script only has Webzen cash-item codes.
   - Tier A (plain items, e.g. fruits, backpack, vault expansion, jewels, boxes, equipment): delivered as real OpenMU items.
   - Tier B (buffs, scrolls, premium, pets, character cards, tickets): delivered if OpenMU already has an equivalent effect/item; otherwise
     the entry stays visible but purchase is refused with "not currently available" (client message exists), listed in a table so nothing silently breaks.
   I will list every one of the 211 packages with its tier for you to review before building.
4. **Buying** (`D2 03`): server validates package, price, currency, free storage, debits and stores in one database transaction (no double spend).
   Items land in the shop **storage** (`D2 05` list), and "use" (`D2 0B`) moves them into the inventory/applies them, as the original shop does.
   Gifting (`D2 04`) is out of the first version.
5. **Safe zone only**: server refuses `D2 02` open (and purchases) outside a safe-zone map area; client also checks before sending.
6. **Packets**: no XML change expected (so no version bump). Client change: script folder + version constant + safe-zone check.

## Testing
Throwaway PostgreSQL + second server on shifted ports; unit tests for packet bytes, wallet, purchase, storage, catalogue-vs-client match.
I do not control the game; you open the shop and tell me what you see. Not tested before your check: the visual UI.

## Order of work and risk
PR #24 merged → server handlers + catalogue + GM command (PR) → client script/safe-zone (PR) → merge server first, then client → you test.
A server merge deploys live (~1 min disconnect); the database gets new tables via migration (needs backup + your yes at that step).

## Open questions for the owner
1. Is it OK that I treat the shipped `512.2012.084` list as "the official S6E3 list" (I will also search the web to cross-check it)?
2. Items that OpenMU cannot do yet (e.g. Gold Channel, Premium Service): show but refuse, or hide?
3. Should I start from PR #24 as is, or do you want to talk to your brother first (it is his open work)?
