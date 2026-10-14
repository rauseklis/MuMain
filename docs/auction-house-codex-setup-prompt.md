# Prompt for Codex: set up the latest Auction House build (server + client) on this laptop

Copy everything below the line into Codex, in the folder that holds your OpenMU server clone and MuMain client clone.

---

You are setting up the latest Auction House build of a MU Online Season 6 private server (OpenMU, C#) and client (MuMain, C++), so I can run and test it locally on this Windows laptop. I already ran an earlier version here, so the toolchain, a PostgreSQL server and the two clones should already exist. Work step by step, run each command, read its output, and stop and tell me if something fails instead of guessing.

## What is new in this build

- **Mixed currencies.** A listing can be priced in up to five different things at once: Zen, the seven jewels (Chaos, Bless, Soul, Life, Creation, Guardian, Harmony) and the five stat fruits. Auctions take such a price (with an optional buyout in the same currencies); **Buy Now** is a fixed price paid at once with no bidding.
- **Bidding rule.** A bid must keep every currency of the current bid at the same amount or higher and raise at least one. The first bid must reach the starting price in every currency of it. A bid is paid in full (every currency) or not at all, and the outbid player's whole bid comes back through the Mailbox.
- **One flat tax: 100,000 Zen per listing.** Taken when the listing is created, returned through the Mailbox if the item does not sell (cancel, no bids at the end, admin removal), kept when it sells. There is no other fee. Single-currency listings follow the same tax.
- Sub-categories, the category rail with icons and the optimized Release client from the previous build are unchanged.

## Ground rules

- Use a **throwaway test database only** (for example `openmu_auction_test`). Never point this at a real or production database. If the existing database is one I care about, copy it first or create a fresh one.
- Do **not** push, force-push, merge, rebase or open pull requests. Use `git fetch` and `git pull --ff-only` only. If a pull is not a fast-forward, stop and tell me.
- Do **not** commit anything. Local edits (for example `nuget.config`) stay local; run `git update-index --assume-unchanged <file>` on them so they never show up in a commit.
- Keep my existing server connection settings and my existing game account and characters.

## 1. Get the latest code

Both repositories use the branch `codex/auction-house` (the same commits are also on the default branches, `master` for the server and `main` for the client):

- Server: `https://github.com/rauseklis/OpenMU.git`
- Client: `https://github.com/rauseklis/MuMain.git`

In each clone: `git fetch origin`, `git checkout codex/auction-house`, `git pull --ff-only origin codex/auction-house`. Report the short commit hash of each. If my clones are git worktrees (for example `server/.worktrees/auction-house` and `client/.worktrees/auction-house`), work in those folders.

## 2. Server

Requirements: .NET 10 SDK and PostgreSQL.

1. From the server repo root: `dotnet build src/Startup/MUnique.OpenMU.Startup.csproj -c Release`.
2. Stop any running `MUnique.OpenMU.Startup.exe` first. Then start `src\Startup\bin\Release\MUnique.OpenMU.Startup.exe` with that folder as the working directory.
3. On start the server applies new database migrations by itself. This build adds three: `AddAuctionSubCategory`, `AddAuctionSaleFee` (the held tax on each listing) and `AddAuctionMixedPrices` (basket columns on listings, bids and Mailbox entries). Expect the log lines "The database schema needs to be updated before the server can be started. Updating..." and "The database schema has been successfully updated". If it fails, show me the error.
4. Next to the exe the server keeps `AuctionHouse.ini`. It lists every Auction House setting with a description and is re-read within seconds of saving. For testing set `Enabled=true` (listing, bidding, buying, cancelling and collecting only work when it is true). Two settings matter for this build: `SaleFeeAmount` under `[Fees]` (the flat tax in Zen; 100000 by default; 0 turns the tax off) and `MixedPricesEnabled` under `[Prices]` (true by default; if the line is missing from an old file, add `MixedPricesEnabled=true`). Leave the other settings alone.
5. A background task re-classifies old listings into the new sub-categories a few seconds after start. There should be no "Auction House expiry batch failed" line in the log.

## 3. Client packet packages (important)

The client does not read the server code directly. It builds its network layer from the NuGet package `MUnique.OpenMU.Network.Packets` **version 0.9.13**, which is packed by hand from the server repo into a local folder (a local NuGet feed). That version is not on nuget.org. Do this:

1. Pick a folder for the feed, for example `C:\Users\<me>\.local-nuget\auction-house`.
2. From the **server** repo root, pack the three packages as 0.9.13. Each project must be rebuilt first and then packed without building, otherwise a stale DLL can end up in the package. Do them in this order, with `<feed>` as your folder:
   - `dotnet build src/PlugIns/MUnique.OpenMU.PlugIns.csproj -c Release -t:Rebuild -p:Version=0.9.13 -p:PackageVersion=0.9.13`
   - `dotnet pack src/PlugIns/MUnique.OpenMU.PlugIns.csproj -c Release --no-build -p:Version=0.9.13 -p:PackageVersion=0.9.13 -o <feed>`
   - `dotnet build src/Network/MUnique.OpenMU.Network.csproj -c Release -t:Rebuild -p:Version=0.9.13 -p:PackageVersion=0.9.13`
   - `dotnet pack src/Network/MUnique.OpenMU.Network.csproj -c Release --no-build -p:Version=0.9.13 -p:PackageVersion=0.9.13 -o <feed>`
   - `dotnet build src/Network/Packets/MUnique.OpenMU.Network.Packets.csproj -c Release -t:Rebuild -p:Version=0.9.13 -p:PackageVersion=0.9.13`
   - `dotnet pack src/Network/Packets/MUnique.OpenMU.Network.Packets.csproj -c Release --no-build -p:Version=0.9.13 -p:PackageVersion=0.9.13 -o <feed>`
3. Check the packed DLL really has the new members. In Python, open `MUnique.OpenMU.Network.Packets.0.9.13.nupkg` as a zip, read `lib/net10.0/MUnique.OpenMU.Network.Packets.dll`, and confirm the bytes `StartingBasket`, `BidBasket`, `RemainingBasket`, `MixedPricesEnabled` and `FeeRefund` all occur in it. If any is missing, repeat the rebuild-then-pack step.
4. In the **client** repo, `nuget.config` has a source `local-auction-house` whose path is on the other developer's machine. Change that path to your `<feed>` folder, then `git update-index --assume-unchanged nuget.config`.
5. If the folder `<client>\.nuget\munique.openmu.network.packets\0.9.13` already exists, delete it so the restore extracts the fresh package. Then run `dotnet restore ClientLibrary/MUnique.Client.Library.csproj --force`. The client project already points at 0.9.13.

## 4. Client build

Requirements: Visual Studio 2022 with the C++ x86 build tools and Windows SDK (it includes CMake, Ninja and vcpkg), the .NET 10 SDK, and internet access for the first NativeAOT and vcpkg restore.

1. Open a command prompt (not PowerShell-only tools) and run, in this order:
   `set "PATH=%PATH%;C:\Program Files (x86)\Microsoft Visual Studio\Installer"` (the NativeAOT link step needs `vswhere.exe` on PATH), then `"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x86` (adjust the edition folder if needed).
2. In the client repo root, configure once if `out\build\windows-x86` does not exist: `cmake --preset windows-x86`.
3. Build the optimized client: `cd out\build\windows-x86` then `ninja -f build-Release.ninja Main`. The first build takes a while. If the `MUnique.Client.Library.dll` link step fails with "exited with code 123" and a line about `vswhere.exe`, the PATH step above was skipped. The build regenerates the C++ packet bindings (`src/source/Dotnet/PacketFunctions_*`) from the 0.9.13 package; they are already committed, so `git status` should stay clean apart from `nuget.config`.
4. Build the **Release** configuration only. The Debug build shows a large FPS/debug panel and runs at roughly a third of the speed.
5. The program is `out\build\windows-x86\src\Release\Main.exe`. Start it with its own folder as the working directory. Make sure it connects to the local server I just started (keep my existing connection settings).
6. Optional checks: `ctest --test-dir out/build/windows-x86 -C Release` should report all tests passing (498 at the time of writing). For the server, with a throwaway database in `AUCTION_TEST_CONNECTION_STRING` (for example `Server=localhost;Port=5432;User Id=postgres;Password=<pw>;Database=openmu_auction_test;`), `dotnet test tests/MUnique.OpenMU.Tests -c Release` should pass (1,598 tests) and `dotnet test tests/MUnique.OpenMU.Network.Packets.Tests -c Release` should pass (656). Never point that variable at a real database: the tests write and delete rows.

## 5. What to test and report

You need **two** accounts with a character each (call them Seller and Buyer) to test bidding. Give both some items, jewels (loose or in packs), stat fruits and at least 1,000,000 Zen. Open the Auction House window as before. Check and report each line as OK or broken, with what you saw:

1. **Category rail and sub-categories** (unchanged): nine rows with icons, the click-open flyout for Armor, Weapon and so on, Reset clears it.
2. **Listing tax card.** On the Sell tab the left card is titled "Listing tax" and shows "100,000 Zen" with a short explanation. There is no percentage fee text.
3. **Single-currency auction.** Sell an item as an auction for one currency (for example 5 Jewels of Soul, buyout 8). Your Zen drops by exactly 100,000. The listing appears in Browse for the Buyer.
4. **Mixed auction.** Sell an item as an auction, press "+ Add currency", and price it in two or three currencies (for example 3 Soul + 50,000 Zen), with a buyout in every one of them (each at least the starting amount) or in none. Create. In Browse the price shows one chip per currency. If you fill a buyout for only some currencies, the form says "Set a buyout for every currency, or leave them all empty."
5. **Bidding in several currencies.** As the Buyer open the mixed listing. The "Your bid" card shows one field per currency, prefilled. Bid the starting amounts: it works and every currency leaves the Buyer's inventory together. As a second player bid again: lowering any currency, or repeating the same amounts, must be refused; raising one currency (or all) must work. The first bidder's whole bid must appear in their Mailbox as "Outbid refund" and be collectable in one click ("Collect" claims every currency at once).
6. **All-or-nothing payment.** Try a bid when you lack one of the currencies (for example enough Soul but not enough Zen): it is refused and nothing at all is taken.
7. **Buyout.** Buy a mixed listing out with the buyout button. If you were the high bidder you only pay the difference. The Seller's Mailbox shows "Sale proceeds" with every currency; the Buyer's shows the item.
8. **Buy Now.** Sell an item in Buy Now mode with two or three currencies. In Browse its price column says "Buy now"; its detail says "Buy now only - no bids", the Bid button is disabled and the buyout button buys it.
9. **Tax refund.** Cancel a listing that has no bids, or let one expire unsold: the Mailbox gets "Listing tax refund" for 100,000 Zen and collecting it gives the Zen back. A listing that sells keeps the tax.
10. **Mailbox and My Listings** open without errors; collecting a basket with a full inventory is refused with the "inventory full" message and takes nothing.
11. **Performance and look**: the Release build runs smoothly, no debug panel.

## 6. If something goes wrong

- Server log shows a migration error: stop, copy the exact error, do not edit the database by hand.
- Client compile error about `StartingBasket`, `BidBasket`, `SubCategory` or a wrong number of arguments for `SendAuctionCreateRequest` / `SendAuctionBidRequest`: the packages are stale. Redo section 3 (rebuild then pack, verify, delete `.nuget\...\0.9.13`, restore).
- NativeAOT link "exited with code 123": add the Visual Studio `Installer` folder to PATH (section 4, step 1) and rebuild.
- Browse shows nothing, or Sell/Bid do nothing: check `Enabled=true` in `AuctionHouse.ini` and that the server was restarted after the update.
- Sell shows "Mixed prices are not available on this server": `MixedPricesEnabled` is false or missing in `AuctionHouse.ini`.

## 7. Known limits of this build

After every successful list, bid, buyout or collect the server disconnects the player once so the character reloads the changed inventory and Zen (live inventory sync is not built yet); log back in to continue. Percentage fees are gone for mixed listings, and for all listings while `SaleFeeAmount` is above 0. WCoin as a tax currency is reserved but not charged.

When you are done, give me a short report: commit hashes, which steps passed, anything that failed with the exact message, and what I should look at in the game.
