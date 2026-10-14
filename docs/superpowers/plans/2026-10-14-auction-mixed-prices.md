# Auction House: flat sale tax, mixed-currency prices and bidding

Status: owner delegated approval on 2026-10-14 ("build everything, push and merge"). This file is the design and the task list;
code is written test-first directly against the repositories. It amends `2026-10-12-auction-mixed-price-design.md` (monorepo
root) in one place: **mixed listings are biddable** (the owner reversed the earlier "mixed is buyout-only" rule).

## Owner decisions (final)

1. Price = a basket of up to 5 distinct kinds out of 13 (Zen, Chaos, Bless, Soul, Life, Creation, Guardian, Harmony, Strength,
   Agility, Vitality, Energy, Command), quantities of at least 1. Starting price and optional buyout have the same shape.
2. Bid rule (dominance): a bid keeps every currency of the current bid at the same amount or higher and raises at least one.
   The first bid must be at least the starting price in every currency of that price. Bidders may add currencies.
3. Tax: one flat 100,000 Zen per listing, held when the listing is created, refunded (Mailbox) if the item does not sell,
   consumed when it sells. No percentage fee of any kind.
4. Auction and Buy Now both stay. Buy Now = fixed mixed price, bought at once, no bids.
5. New listings in a mix of currencies (and buy-now) sit behind `MixedPricesEnabled` (AuctionHouse.ini, default **true** so the
   delivered build works out of the box; set false to stop new mixed listings, bids/buyouts/claims on existing ones keep working)
   and the flat tax behind `SaleFeeAmount` (default 100000; 0 turns the tax off).

## Design

- **Mode.** `AuctionCurrencyMode.Mixed = 9`. A listing priced in 2 or more kinds is Mixed; one kind keeps its legacy mode
  (Zen, one jewel, one fruit). The client chooses; the server validates.
- **Vectors.** A basket is 13 longs in `PriceKind` order, stored as text (13 comma-separated numbers) so the entity generator
  needs nothing special. Listing gets `PriceVector`, `BuyoutVector`, `CurrentBidVector`, `KindMask` (bit per kind, for the
  currency filter); bid gets `AmountVector`; collection gets `OriginalVector`/`RemainingVector`. Ledger rows reuse
  `FruitComponent` as the component (kind) number with `Currency = Mixed`.
- **Tender.** One transaction debits every component with the existing per-kind helpers (Zen money, jewel planner, exact fruit
  items); if any is short, the whole transaction rolls back. Refunds and proceeds are vector collections in the Mailbox; a
  mixed collection is claimed all at once (every component must fit, else nothing moves).
- **Fee.** Listing gets `SaleFeeHeld` and `SaleFeeState` (0 none, 1 held, 2 consumed, 3 refunded). Unsold paths (cancel,
  expire without bid, admin remove) add a Zen collection of the new kind `FeeRefund = 6`; sale paths mark it consumed.
- **Browse.** The currency filter means "price includes this kind". Price filters and price sorts apply only to single-currency
  listings (mixed ones are hidden then).
- **Wire (packets 0.9.13, all appended, nothing removed).** Open response: sale fee amount and mixed flag. Create and bid
  requests: 13 x u32 baskets. Browse, My Listings and detail rows: price, buyout and current-bid baskets. Mailbox rows:
  original and remaining baskets. Enums: `AuctionCurrencyMode.Mixed`, `AuctionCollectionKind.FeeRefund`.
- **Client.** Parse the new fields; draw mixed prices as chips (already built); enable Buy Now and mixed Auction in Sell; a
  bid form with one input per price currency; mixed Mailbox collections show their components with a Claim All button;
  the Sell tab shows the flat tax line.

## Tasks (each: failing test first, green, commit, ledger)

1. Pure domain: `PriceBasket` (parse/format, kind mask, validation with `PriceRules`), dominance bid rule, tests.
2. Flat tax, legacy engine: entity columns + migration; `ItemListingDraft.SaleFee`; hold at create; consume on sale paths;
   refund collection on unsold paths; `FeeRefund` kind; service passes the configured amount; PG tests.
3. Mixed storage: entity columns + migration; create a Mixed listing; Browse/detail rows carry vectors; currency filter;
   PG tests.
4. Mixed buyout and bidding: tender helper, bid (dominance, escrow, outbid refund), buyout, finalize (sold/unsold), cancel,
   admin remove, claim all; PG tests including rollback when one component is short.
5. Packets 0.9.13 + handlers: create, bid, browse, my listings, detail, mailbox, open; repack, packet tests.
6. Client: bindings, parsers (+ tests), Sell (Buy Now and mixed Auction), bid form, detail, Mailbox, fee line, labels.
7. Verify on the test database; docs; push; update the brother's prompt.

## Rulings made while building (2026-10-14)

- `Mixed = 9` (Fruits is 8), not 10 as first drafted.
- Every buy-now listing is Mixed, even in one currency; an auction in one currency keeps its legacy form, an auction in two or
  more currencies is Mixed. The legacy engine is untouched.
- Mixed listings take no percentage fee at all, whatever the percentage settings say; only the flat tax applies.
- A buyer who buys out after bidding pays buyout minus own escrow per currency; escrow beyond the buyout in any currency is
  returned as a refund basket.
- Mixed claims are all-or-nothing; the Mailbox shows no quantity fields for them.
- Browse: the currency filter matches a Mixed listing when its price includes that currency; price sorts and price limits leave
  Mixed listings out.
- Packet baskets are 13 little-endian u32 (52 bytes), appended to Create/Bid/summary/detail/mailbox layouts; the Open response
  gets SaleFee and MixedPricesEnabled before the currency list (configuration version 5).
