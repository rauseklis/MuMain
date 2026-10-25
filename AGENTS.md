# Agent Instructions

This file is the entry point for AI coding assistants (Claude Code, Cursor, Codex,
etc.) working in this repository. It also applies to human contributors.

## Binding rules for every session (apply even if nobody pasted a prompt)

This is a private MU Online project of two brothers: this client repository (`rauseklis/MuMain`, branch `main`) and the server repository
(`rauseklis/OpenMU`, branch `master`). The server runs live on a VPS and a merge to its `master` deploys it within minutes.

1. **If something is blocked, fails or is unclear: stop, say exactly what happened (the real error text) and ask the user. Do not try a different
   approach on your own.** Never state something as fact that you did not verify (for example "Actions is disabled"): say what you checked.
2. **Do only what the user asked.** No extra builds, branches, worktrees, cleanups or "improvements". Never delete, discard or overwrite anything
   (files, branches, worktrees, databases, backups) without asking first.
3. **Never change GitHub settings, Actions, secrets or permissions, and never change the live server's configuration** (firewall, SSH, accounts, `.env`,
   compose files). You may read the live server (logs, read-only database queries). Show the user first, and wait for a yes.
4. **Never touch a real local server or database of the user's.** Test with a throwaway database and a second server on shifted ports.
5. **Do not control the game window** (no automated clicks or keys) and do not log in to the game. The user plays and reports.
6. **Never put passwords, keys or tokens in chat, commits or files.** Do not read or print the server's `.env` or the admin password.
7. **Never work directly on `main` or `master`.** Use a branch and a pull request. You may merge your own pull request (that deploys a server change),
   but only after testing it, and say honestly what you did not test. Merge server and client changes that share packets together, server first.
8. **Install the client only with `scripts/update-live-client.ps1`; never copy game files by hand.** If the finished GitHub build is not there yet,
   wait for it (the script does) or ask. Build locally only for your own code changes, as described in `docs/OPERATIONS.md`, never as a workaround
   for a slow install.
9. **At the start of a session:** `git fetch origin`, read `docs/OPERATIONS.md` (short) from `origin/main`, run the read-only setup check
   `powershell -ExecutionPolicy Bypass -File scripts\check-setup.ps1 -Target <the user's game folder>` and report its FAILs in one line, do not change
   anything, then ask the user "What are we working on today?" and wait. Do not fix setup problems on your own: tell the user what the check says. The full guide is `docs/CODEX_GUIDE.md`.
10. **At the end of every task** report in a few lines: what changed, what you tested, what you did not test, what the user should check.
11. **The client is for Windows only.** Linux and macOS are NOT needed and are not released: their CI builds are switched off (`if: false` in
    `.github/workflows/ci.yml` and `fork-ci.yml`). Do not build, fix, test, re-enable or write code or docs for them, and do not report their
    failures as problems. Only the Windows build and the quality checks matter. (The live server runs on Linux in Docker: that is the server, not the client.)

## Coding rules — read first

**The coding rules in [`docs/CODING_RULES.md`](docs/CODING_RULES.md) apply to all
code changes in this repository, by humans and by AI agents alike.**

Before writing or modifying code:

1. Read [`docs/CODING_RULES.md`](docs/CODING_RULES.md).
2. Apply the rules to any new or changed code.
3. When reviewing a change (including via `/review` or `/ultrareview`), treat
   `docs/CODING_RULES.md` as the authoritative style and quality baseline.

The rules are not exhaustive — they capture the conventions that matter most for
this codebase. Follow the style of surrounding code where the rules don't speak.

## The whole project, the live server and the owner's rules

**New here? Read [`docs/CODEX_GUIDE.md`](docs/CODEX_GUIDE.md), the complete guide (setup, building, testing, the protocol, the live
server, the rules, the traps), then [`docs/OPERATIONS.md`](docs/OPERATIONS.md), the short reference.**
The prompt to start every session with is in [`docs/NEW_SESSION_PROMPT.md`](docs/NEW_SESSION_PROMPT.md). It explains how the server repository (`rauseklis/OpenMU`) and this one fit together
(the packets), the live server on a VPS and how a merge deploys to it, how to test safely, and the rules the owner set.

## Build setup

The project uses CMake. Supported build environments and setup steps are
documented in [`docs/build/README.md`](docs/build/README.md).

Quick references:

- Requirements and IDE-specific instructions: [`README.md`](README.md).
- Cross-platform / WSL build details: [`docs/build/README.md`](docs/build/README.md).

## Branch and PR conventions

- Target branch for PRs is `main`.
- Keep changes focused — one concern per commit (see rule 10 in the coding
  rules). Smaller diffs review faster.
- Match the style of existing commit messages in `git log`.
- Reference the related issue in the PR description when applicable.

## Out of scope for AI changes

Don't perform large retroactive cleanups of existing code to fit the rules unless
the user explicitly asks for it. Apply the rules going forward; pre-existing
code can be refactored opportunistically when you're already touching it.
