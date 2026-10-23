# Operations — how the whole project fits together

Read this first if you start work on the game, whether you are a person, Claude, Codex or any other assistant. It describes the two
repositories, the live server, how changes reach it, how to test safely, and the rules the owner set. Details of single features are in
`docs/` (see the index at the end).

## The project

A private MU Online server and its game client, built by two people (the owner and their brother).

| | Server | Client |
| --- | --- | --- |
| Repository | [`rauseklis/OpenMU`](https://github.com/rauseklis/OpenMU) (C#, .NET 10, PostgreSQL), a fork of `MUnique/OpenMU` | [`rauseklis/MuMain`](https://github.com/rauseklis/MuMain) (C++, CMake), a fork of `sven-n/MuMain` |
| Default branch | `master` | `main` |
| Features added | Auction House, vaults, Jewel Bank, Jewel Rain, king maces, Lorencia lost maps, ... | their windows and rendering |

Both repositories use **pull requests with merge commits**; work is done on branches in git worktrees under `.worktrees/`. Server and
docs-only tasks use separate worktrees; client builds reuse an available long-lived worktree as described below. Keep the client build
worktree after a task; remove a task-specific worktree after its branch is merged. The client repository has `AGENTS.md` and `docs/CODING_RULES.md`: follow them.

### The protocol ties the two together

The client and the server speak a protocol that is defined in XML in the **server** repository (`src/Network/Packets/**/*.xml`). The client
is built from three NuGet packages packed from it (`MUnique.OpenMU.PlugIns`, `.Network`, `.Network.Packets`), kept in this repository's
`nuget-feed/` folder; `nuget-feed/README.md` says how to update them. The version (now 0.9.21) is in the three server project files and in
`ClientLibrary/MUnique.Client.Library.csproj` (`OpenMuPacketsVersion`). **A change to the packets must be merged to both repositories
together**, because a client with other packets cannot talk to the server. Encrypted requests longer than about 180 bytes must be C4
packets (a C3 packet cannot carry them); see `docs/jewel-bank-handover-2026-10-20.md`.

## The live server

A Contabo VPS (Cloud VPS 4: 4 vCPU, 8 GB, Ubuntu 24.04, location EU), billed monthly by the owner. It runs 24/7.

- **Address:** `109.199.104.230`. Connect server `44405` (GMO client definition) and `44406` (open-source client definition; use this one), game
  servers `55901`-`55906`, chat `55980`. The admin panel is **not** public: it listens on the server's `127.0.0.1:8080` only.
- **What runs:** `/opt/openmu` holds `docker-compose.yml`, the scripts and `.env` (the generated database and admin passwords; never commit or
  print it). Two containers: `openmu` (the image `ghcr.io/rauseklis/openmu`, built from `src/Startup/Dockerfile`) and `database` (PostgreSQL 17,
  the data in a Docker volume). Both restart by themselves. The server clock is UTC.
- **Deploys happen by themselves.** Every merge to `master` of the server repository runs `.github/workflows/deploy-live.yml`: build the image,
  push it to GitHub's container registry, copy the compose files to the VPS over SSH, run `/opt/openmu/deploy.sh <image>` (backup, pull, restart).
  About 5 to 10 minutes; players are disconnected for about a minute. The workflow needs the repository secrets `DEPLOY_HOST`, `DEPLOY_USER`
  (`deploy`), `DEPLOY_SSH_KEY` and `DEPLOY_KNOWN_HOSTS`; without them it only builds the image. Documentation-only changes do not deploy.
  Setup details and the roll-back steps: `deploy/live/README.md` in the server repository.
- **Backups:** `/opt/openmu/backups` (a dump before every deploy and nightly at 03:30, 14 days) plus Contabo's Auto Backup snapshots. Copy the dumps
  off the server now and then.
- **Looking at it** (needs an SSH key that is on the server): `ssh <user>@109.199.104.230`, then `cd /opt/openmu && docker compose ps`,
  `docker compose logs --tail 100 openmu`; the database: `docker compose exec database psql -U postgres -d openmu`.
- **The owner's PC:** `C:\Users\rause\.ssh\config` has the shortcut `openmu`: `ssh -N openmu` opens the admin panel at `http://localhost:8080`.
  The keys are in `C:\Users\rause\.ssh\openmu-vps\` and `openmu_admin_ed25519`; the game account and admin panel logins are in
  `C:\Users\rause\.ssh\openmu-vps\game-accounts.txt`. None of this is in any repository. These paths describe the owner's PC, not every
  contributor's machine; do not assume they exist or read credential files to prepare a client. The owner enters game credentials manually.
- **Accounts:** the 20 default OpenMU test accounts were banned (they have well-known passwords); only `testgm` and `testgm2` (game masters) are
  active, with random passwords. Make real accounts in the admin panel. Note: the Auction House refuses accounts younger than 24 hours
  (`MinimumAccountAgeHours`), and `Account.RegistrationDate` of the two GM accounts was set back to get round it.
- **Security rules:** SSH accepts keys only; fail2ban runs; the firewall (`ufw`) allows SSH and the game ports only. Never print or commit
  secrets or the Contabo API credentials. Other people's keys are added to `/home/deploy/.ssh/authorized_keys`.

## The client

- **Is this computer set up properly?** `powershell -ExecutionPolicy Bypass -File scripts\check-setup.ps1 -Target <client folder>` is a read-only check (git, GitHub login,
  Visual Studio, clone, dev worktree and its build, game folder and `config.ini`, live server, `ssh openmu`). It prints OK / WARN / FAIL with what to do. Run it at the start of a session.
- **The fast path for your own testing: build locally, install locally.** GitHub's build takes 20 to 40 minutes (its machines start empty and recompile
  the whole client; builds also queue one after another). A local incremental build in the warm dev worktree takes about a minute. So after a client
  change: in the dev worktree run `scripts\build-client-windows.cmd x64` (finds Visual Studio, activates it, configures and builds), then
  `powershell -ExecutionPolicy Bypass -File scripts\update-live-client.ps1 -Target <client folder> -LocalBuild out\build\windows-x64\src\Release`
  (keeps `config.ini`, syncs new and changed `Data` and `fonts` files from the clone itself, works from any folder, refuses while the game runs; never copy game files by hand). Use the GitHub bundle (`update-live-client.ps1` without `-LocalBuild`) for a person
  who does not build locally, for example to give the brother's PC the same build as the merged `main`.
- **After every merged client change, update the client folder with `scripts/update-live-client.ps1` — nobody downloads or unpacks anything by hand.**
  Run `powershell -ExecutionPolicy Bypass -File scripts\update-live-client.ps1 -Target <the client folder>` (the owner's is
  `C:\Users\rause\OneDrive\Desktop\OpenMU Live Client`, the brother's `C:\OpenMU Live Client`). It finds the newest bundle that matches `main` (a build that only misses documentation changes counts),
  waits for a running build or starts one if none exists, downloads the artifact, unpacks it and copies it over the client folder while keeping
  `config.ini`, `Data` and `fonts`. It refuses to run while `Main.exe` of that folder is running (it never closes the game): ask the owner to close
  the game, run it again, then tell them to start `Main.exe`. `-RunId <id>` installs an older build; `-Force` reinstalls the same one. The assistant
  runs this itself as the last step of a client change: the owner only restarts the game. Downloading bundles built by this repository's own
  workflows into the client folder is permitted (standing permission, 2026-10-21).
- **Bundle:** the Windows x64 no-data bundle is built by the `CI` workflow of the client repository (`Windows Native Build`) on pushes to `main`
  and can be started by hand (Actions → CI → Run workflow, on `main`). Download the artifact
  `mu-client-windows-native-x64-release-editor-off-no-data-main`, extract it, and add the `Data` and `fonts` folders from `src/bin/` of this
  repository (the bundle has the program only). The ZIP may contain a runtime `.tar.gz` that must also be unpacked. A locally built x86
  or x64 Release runtime is also supported. Use a standalone folder outside Git repositories, for example `C:\OpenMU Live Client`; existing
  paths on another person's PC are examples, not prerequisites. Copy all matching runtime DLLs and shaders, seed a clean configuration,
  and launch with that folder as the working directory. See [the assembly checklist](CODEX_GUIDE.md#assemble-a-standalone-client-from-a-local-build).
- **`config.ini`** decides the server: `ServerIP=109.199.104.230`, `ServerPort=44406`. The client **ignores the `/p` argument**; it reads
  `config.ini`. The owner signs in manually; assistants do not log in or control the window. File/build verification does not verify gameplay.
- **Readiness:** verify TCP connections and at least four greeting bytes on both live ports 44406 and 55901, without sending login packets.
  Record failures/timeouts separately; an open port does not prove login succeeds. Report the full prepared `Main.exe` path.
- **Shipping access:** `gh auth status` and `gh api user --jq .login` must identify your active account; check `.permissions.push` using
  `gh api repos/rauseklis/OpenMU --jq .permissions` and the equivalent for `rauseklis/MuMain`. When requested, prove Git push access with
  a throwaway branch/worktree in each repository, delete only your probes afterward, and do not merge them. Never print an auth token.
- The CI also runs a test that pins the Windows job's workflow text (`tests/test_windows_workflow_contract.py`); change both together.
- The production game client the owner runs elsewhere (`C:\Games\Emberion Client`) is an older build; leave it alone.

## Working and testing

- **Never use the real database or the real server.** The owner has their own local server and database (`openmu`, started from the
  `auction-create-c4` worktree); do not stop, change or point anything at it. Test with a throwaway PostgreSQL database and a second server
  with shifted ports (`docs/jewel-bank-handover-2026-10-20.md`, "Trying it in the game"), or on the live server when the owner says so.
  The shipped `ConnectionSettings.xml` defaults to `openmu`: before starting, point every runtime context to a disposable database/cluster
  and port. `DB_HOST`/admin environment variables do not change the database name or port. Fresh seed data enables automatic startup,
  even without `-autostart`; prepare the fixture and shift game endpoints before running beside the real server. The complete guide
  describes these safeguards and the clean-build source-generator prerequisite.
- **Client builds: use the long-lived "dev" worktree.** A new worktree has an empty build folder, so its first client build compiles about 820 steps
  (10 to 20 minutes); later builds in the same worktree only recompile what changed. The owner's machine has one with a warm x64 Release build:
  `C:\Users\rause\OneDrive\Desktop\mu-online-openmu\client\.worktrees\dev`. Start a client task there with `git switch -c <branch> origin/main`
  (never edit `main` itself), build with `cmake --build --preset windows-x64-release` from a Developer PowerShell (or `cmd` after
  `vcvarsall.bat x64`, with `VCPKG_ROOT` set), and when the branch is merged put it back with `git switch --detach origin/main`. Two sessions must not
  build in the same worktree at the same time: a second session creates its own worktree and accepts the one-time slow build. Server builds take
  1 to 2 minutes, so a fresh server worktree per task is fine.
- **Do not control the game** (no automated clicks or keys into the game window); starting and stopping your own test processes is fine, and
  the owner plays and reports.
- GM commands for staging items and events on a test server: `/item <group> <number> [level]`, `/jewelrain` (see the Jewel Rain design).
- Build the client with `-DBUILD_TESTING=OFF` for the game; unit tests are built separately (see the Jewel Bank handover).
- The repository mixes LF and CRLF line endings: keep the file's own endings when you edit.

## Rules set by the owner

1. **A session may push, open pull requests and merge, and so deploy** (the owner said on 2026-10-21 that any new session is allowed to
   deploy). A merge to `master` goes **live within minutes** and disconnects players for about a minute, so before merging: run the tests,
   test the change on a throwaway server when it touches the game, say plainly what was and was not tested, and merge server and client
   changes that share packets together. Do not merge a change you have not built and tested.
2. Do not change GitHub settings or secrets, or the live server's configuration (firewall, SSH, accounts, the `.env` file), without showing the owner first and getting a yes.
3. Never touch the owner's real server or database. Do not delete the live database, backups, or other people's branches or worktrees without a yes.
4. Do not control the game window.
5. Do not ask for or handle passwords; secrets go through files on the owner's PC or GitHub secrets, not into chat.
6. Be honest about what was tested and what was not.

## Common tasks

- **Ship a server change:** PR to `master` → merge → watch Actions → Deploy live server → check `docker compose ps` and that a login still works.
- **Ship a client change:** PR to `main` → merge → run CI by hand if no push run started → download the artifact → tell the owner where it is.
- **Roll back the server:** run the deploy workflow from an older commit, or on the VPS `./deploy.sh ghcr.io/rauseklis/openmu:<older commit>`;
  restore a dump if a database change must be undone (`deploy/live/README.md`).
- **Change a setting that lives in a file the container cannot write** (for example `AuctionHouse.ini`, which the server cannot read inside the
  container yet): it needs a mount in `deploy/live/docker-compose.yml` first.
- **Time zone:** periodic events and Jewel Rain use `SystemConfiguration.TimeZoneId`; empty or invalid values fall back to UTC.
  `Europe/Riga` selects Latvian time; changing only the container's `TZ` does not configure the event time zone. Verify the actual live value
  and obtain the owner's approval before any live configuration change.

## Index of feature documents

- Jewel Bank: `docs/superpowers/specs/2026-10-20-jewel-bank-design.md`, `docs/jewel-bank-handover-2026-10-20.md`
- Jewel Rain: `docs/superpowers/specs/2026-10-21-jewel-rain-event-design.md`, `docs/superpowers/plans/2026-10-21-jewel-rain-event.md`
- Auction House: `docs/auction-house-claude-handover-2026-10-13.md`, `docs/auction-house-claude-handover-2026-10-11.md`
- Packets folder: `nuget-feed/README.md`
- Live server setup (server repository): `deploy/live/README.md`
- HD interface textures: `docs/ui-hd-textures.md`

## MU Helper lifecycle and minimized-window regression

- The official MU Helper (the Z settings/helper button, not held right-click)
  must stop locally on Escape, death, safe-zone entry, teleport and logout.
  Stopping does not depend on receiving a server acknowledgement. A delayed
  start acknowledgement must not restart a helper explicitly stopped by the player.
- Inactive helper stops leave manual attacks and movement alone. An active stop
  cancels the queued repeating attack, spell target and walking path.
- Windows messages already owned by SDL must not also pass through the legacy
  default window procedure. Keep legacy Enter/IME, F10 and protection handling;
  SDL handles minimize/restore, resize, paint and close.
- Reproduction on the installed Windows client: minimizing left the helper
  active while WorldTime, animation and helper ticks froze until restoration.
  Two in-memory snapshots showed the main thread in NtUserMessageCall through
  SDL WIN_WindowProc/WIN_PumpEvents. Removing duplicate native default handling
  is a candidate correction; Windows build/tests and the same minimize/restore
  gameplay check are still required before calling it verified.
- Actions is intentionally disabled by the owner. The prepared changes are
  unmerged; do not enable Actions or substitute a local build without permission.
  When Actions is allowed again, first run the test-only helper baseline
  982bf419973fa83a4ee1e5a5b30de88a2bac3099, then the fix. The earlier Windows
  test run had a fixture include error and is not a valid failing regression run.
- Verify after installation: Escape with a UI open, death and respawn, stop with
  no server reply, reconnect resume, minimize for at least 20 seconds, then
  restore. Also check ordinary movement, F10, chat/IME, resize and titlebar close.
