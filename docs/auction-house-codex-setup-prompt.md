# Complete handover and setup prompt for Codex: the Auction House (OpenMU server + MuMain client)

Copy everything below the line into Codex. It is written for a Codex that has NOT seen this project before and whose laptop holds an OLDER copy of it. It tells you what the project is, what changed, how to bring both repositories up to date without losing anything, how to build and run them, how to test them, and what to report back.

---

## 0. Your role, in one paragraph

You are Codex, running on a Windows laptop that belongs to the brother of the project owner. The laptop has an older copy of a MU Online Season 6 private-server project: the **OpenMU server** (C#) and the **MuMain game client** (C++). Your job is to (1) look at what is on the laptop and protect it, (2) bring both repositories to the newest commit of the branch `codex/auction-house`, (3) build the server and the client, (4) run the automated tests, (5) start the server and the client and test the new **Auction House** features by hand with two game accounts, and (6) send me a precise written report. You may read anything, run builds and tests, and diagnose problems. You must **not** publish or rewrite anything: no commits, no pushes, no merges, no rebases, no force operations. Work step by step, run each command, read its output, and when something fails stop and tell me instead of guessing.

## 1. The project in plain words

- **MU Online** is an old MMORPG. This project is a private Season 6 server and a modernised client for it.
- **OpenMU server**: C# on .NET 10 with PostgreSQL. Game state (accounts, characters, inventories, items) and the Auction House state live in PostgreSQL. The game server process is `MUnique.OpenMU.Startup.exe`. It runs a connect server (ports 44405 and 44406 in the owner's setup), game servers, a chat server and an admin web page.
- **MuMain client**: C++ (SDL GPU rendering, 32-bit Windows build) plus a small .NET **NativeAOT library** (`ClientLibrary`, built into `MUnique.Client.Library.dll`) that does all networking and packet encoding. The C++ side calls generated functions such as `SendAuctionBidRequest(...)` that live in `src/source/Dotnet/PacketFunctions_*.cpp/.h`. Those files are **generated** from XML packet definitions that ship inside the NuGet package `MUnique.OpenMU.Network.Packets`. This is why the NuGet packaging in section 6 matters.
- **The Auction House** is a server-authoritative marketplace window in the client (press the **A** key in the game; the **S** key opens the skill tree, the Master Level window). It has four tabs:
  - **Browse**: pick a category (nine-row rail on the left with a sub-category flyout), search, filter by currency, page through listings, click one to open its detail card, then Bid or Buyout.
  - **Sell**: pick an item by clicking it in the inventory (the inventory docks beside the window and becomes selection-only while Sell is open), choose Auction or Buy Now, set the price, duration and an optional note, press Create Listing.
  - **My Listings**: your own listings, with a Cancel button (only while a listing has no bids).
  - **Mailbox**: everything the server owes you (bought items, returned items, sale proceeds, refunds of outbid bids, jewel change, the refunded listing tax). Click an entry, then Collect.
- "Server-authoritative" means the client only asks; the server checks every rule and moves every item and every unit of money inside a database transaction, so nothing can be duplicated or lost, and a repeated request (same operation id) is recognised and not applied twice.
- Money and goods in the Auction House: **Zen** (the money, held on the character's inventory), the **seven jewels** (Chaos, Bless, Soul, Life, Creation, Guardian, Harmony; loose jewels or "packs" of 10, 20 or 30), and the **five stat fruits** (Strength, Agility, Vitality, Energy, Command). Thirteen kinds in total. The listed item itself leaves the seller's inventory when the listing is created and lives in the database until it is sold, returned or removed.

## 2. How the project got here (so you can recognise the older copy)

Everything is on the branch `codex/auction-house` of both repositories. The history, oldest first:

1. **First Auction House (about 6 to 11 October)**: one currency per listing (Zen, one jewel, or a "fruit basket"), Browse / Sell / My Listings / Mailbox, bids with a minimum raise, buyout, cancel, expiry worker, Mailbox collection, notifications, an `AuctionHouse.ini` settings file. Database migrations `AddAuctionListing`, `AddAuctionRemainingEntities`, `AddAuctionSearchSnapshots`. Packet package versions 0.9.10 / 0.9.11.
2. **Polish and categories (12 to 13 October)**: modern window chrome, wallet cards, item previews, sub-categories (migration `AddAuctionSubCategory`), the category rail with icons and flyout, a Release client without the debug panel. Packet package 0.9.12.
3. **This build (14 October)**: mixed currencies, Buy Now, the new bidding rule, and one flat listing tax. Migrations `AddAuctionSaleFee` and `AddAuctionMixedPrices`. Packet package **0.9.13**. This is what you are installing.

Expected tips of the branch when this prompt was written: server `29e189c72`, client `02275d07` (or later commits on top). The same commits are also on the default branches (`master` of the server repository, `main` of the client repository).

Repositories:

- Server: `https://github.com/rauseklis/OpenMU.git` (fork of MUnique/OpenMU)
- Client: `https://github.com/rauseklis/MuMain.git`

On the owner's machine both are **git worktrees**: `...\mu-online-openmu\server\.worktrees\auction-house` and `...\mu-online-openmu\client\.worktrees\auction-house`. On this laptop they may be ordinary clones, worktrees, or something else. Find out in section 3.

## 3. What is new in this build (the feature, in detail)

**3.1 Mixed prices.** A price is a *basket*: up to **five different kinds** out of the thirteen (Zen, Chaos, Bless, Soul, Life, Creation, Guardian, Harmony, Strength fruit, Agility fruit, Vitality fruit, Energy fruit, Command fruit), each with a whole quantity. Limits (configurable in the ini): Zen 1,000 to 2,000,000,000 per price; jewels and fruits 1 to 10,000 each. There is no exchange rate between kinds: "3 Soul + 50,000 Zen" is never converted into anything else.

**3.2 Two ways to sell.**
- **Auction**: a starting price (one or more currencies) and an optional **buyout** price. A buyout, if given, must name exactly the same currencies as the starting price, each at least as high.
- **Buy Now**: a fixed price (one to five currencies), bought at once, **no bidding**.
Both modes exist side by side. An Auction in only one currency is stored the old way (so old behaviour is unchanged); a Buy Now, or an Auction in two or more currencies, is stored as a *Mixed* listing.

**3.3 The bidding rule (owner decision).** A bid must keep **every currency of the current bid at the same amount or higher** and **raise at least one**. The first bid must reach the starting price in **every** currency of that price. A bidder may add currencies the previous bid did not have. A bid names at most five currencies. Examples with a current bid of 50,000 Zen + 3 Soul:
- 50,000 Zen + 4 Soul: allowed (raised Soul).
- 60,000 Zen + 3 Soul + 1 Bless: allowed (raised Zen, added Bless).
- 50,000 Zen + 3 Soul: refused (nothing raised).
- 90,000 Zen + 2 Soul: refused (Soul went down).
- 9 Soul: refused (Zen was dropped).

**3.3a Paying.** A bid or buyout is paid in full or not at all: every component is taken from the character's main inventory (Zen from the money; jewels from loose jewels and packs, chosen with the least overpayment first, then the fewest items, then the lowest slots, with any overpayment returned as "tender change" in the Mailbox; fruits as exact clean fruits) inside one transaction. If one component is short, nothing is taken. The previous high bidder's whole bid is returned through the Mailbox as one "Outbid refund". If the high bidder later buys the listing out, they pay only the part of the buyout they have not already put down (per currency); anything they had put down beyond the buyout comes back as a refund.

**3.4 One flat tax: 100,000 Zen per listing (owner decision).** Taken from the seller's Zen when the listing is created, **held**, then: returned through the Mailbox as a "Listing tax refund" (collection kind 6) if the listing is cancelled, ends with no bids, or is removed by an administrator; **kept** when the listing sells. There is no listing fee, no success fee and no percentage of any kind while `SaleFeeAmount` is above 0 (it is 100000 by default; 0 turns the tax off). Mixed listings never take a percentage fee at all. A seller who cannot pay the 100,000 Zen cannot list.

**3.5 Collecting.** A Mailbox entry that holds several currencies is collected **whole**: the Zen is credited and the jewels and fruits are created as items in free main-inventory slots, or, if the Zen would exceed the inventory limit (2,000,000,000) or there is no room, nothing at all happens and the entry stays. Single-currency entries keep their old partial-claim behaviour.

**3.6 Browse behaviour.** The currency filter shows a Mixed listing when its price *includes* the chosen currency. Price sort and price-range filters only apply to single-currency listings, so Mixed listings disappear while one of them is active (there is no price order between different currencies). In the list a Buy Now row shows "Buy now" in the price column and its price in the buyout column.

**3.7 Switches.** `MixedPricesEnabled` (default true) controls only whether NEW Mixed listings can be created. Bids, buyouts and collections on existing Mixed listings keep working when it is false, so nobody's escrow is ever trapped.

**3.8 Known limitations (not bugs to report).**
- After every successful list, bid, buyout or collect the server disconnects the player once so the character reloads the changed inventory and Zen. Log back in. (Live inventory sync is future work.)
- New texts in the Sell tab and bid card are English literals, not yet in the translation files.
- WCoin as a tax currency is reserved but not charged.
- The mixed screens have not been looked at on a real screen by the developer yet, so layout details (spacing, clipping) may need tweaks. **Do report visual problems with exact descriptions.**

## 4. Ground rules (read twice)

1. **Throwaway database only.** Never run any of this against a database the owners care about. Section 5.3 shows how to back up and how to work on a copy.
2. **No publishing.** No `git commit`, `git push`, `git merge`, `git rebase`, `git reset --hard`, `git clean`, `git checkout -- .`, `git stash drop`, no pull requests. Allowed: `git fetch`, `git status`, `git log`, `git diff`, `git branch`, `git checkout <branch>`, `git pull --ff-only`, `git stash push` (to *protect* local work, never to drop it), `git update-index --assume-unchanged`.
3. **Never discard local work.** If a clone has uncommitted changes or local commits that are not on the remote, keep them (stash with a clear message, or create a backup branch) and tell me what they were.
4. **Leave one file alone:** `src/Web/Map/wwwroot/js/MUnique.OpenMU.Web.Map.js` in the server repository is rewritten by every server build. If `git status` shows it modified, ignore it. Do not stage, edit or revert it.
5. **Release builds only** for the client. A Debug client shows a large debug panel and runs at about a third of the speed.
6. **Do not print passwords** in your report. Read database connection details from the server's `ConnectionSettings.xml` and use them, but do not paste them.
7. **Stop and report** on any failure (build error, test failure, migration error, crash) with the exact message. You may investigate read-only (logs, SQL SELECTs, reading code) before reporting.
8. You may *propose* a fix as a diff inside your report. Do not apply it to tracked files.

## 5. Step by step

### 5.1 Look at what is on the laptop (read-only)

Run these in **each** repository (server and client) and keep the output for your report:

```
git rev-parse --show-toplevel
git worktree list
git branch --show-current
git status --short
git log --oneline -5
git remote -v
git stash list
git branch -a
```

Then answer, in your report: Which commit is each repository on? Is it a plain clone or a worktree? Is the working tree clean? Are there local commits that are not on `origin`? (`git log --oneline origin/<branch>..HEAD` after a `git fetch`.) Which of the three versions in section 2 does it look like? Also check what is running:

```
tasklist /FI "IMAGENAME eq Main.exe"
tasklist /FI "IMAGENAME eq MUnique.OpenMU.Startup.exe"
```

Stop (close) `Main.exe` before you build the client and `MUnique.OpenMU.Startup.exe` before you build the server, otherwise the linker cannot overwrite the files. Only stop processes that belong to these repositories.

If a working tree is NOT clean: do `git stash push -u -m "before-auction-update"` after listing what is in it (`git status --short`, and `git diff --stat`). Never discard it. If there are local commits that are not on `origin`, create a backup branch first: `git branch backup/before-auction-update`.

### 5.2 Prerequisites

Check, and install or report anything missing:

- **.NET SDK 10**: `dotnet --list-sdks` must show a `10.0.x` entry.
- **PostgreSQL** with `psql` and `pg_dump` (`psql --version`). The server already needs a running PostgreSQL; keep using the one that is installed.
- **Visual Studio 2022** (Community is fine) with the workload "Desktop development with C++", including the **x86** (32-bit) MSVC tools and a Windows 10/11 SDK. This brings CMake and Ninja.
- **vcpkg**: the client's dependencies come from a vcpkg checkout through the manifest `vcpkg.json`. The environment variable `VCPKG_ROOT` must point at it. On the owner's machine it is `C:\Users\rause\OneDrive\Desktop\OpenMU-trial\vcpkg`. If the build directory `out\build\windows-x86` already exists on this laptop, look in its `CMakeCache.txt` for `CMAKE_TOOLCHAIN_FILE` to find the vcpkg that was used before, and reuse that one.
- **Python 3** (the client build runs small code generators).
- Internet access for the first NativeAOT and vcpkg restore.
- The repositories may live inside OneDrive. If a build fails with "file in use" or "access denied" on files under `bin` or `obj`, pause OneDrive sync and retry.

### 5.3 Protect the database

1. Find the server's connection settings: `<server>\src\Startup\bin\Release\ConnectionSettings.xml` (or next to whichever `MUnique.OpenMU.Startup.exe` this laptop uses). It has several `<ConnectionString>` elements (database names, host, user, password). Do not paste the password anywhere.
   **If PostgreSQL answers `FATAL: password authentication failed for user "postgres"`:** the freshly built `ConnectionSettings.xml` is a default file, not the one the old server used. Do not stop and ask yet. (a) Find the folder of the old `MUnique.OpenMU.Startup.exe` you stopped (from its process path) and read the `ConnectionSettings.xml` next to it, and in the older `bin\Debug` / `bin\Release` folders of the old clone. (b) Search the user folder for every `ConnectionSettings.xml`, newest first. (c) Test each candidate without printing the password: set `PGPASSWORD` for the session and run `psql -h <host> -p <port> -U <user> -d postgres -c "select 1"`; also check the port (5432 by default, but 5433 or a Docker/podman container is possible: `docker ps` / `podman ps`). (d) Copy the working host, port, user and password into every `<ConnectionString>` of the new `ConnectionSettings.xml` (never commit it, never paste the password). (e) Only if nothing works, report which candidates you tried (no passwords) and wait for me; resetting the `postgres` password (temporarily set the `127.0.0.1/32` and `::1/128` lines of `pg_hba.conf` to `trust`, restart the service, `ALTER USER postgres PASSWORD '...'`, restore `scram-sha-256`, restart) is allowed only after I say so.
2. Back up whatever database it points at, with a custom-format dump, before the new server touches it:
   `pg_dump -Fc -h <host> -p <port> -U <user> -d <database> -f auction_update_backup.dump`
3. **If that database matters to anyone, do not use it.** Make a throwaway copy: `createdb -h <host> -U <user> -T <database> openmu_auction_test` (needs no other connection to `<database>`; alternatively `createdb` and `pg_restore -d openmu_auction_test auction_update_backup.dump`), then change **every** `<ConnectionString>` in `ConnectionSettings.xml` to `Database=openmu_auction_test`. The tests and all manual testing below happen on that copy only.
4. The database should be a *populated* one: it needs at least three accounts that each have a character, plus the item definitions. Check with `SELECT count(*) FROM data."Character";` (three or more is enough) and `SELECT count(*) FROM config."ItemDefinition";`.

### 5.4 Get the new code

In each repository:

```
git fetch origin
git checkout codex/auction-house          (or: git checkout -b codex/auction-house origin/codex/auction-house, if there is no local branch)
git pull --ff-only origin codex/auction-house
git submodule update --init --recursive   (client only; harmless if nothing is configured)
git log --oneline -3
```

If `--ff-only` refuses (the local branch has commits the remote does not), stop and report `git log --oneline HEAD..origin/codex/auction-house` and `git log --oneline origin/codex/auction-house..HEAD`. Do not merge. The same commits are on `master` (server) / `main` (client); `git pull --ff-only origin master` works for a clone that tracks the default branch.

### 5.5 Build and start the server

```
cd <server repo>
dotnet build src/Startup/MUnique.OpenMU.Startup.csproj -c Release
cd src\Startup\bin\Release
MUnique.OpenMU.Startup.exe
```

Start it with its own folder as the working directory (the settings files are read from there). For a background start from PowerShell: `Start-Process -FilePath .\MUnique.OpenMU.Startup.exe -WorkingDirectory (Get-Location) -RedirectStandardOutput server-out.log -RedirectStandardError server-err.log -WindowStyle Hidden`.

**Migrations.** On start the server applies pending database migrations by itself. From an older copy it will apply, in order, whichever of these it has not run yet:

| Migration | What it does |
|---|---|
| `20261006141235_AddAuctionListing` | the Auction House tables: listings, bids, collections, currency movements, operations, notifications, audit log |
| `20261006142726_AddAuctionRemainingEntities` | remaining entities of the first version |
| `20261007105316_AddAuctionSearchSnapshots` | item-search snapshot columns on listings (name, level, class mask, option flags) |
| `20261013120000_AddAuctionSubCategory` | `SubCategory` column on `AuctionListing` (255 = not yet classified; a background task fills it in) |
| `20261014090000_AddAuctionSaleFee` | `SaleFeeHeld` and `SaleFeeState` on `AuctionListing` (the flat tax) |
| `20261014120000_AddAuctionMixedPrices` | `PriceVector`, `BuyoutVector`, `CurrentBidVector`, `KindMask` on listings; `AmountVector` on bids; `OriginalVector`, `RemainingVector` on collections |

Expect the log lines "The database schema needs to be updated before the server can be started. Updating..." and "The database schema has been successfully updated". Afterwards confirm in SQL that the newest one is recorded: `SELECT "MigrationId" FROM public."__EFMigrationsHistory" ORDER BY 1 DESC LIMIT 3;` must list `20261014120000_AddAuctionMixedPrices` first (the history table is in the `public` schema; there is a second one in `admin` for the admin context, which is not relevant), and that `data."AuctionListing"` has a `PriceVector` column (`\d data."AuctionListing"`). If a migration fails, **stop** and show me the exact error. Do not edit the database by hand to get around it.

**The settings file.** The server keeps `AuctionHouse.ini` next to the exe. It is created from the stored settings, lists every setting with a description, and is **re-read within seconds** of saving (no restart needed). A value in the file wins over the admin panel. If the file already exists from an older build it may lack newer keys: add any missing key from the table below (they are all optional; defaults apply when absent). For testing set these:

| Section | Key | Set to | Why |
|---|---|---|---|
| General | `Enabled` | `true` | listing, bidding, buying, cancelling and collecting only work when true; browsing always works |
| General | `MinimumAccountAgeHours` | `0` | the default is 24: a test account younger than that gets "account too new" |
| General | `MinimumCharacterLevel` | `1` (or leave 6 if your test characters are level 6+) | otherwise "level too low" |
| Fees | `SaleFeeAmount` | `100000` | the flat tax in Zen; 0 turns it off |
| Prices | `MixedPricesEnabled` | `true` | allows new Mixed listings (add the line if it is missing) |

Other keys you may see: `PageSize` (25), `MaximumActiveListingsPerAccount` (10) and per character (5), `ListingFeeBasisPoints` and `SuccessFeeBasisPoints` (legacy percentages; ignored while `SaleFeeAmount` is above 0), `MaximumPriceComponents` (5), `ZenMinimum`/`ZenMaximum`, `JewelMinimum`/`JewelMaximum`, `FruitMinimum`/`FruitMaximum`, `ExpiryIntervalSeconds` (15: how often ended listings are settled), `ExpiryBatchSize`, `NotificationsEnabled`, `OperationRetentionDays`, `PurgeBatchSize`. The log line "Auction House settings read from ...AuctionHouse.ini: N applied" tells you the file was read (24 settings in this build, with a duplicate-free file).

**What to look for in the server log:** no "Auction House expiry batch failed", no unhandled exceptions, and the connect server listening ("Client Listener started, Port ..."). A few seconds after start the sub-category re-classification runs once for old listings.

### 5.6 The client's packet package (the part that is easy to get wrong)

The client does not read the server code. Its network layer is generated from the NuGet package **`MUnique.OpenMU.Network.Packets` version 0.9.13**. That exact version is not on nuget.org: it is packed by hand from the server repository into a local folder (a "local NuGet feed"). The server projects must be rebuilt from the new source and packed, otherwise the client is generated from stale definitions and either fails to compile or sends packets of the wrong length.

1. Choose a feed folder, for example `C:\Users\<you>\.local-nuget\auction-house`.
2. From the **server** repository root, rebuild then pack the three projects as 0.9.13, in this order, using your folder instead of `<feed>`. Always `-t:Rebuild` first and `--no-build` when packing, otherwise a stale DLL can end up inside the package:

```
dotnet build src/PlugIns/MUnique.OpenMU.PlugIns.csproj -c Release -t:Rebuild -p:Version=0.9.13 -p:PackageVersion=0.9.13
dotnet pack  src/PlugIns/MUnique.OpenMU.PlugIns.csproj -c Release --no-build -p:Version=0.9.13 -p:PackageVersion=0.9.13 -o <feed>
dotnet build src/Network/MUnique.OpenMU.Network.csproj -c Release -t:Rebuild -p:Version=0.9.13 -p:PackageVersion=0.9.13
dotnet pack  src/Network/MUnique.OpenMU.Network.csproj -c Release --no-build -p:Version=0.9.13 -p:PackageVersion=0.9.13 -o <feed>
dotnet build src/Network/Packets/MUnique.OpenMU.Network.Packets.csproj -c Release -t:Rebuild -p:Version=0.9.13 -p:PackageVersion=0.9.13
dotnet pack  src/Network/Packets/MUnique.OpenMU.Network.Packets.csproj -c Release --no-build -p:Version=0.9.13 -p:PackageVersion=0.9.13 -o <feed>
```

3. **Verify the packed DLL.** In Python, open `<feed>\MUnique.OpenMU.Network.Packets.0.9.13.nupkg` with `zipfile`, read `lib/net10.0/MUnique.OpenMU.Network.Packets.dll`, and confirm each of these byte strings occurs in it: `StartingBasket`, `BuyoutBasket`, `BidBasket`, `CurrentBasket`, `OriginalBasket`, `RemainingBasket`, `MixedPricesEnabled`, `SaleFee`, `FeeRefund`, `SubCategory`. If any is missing the package is stale: repeat the rebuild-then-pack step for that project.
4. **Create** the file `nuget.config` in the root of the **client** repository. It is NOT in git (it is a local file on the owner's machine, excluded through `.git/info/exclude`), so on this laptop it will not exist: create it with exactly this content, using your `<feed>` folder in the second source:

```
<?xml version="1.0" encoding="utf-8"?>
<configuration>
  <packageSources>
    <add key="nuget.org" value="https://api.nuget.org/v3/index.json" />
    <add key="local-auction-house" value="<feed>" />
  </packageSources>
</configuration>
```

   Keep it out of git: append the line `nuget.config` to the file `<client repo>/.git/info/exclude` (for a worktree, the exclude file of the main repository's `.git` folder; `git status` must not list `nuget.config`). If a `nuget.config` already exists (a clone that has one), only make sure the `local-auction-house` source points at your `<feed>`.
5. Make the client restore into its own cache: set `NUGET_PACKAGES=<client repo>\.nuget` (in cmd: `set "NUGET_PACKAGES=C:\path\to\client\.nuget"`). If the folder `<client repo>\.nuget\munique.openmu.network.packets\0.9.13` already exists, delete it so the restore extracts the fresh package. Then:
   `dotnet restore ClientLibrary/MUnique.Client.Library.csproj --force`
   The client project `ClientLibrary/MUnique.Client.Library.csproj` already references 0.9.13 (`<OpenMuPacketsVersion>`).
6. The CMake cache of an old build directory may say `OPENMU_PACKETS_VERSION 0.9.10`. That value only feeds an unrelated wire-size check for old structures; leave it alone.

### 5.7 Build the client (Release only)

Open a **cmd** prompt (not just PowerShell) so the Visual Studio environment script works, and run in this order:

```
set "VCPKG_ROOT=C:\path\to\vcpkg"
set "NUGET_PACKAGES=C:\path\to\client\.nuget"
call "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" -arch=x86 -host_arch=x64
set "PATH=C:\Program Files (x86)\Microsoft Visual Studio\Installer;%PATH%"
cd /d C:\path\to\client
```

- The `PATH` line matters: the NativeAOT link step of the client library needs `vswhere.exe`, which lives in that `Installer` folder. If the link of `MUnique.Client.Library.dll` fails with "exited with code 123" and a line about `vswhere.exe`, this line was skipped.
- Use the Professional or Enterprise folder instead of `Community` if that is what is installed.
- Configure once, if `out\build\windows-x86` does not exist: `cmake --preset windows-x86`. (Ninja Multi-Config; Debug and Release share the folder.)
- Build the optimized client: `cmake --build out/build/windows-x86 --config Release --target Main`. The first build takes a long time (vcpkg packages, NativeAOT). The build also regenerates the C++ packet bindings in `src/source/Dotnet/PacketFunctions_*` from the 0.9.13 package. They are already committed, so afterwards `git status` should show nothing changed (`nuget.config` is excluded and does not appear). **If the bindings show up as modified after the build, tell me which files**: it means the package you packed differs from the one the developer used.
- The program is `out\build\windows-x86\src\Release\Main.exe`. Start it with its own folder as the working directory. It reads `config.ini` there. By default it connects to localhost port 44406; to be explicit: `Main.exe connect /u127.0.0.1 /p44406`. Keep the connection settings this laptop already uses for its own server.
- The client writes `MuError.log` in its working folder; read it for errors if the window misbehaves.

### 5.8 Run the automated tests first

Every command below must pass. Report the totals.

1. **Server** (needs the throwaway database; the variable makes the PostgreSQL tests run, otherwise they are skipped, which you must then report as "skipped"):
   `set "AUCTION_TEST_CONNECTION_STRING=Server=localhost;Port=5432;User Id=<user>;Password=<password>;Database=openmu_auction_test;Command Timeout=120;"`
   `dotnet test tests/MUnique.OpenMU.Tests -c Release`
   Expected at the time of writing: **1,598 passed, 0 failed** (it takes a few minutes). The Auction House part alone: add `--filter "FullyQualifiedName~Auction"` (about 473 tests). These tests create and delete rows (they need three accounts that each own a character) and **must never be pointed at a real database**.
2. **Packet definitions**: `dotnet test tests/MUnique.OpenMU.Network.Packets.Tests -c Release`. Expected **656 passed**. This includes a test that locks the exact byte layout of every Auction House packet.
3. **Client**: `ctest --test-dir out/build/windows-x86 -C Release --output-on-failure` (build the test targets first with `cmake --build out/build/windows-x86 --config Release`). Expected **498 of 498 passed**. The Auction House unit tests are the cases tagged `[ui][auction_model]`, `[ui][auction_sell]`, `[ui][auction_wire]` and `[ui][auction_taxonomy]` (125 in total); the test named `dotnet_binding_generator` checks the generated C++ send functions against the design.

If a test fails, show its name and its full output. Do not "fix" tests.

## 6. Manual test plan (two or three players, one server)

### 6.1 Preparation

- **Players.** Prepare three characters on three different accounts: **Seller (S)**, **Buyer (B)** and **Rival (C)**. Each must be at least `MinimumCharacterLevel`, on an account older than `MinimumAccountAgeHours` (or set that to 0, section 5.5), and must stand **inside a safe zone** (the town, for example Lorencia): the server refuses every list, bid, buyout, cancel and collect request from a player who is outside a safe zone, dead, trading, running a personal store, with an NPC window open, dueling, in a mini game or in a guild war. Browsing works anywhere.
- **Stock.** Give S: four or five tradable items that are not quest items (any weapons or armour; keep them in the main inventory, not equipped), 1,000,000 Zen. Give B and C: 1,000,000 Zen each, about 10 loose Jewels of Soul, 5 Jewels of Bless, and a few Fruits (for example 4 Fruits of Strength and 2 of Energy), in the **main inventory** (the 64 slots below the equipment; the Auction House never touches equipped items or the vault). To give items and Zen use whatever you normally use for test characters (GM chat commands, the admin web page, or SQL). **If you use SQL, log that character out first**: an online character is rewritten from the server's memory and would overwrite your change. Example for Zen: `UPDATE data."ItemStorage" s SET "Money" = 1500000 FROM data."Character" c WHERE c."InventoryId" = s."Id" AND c."Name" = '<character>';`
- **Item identities** (group/number): Jewel of Chaos 12/15, Bless 14/13, Soul 14/14, Life 14/16, Creation 14/22, Guardian 14/31, Harmony 14/42. Stat fruits are all item 13/15 and the item **level** says which: Energy 0, Vitality 1, Agility 2, Strength 3, Command 4 (durability must be above 0 and the fruit must have no options or sockets). Jewel packs ("bundles of 10/20/30") count as that many loose jewels.
- **Bookkeeping.** Before each scenario write down each player's Zen and counts of the relevant jewels/fruits (in-game, or by SQL when the character is offline), so you can state exact differences.
- Open the Auction House with the **A** key. After every successful list/bid/buyout/collect you are disconnected once: log in again.

### 6.2 SQL you will use to verify (read-only unless stated)

Table and column names are exactly these (PostgreSQL, quoted identifiers, schema `data`):

```
-- listings: Status 0 active, 1 sold, 2 ended without a bid, 3 cancelled, 4 removed by an administrator
SELECT "ListingNumber","Status","Currency","StartingPrice","BuyoutPrice","CurrentBid","PriceVector","BuyoutVector","CurrentBidVector","KindMask","BidCount","SaleFeeHeld","SaleFeeState","EndsAt"
FROM data."AuctionListing" ORDER BY "ListingNumber" DESC LIMIT 10;
-- SaleFeeState: 0 none, 1 held, 2 consumed (sold), 3 refunded (not sold)
-- Currency: 0 Zen, 1 Chaos, 2 Bless, 3 Soul, 4 Life, 5 Creation, 6 Guardian, 7 Harmony, 8 fruit basket (old form), 9 Mixed

-- bids: Status 0 current high bid (escrowed), 1 outbid/released, 2 won
SELECT "ListingId","BidderAccountId","Currency","ScalarAmount","AmountVector","Status","CreatedAt" FROM data."AuctionBid" ORDER BY "CreatedAt" DESC LIMIT 10;

-- what the server owes people (the Mailbox)
-- Kind: 0 purchased item, 1 returned item, 2 sale proceeds, 3 outbid refund, 4 tender change, 5 admin refund, 6 listing tax refund
-- Status: 0 pending, 1 partially claimed, 2 claimed
SELECT "CollectionNumber","AccountId","Kind","Currency","Status","OriginalScalar","OriginalVector","RemainingScalar","RemainingVector","ItemId"
FROM data."AuctionCollection" ORDER BY "CollectionNumber" DESC LIMIT 20;

-- the ledger: every unit that moved
-- Kind: 0 taken as payment, 1 change given back, 2 escrowed or paid for a bid/buyout, 5 fee taken, 6 collected from the Mailbox
SELECT "Kind","Currency","FruitComponent","Amount","AccountId","ListingId","CreatedAt" FROM data."AuctionCurrencyMovement" ORDER BY "CreatedAt" DESC LIMIT 30;
```

**The basket text format.** A vector is 13 comma-separated whole numbers in this fixed order: `Zen, Chaos, Bless, Soul, Life, Creation, Guardian, Harmony, StrengthFruit, AgilityFruit, VitalityFruit, EnergyFruit, CommandFruit`. So `50000,0,0,3,0,0,0,0,0,0,0,0,0` is 50,000 Zen plus 3 Jewels of Soul. `KindMask` has bit *n* set for every kind *n* in the price: Zen and Soul give 1 + 8 = 9.

**Making a listing end now (test database only):** `UPDATE data."AuctionListing" SET "EndsAt" = now() - interval '1 minute' WHERE "ListingNumber" = <number> AND "Status" = 0;` The settlement worker runs every `ExpiryIntervalSeconds` (15 by default) and then sells the listing to its high bidder or returns it.

### 6.3 Scenarios

For each scenario report **OK** or **BROKEN** with what you saw (screen text, numbers, SQL rows).

**T1. Window and Sell tab basics.** Log in as S in town, press A. Four tabs appear (Browse, Sell, My Listings, Mailbox). Open Sell. Expect: the window is the same size as Browse and docks beside the inventory; the inventory becomes selection-only (clicking an item selects it, nothing can be dragged out). Left rail: an item card and a card titled **Listing tax** showing **100,000 Zen** and the text that it is held now, returned to the Mailbox if the item does not sell, kept when it sells, "No other fees". There must be no percentage fee text. The form has two mode buttons (Auction / Buy Now), a header row ("Currency", "Starting amount", "Buyout (optional)" in Auction mode), one price row, a "+ Add currency" button, Duration, Note, Clear and Create Listing.

**T2. Single-currency auction (the unchanged old path).** As S select an item. Auction mode, one row: Jewel of Soul, starting amount 5, buyout 8. Create. Expect the message "Listing created." (and then the one-time disconnect). After logging back in: S's Zen is down by **exactly 100,000** and the item is gone from the inventory. SQL: one new row with `Status` 0, `Currency` 3, `StartingPrice` 5, `BuyoutPrice` 8, `SaleFeeHeld` 100000, `SaleFeeState` 1, `PriceVector` null.

**T3. Mixed auction (new).** As S select another item. Auction mode; row 1 Zen 50,000; press "+ Add currency", and set row 2 to Jewel of Soul 3 (a new row starts on the first currency no other row uses; picking a currency another row already has swaps the two). Fill the buyout column: Zen 80,000 and Soul 5. Create. Expect success and again exactly 100,000 Zen taken. SQL: `Currency` 9, `PriceVector` `50000,0,0,3,0,0,0,0,0,0,0,0,0`, `BuyoutVector` `80000,0,0,5,0,0,0,0,0,0,0,0,0`, `KindMask` 9, `SaleFeeState` 1.
Form checks (no listing is created): a buyout in only one of the two rows shows "Set a buyout for every currency, or leave them all empty."; a buyout below its starting amount shows "A buyout cannot be below its starting amount."; an amount of 0 or an empty amount keeps Create disabled; with five rows the "+ Add currency" button disappears; the "x" box removes a row.

**T4. Browse and filters.** As B open Browse. Both listings appear; the mixed one shows one chip per currency (Zen and Soul icons with amounts) in the price column and in the buyout column. Currency filter = Jewel of Soul shows **both** listings; Jewel of Bless shows neither; Zen shows only the mixed one. With Soul selected, clicking the Price header to sort makes the mixed listing disappear (there is no price order between currencies). Hovering a row's item shows the normal item tooltip.

**T5. First bid on the mixed listing.** As B click the mixed listing. The detail view shows a **Your bid** card with one field per currency of the starting price (Zen, Soul), prefilled with 50,000 and 3, and a line "Keep every currency and raise at least one...". Change Soul to 2: the Bid button turns grey (below the starting price). Put it back to 3 and press Bid. Expect "Bid placed." SQL: listing `BidCount` 1, `CurrentBidVector` `50000,0,0,3,...`; one `AuctionBid` row `Status` 0 with the same `AmountVector`. B's inventory after relogin: Zen **-50,000**, Soul **-3**. Ledger rows exist for Zen and for Soul.

**T6. Being outbid (the rule).** As C open the same listing. The fields are prefilled with the smallest valid raise: 50,000 Zen and **4** Soul. Check that setting Soul back to 3 (equal to the current bid) or Zen to 40,000 with Soul 9 (Zen lowered) keeps the Bid button grey. Bid 50,000 + 4. Expect success. Then as B open the Mailbox: one entry **Outbid refund** holding 50,000 Zen + 3 Soul as ONE entry. Open it: the card shows both currencies and there is no per-currency quantity field (a multi-currency entry is collected whole). Press Collect: B gets exactly 50,000 Zen and 3 Jewels of Soul back (as loose jewels). SQL: the old `AuctionBid` has `Status` 1, the collection `Status` 2 and `RemainingVector` all zeros.

**T7. Not enough of one currency.** Give a player 1 Jewel of Soul but plenty of Zen, and have them place the 3-Soul bid on some mixed listing. Expect a refusal message (they cannot afford it) and **nothing taken at all**: Zen and Soul unchanged, no new bid row.

**T8. Buyout after bidding.** As C (the high bidder with 50,000 Zen + 4 Soul escrowed) press Buyout on the mixed listing (buyout 80,000 Zen + 5 Soul). C pays only the difference: **30,000 Zen + 1 Soul**. Expect success. SQL: listing `Status` 1, `SaleFeeState` 2 (kept), `CurrentBidVector` = the buyout vector. S's Mailbox has ONE **Sale proceeds** entry of 80,000 Zen + 5 Soul; C's Mailbox has the **Purchased item**; collecting both works (Zen and jewels appear in S's inventory, the item in C's). S's tax is not returned.

**T9. Buy Now.** As S sell an item in **Buy Now** mode with 2 or 3 currencies (for example 20,000 Zen + 2 Jewel of Bless + 1 Fruit of Energy). Create (tax taken as usual). As B in Browse the row's price column says **Buy now** and the buyout column shows the chips; opening it shows "Buy now only - no bids", the Bid button is grey, and Buyout works: B pays all three components at once (a missing component refuses the whole purchase and takes nothing). S gets one proceeds entry with all three currencies.

**T10. A listing that ends with a bid.** Create a mixed auction, let B bid, then force the end with the SQL in 6.2. Within about 15 seconds: `Status` 1, B has a **Purchased item** entry, S has **Sale proceeds** equal to B's bid vector, `SaleFeeState` 2.

**T11. A listing that ends without a bid.** Create one, force the end. Expect `Status` 2; S's Mailbox gets **Returned item** and **Listing tax refund** (100,000 Zen); collecting the refund adds exactly 100,000 Zen; `SaleFeeState` 3.

**T12. Cancel.** In My Listings open a listing with no bids and press Cancel Listing (confirm in the dialog). Expect the same two Mailbox entries as T11 and `Status` 3. A listing that has a bid cannot be cancelled (the button is grey, or the server says it has bids).

**T13. Cannot afford the tax.** Give S less than 100,000 Zen and try to list: refused with a "cannot afford" message, the item stays in the inventory, Zen unchanged.

**T14. Collecting a basket with no room.** Fill every one of B's 64 main-inventory slots, then try to collect a multi-currency entry (for example from T6). Expect an "inventory full" message, the entry stays pending, and B's Zen did **not** change (the whole claim is undone). Free a slot or two and collect again: it works.

**T15. The switches.** In `AuctionHouse.ini` set `MixedPricesEnabled=false`, wait a few seconds, close and reopen the window (the setting is sent when the window opens). Try to create a Buy Now or a two-currency auction: the Sell tab says mixed prices are not available. Existing mixed listings can still be bid on, bought out and collected. Set it back to `true`. Optionally set `SaleFeeAmount=0`: listing then costs nothing and the left card falls back to the old fee text; restore 100000.

**T16. Look and feel.** Report anything that looks wrong: overlapping text, clipped labels, misaligned fields, a price of five currencies not fitting its cell (it should wrap to two lines), hover and pressed states, dropdown lists drawn under item icons, the flyout of the category rail (click Armor: All Armor, Helms, Armors, Pants, Gloves, Boots, Shields; Escape or an outside click closes it; Reset clears it), the nine rail icons (sword, helm, wing, paw, gem, flask, scroll with orb, chest), and the frame rate (the Release build must be smooth with no debug panel or red graph in the corner). Describe each problem precisely (which tab, which control, what you expected).

Known limit of this client build, so do not report it as a bug: the bid card only offers the currencies the listing already has. The server also accepts a bid that **adds** a currency, but this client has no control for it yet.

## 7. If something goes wrong

| Symptom | Likely cause and what to do |
|---|---|
| Server: migration error on start | Stop. Copy the exact error. Do not edit the database. Check you are on the throwaway copy and that the backup exists. |
| Client compile error mentioning `StartingBasket`, `BidBasket`, `SubCategory`, or "too few arguments" for `SendAuctionCreateRequest` / `SendAuctionBidRequest` | The packet package is stale or the old cache was used. Redo 5.6: rebuild then pack all three projects, verify the DLL strings, delete `<client>\.nuget\munique.openmu.network.packets\0.9.13`, set `NUGET_PACKAGES`, restore with `--force`, build again. |
| `git status` shows `src/source/Dotnet/PacketFunctions_*` modified after the build | The package differs from the committed bindings. Report the file list and the output of `git diff --stat`. |
| NativeAOT link: "exited with code 123" | `vswhere.exe` is not on PATH. Add `C:\Program Files (x86)\Microsoft Visual Studio\Installer` before the build (5.7). |
| "file in use" / "access denied" in `bin`, `obj` or `out` | A process still runs from that folder (stop `Main.exe` / `MUnique.OpenMU.Startup.exe`), or OneDrive is syncing the folder. |
| Browse is empty, or Sell/Bid/Buyout do nothing | `Enabled=true` missing in `AuctionHouse.ini`; or the server was not restarted after the update; or the character is outside a safe zone. |
| "Your character level is too low" / "Your account is too new" | `MinimumCharacterLevel` / `MinimumAccountAgeHours` in the ini (5.5). |
| Sell says mixed prices are not available | `MixedPricesEnabled` is false or missing in the ini; reopen the window after fixing. |
| PostgreSQL tests all "skipped" | `AUCTION_TEST_CONNECTION_STRING` not set, or the database has fewer than three characters. |
| A test that passed on the owner's machine fails here | Report the full output. Do not alter the test. |
| Client closes right after a successful action | Expected (section 3.8): log in again. |

## 8. What to report back

Write one report, in plain text, with these parts:

1. **Starting state**: for each repository the path, plain clone or worktree, the commit and branch it started on, whether the tree was clean, any local commits or stashes you made or found, which of the three versions in section 2 it was.
2. **Updated state**: the short commit hash of each repository after the update (expected `29e189c72` server and `02275d07` client, or later), and whether `--ff-only` worked.
3. **Environment**: .NET SDK versions, Visual Studio edition, how `VCPKG_ROOT` was resolved, the PostgreSQL version, the name of the test database (not the password), where the backup dump is.
4. **Build results**: server build, package verification (which strings were found), client build; any warnings that look new.
5. **Test results**: totals for the three test commands (passed / failed / skipped).
6. **Manual results**: T1 to T16, each OK or BROKEN with evidence (exact on-screen text, the SQL rows or numbers, screenshots if you can take them).
7. **Problems**: for each, the exact message, the steps to reproduce, what you already checked, and (optional) a suggested fix as a diff that you did **not** apply.
8. **Leftovers**: anything you changed outside git (ini edits, database rows created, files created) so it can be cleaned up.

## 9. Map of the code (for diagnosis; read-only)

**Server repository** (`OpenMU`):

- `src/Persistence/AuctionHouse/` pure rules and data types: `PriceBasket`, `PriceRules` (the bidding rule), `PriceLimits`, `JewelPaymentPlanner`, `AuctionTaxonomy` (item to category), the draft and result records, `IAuctionHouseRepository`.
- `src/Persistence/EntityFramework/AuctionHouse/AuctionHouseRepository.cs` is the original single-currency engine; `AuctionHouseRepository.Mixed.cs` holds the mixed-currency bid, buyout, basket tender, basket claim and basket materialisation. Every operation is one database transaction with a version check on the listing.
- `src/DataModel/Entities/Auction*.cs` the entities; `src/Persistence/EntityFramework/Migrations/` the migrations.
- `src/GameLogic/AuctionHouse/`: `AuctionHouseService` (validates a request, then calls the repository), `AuctionHouseConfiguration` and `AuctionHouseIni` (the settings and the ini file), `AuctionExpiryPlugIn` (the settlement and maintenance worker), `AuctionMutationEligibility` (safe-zone and state checks), `AuctionRateLimiter`.
- `src/GameServer/MessageHandler/AuctionHouse/` one handler per packet (open, browse, detail, create, bid, buyout, cancel, my listings, mailbox, collect).
- `src/Network/Packets/ClientToServer/ClientToServerPackets.xml`, `ServerToClient/ServerToClientPackets.xml` and `CommonEnums.xml` are the packet definitions (the single source of truth; the C# classes and `docs/Packets` are generated from them).
- Tests: `tests/MUnique.OpenMU.Tests/AuctionHouse/` (for example `AuctionMixedTests.cs`, `AuctionSaleFeeTests.cs`, `PriceBasketTests.cs`) and `tests/MUnique.OpenMU.Network.Packets.Tests/AuctionHousePacketDefinitionTests.cs`.

**Client repository** (`MuMain`):

- `src/source/UI/NewUI/AuctionHouse/`: `NewUIAuctionWindow.cpp/.h` (tabs, Browse, detail, bid card, Mailbox, sending requests), `NewUIAuctionSellPanel.cpp/.h` (the Sell form), `AuctionModel.*` (pure, unit-tested logic: price components, sell-draft validation, create-field mapping, the bid rule), `AuctionWireResponses.*` (hand-written parsers for the server's responses), `AuctionTaxonomy.*` (category rail and flyout), `AuctionChrome.*` (the shared look).
- `src/source/Dotnet/PacketFunctions_*` are the generated send functions and enums; `ClientLibrary/` is the NativeAOT networking library, and its project file pins the packet package version.
- Tests: `tests/ui/test_auction_*.cpp` and `tests/network/binding_generator_self_check`.
- Notes for humans: `docs/auction-house-claude-handover-2026-10-13.md` (with a 2026-10-14 section) and `docs/superpowers/plans/2026-10-14-auction-mixed-prices.md` (design decisions and rulings).

**Wire sizes added in this build** (little-endian; a *basket* is 13 unsigned 32-bit numbers, 52 bytes, in the kind order of section 6.2):

- create request 256 bytes (adds `StartingBasket` at 152 and `BuyoutBasket` at 204);
- bid request 108 (adds `BidBasket` at 56);
- each Browse / My Listings row 199 (adds `CurrentBasket` at 95 and `BuyoutBasket` at 147);
- detail response 360 (adds `StartingBasket` at 204, `CurrentBasket` at 256, `BuyoutBasket` at 308);
- each Mailbox row 192 (adds `OriginalBasket` at 88 and `RemainingBasket` at 140);
- the open response (configuration version 5) carries `SaleFee` at 28 and `MixedPricesEnabled` at 32 before the currency list, which now starts at 33.

## 10. Glossary

- **Basket / vector**: a quantity for each of the 13 price kinds. Used for prices, bids, refunds and proceeds of Mixed listings.
- **Collection / Mailbox entry**: a durable record of something the server owes an account (an item, Zen, jewels, fruits). Claimed by the owning account only.
- **Escrow**: the bid amount held by the server while a bid is the high bid.
- **Tender**: paying a price out of the inventory. **Tender change**: overpayment with a jewel pack, returned as a Mailbox entry.
- **Listing tax / sale fee**: the flat 100,000 Zen held per listing.
- **Mixed listing**: stored with currency 9; any Buy Now, and any auction in two or more currencies.
- **Safe zone**: a town area where the game's fighting rules are off; the server only accepts Auction House changes from a character standing in one.
- **Local NuGet feed**: a folder used as a package source, here holding the hand-packed 0.9.13 packages.

When you are done, send me the report from section 8. If anything blocked you, say exactly where you stopped and why.
