# Prompt for Codex: set up the latest Auction House build (server + client) on this laptop

Copy everything below the line into Codex, in the folder that holds your OpenMU server clone and MuMain client clone.

---

You are setting up the latest Auction House build of a MU Online Season 6 private server (OpenMU, C#) and client (MuMain, C++), so I can run and test it locally on this Windows laptop. I already ran an earlier version here, so the toolchain, a PostgreSQL server and the two clones should already exist. Work step by step, run each command, read its output, and stop and tell me if something fails instead of guessing.

## Ground rules

- Use a **throwaway test database only** (for example `openmu_auction_test`). Never point this at a real or production database. If the existing database is one I care about, copy it first or create a fresh one.
- Do **not** push, force-push, merge, rebase or open pull requests. Use `git fetch` and `git pull --ff-only` only. If a pull is not a fast-forward, stop and tell me.
- Do **not** commit anything. Local edits (for example `nuget.config`) stay local; run `git update-index --assume-unchanged <file>` on them so they never show up in a commit.
- Keep my existing server connection settings and my existing game account and characters.

## 1. Get the latest code

Both repositories use the branch `codex/auction-house`:

- Server: `https://github.com/rauseklis/OpenMU.git`
- Client: `https://github.com/rauseklis/MuMain.git`

In each clone: `git fetch origin`, `git checkout codex/auction-house`, `git pull --ff-only origin codex/auction-house`. Report the short commit hash of each. If my clones are git worktrees (for example `server/.worktrees/auction-house` and `client/.worktrees/auction-house`), work in those folders.

## 2. Server

Requirements: .NET 10 SDK and PostgreSQL.

1. From the server repo root: `dotnet build src/Startup/MUnique.OpenMU.Startup.csproj -c Release`.
2. Stop any running `MUnique.OpenMU.Startup.exe` first. Then start `src\Startup\bin\Release\MUnique.OpenMU.Startup.exe` with that folder as the working directory.
3. On start the server applies new database migrations by itself. The latest one is `AddAuctionSubCategory` (adds a `SubCategory` column to `AuctionListing`). Expect the log lines "The database schema needs to be updated before the server can be started. Updating..." and "The database schema has been successfully updated". If it fails, show me the error.
4. Next to the exe the server creates `AuctionHouse.ini`. It lists every Auction House setting with a description and is re-read within seconds of saving. For testing, set `Enabled=true` (listing, bidding, buying, cancelling and collecting only work when it is true). Leave the other settings alone.
5. A background task re-classifies old listings into the new sub-categories a few seconds after start. There should be no "Auction House expiry batch failed" line in the log.

## 3. Client packet packages (important)

The client does not read the server code directly. It builds its network layer from the NuGet package `MUnique.OpenMU.Network.Packets` **version 0.9.12**, which is packed by hand from the server repo into a local folder (a local NuGet feed). That version is not on nuget.org. Do this:

1. Pick a folder for the feed, for example `C:\Users\<me>\.local-nuget\auction-house`.
2. From the **server** repo root, pack the three packages as 0.9.12. The packets package must be rebuilt first and then packed without building, otherwise a stale DLL can end up in the package:
   - `dotnet pack src/Network/MUnique.OpenMU.Network.csproj -c Release -p:Version=0.9.12 -p:PackageVersion=0.9.12 -o <feed>`
   - `dotnet pack src/PlugIns/MUnique.OpenMU.PlugIns.csproj -c Release -p:Version=0.9.12 -p:PackageVersion=0.9.12 -o <feed>`
   - `dotnet build src/Network/Packets/MUnique.OpenMU.Network.Packets.csproj -c Release -t:Rebuild -p:Version=0.9.12 -p:PackageVersion=0.9.12`
   - `dotnet pack src/Network/Packets/MUnique.OpenMU.Network.Packets.csproj -c Release --no-build -p:Version=0.9.12 -p:PackageVersion=0.9.12 -o <feed>`
3. Check the packed DLL really has the new members. In Python, open `MUnique.OpenMU.Network.Packets.0.9.12.nupkg` as a zip, read `lib/net10.0/MUnique.OpenMU.Network.Packets.dll`, and confirm the bytes `SubCategory` and `ScrollOrb` both occur in it. If either is missing, repeat the rebuild-then-pack step.
4. In the **client** repo, `nuget.config` has a source `local-auction-house` whose path is on the other developer's machine. Change that path to your `<feed>` folder, then `git update-index --assume-unchanged nuget.config`.
5. If the folder `<client>\.nuget\munique.openmu.network.packets\0.9.12` already exists, delete it so the restore extracts the fresh package. Then run `dotnet restore ClientLibrary/MUnique.Client.Library.csproj --force`.

## 4. Client build

Requirements: Visual Studio 2022 with the C++ x86 build tools and Windows SDK (it includes CMake, Ninja and vcpkg), the .NET 10 SDK, and internet access for the first NativeAOT and vcpkg restore.

1. Open a command prompt (not PowerShell-only tools) and run, in this order:
   `set "PATH=%PATH%;C:\Program Files (x86)\Microsoft Visual Studio\Installer"` (the NativeAOT link step needs `vswhere.exe` on PATH), then `"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x86` (adjust the edition folder if needed).
2. In the client repo root, configure once if `out\build\windows-x86` does not exist: `cmake --preset windows-x86`.
3. Build the optimized client: `cd out\build\windows-x86` then `ninja -f build-Release.ninja Main`. The first build takes a while. If the `MUnique.Client.Library.dll` link step fails with "exited with code 123" and a line about `vswhere.exe`, the PATH step above was skipped.
4. Build the **Release** configuration only. The Debug build shows a large FPS/debug panel and runs at roughly a third of the speed.
5. The program is `out\build\windows-x86\src\Release\Main.exe`. Start it with its own folder as the working directory. Make sure it connects to the local server I just started (keep my existing connection settings).

## 5. What to test and report

Log in with a character that has some items, jewels and Zen, and open the Auction House window (the same way as in the earlier version). Check and report each line as OK or broken, with what you saw:

1. **Browse category rail.** Nine rows: All Categories, Weapon, Armor, Wing, Pet & Helper, Jewel & Material, Consumable, Scrolls & Orbs, Miscellaneous. Each row except the first has a small icon (sword, helmet, wing, paw, gem, flask, scroll with orb, chest) and no ">" arrow.
2. **Sub-category flyout.** Click Armor: a list opens beside the rail with "All Armor", Helms, Armors, Pants, Gloves, Boots, Shields. Pick Gloves: only gloves are listed, the Armor row now reads "Gloves", and paging keeps the filter. Click outside or press Escape: the flyout closes without changing anything. Check the same for Weapon, Wing, Pet & Helper, Jewel & Material, Consumable and Scrolls & Orbs. Miscellaneous and All Categories select directly. The Reset button clears the category and the sub-category. While the flyout is open, table item icons must not show through it.
3. **Re-classification.** Listings created before this update appear under the right sub-category (for example a sword under Weapon, Swords).
4. **Sell tab.** It is the same size as Browse and docked beside the inventory. Items are chosen by clicking them in the inventory; nothing can be dragged out while the Sell tab is open. Auction mode lists an item; Buy Now shows a "not available on this server yet" notice.
5. **Tooltips.** Hovering Dark Horse / Dark Raven rows in Browse shows the normal item tooltip.
6. **Mailbox and My Listings** open without errors, and their dropdown lists are not overprinted by item icons.
7. Performance: the Release build should run smoothly, with no debug panel or red graph in the corner.

## 6. If something goes wrong

- Server log shows a migration error: stop, copy the exact error, do not edit the database by hand.
- Client compile error about `SubCategory`, or `SendAuctionBrowseRequest` taking the wrong number of arguments: the packages are stale. Redo section 3 (rebuild then pack, verify, delete `.nuget\...\0.9.12`, restore).
- NativeAOT link "exited with code 123": add the Visual Studio `Installer` folder to PATH (section 4, step 1) and rebuild.
- Browse shows nothing: check `Enabled` in `AuctionHouse.ini` and that the server was restarted after the update.

## 7. Known limits of this build

Not in this build yet (being built next): bidding with mixed currencies, the flat 100,000 Zen sale tax, and Buy Now listings. The three PostgreSQL integration tests for the sub-category filter and re-classification are skipped unless a disposable test database connection string is provided in `AUCTION_TEST_CONNECTION_STRING`; do not point it at a real database.

When you are done, give me a short report: commit hashes, which steps passed, anything that failed with the exact message, and what I should look at in the game.
