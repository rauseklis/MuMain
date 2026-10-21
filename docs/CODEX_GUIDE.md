# The complete guide to this project — for Codex (and for anyone starting from zero)

You are joining a project that is already running. This guide explains **everything**: what the project is, how the two repositories fit
together, how to set up a development machine, how to build, run and test, how a change reaches the live server, which rules the owner
set, and the traps other people already fell into. It assumes you know nothing about the project, but that you can read code, use git and
use a command line.

Read it top to bottom once. After that, [`docs/OPERATIONS.md`](OPERATIONS.md) is the short reference and the feature documents (index at
the end) describe single features.

> **No secrets are in this file or in any repository.** Passwords, SSH keys and the logins of the live server are on the owner's PC and in
> GitHub secrets. When this guide says "ask the owner", it means exactly that.

---

## 0. The ten facts you must not forget

1. This is a **private MU Online server and game client**, built by two brothers. The server is **OpenMU** (C#, .NET 10, PostgreSQL); the
   client is **MuMain** (C++, CMake, plus a .NET NativeAOT network library).
2. There are **two GitHub repositories**: the server `rauseklis/OpenMU` (default branch **`master`**) and the client `rauseklis/MuMain`
   (default branch **`main`**). They are forks of `MUnique/OpenMU` and `sven-n/MuMain`.
3. They are tied together by the **network protocol**, defined in XML in the server repository. A change to the packets must be merged to
   **both** repositories together.
4. A **live server runs 24/7** on a Contabo VPS (`109.199.104.230`). **Every merge to `master` of the server repository deploys to it within
   minutes** and disconnects the players for about a minute.
5. The owner allows any assistant session to push, open pull requests, merge and so deploy — **but test first and be honest about what you
   tested.**
6. **Never touch the owner's real local server or database** (the database named `openmu` on their PC). Test with a throwaway database.
7. **Do not control the game window** (no automated clicks or key presses into it).
8. **Do not change GitHub settings or secrets, or the live server's configuration (firewall, SSH, accounts, `.env`), without showing the owner
   first.**
9. **Never put passwords or keys in chat, in commits or in files in the repositories.**
10. When in doubt, read the code and the documents named in this guide, then ask the owner a short, concrete question.

---

## 1. The project and the people

- **Goal:** a MU Online server (Season 6 data) with custom features, played and tested by the owner and their brother together.
- **Custom features already built:** the **Auction House** (a full auction window and server), ten **vaults** per account, the **Jewel Bank**
  (a per-account store for jewels, fruits and jewel bundles, with auto-deposit on pickup), the **Jewel Rain** drop event (a scheduled event
  that drops jewels over a town), king maces, Lorencia lost maps, and a **live server** with automatic deployment.
- **People:** the owner (does most of the work with Claude) and their brother (works with Codex). Both have write access to both
  repositories. There is no code review process other than the pull request and the tests, so tests and care matter.
- **Other automated sessions** (Claude sessions) may be working at the same time. Check `git log` and open pull requests before you start,
  and keep your changes small and focused.

## 2. The repositories and where things are

Both repositories are normal git repositories. (The owner keeps them as two folders, `client` and `server`, inside one local meta-folder
called `mu-online-openmu`; that is only how they organise their disk. You can clone each repository anywhere.)

### 2.1 Server — `rauseklis/OpenMU`

```
src/
  Startup/            the program that starts everything (all-in-one: connect server, game servers, chat, admin panel). Dockerfile here.
  GameLogic/          the game rules: players, items, skills, monsters, plug-ins, chat commands, events
    AuctionHouse/  JewelBank/  PlugIns/ (chat commands, invasion events, JewelRain, ...)  PlayerActions/  Views/  ...
  GameServer/         the game server: network message handlers (MessageHandler/), the views that send packets to clients (RemoteView/)
  DataModel/          the entities and configuration (items, monsters, maps, accounts, characters)
  Persistence/        saving to PostgreSQL (EntityFramework/), plus the initial game data (Initialization/)
  Network/            the network library; Network/Packets/ holds the packet XML definitions (the protocol)
  PlugIns/            the plug-in system (attributes such as [PlugIn], [Guid])
  ConnectServer/ LoginServer/ ChatServer/ FriendServer/ GuildServer/  the other servers
  Web/                the admin panel (Blazor) and public API
tests/                MUnique.OpenMU.Tests (most tests, some need PostgreSQL) and MUnique.OpenMU.Network.Packets.Tests
deploy/live/          the live server: docker-compose.yml, backup and deploy scripts, setup guide (README.md)
docs/  docs-website/  documentation (the website is built from docs-website/)
.github/workflows/    CI and the automatic deployment
```

Important mechanisms:

- **Plug-ins.** Most server features are plug-ins: a class with `[PlugIn]` and a unique `[Guid("...")]` that the plug-in manager finds by
  itself. Packet handlers are plug-ins with `[BelongsToGroup(...)]` of a group handler (`GroupPacketHandlerPlugIn`). Chat commands are plug-ins
  (`/item`, `/move`, `/jewelrain`, ...). Copy an existing one (for example the Jewel Bank handlers in `GameServer/MessageHandler/JewelBank/`).
- **Packets.** Defined in `src/Network/Packets/**/*Packets.xml`. A build generates C# structs and documents (`docs/Packets/*.md`) from them. See
  section 8.
- **Persistence.** Entities are saved with EntityFramework. The Auction House and Jewel Bank use **plain SQL with their own tables created by a
  raw-SQL migration** (`src/Persistence/EntityFramework/Migrations/`). New tables are created automatically when the server starts.
- **Configuration data** (items, maps, drops) lives in the database, edited in the admin panel.

### 2.2 Client — `rauseklis/MuMain`

```
src/source/            the game client code (C++)
  UI/NewUI/            the windows: AuctionHouse/  JewelBank/  Inventory/  Dialogs/  HUD/ ...  (INTERFACE_* ids in Core/Globals/_enum.h)
  Network/Server/WSclient.cpp   receives and dispatches the server's packets (one big switch on the packet code)
  Engine/ Render/ World/ Character/ ...   the engine
  Dotnet/              C++ wrappers GENERATED from the packets (do not edit by hand)
ClientLibrary/         the .NET network library (NativeAOT) that sends and receives packets
nuget-feed/            the three packet packages the client is built from (see section 8)
nuget.config           makes nuget-feed a package source
src/bin/Data, fonts    the game assets (2.5 GB). Not in the downloadable bundle.
tests/                 unit tests (doctest); tests/ui has the Jewel Bank model tests
docs/                  documentation: CODING_RULES.md (read it), build/, feature handovers, specs
.github/workflows/     CI: ci.yml (builds the bundle), fork-ci.yml (extra builds)
```

Key facts about the client:

- **Coding rules:** `docs/CODING_RULES.md` applies to every change (also by assistants). `AGENTS.md` points to it.
- **`config.ini`** next to `Main.exe` decides which server the client connects to (`ServerIP`, `ServerPort`). **The client ignores the `/p`
  command-line argument**; it reads `config.ini`.
- The client's windows are drawn by custom code (not a UI toolkit). New windows borrow textures and widgets from existing ones (the Auction
  House frame is reused by the Jewel Bank window).

## 3. What you need from the owner

Verify GitHub access as described in section 4. The owner may need the following for manual play or administration:

| You need | Why | How it is handled |
| --- | --- | --- |
| A **game account** on the live server | for the owner to play and test manually | The owner creates or selects an account and enters its credentials themselves. Assistants do not request the password, control the game, or log in. |
| Your **SSH public key added to the live server** | to open the admin panel and read logs (optional) | You create a key (`ssh-keygen -t ed25519`), send the `.pub` file's single line; the owner (or Claude) adds it to `/home/deploy/.ssh/authorized_keys`. |
| The **game's `Data` folder** | the client needs the assets | It is in the client repository (`src/bin/Data`, 2.5 GB); a normal clone has it. |

The Contabo account (billing, the server's control panel) belongs to the owner and is protected with two-factor login. You do not need it.

## 4. Setting up your machine (Windows 10/11)

Install (PowerShell, with `winget`; reboot or open a new terminal afterwards):

```powershell
winget install Git.Git
winget install GitHub.cli
winget install Microsoft.DotNet.SDK.10
winget install Python.Python.3.12
winget install OpenJS.NodeJS.LTS
winget install PostgreSQL.PostgreSQL.17      # remember the password you set for the "postgres" user
winget install Microsoft.VisualStudio.2022.Community   # then add the workload "Desktop development with C++" (it brings MSVC, CMake and Ninja)
```

Then:

- **vcpkg** (the client's C++ libraries): `git clone https://github.com/microsoft/vcpkg C:\vcpkg` and `C:\vcpkg\bootstrap-vcpkg.bat`, then set
  `VCPKG_ROOT=C:\vcpkg` (the Visual Studio copy also works). The CMake presets find it through `VCPKG_ROOT`.
- **GitHub:** `gh auth login` (HTTPS, your own account).
- Optional: **Docker Desktop**, only if you want to run the server's Docker image locally.

Clone (the client has a git submodule, SDL):

```powershell
git clone --recurse-submodules https://github.com/rauseklis/MuMain.git client
git clone https://github.com/rauseklis/OpenMU.git server
```

**Work in worktrees.** Do not work on `main`/`master` directly. For a server task or a docs-only task, create a separate worktree with
`git worktree add .worktrees/<name> -b <branch> origin/<main|master>` and remove it after its branch is merged. For client tasks needing a
build, reuse an available long-lived build worktree as described in section 6; keep that worktree after the task. Never reuse one another
session is using. Initialize submodules inside a new client worktree with `git submodule update --init --recursive`.

**Verify access before shipping.** Check the active CLI account and both repositories:

```powershell
gh auth status
gh api user --jq .login
gh api repos/rauseklis/OpenMU --jq .permissions
gh api repos/rauseklis/MuMain --jq .permissions
```

The active account must be yours (for example `AuseklisR`), and both permission objects must contain `"push": true`.
Do not print an authentication token or paste unredacted credentials into reports. A permission flag alone does not prove Git transport
works: when requested, create a uniquely named throwaway branch in your own worktree, push it, delete that remote branch, then remove only
that probe's local branch and worktree. Do not merge it or change repository settings. Record success or the actual failure for each repository.

Line endings: the repositories mix LF and CRLF. Keep each file's own line endings when you edit it (an editor that converts them creates huge
diffs). Git may print "LF will be replaced by CRLF" warnings; they are harmless.

## 5. Build, run and test the server locally

```powershell
cd server
dotnet restore
# A clean checkout needs this executable before the persistence projects run their pre-build steps.
dotnet build src/Persistence/SourceGenerator/MUnique.OpenMU.Persistence.SourceGenerator.csproj -c Release
dotnet build src/Startup/MUnique.OpenMU.Startup.csproj -c Release
```

**Database: configure isolation before running anything.** The server creates the configured database and tables itself. The checked-in
`ConnectionSettings.xml` targets `openmu`; do not run it unchanged on a machine with the owner's real database. `DB_HOST`, `DB_ADMIN_USER`, and
`DB_ADMIN_PW` override the host/admin credentials, **not the database name or port**. A different admin-panel port does not isolate a database.

Prefer a separate disposable PostgreSQL cluster on a dedicated loopback port. After building, edit only the ignored runtime copy at
`src/Startup/bin/Release/ConnectionSettings.xml`: point **every connection/context**, including admin authentication and the restricted roles,
to the disposable cluster and a unique test database. Keep the key spelling `User Id=...;` because the migration role parser expects it.
Verify all connection targets before startup or `-reinit`; rebuilding may copy the template again, so verify again after builds. Never edit
the tracked template to contain real credentials. Supply required admin credentials privately through the process environment:

```powershell
$env:DB_ADMIN_USER = 'postgres'
$env:DB_ADMIN_PW   = '<your postgres password>'     # avoid ';' in the password
$env:ASPNETCORE_URLS = 'http://127.0.0.1:8090'               # the admin panel address (choose a free port)
dotnet run --no-build --no-launch-profile --project src/Startup/MUnique.OpenMU.Startup.csproj -c Release -- -autostart
```

- The first start **creates and fills the database** (a few minutes) with the default Season 6 data, including ~20 **default test accounts**
  (`test0` ... `test9`, `testgm`, `testgm2`, ... — see `docs-website/docs/getting-started/test-accounts.md`; on a local server their passwords are
  well known, that is fine here, **not** on a public server).
- Ports: connect server **44405** (GMO client definition) and **44406** (open-source client definition — use this one), game servers
  **55901–55906**, chat **55980**.
- **Use a throwaway database for development**: `-reinit` recreates it. Never copy, query, or modify the real local database as part of testing.
  To run a second server, prepare fresh disposable data or copy an existing throwaway database, shift its ports
  (`ConnectServerDefinition.ClientListenerPort`, `GameServerEndpoint.NetworkPort`, `ChatServerEndpoint.NetworkPort`), and use another
  `ASPNETCORE_URLS`. Fresh seed data enables automatic server startup: omitting `-autostart` does not prevent listeners from starting.
  Beside a real server, initialize the fixture separately and shift endpoints before launching it; do not run the unmodified first-start
  command against default game ports. Stop only processes you started, after verifying their executable paths.
- The server's console and log show almost nothing about players. To check what happened, query the database (for example the Jewel Bank tables
  `data."JewelBankBalance"` and `"JewelBankMovement"`).

**Tests.**

```powershell
dotnet test tests/MUnique.OpenMU.Tests/MUnique.OpenMU.Tests.csproj -c Release
dotnet test tests/MUnique.OpenMU.Network.Packets.Tests -c Release
```

Tests marked `PostgreSql` are skipped unless you set `AUCTION_TEST_CONNECTION_STRING` to a **throwaway** database that already has the
Auction House / Jewel Bank tables (start a server against it once). Never point it at a database you care about. The packet tests check
that the XML layouts stay as pinned.

## 6. Build, run and test the client locally

The presets are in `CMakePresets.json` (`windows-x86`, `windows-x64`, with `-mueditor` variants). From a **Developer PowerShell for VS**
(so `cl`, CMake and Ninja are on `PATH`):

```powershell
cd client
$env:VCPKG_ROOT = 'C:\vcpkg'
cmake --preset windows-x64 -DBUILD_TESTING=OFF          # or windows-x86 (32-bit); OFF avoids compiler out-of-memory on the big files
cmake --build --preset windows-x64-release
cd out\build\windows-x64\src\Release
.\Main.exe
```

- The first configure restores the NuGet packages (the packets, from `nuget-feed/`) and builds the C++ libraries with vcpkg: it takes a long time
  once, then it is cached. Both the .NET 10 SDK and the C++ workload are needed (the network library is compiled ahead of time).
- The output folder gets `Data`, `fonts` and a `config.ini` copied in; edit its `ServerIP` / `ServerPort` to choose the server
  (`localhost` + `44406` for your local server, `109.199.104.230` + `44406` for the live server).
- The CMake option `OPENMU_PACKETS_VERSION` follows `ClientLibrary/MUnique.Client.Library.csproj` (`OpenMuPacketsVersion`, now **0.9.20**).
  A build directory configured earlier may have the old value cached: delete its `CMakeCache.txt` or pass `-DOPENMU_PACKETS_VERSION=0.9.20`.
- **Unit tests:** configure with `-DBUILD_TESTING=ON`, build a test target (for example `jewel_bank_model_tests`) and run its exe from
  `out/build/<preset>/tests/...`. (With testing ON the whole client can fail to compile for lack of memory; build only the test target.)
- More: `docs/build/README.md` and `docs/build/windows/*.md`.

**Keep one long-lived worktree for client builds.** The first build in any new worktree compiles everything (about 820 steps, 10 to 20 minutes);
later builds in the same worktree are incremental and fast. So do not create a new client worktree for every task: keep one (the owner's is
`client\.worktrees\dev`), switch branches inside it (`git switch -c <branch> origin/main`), and put it back with `git switch --detach origin/main`
when the branch is merged. Only one session at a time may build in a worktree.

**To avoid building the client at all:** download the ready-made bundle from GitHub (section 7).

## 7. Playing on the live server

- **Server:** `109.199.104.230`, port **44406**. Needs a client built from the current `main` of the client repository (the same protocol).
- **Get a client:**
  1. In `rauseklis/MuMain` → **Actions** → the latest successful **CI** run on `main` → artifact
     `mu-client-windows-native-x64-release-editor-off-no-data-main` (about 260 MB; the program without the game's data). If no run exists, start one:
     Actions → CI → **Run workflow** on `main` (the Windows job takes 20–40 minutes the first time).
  2. Extract the artifact ZIP, then unpack its runtime `.tar.gz` if present. Put the complete runtime in a new folder outside Git
     repositories, then copy `Data` and `fonts` into it from `src/bin/` of the matching client checkout.
  3. In its `config.ini` set `ServerIP=109.199.104.230` and `ServerPort=44406`.
  4. The owner runs `Main.exe` from that folder and logs in manually. Assistants may prepare and verify files and connectivity, but must
     not control the game window or log in.
- **Updating the client after a merge is automated:** `scripts/update-live-client.ps1 -Target <client folder>` downloads the newest bundle that
  matches `main`, unpacks it and copies it over the client folder (keeping `config.ini`, `Data`, `fonts`); it waits for or starts the build if needed and
  refuses to touch a folder whose game is running. Run it yourself as the last step of every client change and then tell the person to restart `Main.exe`.
- **GM commands** (a game-master account): `/item <group> <number> [level]` drops an item at your feet (Jewel of Bless `14 13 0`; Fruit of Strength
  `13 15 3`), `/move <map>`, `/jewelrain` starts the Jewel Rain event; the list of commands is in the chat-commands plug-ins
  (`src/GameLogic/PlugIns/ChatCommands/`) and `docs/chat-commands.md` in the client repository.
- Jewel Bank window: **Ctrl+J**. Auction House: **A**. Skill tree (Master Level): **S**.
- The Auction House refuses accounts younger than 24 hours (`MinimumAccountAgeHours`); the owner set the registration date of the two game-master
  accounts back to get round it.
- Remember: on the live server **your actions are real for the other player**. Do not wipe or damage characters or the database.

### Assemble a standalone client from a local build

A locally built x86 or x64 Release client from this repository is also suitable; keep its runtime DLLs from the same build and architecture.
Use a new folder **outside all Git repositories**, for example `C:\OpenMU Live Client`. Copy the complete runnable build output
(`out/build/<preset>/src/Release`), including `Main.exe`, `MUnique.Client.Library.dll`, dependency DLLs, `shaders`, and the icon.
Copy `src/bin/Data` and `src/bin/fonts` from the matching client checkout into that folder as `Data` and `fonts`; do not copy just `Main.exe`.
Seed a clean `config.ini` from `src/bin/config.ini.template` so saved login credentials are not carried over, then set:

```ini
[CONNECTION SETTINGS]
ServerIP=109.199.104.230
ServerPort=44406
```

Run `Main.exe` with the standalone folder as its working directory. A shortcut's **Start in** field must point there. Leave other installed
clients and their configuration alone. Report the full executable path, source commit/build, and whether the executable, native network DLL,
runtime DLLs, `Data`, `fonts`, and shaders were verified. File checks and a successful build do not verify rendering or login: the owner
launches the game and signs in manually; assistants must not control the window or log in.

### Check the live connection without a game login

From the player's PC, open a TCP connection to each of `109.199.104.230:44406` and `109.199.104.230:55901`, wait up to five seconds for
connection and greeting data, and read at least four bytes (TCP may split them across reads). Close each connection without sending a login.
Report each port independently, including the received byte count or timeout/error. An open port alone is insufficient; neither greeting
proves account login or gameplay works. These checks are read-only and need no game credentials.

## 8. The protocol: how the client and server stay in step

The packets are defined in `server/src/Network/Packets/{ClientToServer,ServerToClient,...}/*.xml`. Each packet has a header type, code,
sub-code, length and fields. A build generates the server's C# structs and the packet documents. The **client** takes the same XML from three
NuGet packages packed from the server repository and generates its C++ wrappers (under `src/source/Dotnet/`) from them.

**When you change a packet** (add, remove or move a field):

1. Edit the XML in the server repository. Update `tests/MUnique.OpenMU.Network.Packets.Tests` if a layout is pinned there.
2. Raise `<Version>` and `<PackageVersion>` in the three project files `src/PlugIns/MUnique.OpenMU.PlugIns.csproj`,
   `src/Network/MUnique.OpenMU.Network.csproj`, `src/Network/Packets/MUnique.OpenMU.Network.Packets.csproj` to the next version (for example 0.9.21).
3. Pack them into the client repository's `nuget-feed/` folder (per project: `dotnet build <csproj> -c Release -t:Rebuild`, then
   `dotnet pack <csproj> -c Release --no-build -o <client>/nuget-feed`). Delete the old `.nupkg` files. `nuget-feed/README.md` has the details.
4. In the client set `OpenMuPacketsVersion` to the new version, build once so the C++ wrappers regenerate (`dotnet build
   ClientLibrary/MUnique.Client.Library.csproj -c Release -p:Platform=x86`, or the x64 platform), and commit the regenerated files.
5. Write the client code for the new packets (`WSclient.cpp` dispatch, the window).
6. **Merge the server and the client pull requests together** (server first). Merging only one breaks the connection between them.

**Encryption and packet size — a trap.** Requests from the client are encrypted (SimpleModulus: every 8 bytes become 11). A **C3** packet has
a one-byte length, so an encrypted request must be at most 255 bytes: that means the packet definition must be shorter than about **180 bytes**.
Longer requests must be **C4** packets (two-byte length, like `AuctionCreateRequest` and the Jewel Bank deposit/withdraw requests).
If you get it wrong, the server rejects the packet ("The packet has an unexpected content size. It must be a multiple of 11") and drops the
connection: the client reconnects and nothing happens. A test in the Jewel Bank packet tests guards this. Server-to-client packets longer than
255 bytes must be **C2**. For C2/C4 the sub-code is at byte 4, for C1/C3 at byte 3.

## 9. How to build a feature end to end (a checklist)

Use the Jewel Bank as the model (`docs/superpowers/specs/2026-10-20-jewel-bank-design.md` and `docs/jewel-bank-handover-2026-10-20.md`).

1. **Understand the goal** and write down the rules (what is allowed, limits, edge cases, what must never happen — duplication, loss, double
   submits). For a larger feature write a short design document under `docs/superpowers/specs/`.
2. **Server logic first, with tests.** Put the rules in `GameLogic/<Feature>/`, persistence in `Persistence/`, the handlers in
   `GameServer/MessageHandler/<Feature>/`. Make mutations idempotent (a client-made operation id) and transactional.
3. **Packets** (section 8) — keep them small; think about C3/C4 and C1/C2.
4. **Client:** keep pure logic (parsing, validation, amounts) in a separate file with doctest tests, as `JewelBankModel` does; keep the window
   code thin. The server is authoritative: the client sends requests and shows what the server says.
5. **Test for real:** start a throwaway server, run the client, try the feature, and **check the effect in the database**.
6. **Pull requests:** one concern per commit, messages in the style of `git log` (`feat(scope): ...`, `fix(scope): ...`), a description that says
   what changed, how it was tested and what was **not** tested. Docs: update the feature's document and, if it changes how things work here,
   `docs/OPERATIONS.md`.
7. **Merge** (server first, then client). Watch the deployment (section 10). Check that a login still works.

## 10. Git, GitHub and the automatic deployment

- **Branches:** short-lived, from `origin/master` (server) or `origin/main` (client). **Merge commits** through pull requests, as the history shows.
  After a merge delete your branch and worktree. Other people's branches and worktrees: leave them.
- **Server repository workflows** (`.github/workflows/`): `.NET Core` builds on every push; `deploy-live.yml` **builds the Docker image and deploys it
  to the live server on every merge to `master`** (documentation-only changes — `docs/**`, `docs-website/**`, `*.md` — do not deploy); the other files
  are upstream's and mostly do not matter here (the nuget.org publish only runs on upstream).
- **Client repository workflows:** `ci.yml` has quality gates, Linux and macOS builds, and the **Windows build that makes the downloadable
  bundle** (runs on pushes to `main` and can be started by hand); `fork-ci.yml` runs more builds on pull requests. A test
  (`tests/test_windows_workflow_contract.py`) pins the text of the Windows job: if you change that job, change the test with it.
  Pushes to `main` do not always start a run by themselves: start it by hand if none appears.
- **A deploy** = image build (~5 min) → copy `deploy/live/*` to the VPS → `deploy.sh` (database backup, pull, restart) → check the connect server. Only
  one deploy runs at a time. If it fails, the old version keeps running; read the failing step in the Actions tab.
- **Never** change repository settings or secrets without the owner (secrets: `DEPLOY_HOST`, `DEPLOY_USER`, `DEPLOY_SSH_KEY`,
  `DEPLOY_KNOWN_HOSTS`).

## 11. The live server in detail

- **Machine:** a Contabo Cloud VPS 4 (4 vCPU, 8 GB RAM, 100 GB SSD, Ubuntu 24.04, EU), about £8 a month, owner's account. It also has Contabo's
  daily Auto Backup snapshots.
- **On the machine** (`/opt/openmu`, owned by the user `deploy`): `docker-compose.yml`, `backup.sh`, `deploy.sh`, `.env` (generated passwords; never
  print it), `backups/`. Two containers: `openmu` (image `ghcr.io/rauseklis/openmu`) and `database` (PostgreSQL 17; data in the volume `openmu_dbdata`;
  not reachable from outside). Both restart automatically. Time zone: UTC.
- **Firewall:** `ufw` allows SSH and the game ports (44405-44406, 55901-55906, 55980) only. (Docker publishes its own ports; the admin panel is bound to
  `127.0.0.1:8080` on purpose.) SSH accepts keys only; fail2ban is running. Contabo's own firewall has no rules.
- **Admin panel:** not public. With your key on the server: `ssh -N -L 8080:127.0.0.1:8080 deploy@109.199.104.230`, then
  `http://localhost:8080`. Log in as `admin` (the password is in the server's `.env`; ask the owner). There you manage accounts, characters, game
  configuration and API keys (an API key is only for external apps such as a website; not needed).
- **Looking at it:** `ssh deploy@109.199.104.230`, then `cd /opt/openmu`, `docker compose ps`, `docker compose logs --tail 100 openmu`,
  `docker compose exec database psql -U postgres -d openmu`. The deploy user is in the `docker` group, so it can do all of this.
- **Accounts:** the 20 default test accounts were banned (known passwords); `testgm` and `testgm2` are active with random passwords. Make real accounts
  in the admin panel. The default ban states are recorded in `/opt/openmu/default-account-states.txt`.
- **Backups:** before every deploy and every night at 03:30 (cron of `deploy`), kept 14 days, on the server; copy them off it now and then.
  Restore steps: `deploy/live/README.md` in the server repository.
- **Rollback:** run the deploy workflow from an older commit (Actions → Deploy live server → Run workflow, on that commit), or on the server
  `./deploy.sh ghcr.io/rauseklis/openmu:<older commit>`. Database changes are not undone by that: restore a dump if needed.
- **Deployment checks (verify current live state):** event schedules use `SystemConfiguration.TimeZoneId`; an empty or invalid value falls
  back to UTC. For Latvian schedules, the intended value is `Europe/Riga`, independent of the container clock. Confirm any required
  `AuctionHouse.ini` mount and the brother's SSH access with the owner. Historical handover notes do not establish their current live state.
  Do not change live configuration or authorized keys without showing the owner and obtaining approval.

## 12. Rules for assistants and security

1. Push, open pull requests, merge and deploy are allowed (standing permission from the owner) — after you tested, and with an honest report.
2. Ask the owner before: changing GitHub settings or secrets; changing the live server's configuration; deleting the live database, backups or someone
   else's branches/worktrees; touching the owner's real local server or database.
3. Do not control the game window. Start and stop **your own** test processes only (check the process path before you stop anything).
4. Never print, commit or send passwords, keys, tokens or the Contabo API credentials. Use environment variables, files outside the repository, or
   GitHub secrets.
5. Use builds from the repository's own workflows or local builds from its trusted source. Do not download or run unrelated/untrusted launchers.
6. Say plainly what you did **not** test (for example "I could not run the game").
7. Do not make large unrelated clean-ups; follow the style of the code around you and `docs/CODING_RULES.md`.

## 13. Pitfalls we already fell into

| Symptom | Cause and fix |
| --- | --- |
| The client connects to a different server than you set | It reads `config.ini` (`ServerIP`, `ServerPort`); `/p` is ignored. Check with `Get-NetTCPConnection -OwningProcess <Main pid>`. |
| The client is disconnected after a minute at the login | The connect server drops idle clients after 60 s: log in right away. Use port **44406** (open-source client definition), not 44405 (GMO). |
| A request makes the client reconnect, nothing happens | A C3 request is too long: server log `unexpected content size ... multiple of 11`. Make it a C4 packet (section 8). |
| Client build: "wire-size codegen needs ... packets v0.9.10" | A build directory that cached the old version: delete `CMakeCache.txt` or pass `-DOPENMU_PACKETS_VERSION=<current>`. |
| New packet fields are missing in the C++ wrappers | The generator only runs when its inputs are newer than its outputs: build `ClientLibrary` once (`dotnet build ClientLibrary/MUnique.Client.Library.csproj -c Release -p:Platform=x86`), clear a stale `.nuget` copy of the old version if needed. |
| `nuget.config` is not committed on your machine | A local git exclude rule may hide it; use `git add -f nuget.config` only if you really change it. |
| The whole client fails to compile with "out of heap" | You configured with `-DBUILD_TESTING=ON`: configure with `OFF` for the game; build test targets separately. |
| Server: no log lines for what a player did | The log shows almost nothing about players. Check the database. |
| `docker compose up` seems up but the game port is closed | Docker accepts connections on a published port while the app is still starting; wait until the admin panel answers (HTTP 200 or the expected login redirect; do not require exactly 302). |
| A first deploy or first start takes minutes | The database is created and filled the first time. The Kerberos library message and the `AuctionHouse.ini` warning in the log are harmless. |
| Anyone can log in as a game master on a new server | Default test accounts have known passwords. Ban or re-password them before the server is reachable (as done on the live server). |
| Jewels cannot be dropped by a player | The server refuses it ("expensive item"); use the `/item` GM command to stage items. |
| The Auction House says the account is too new | Accounts must be 24 hours old (`MinimumAccountAgeHours`). Log out and back in after changing `RegistrationDate`. |
| A picked-up item does not go into the Jewel Bank with a full inventory | Fixed in the client; needs the current client (it asks for the bank state when you enter the world). |
| Windows `tar` says "Cannot connect to C: resolve failed" | `tar` treats `C:` as a host name: use `--force-local` or a relative path. |
| Git warns "LF will be replaced by CRLF" | Harmless; keep each file's own line endings when editing. |
| CI "Quality Gates" fails on "Check formatting" | It runs `clang-format` only on the lines your pull request changed. Visual Studio ships it: `C:\Program Files\Microsoft Visual Studio2\Community\VC\Tools\Llvmin\clang-format.exe`; run it with `-i --lines=<start>:<end>` on your changed lines (or `--dry-run --Werror` to check). |
| The macOS CI job fails with "The job was not acquired by Runner" | GitHub could not provide a macOS runner: an infrastructure problem, not your change. Ignore it. |

## 14. Cheat sheet

```powershell
# server: build, run locally, test (only after disposable database/port isolation above)
dotnet build src/Persistence/SourceGenerator/MUnique.OpenMU.Persistence.SourceGenerator.csproj -c Release
dotnet build src/Startup/MUnique.OpenMU.Startup.csproj -c Release
dotnet run --no-build --no-launch-profile --project src/Startup/MUnique.OpenMU.Startup.csproj -c Release -- -autostart
dotnet test tests/MUnique.OpenMU.Tests/MUnique.OpenMU.Tests.csproj -c Release

# client: configure, build, run
cmake --preset windows-x64 -DBUILD_TESTING=OFF
cmake --build --preset windows-x64-release

# a branch in its own worktree
git fetch origin; git worktree add .worktrees/my-feature -b feature/my-feature origin/main     # or origin/master in the server repo

# watch a workflow
gh run list --repo rauseklis/OpenMU --limit 5
gh run watch <run id> --repo rauseklis/OpenMU --exit-status

# the live server (needs your key on it)
ssh deploy@109.199.104.230 "cd /opt/openmu && docker compose ps"
```

## 15. Glossary

- **OpenMU** — the open-source MU Online server this project is based on. **MuMain** — the open-source client.
- **Connect server / game server** — the connect server lists the game servers and hands the player over; the game server runs the world.
- **GMO / open-source client definition** — two client versions the connect server serves (ports 44405 / 44406).
- **Plug-in** — a class the server discovers by attribute and runs (handlers, commands, events).
- **Packet / head code / sub-code** — one message of the protocol; C1, C2, C3, C4 are its header types (C3/C4 are encrypted; C2/C4 are the long ones).
- **GM** — game master: an account/character with extra commands. **MU Helper** — the in-game auto-hunting helper.
- **Jewel Bank** — a per-account store of jewels, fruits and bundles. **Jewel Rain** — a scheduled event that drops jewels. **Auction House** — the auction window and server.
- **VPS** — the rented server the live game runs on. **GHCR** — GitHub's container registry, where the server image is stored.
- **Worktree** — an extra working folder of a git repository for one branch (`git worktree add`).

## 16. Index of documents

- `docs/OPERATIONS.md` — the short operations reference (this guide's summary).
- Feature documents: `docs/superpowers/specs/2026-10-20-jewel-bank-design.md`, `docs/jewel-bank-handover-2026-10-20.md`,
  `docs/superpowers/specs/2026-10-21-jewel-rain-event-design.md`, `docs/superpowers/plans/2026-10-21-jewel-rain-event.md`,
  `docs/auction-house-claude-handover-2026-10-13.md`, `docs/auction-house-claude-handover-2026-10-11.md`.
- Client: `AGENTS.md`, `docs/CODING_RULES.md`, `docs/build/README.md`, `docs/ui-hd-textures.md`, `docs/chat-commands.md`, `nuget-feed/README.md`.
- Server repository: `AGENTS.md`, `deploy/live/README.md`, `docs-website/docs/getting-started/*.md` (run from source, test accounts, connecting a client),
  `docs-website/docs/deployment/*.md`, `docs-website/docs/admin-panel/*.md`.
