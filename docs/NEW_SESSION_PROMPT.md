# The prompt to start every session with

Paste the text in the box below as the **first message** of every new session (Claude Code, Codex or any other assistant), in the folder of
your local clone of the **client** repository (`rauseklis/MuMain`). It makes the assistant read the project documents and your rules before it
does anything, then ask you what you want to work on. Nothing in it is specific to one person or machine.

```
You are working on a private MU Online project with two GitHub repositories:
- Server: rauseklis/OpenMU (C#, default branch: master)
- Client: rauseklis/MuMain (C++, default branch: main)
My local clones of both are on this computer (look for folders with those names, next to each other; the current folder is the client).

SETUP - DO THIS NOW, BEFORE YOU ASK ME ANYTHING
1. Run `git fetch origin` in the client clone, then read these from origin/main (git show origin/main:<path>, or from GitHub if you cannot):
   docs/OPERATIONS.md (short reference: the live server, how a merge deploys, the owner's rules)
   docs/CODEX_GUIDE.md (the complete guide: setup, building, testing, the protocol, pitfalls)
   Also read AGENTS.md and docs/CODING_RULES.md in the client clone, and AGENTS.md in the server clone.
2. Do not create anything or change anything yet.

RULES (binding for the whole session)
- Never touch a real local server or database of mine (a database named "openmu"). Test with a throwaway database and a second server on shifted ports.
- Do not control the game window (no automated clicks or keys). You may start and stop your own test processes; I will play and report.
- Never put passwords or keys in chat, commits or repository files.
- Show me before you change GitHub settings or secrets, or the live server's configuration (firewall, SSH, accounts, .env), and ask before deleting the live database, backups, or anyone else's branches or worktrees.
- You MAY push, open pull requests, merge and so deploy. A merge to server master goes LIVE within minutes and disconnects the players for about a minute. Therefore: test first, and tell me honestly what you did NOT test. Merge server and client changes that share packets together (server first).
- Never work directly on main or master. For SERVER work create a new git worktree from origin/master (server builds take 1-2 minutes).
- For CLIENT work that needs a build, keep ONE long-lived worktree for builds (for example <client clone>\.worktrees\dev, created with `git worktree add --detach .worktrees/dev origin/main`; its first build takes 15+ minutes, later builds are fast): inside it run `git switch -c <branch> origin/main`, and when the branch is merged put it back with `git switch --detach origin/main`. If another session is using it, create your own worktree and expect one slow first build. Docs-only client changes need no build.

THEN ASK ME
When you have finished the setup, reply with ONE short paragraph confirming what you read, and then ask me exactly: "What are we working on today?" Wait for my answer. Do not suggest tasks and do not start anything.

AFTER MY ANSWER
1. Tell me in a few lines what you understood and your plan (and ask me only if you must choose between different designs).
2. Build it with tests, try it on a throwaway server, and check the effect in the database.
3. Open a pull request with a clear description, merge it, watch the deployment (Actions tab), and check that a login still works.
4. Finish with a short report: what changed, what you tested, what you did not test, and what I should check myself.
If something in the docs is wrong or missing, tell me and fix the document in the same pull request.
```

Notes:

- Do not start sessions in an old worktree on an old branch: the documents only exist on the current `main`. If an assistant says it cannot find
  `docs/OPERATIONS.md`, it is in the wrong folder or its clone is behind; `git fetch origin` and `git show origin/main:docs/OPERATIONS.md` work from
  anywhere in the clone.
- To use it for a specific task right away, add a line before "THEN ASK ME": `My task today: <one or two sentences>`, and replace the last
  instruction of "THEN ASK ME" with "then start with the plan".
- The first things a new person needs from the owner are listed in section 3 of `docs/CODEX_GUIDE.md`.
