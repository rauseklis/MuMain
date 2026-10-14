# Kickoff prompt: ultra-HD regeneration of the MuMain in-game interface (phase 1: the core windows)

Copy everything below the line into a new Claude Code session whose working folder is the MuMain client repository (see section 2). It is written for a session that has never seen this project. It explains the goal, what the owner already decided, how the client draws its interface today (measured from the code and the data files, not guessed), a recommended technical design, a phase plan with acceptance criteria, how to make and check the art, how to build and test, and what to ask the owner before you start.

---

## 0. Mission, in one paragraph

The MU Online client (MuMain, C++ with SDL GPU and a .NET NativeAOT networking library) draws almost its whole in-game interface from tiny, decades-old textures: the HP orb is a 45 x 39 JPEG, the window frame pieces are 21 x 320 and 190 x 64 images, the stat and menu buttons are 36 x 58. On a modern display the interface is stretched 1.6 to 4 times and looks soft and pixelated. The owner wants the **same interface, same look, same layout** regenerated in **ultra high definition** so that nothing looks pixelated any more. This session builds the engine support for high-resolution interface textures, the tooling to make and verify them, and the first batch of art: the **core in-game windows** (Character info, Inventory, the bottom bar with the HP/Mana orbs, skill and hotkey slots and menu buttons). Other windows follow in later sessions using the machinery you build, so design it to scale to all 748 interface textures.

## 1. What the owner decided (do not re-litigate; ask only where section 9 says to)

1. **Same look, ultra HD.** Not a redesign. Same composition, colours, ornaments, proportions and positions. The owner's words: "regenerate everything so that it looks the same but is ultra high definition. I don't want anything to look pixelated anymore."
2. **Phase 1 = the core in-game windows**: Character info (the stats window with STR / AGI / STA / ENG), Inventory (with the equipment slots and the Zen bar) and the bottom HUD bar (HP and Mana orbs, skill and hotkey slots, the menu buttons at the bottom right, the experience bar). The window frame pieces are shared by many other windows (see 4.8), so upgrading them lifts those windows as well.
3. This is a **separate, long-running piece of work**. Ship it in small, playable, individually testable steps; the game must keep working after every commit, with and without the HD assets.
4. The owner tests by playing and takes his own screenshots. Do not drive his screen; ask him for screenshots (and tell him exactly what to open and where).
5. Earlier in the same project the owner redesigned the **Auction House** window; it already shows the quality bar and a technique to reuse (see 4.9). Do not touch the Auction House look.

Non-goals: no layout changes, no new colours or styles, no gameplay or network changes, no changes to the 3D world, items, monsters or effects, no localisation work beyond keeping existing text legible.

## 2. Where everything is

- **Client repository** (the one to edit): `C:\Users\rause\OneDrive\Desktop\mu-online-openmu\client\.worktrees\auction-house` (a git worktree of the monorepo's `client` module; remote `https://github.com/rauseklis/MuMain.git`). Branch `codex/auction-house`, which equals `main` at the time of writing (tip `3d3594d7` or later). The UI source is under `src\source\UI\...`, the assets under `src\bin\Data\...` (tracked in git: `Data` is 733 MB, `Data\Interface` is 39 MB and 771 files).
- **Strong recommendation: work in a NEW worktree**, so the owner's playtest client and any other session keep a stable tree. From the client repository: `git worktree add ..\ui-hd -b codex/ui-hd codex/auction-house`. Then copy `nuget.config` into it (it is not in git; it lists the local feed `C:\Users\rause\.local-nuget\auction-house`, which holds the hand-packed NuGet packages `MUnique.OpenMU.Network.Packets` 0.9.13 and its siblings) and reuse the existing package cache by setting `NUGET_PACKAGES=C:\Users\rause\OneDrive\Desktop\mu-online-openmu\client\.worktrees\auction-house\.nuget`. The first build in a fresh build directory takes a long time (vcpkg, NativeAOT).
- **Uncommitted change from someone else:** the existing worktree has an unrelated uncommitted edit to `src\source\Camera\CameraMove.cpp` (another session's camera fix). Never revert, stage or commit it.
- **Running processes:** the owner usually has a game server running locally (`MUnique.OpenMU.Startup.exe`, database `openmu_auction_test`) and may have `Main.exe` running from the auction worktree's `out\build\windows-x86\src\Release`. Stop only a `Main.exe` that runs from the build directory you are about to rebuild.
- **Server and accounts:** the client connects to `localhost` port 44406 by default (`Main.exe connect /u127.0.0.1 /p44406`). You need a game account to log in; ask the owner for one (his test character is a level 400 Dark Wizard in Devias, as in his screenshot). Never invent or store credentials.
- **Memory of the previous sessions:** the owner's auto-memory notes live in `C:\Users\rause\.claude\projects\C--Users-rause-OneDrive-Desktop-Emberion-public-html\memory\` (see `project_ui_hd_regeneration.md` and `project_auction_house.md`). Read them if you can.
- **Hook warning:** if your session's working folder is the Emberion website folder (`C:\Users\rause\OneDrive\Desktop\Emberion\public_html`), a user hook runs `python ftp_deploy.py` after every Edit or Write tool call and uploads that website's pending changes to production, whatever file you edited. Start the session in the client repository, or edit files with Bash/Python instead of the Edit and Write tools, and tell the owner if it happens.

## 3. Working rules

1. Read `AGENTS.md` and `docs/CODING_RULES.md` in the client repository **before writing any code** and follow them: short focused functions, early exits, no hard-coded values (name every constant), reuse before writing, one class per file, namespaces `<Layer>::<Concern>` and no `NewUI` prefix for new files, RAII, deliberate hot paths, document usage not internals (extend an existing file in `docs/`; for example `options-window.md` if you add a setting). One concern per commit, in the style of `git log` (for example `feat(ui-hd): ...`, `fix(ui-hd): ...`).
2. **Commit locally only.** Do not push, merge or open pull requests unless the owner tells you to. Stage specific paths; never `git add -A` in a tree that holds other people's changes.
3. **Release builds only** for anything the owner will look at (Debug runs at a third of the speed and shows a debug panel). Stop `Main.exe` before you rebuild it.
4. **Ask before downloading** anything (an upscaling model, an executable, a font, a dataset): say what it is, where it comes from and how big it is, and wait for a yes. Keep downloaded files out of the repository.
5. **Models:** do not start subagents on a different model than your own without asking the owner ("do not switch to Opus" is a standing instruction on this project).
6. Use the superpowers skills if they are installed: `brainstorming` (for the design in section 5; the owner has already answered the main questions), `writing-plans`, then `executing-plans` or `subagent-driven-development`, `test-driven-development` for every piece of logic, `verification-before-completion` before you claim anything works.
7. The owner reads results, not process. Report what you did and what he should look at, in plain, short sentences.

## 4. How the client draws interface art today (measured, with file and line pointers)

### 4.1 Formats and loaders (`src/source/Render/Sprites/GlobalBitmap.cpp`)

- **`.OZT`** (523 interface files) is a 32-bit **uncompressed TGA with an extra 4-byte prefix**. The loader (`CGlobalBitmap::OpenTga`, around line 930) reads width and height from file offsets 16 and 18, requires 32 bits per pixel, and reads rows bottom-up as BGRA. There is a small TGA footer. Appendix A has the exact byte layout and a working PNG-to-OZT converter.
- **`.OZJ`** (225 interface files) is a **JPEG with a 24-byte header**, decoded with TurboJPEG (`OpenJpegTurbo`, around line 800). JPEG has no alpha channel and carries compression artefacts. The whole bottom HUD bar (`newui_menu01.jpg`, the HP/Mana/AG/SD gauges), the skill icon atlases and the window background `newui_msgbox_back.jpg` are OZJ.
- Code asks for `Interface\name.jpg` or `Interface\name.tga`; the loader swaps the extension to `.OZJ` or `.OZT` and looks under `Data\`. A window loads its textures in its `LoadImages()` (for example `CNewUIMainFrameWindow`, `CNewUICharacterInfoWindow`, `CNewUIMyInventory`) with `LoadBitmap(path, IMAGE_X, GL_LINEAR[, wrap])`, where `IMAGE_X` is an index from an enum (`Core/Globals/_TextureIndex.h` and window headers), and releases them with `DeleteBitmap`.
- There is **no PNG, DDS or compressed-texture loader for textures** and no atlas packer. SDL3's own `SDL_LoadPNG` is already used once (`App/Platform/Windows/Winmain.cpp`, around line 2054, for the window icon), so **PNG decoding is available without a new dependency**: verify the SDL version (`src/CMakeLists.txt`, line 16 onward) and that it works on the Linux and macOS builds before relying on it. TurboJPEG is linked in `src/CMakeLists.txt`. Anything else new needs a justification (coding rule 10).

### 4.2 Sizes, padding and sampling

- Every texture is at most **1024 x 1024** (`MAX_WIDTH`, `MAX_HEIGHT` in `GlobalBitmap.h`; anything larger is rejected with an error report), is **rounded up to a power of two** (`NextPowerOfTwo`), and the image sits in the top-left corner of the padded buffer. `BITMAP_t.Width` and `.Height` hold the **padded** size, not the file size.
- Interface textures (path under `Data\Interface`, see `Render::UiTextureQuality::IsInterfaceTexturePath`) are sampled with **LINEAR** filtering, **no mipmaps, no anisotropy**. Only the "enhanced" world texture paths get mipmaps and anisotropy (`Render::GraphicsQuality::IsEnhancedTexturePath`, config `[Render] TextureMipmaps`, `Anisotropy`). Interface textures also get `BleedTransparentRgb` (copies neighbouring colours into fully transparent pixels to avoid dark halos). That helper has a unit test, `tests/render/test_ui_texture_quality.cpp`: copy its shape for your tests.
- Texture memory is counted in `m_dwUsedTextureMemory`. The renderer is SDL GPU (`UploadTextureSDLGpu`, around line 130, creates the GPU texture and sampler).

### 4.3 Coordinates and scaling (`src/source/UI/Scaling/UITransform.*`)

- All UI is laid out in a **640 x 480 logical canvas**. `UI::Scaling` maps it to the window with a transform per layout mode (`Hud`, `HudLeft/Center/Right/Experience`, `DockLeft/Right`, `PanelDockRight`, `FloatingWorkspace`, `Dialog`, `WorldOverlay`).
- Scale factors (from the source): bottom HUD = `clamp(min(W/640, H/480), 1 x c, 2 x c)`; panels 1 to 2 x c; docks at most 2.25; typography at most 2.25, where **c** is the display's content scale (the Windows DPI scale). The owner's 1025 x 768 window therefore draws the UI at **1.6x**; 1920 x 1080 at c = 1 gives 2x; a 200 % HiDPI laptop screen can reach 4x.
- Consequence for asset resolution: **2x assets are about 1:1 on a normal full-HD display at the cap, 4x assets are about 1:1 on a 200 % display.** Decide the tiers with the owner (section 9) and measure with a pilot (section 6, step 2).
- Text is not the problem: fonts are bundled and rendered by the font system at the typography scale. Check, though, that glyph rendering stays crisp at all scales (section 6, step 7) and report it separately if not.

### 4.4 The UV convention: the crux of the whole design

- `SEASON3B::RenderImage(type, x, y, width, height)` (`UI/NewUI/NewUICommon.cpp`, line 58) computes texture coordinates from `Bitmaps[type].Width/Height` and assumes **one texel per logical unit**: `u = 0.5 / Width`, `uw = (width - 0.5) / Width`. The variants with `su, sv` and `RenderImageStretch` (source rectangle in texels) do the same kind of arithmetic. `RenderBitmap` (`Render/Textures/ZzzOpenglUtil.cpp`, line 1188) itself takes **normalised** UVs.
- About **47 places** read `Width`/`Height` of a bitmap; **nothing outside `GlobalBitmap.cpp` reads the pixel buffer** (checked with grep).
- Therefore: if `BITMAP_t.Width/Height` keep reporting the **logical** (original, padded) size while the GPU texture holds k times as many texels, every existing caller keeps working unchanged, because UVs are fractions of the texture. The HD file must be an exact k-times version of the original's layout: same content position, content size = k x original, padding scaled likewise. A file whose size is not an integer multiple must be rejected and the original used.
- Watch the cases that break this: callers that use a texture's size as a **screen size** (still correct, it is the logical size), 1-pixel strips that are stretched (`newui_item_table03(Up).tga` is 1 x 14), atlases where neighbouring cells bleed under LINEAR sampling when the cell pitch is not an exact multiple, and additive-blended sprites that rely on a pure black background.

### 4.5 Where the pixelation comes from

1. **Tiny source art stretched 1.6x to 4x** with LINEAR filtering (the blur) and small sprites such as the 45 x 39 orb gauges.
2. **JPEG artefacts** in the 225 OZJ textures (ringing around edges, no alpha, so sprites are baked onto black).
3. **No mipmaps** on interface textures: fine once the art is HD and shown at or above its native size, but heavy minification of a 4x asset shown at 1x would shimmer. Decide this in the design (5.6).
4. Hard 1-pixel lines and bevels designed for exact 1:1 display now land between device pixels.

### 4.6 Asset inventory (measured with a header-parsing script over `src\bin\Data\Interface`)

- **748 textures**: 523 OZT (32-bit TGA) and 225 OZJ (JPEG); plus 22 loose `.tga` files and one `Thumbs.db`. Sub-folders: `InGameShop` (28), `partCharge1` with `MainBackPanel` (28), `SubBackPanel` (16), `ShopDisplay` (17), `LeftBt` (9), `RightBt` (5), `MacroUI` (17).
- **679 of 748** have no side longer than 256; 68 are 257 to 512; **one** (`InGameShop/Ingame_Shopback.OZJ`, 640 x 429) exceeds 512.
- Total **14.9 million texels = about 60 MB as RGBA** at 1x. At 2x that is about 240 MB, at 4x about **950 MB**: **4x for everything is not affordable**, so tiers and per-texture choices matter (5.2).
- The **core set** (the three windows of phase 1, 62 distinct textures) is 3.8 MB at 1x, **15 MB at 2x, 61 MB at 4x** (padded size): affordable at any tier.
- Assets are tracked in git (the repository pack is already about 900 MB). Uncompressed 32-bit TGAs are large, so decide the storage format with the owner (5.7).

### 4.7 The core textures (full table in Appendix B)

- **Bottom bar** (`HUD/NewUIMainFrameWindow.cpp`, loads around lines 70 and 1413): the frame pieces `newui_menu01.jpg` 256 x 51, `newui_menu02.jpg` 128 x 51, `partCharge1/newui_menu03.jpg` 256 x 51 and `newui_menu02-03.jpg` 160 x 42 (read the `Render...` functions of `CNewUIMainFrameWindow` to see how they are assembled across the 640-unit width), the gauges `newui_menu_red/blue/green.jpg` 45 x 39 and `_ag`, `_sd` 16 x 39, the experience bars `newui_exbar.jpg` and `Exbar_Master.jpg` (6 x 4, stretched across the screen), the five menu button strips `partCharge1/newui_menu_Bt01..05.jpg` 30 x 164, the skill atlases `newui_skill.jpg`, `newui_skill2.jpg`, `newui_skill3.jpg`, `newui_command.jpg` and their greyed `newui_non_*` twins (each 256 x 256), and `newui_skillbox.jpg`, `newui_skillbox2.jpg` 32 x 38.
- **Character info** (`Character/NewUICharacterInfoWindow.cpp`, around line 1606): the shared window frame kit plus `newui_cha_textbox02.tga` 170 x 21 (a text box), `newui_chainfo_btn_level.tga` 16 x 30, `newui_chainfo_btn_quest/pet/master.tga` 36 x 58, `newui_exit_00.tga` 36 x 58.
- **Inventory** (`Inventory/NewUIMyInventory.cpp`, around line 1219): the same frame kit, nine equipment-slot backgrounds (`newui_item_boots/cap/fairy/wing/weapon(L)/weapon(R)/upper/gloves/lower/ring/necklace.tga`, 46 x 46 up to 46 x 66, rings 28 x 28), `newui_item_money.tga` 170 x 26 (the Zen bar), and the buttons `newui_repair_00`, `newui_expansion_btn`, `newui_Bt_openshop`, `newui_Bt_closeshop` (36 x 58 each). The inventory grid itself (`Inventory/NewUIInventoryCtrl.cpp`) loads `newui_item_box.tga` 21 x 21 and the table pieces.
- **The shared window frame kit**: `newui_msgbox_back.jpg` 225 x 512 (stone background, JPEG), `newui_item_back01.tga` and `newui_item_back04.tga` 190 x 64 (top), `newui_item_back02-L.tga` and `-R.tga` 21 x 320 (sides), `newui_item_back03.tga` 190 x 45 (bottom), and the table corner and edge pieces `newui_item_table01..03` (14 x 14 corners, 1 x 14 and 14 x 1 edges). **Almost every dialog window in the game uses these**, so regenerating about ten small textures upgrades dozens of windows at once.

### 4.8 What other windows are made of

About 94 NewUI window classes (about 69,000 lines under `src/source/UI/NewUI`) load their own textures the same way; the HUD pieces next to the core set are the chat window (6 textures), the mini-map (7), the buffs (2), the hero position info (8) and the shared scrollbar (5). They are out of phase 1 but share the loader, so test them for regressions.

### 4.9 Existing helpers and the technique already used for the Auction House

- `Widgets/NineSlice.{h,cpp}` draws a stretchable frame from one texture with corner insets; `UI/NewUI/AuctionHouse/AuctionChrome.*` shows how skins are defined (textures **authored at 2x** with a separate alpha mask, drawn with explicit sizes). `RenderImageStretch(type, x, y, w, h, srcX, srcY, srcW, srcH, tint)` draws a source rectangle with a tint.
- `tools/auction_icons/make_category_icons.py` is a worked example of generating crisp art with Pillow: draw on a large grid at 8x supersampling, reduce, write an OZT. It also writes a preview PNG of the result in context.
- The Auction House textures were converted with a small `png_to_ozt` script (Appendix A). There is **no converter in the repository yet**: creating `tools/ui_hd/` with a converter (both directions), a texture survey and a verification script is part of phase 1.
- The owner's judgment of the Auction House polish: he liked the unified steel and gold look, the crisp icons and the consistent control sizes. Match that level of care.

## 5. Recommended technical design (verify each point against the code before you build on it)

The aim is **optional, additive, reversible** HD art: the game must look and behave exactly as today when no HD file exists or the feature is switched off.

### 5.1 Principle: logical size stays, texels multiply

Keep `BITMAP_t.Width/Height` equal to the **original padded size** (the logical size every legacy caller assumes). Give the HD texture k times as many texels (k = the tier, an integer). Because UVs are fractions of the texture, **no existing draw call changes**. Add fields next to the existing ones (for example `TexelScale`, `RealWidth`, `RealHeight`) only if something needs them. The GPU texture and the memory accounting use the **real** size.

### 5.2 Tiers and where the files live

- Tiers are integers: 1 (the original files, always present), 2, and probably 4 (see 4.3 for why). Each HD tier is **optional per texture**; a missing file falls back to the next lower tier, silently, with one debug-log line.
- Recommended layout: **mirror the path** under a tier folder, for example `Data\Interface@2x\newui_item_back01.OZT` for `Data\Interface\newui_item_back01.OZT` (same names and sub-folders). It keeps the original 771-file folder untouched and makes a tier easy to delete, to ship or to leave out. The asset copy step globs everything under `Data` (`src/CMakeLists.txt`, `MU_COPY_RUNTIME_ASSETS`), so new folders ship automatically.
- An HD file may use a different **format** than its original (an original `.jpg` can have a lossless HD replacement with or without alpha). Say in the design how a JPEG-sourced texture stays visually identical: it was opaque and baked on black, often drawn with additive blending, so the replacement must stay opaque, keep pure black where black was, and be drawn with the same blend state.

### 5.3 Loader contract (a small new unit in `Render/Textures/`, called from the two loaders)

1. Only for paths accepted by `IsInterfaceTexturePath`, only when the feature is on and a tier is selected.
2. Read the **original** file's header (cheap) to get its size (nx, ny); read the HD file's size (hx, hy). Accept only if `hx = k x nx`, `hy = k x ny` with the same integer k for both, and k within the supported tiers. Otherwise reject with one `g_ErrorReport` line and use the original.
3. Real buffer size = **k x the original's padded size**, content at the top-left, padding scaled likewise (this is not always a power of two when k is 3; NPOT textures are fine for SDL GPU, mipmaps included; confirm on the three graphics backends the project supports).
4. Report logical `Width/Height`; upload with the real size; count real bytes in `m_dwUsedTextureMemory`; run `BleedTransparentRgb` on the real buffer.
5. Raise the 1024 limit for HD uploads to what the GPU supports (query it; keep a sane cap), but keep rejecting a corrupt header.
6. Pure, unit-testable function: `ComputeHdScale(originalWidth, originalHeight, hdWidth, hdHeight) -> std::optional<int>`, plus the path mapping function and the tier choice function. Test them (valid k, non-integer ratio, mismatched axes, zero or negative sizes, tier not available, feature off).

### 5.4 Choosing a tier

Choose from the effective scale the UI will use (the `UI::Scaling` scale for the layout mode times the display content scale), for example: effective scale up to 2: tier 2; above 2: tier 4; tier 1 when the feature is off or no HD exists. Expose `[UI] HdTextures = auto | off | 2 | 4` in `config.ini` (document it in `docs/options-window.md`). Textures are loaded when a window is created, so a change taking effect after a restart is acceptable for phase 1; say so in the docs.

### 5.5 Filtering and mipmaps

HD interface textures should generate **mipmaps** and use trilinear sampling when they can be shown below their native density (a 4x asset on a 1.6x layout), otherwise they will shimmer. Keep the 1x path exactly as it is. Measure the cost.

### 5.6 Atlases and strips

Everything stays valid only if cell pitches scale exactly by k. For atlases (the skill icons, button state strips such as `newui_menu_Bt01.jpg` 30 x 164) keep the cell layout, and **replicate each cell's edge pixels into a gutter** inside the padding so LINEAR sampling at cell borders cannot pull in a neighbour. One-pixel strips that the game stretches (`newui_item_table03(Up).tga`, 1 x 14) become k x (14k) strips and must still stretch cleanly.

### 5.7 Storage format and repository weight

Uncompressed 32-bit OZT is simple but large (the 62 core textures are 61 MB at 4x). Options to compare with measurements, then recommend to the owner: (a) OZT as is, (b) RLE-compressed TGA (a small decoder), (c) **PNG decoded with the already-used `SDL_LoadPNG`** (lossless, small, no new dependency; measure decode time for the whole set), (d) GPU-compressed DDS/BC7 (smallest in VRAM, needs an encoder and loader, lossy). Also decide where the **masters** live: a sensible rule is one master per texture at the highest tier (PNG), with lower tiers generated by a script so the result is reproducible; ask whether the repository, Git LFS or a separate art folder should hold them.

## 6. Phases and acceptance criteria

Work through these in order. After each step the game must start, the core windows must open, and the existing tests must pass. Each step ends with a local commit (or a few) and a short note to the owner saying what to look at.

**Step 0. Orientation and baseline.**
Read `AGENTS.md`, `docs/CODING_RULES.md`, `docs/build/README.md`, `docs/control-socket.md`, `docs/in-game-tests.md`. Create the worktree (section 2), configure and build a Release client, run the full test suite (expected: **498 of 498** pass in `ctest --test-dir out/build/windows-x86 -C Release`). Ask the owner for **baseline screenshots** (Character info `C`, Inventory `I`, the bottom bar, at his normal window size, and if he can also at full screen on his display) and keep them in a scratch folder **outside the repository**. These are the "before" images for every later comparison.

**Step 1. Design note and plan.**
Write a short design note (docs or a plan file under `docs/superpowers/plans/`) covering sections 5 and 7 as you will actually build them, the answers to the questions in section 9, the tiers, the storage format and the memory budget. Get the owner's sign-off on the points that cost him money, disk or taste (section 9) before writing engine code.

**Step 2. Tooling and a pilot.**
Create `tools/ui_hd/`: a converter in both directions (PNG to OZT/OZJ-compatible and OZT/OZJ to PNG, see Appendix A), a survey script (size, format, padded size, owner window, status per texture), and a compare script (downscale an HD texture by k and report mean error and SSIM against the original, and write a side-by-side). Add the loader contract of 5.3 with unit tests (test-first). Make **two pilot textures**: the red HP gauge `newui_menu_red.jpg` (45 x 39) and one frame piece `newui_item_back01.tga` (190 x 64), each at 2x and 4x. Build, and ask the owner for screenshots of the same state with HD off, tier 2 and tier 4 at his window size and, if he can, at a large window. **Decide the tiers from what he sees.**
*Acceptance:* unit tests pass; the 1x path is byte-for-byte unchanged (a render with the feature off matches the baseline); the pilot looks the same as the original, only sharper; memory and load time are measured and noted.

**Step 3. The shared window frame kit.**
Regenerate `newui_msgbox_back.jpg`, `newui_item_back01/04.tga`, `newui_item_back02-L/-R.tga`, `newui_item_back03.tga`, the nine `newui_item_table0x` pieces, the close button `newui_exit_00.tga` and the related window button textures that are shared. Check at least the Character info, Inventory and a window you did not target (warehouse, trade, a shop) for seams, offsets or stretched strips.
*Acceptance:* HD vs 1x screenshots of those windows differ only by sharpness; no one-pixel seams; no layout movement; fallback still works when the HD folder is removed.

**Step 4. The bottom bar.**
The frame pieces `newui_menu01/02/03/02-03`, the HP, Mana, AG and SD gauges, the experience bars, the five menu button strips, `newui_skillbox*.jpg`, and the skill atlases `newui_skill*`, `newui_command`, `newui_non_*` (**these are the hardest**: hundreds of small icons, see 7.3). Verify the gauges fill correctly at 0 %, 50 % and 100 %, and that the greyed `non_` atlases line up cell for cell with their coloured twins.

**Step 5. Character info and Inventory specifics.**
The stat buttons, text boxes, level and quest and pet and master buttons, the nine equipment slot backgrounds, the Zen bar, repair, expand and shop buttons, and the inventory grid cell textures. Verify every button state (up, hover, down, locked).

**Step 6. Verification sweep.**
Open every window you can (Character, Inventory, Skill list, Party, Guild, Quest, Warehouse, Trade, Shop, Options, Chat, mini-map, the Auction House, the login and character-selection screens if they share textures) with the feature on and off. Collect numbers: total HD texture memory in use, extra startup or window-open time, frame time with the HUD visible (before and after). Make sure nothing regressed in the other windows that share the frame kit.

**Step 7. Text and remaining softness.**
Check whether text stays crisp at every UI scale (fonts are rendered by the font system at the typography scale). If any text is blurry, say where and why; do not fix it unasked, but report it. List every interface texture that is **still low resolution** in a manifest file (`tools/ui_hd/manifest.*`), by window, so the next sessions can continue systematically.

**Step 8. Handover.**
Update the usage docs (the setting, the tiers, how to add a new HD texture), commit, and give the owner a plain summary with before and after pairs, the measured costs, and the proposed next phase (for example: shops, trade, warehouse, party, guild, quests, then the rest by popularity).

## 7. Making the art

### 7.1 The standard: "same, but sharp"

- **Fidelity test (use it on every texture):** reduce the HD image by k with a good filter; it must look the same as the original (same shapes, positions, colours, shading, texture grain). Report the mean error and SSIM from your compare script; calibrate thresholds on the pilot and write them down. For JPEG-sourced originals compare after removing the compression noise, and prefer the HD image to be *cleaner* than the original, never different.
- **Do not invent.** No new ornaments, no changed colours or proportions, no extra glow. If an original detail is ambiguous, keep the most plausible reading and flag it to the owner.
- **Colour and gamma:** keep sRGB values consistent: compare mean colours per region before and after. The interface is drawn without colour management.
- **Alpha:** keep each texture's alpha semantics. Sprites on black that were drawn with additive blending must stay pure black outside the shape.
- **Pixel alignment:** content positions scale exactly by k (a feature at x = 12 stays at x = 12k). Do not crop, pad or re-centre.

### 7.2 Methods, by kind of art (decide with a pilot; ask before downloading anything)

1. **Flat and geometric art** (gauges, boxes, button faces, bars, thin lines, bevels, the table corners): **redraw procedurally** at the master resolution with Pillow, numpy or a vector renderer (supersample and reduce, as `tools/auction_icons/make_category_icons.py` does). Measure colours and gradients from the original and reproduce them as parametric shapes. This gives the cleanest result.
2. **Ornate frames and stone textures** (`newui_msgbox_back`, the corner ornaments in `newui_item_back01..04`): a **high-quality upscale plus re-synthesised fine detail**. Candidates: Lanczos upscale with sharpening and noise-matched grain (no downloads needed), or a learned upscaler such as Real-ESRGAN, waifu2x or SwinIR (**ask the owner before downloading** a model or executable; say what it is, where from and how big). For ornaments with clear edges, tracing to vector shapes and re-rendering can beat an upscale.
3. **Icons** (the skill atlases, the equipment slot silhouettes): per-icon upscale with a learned model, or redraw. Keep each icon's silhouette and colours. Work cell by cell, never on the whole atlas at once, so models cannot bleed content across cells.
4. **Textures with baked text** (some button and label textures contain letters): upscale them with care so the text stays legible, and list them in the manifest; text itself may later be better rendered by the font system.
5. If your session has an image-generation or upscaling tool of its own, tell the owner what it is before you use it; keep generated output as **masters under version control**, not as untracked files.

### 7.3 The skill atlases deserve a plan of their own

`newui_skill.jpg`, `newui_skill2.jpg`, `newui_skill3.jpg`, `newui_command.jpg` and the greyed `newui_non_*` twins are 256 x 256 JPEG atlases of small icons (a few dozen to a few hundred cells). The coloured and grey versions must stay **cell-aligned**, and some are also drawn by other windows (the MU Helper window uses `newui_skill.jpg`; see `NewUIMuHelper.cpp`). Inspect them first (convert to PNG, view them), decide a per-cell workflow, and do the pilot on one atlas before committing to all of them. Consider deriving the grey twin from the coloured HD cell with the same desaturation the originals use, if that reproduces them.

### 7.4 Keep everything reproducible

Masters (highest tier, PNG) in version control, lower tiers generated by a script (`tools/ui_hd/`), a manifest listing each texture with its original size, its tier files, its method and its status. A future session must be able to regenerate any tier from the masters in one command.

## 8. Build, test, run and verify

**Build (cmd, from the repository or worktree root; adjust the folder):**

```
set "VCPKG_ROOT=C:\Users\rause\OneDrive\Desktop\OpenMU-trial\vcpkg"
set "NUGET_PACKAGES=C:\Users\rause\OneDrive\Desktop\mu-online-openmu\client\.worktrees\auction-house\.nuget"
call "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" -arch=x86 -host_arch=x64
set "PATH=C:\Program Files (x86)\Microsoft Visual Studio\Installer;%PATH%"
cd /d C:\Users\rause\OneDrive\Desktop\mu-online-openmu\client\.worktrees\ui-hd
cmake --preset windows-x86
cmake --build out/build/windows-x86 --config Release --target Main
```

The `PATH` line is required (the NativeAOT link of `MUnique.Client.Library.dll` needs `vswhere.exe`; the symptom is "exited with code 123"). `cmake --preset windows-x86` is only needed once per new build directory. The program is `out\build\windows-x86\src\Release\Main.exe`; start it with its own folder as the working directory. It reads `config.ini` there (created from a template on the first build and never overwritten afterwards).

**Tests:** build everything (`cmake --build out/build/windows-x86 --config Release`) and run `ctest --test-dir out/build/windows-x86 -C Release --output-on-failure`. New tests go next to the existing render tests (`tests/render/`, see `test_ui_texture_quality.cpp`) and are registered in that folder's `CMakeLists.txt`. Test the pure logic (scale computation, path mapping, tier choice, gutter generation, the compare metric); you cannot unit-test the GPU path, so verify that with screenshots.

**Automated screenshots (optional, with the owner's permission):** the repository has a developer build with a **control socket** (`docs/control-socket.md`). Configure a second build directory with the preset `windows-x86-mueditor` (editor on, `ENABLE_CONTROL_SOCKET=ON`; it needs the `imgui` submodule), start `Main.exe` with the environment variable `MU_CONTROL_SOCKET` set to a socket path, and you can `login`, `select-char`, press hotkeys (`hotkey` with `c`, `i`), `click-ui`, and `screenshot` to a file, then compare before and after automatically. Use it only with a test account the owner gives you, tell him before a window appears on his screen, and prefer his own screenshots for the final judgement.

**Verification you must do before saying a step is done:** (1) the feature **off** renders as before (compare with the baseline screenshots); (2) the feature **on** looks the same but sharper (side by side, crops at 400 %); (3) removing the HD folder falls back cleanly; (4) memory and time numbers; (5) tests pass; (6) nothing regressed in windows that share the textures. Never claim success without having seen the evidence.

## 9. Ask the owner these early (batch them in one message, with your recommendation for each)

1. **Display and tiers.** What is his monitor (resolution and Windows scale in Settings, Display)? My recommendation: tiers 2 and 4 with automatic choice, 4x for the core windows and 2x elsewhere, decided after the pilot.
2. **Storage.** May the repository grow (the core set alone is about 15 MB at 2x and 61 MB at 4x as raw OZT, much less as PNG)? Options: PNG masters in the repository, Git LFS, or a separate art folder. My recommendation: PNG masters in the repository for phase 1, revisit before the other 700 textures.
3. **Tools.** May you download an upscaling model or tool (name, source, size to be stated)? Is procedural redrawing for most of the art acceptable? Is he happy for you to use an image-generation tool if your session has one?
4. **Account and a second client.** A test account and character for the automated screenshots, and permission to open a second client window on his machine for a few minutes at a time.
5. **Priorities inside phase 1.** Which of the three (Character, Inventory, bottom bar) first, and anything he especially dislikes or wants kept exactly (the orbs, the stone texture, the ornaments).
6. **Opt-out.** Should the HD setting be switchable for weaker machines? (Recommended: yes, `[UI] HdTextures = auto | off | 2 | 4`.)
7. **Publishing.** Default is local commits only. Ask when he wants a push, and to which branch (this work is on `codex/ui-hd`).

## 10. Deliverables and how to report

- **Code:** the loader support with unit tests, the configuration setting and its documentation, the `tools/ui_hd/` tooling, all as small commits on `codex/ui-hd`.
- **Assets:** the HD masters and tiers for the phase-1 textures, the manifest of what is done and what remains.
- **Evidence:** before and after pairs for every step, the memory and timing numbers, the test results (counts passed), and a list of anything you could not verify.
- **At each step end, tell the owner in a few plain sentences:** what changed, what to open in the game to see it, and what you need from him next. Mention honestly anything that looks worse or different, even slightly.
- **Final report for phase 1:** what was done, what is measured, what is left (the manifest), the recommended next phase, and any decision you took on his behalf with the reason and what it costs if wrong.

---

## Appendix A. File formats and the converter

**`.OZT` (what the engine reads, `CGlobalBitmap::OpenTga`):** an uncompressed true-colour 32-bit TGA with an extra 4 bytes after the first 12 header bytes. Byte layout of the file:

| Offset | Size | Content |
|---|---|---|
| 0 | 1 | ID length (0) |
| 1 | 1 | colour map type (0) |
| 2 | 1 | data type code (2 = uncompressed true colour) |
| 3 | 5 | colour map specification (zeros) |
| 8 | 2 + 2 | x origin, y origin (zeros) |
| 12 | 4 | **extra OZT prefix** (zeros; the engine skips it) |
| 16 | 2 | width, little-endian signed 16-bit |
| 18 | 2 | height, little-endian signed 16-bit |
| 20 | 1 | bits per pixel (**must be 32**) |
| 21 | 1 | image descriptor (0) |
| 22 | width x height x 4 | pixels as **B, G, R, A**, rows stored **bottom to top** |
| end | 26 | footer: 8 zero bytes then `TRUEVISION-XFILE.` and a zero byte (optional for the engine) |

**`.OZJ`:** 24 arbitrary header bytes followed by an ordinary baseline JPEG (TurboJPEG decodes it; 4:4:4 or other sub-sampling, RGB). The size is read from the JPEG header.

**Working PNG-to-OZT converter (Pillow):**

```python
import struct
from PIL import Image

def png_to_ozt(src_path, dst_path):
    im = Image.open(src_path).convert("RGBA")
    nx, ny = im.size                      # HD tiers may exceed 1024 once the loader limit is raised
    px = im.load()
    header = bytearray()
    header += bytes([0, 0, 2]) + bytes(5) + bytes(2) + bytes(2)   # id len, cmap type, data type, cmap spec, x/y origin
    header += bytes(4)                                            # OZT prefix
    header += struct.pack("<h", nx) + struct.pack("<h", ny)
    header += bytes([32, 0])                                      # 32 bpp, descriptor
    assert len(header) == 22
    body = bytearray(nx * ny * 4)
    i = 0
    for y in range(ny - 1, -1, -1):                               # bottom row first
        for x in range(nx):
            r, g, b, a = px[x, y]
            body[i:i + 4] = bytes((b, g, r, a))
            i += 4
    footer = bytes(8) + b"TRUEVISION-XFILE.\x00"
    open(dst_path, "wb").write(bytes(header) + bytes(body) + footer)
```

The reverse (OZT or OZJ to PNG) is the same arithmetic backwards; for OZJ strip 24 bytes and decode the JPEG. The previous session's survey script parsed OZJ by scanning for the JPEG start-of-frame marker (`0xFFC0`, `0xFFC1` or `0xFFC2`) after the 24-byte header.

## Appendix B. The phase-1 textures (file size, padded size, format; measured from `src/bin/Data/Interface`)

A texture used by more than one window is listed once, under the first window that loads it. Padded size is what the GPU texture is today (power of two, content top-left).


**Bottom bar and menu (`HUD/NewUIMainFrameWindow.cpp`)**

| Texture | File size | Padded | Format |
|---|---|---|---|
| `Interface/newui_menu01.jpg` | 256 x 51 | 256 x 64 | JPEG (OZJ) |
| `Interface/newui_menu02.jpg` | 128 x 51 | 128 x 64 | JPEG (OZJ) |
| `Interface/partCharge1/newui_menu03.jpg` | 256 x 51 | 256 x 64 | JPEG (OZJ) |
| `Interface/newui_menu02-03.jpg` | 160 x 42 | 256 x 64 | JPEG (OZJ) |
| `Interface/newui_menu_blue.jpg` | 45 x 39 | 64 x 64 | JPEG (OZJ) |
| `Interface/newui_menu_green.jpg` | 45 x 39 | 64 x 64 | JPEG (OZJ) |
| `Interface/newui_menu_red.jpg` | 45 x 39 | 64 x 64 | JPEG (OZJ) |
| `Interface/newui_menu_ag.jpg` | 16 x 39 | 16 x 64 | JPEG (OZJ) |
| `Interface/newui_menu_sd.jpg` | 16 x 39 | 16 x 64 | JPEG (OZJ) |
| `Interface/newui_exbar.jpg` | 6 x 4 | 8 x 4 | JPEG (OZJ) |
| `Interface/Exbar_Master.jpg` | 6 x 4 | 8 x 4 | JPEG (OZJ) |
| `Interface/partCharge1/newui_menu_Bt05.jpg` | 30 x 164 | 32 x 256 | JPEG (OZJ) |
| `Interface/partCharge1/newui_menu_Bt01.jpg` | 30 x 164 | 32 x 256 | JPEG (OZJ) |
| `Interface/partCharge1/newui_menu_Bt02.jpg` | 30 x 164 | 32 x 256 | JPEG (OZJ) |
| `Interface/partCharge1/newui_menu_Bt03.jpg` | 30 x 164 | 32 x 256 | JPEG (OZJ) |
| `Interface/partCharge1/newui_menu_Bt04.jpg` | 30 x 164 | 32 x 256 | JPEG (OZJ) |
| `Interface/newui_skill.jpg` | 256 x 256 | 256 x 256 | JPEG (OZJ) |
| `Interface/newui_skill2.jpg` | 256 x 256 | 256 x 256 | JPEG (OZJ) |
| `Interface/newui_command.jpg` | 256 x 256 | 256 x 256 | JPEG (OZJ) |
| `Interface/newui_skillbox.jpg` | 32 x 38 | 32 x 64 | JPEG (OZJ) |
| `Interface/newui_skillbox2.jpg` | 32 x 38 | 32 x 64 | JPEG (OZJ) |
| `Interface/newui_non_skill.jpg` | 256 x 256 | 256 x 256 | JPEG (OZJ) |
| `Interface/newui_non_skill2.jpg` | 256 x 256 | 256 x 256 | JPEG (OZJ) |
| `Interface/newui_non_command.jpg` | 256 x 256 | 256 x 256 | JPEG (OZJ) |
| `Interface/newui_skill3.jpg` | 256 x 256 | 256 x 256 | JPEG (OZJ) |
| `Interface/newui_non_skill3.jpg` | 256 x 256 | 256 x 256 | JPEG (OZJ) |

**Character info (`Character/NewUICharacterInfoWindow.cpp`)**

| Texture | File size | Padded | Format |
|---|---|---|---|
| `Interface/newui_msgbox_back.jpg` | 225 x 512 | 256 x 512 | JPEG (OZJ) |
| `Interface/newui_item_back04.tga` | 190 x 64 | 256 x 64 | 32-bit TGA (OZT) |
| `Interface/newui_item_back02-L.tga` | 21 x 320 | 32 x 512 | 32-bit TGA (OZT) |
| `Interface/newui_item_back02-R.tga` | 21 x 320 | 32 x 512 | 32-bit TGA (OZT) |
| `Interface/newui_item_back03.tga` | 190 x 45 | 256 x 64 | 32-bit TGA (OZT) |
| `Interface/newui_item_table01(L).tga` | 14 x 14 | 16 x 16 | 32-bit TGA (OZT) |
| `Interface/newui_item_table01(R).tga` | 14 x 14 | 16 x 16 | 32-bit TGA (OZT) |
| `Interface/newui_item_table02(L).tga` | 14 x 14 | 16 x 16 | 32-bit TGA (OZT) |
| `Interface/newui_item_table02(R).tga` | 14 x 14 | 16 x 16 | 32-bit TGA (OZT) |
| `Interface/newui_item_table03(Up).tga` | 1 x 14 | 1 x 16 | 32-bit TGA (OZT) |
| `Interface/newui_item_table03(Dw).tga` | 1 x 14 | 1 x 16 | 32-bit TGA (OZT) |
| `Interface/newui_item_table03(L).tga` | 14 x 1 | 16 x 1 | 32-bit TGA (OZT) |
| `Interface/newui_item_table03(R).tga` | 14 x 1 | 16 x 1 | 32-bit TGA (OZT) |
| `Interface/newui_exit_00.tga` | 36 x 58 | 64 x 64 | 32-bit TGA (OZT) |
| `Interface/newui_cha_textbox02.tga` | 170 x 21 | 256 x 32 | 32-bit TGA (OZT) |
| `Interface/newui_chainfo_btn_level.tga` | 16 x 30 | 16 x 32 | 32-bit TGA (OZT) |
| `Interface/newui_chainfo_btn_quest.tga` | 36 x 58 | 64 x 64 | 32-bit TGA (OZT) |
| `Interface/newui_chainfo_btn_pet.tga` | 36 x 58 | 64 x 64 | 32-bit TGA (OZT) |
| `Interface/newui_chainfo_btn_master.tga` | 36 x 58 | 64 x 64 | 32-bit TGA (OZT) |

**Inventory (`Inventory/NewUIMyInventory.cpp`)**

| Texture | File size | Padded | Format |
|---|---|---|---|
| `Interface/newui_item_back01.tga` | 190 x 64 | 256 x 64 | 32-bit TGA (OZT) |
| `Interface/newui_item_boots.tga` | 46 x 46 | 64 x 64 | 32-bit TGA (OZT) |
| `Interface/newui_item_cap.tga` | 46 x 46 | 64 x 64 | 32-bit TGA (OZT) |
| `Interface/newui_item_fairy.tga` | 46 x 46 | 64 x 64 | 32-bit TGA (OZT) |
| `Interface/newui_item_wing.tga` | 61 x 46 | 64 x 64 | 32-bit TGA (OZT) |
| `Interface/newui_item_weapon(L).tga` | 46 x 66 | 64 x 128 | 32-bit TGA (OZT) |
| `Interface/newui_item_weapon(R).tga` | 46 x 66 | 64 x 128 | 32-bit TGA (OZT) |
| `Interface/newui_item_upper.tga` | 46 x 66 | 64 x 128 | 32-bit TGA (OZT) |
| `Interface/newui_item_gloves.tga` | 46 x 46 | 64 x 64 | 32-bit TGA (OZT) |
| `Interface/newui_item_lower.tga` | 46 x 46 | 64 x 64 | 32-bit TGA (OZT) |
| `Interface/newui_item_ring.tga` | 28 x 28 | 32 x 32 | 32-bit TGA (OZT) |
| `Interface/newui_item_necklace.tga` | 28 x 28 | 32 x 32 | 32-bit TGA (OZT) |
| `Interface/newui_item_money.tga` | 170 x 26 | 256 x 32 | 32-bit TGA (OZT) |
| `Interface/newui_repair_00.tga` | 36 x 58 | 64 x 64 | 32-bit TGA (OZT) |
| `Interface/newui_expansion_btn.tga` | 36 x 58 | 64 x 64 | 32-bit TGA (OZT) |
| `Interface/newui_Bt_openshop.tga` | 36 x 58 | 64 x 64 | 32-bit TGA (OZT) |
| `Interface/newui_Bt_closeshop.tga` | 36 x 58 | 64 x 64 | 32-bit TGA (OZT) |

**Inventory grid and shared pieces (`Inventory/NewUIInventoryCtrl.cpp`)**

| Texture | File size | Padded | Format |
|---|---|---|---|
| `Interface/newui_item_box.tga` | 21 x 21 | 32 x 32 | 32-bit TGA (OZT) |
| `Interface/newui_inven_usebox_01.tga` | 19 x 19 | 32 x 32 | 32-bit TGA (OZT) |
| `Interface/newui_inven_usebox_02.tga` | 19 x 20 | 32 x 32 | 32-bit TGA (OZT) |
| `Interface/newui_inven_usebox_03.tga` | 19 x 20 | 32 x 32 | 32-bit TGA (OZT) |


## Appendix C. Key files

| What | Where (client repository) |
|---|---|
| Texture loaders, padding, sampler, memory counter | `src/source/Render/Sprites/GlobalBitmap.cpp` and `.h` (`OpenTga`, `OpenJpegTurbo`, `UploadTextureSDLGpu`, `MAX_WIDTH`) |
| Interface texture helpers and their test | `src/source/Render/Textures/UiTextureQuality.h`, `tests/render/test_ui_texture_quality.cpp` |
| Draw helpers and UV arithmetic | `src/source/UI/NewUI/NewUICommon.cpp` (`RenderImage`, `RenderImageStretch`), `src/source/Render/Textures/ZzzOpenglUtil.cpp` (`RenderBitmap`) |
| Logical canvas and scaling | `src/source/UI/Scaling/UITransform.h` and `.cpp` |
| Layout policy | `src/source/UI/NewUI/UILayoutPolicy.*`, `src/source/UI/NewUI/NewUISystem.*` |
| Bottom bar | `src/source/UI/NewUI/HUD/NewUIMainFrameWindow.cpp` |
| Character info | `src/source/UI/NewUI/Character/NewUICharacterInfoWindow.cpp` |
| Inventory | `src/source/UI/NewUI/Inventory/NewUIMyInventory.cpp`, `NewUIInventoryCtrl.cpp` |
| Hotkeys (`C` character, `I` inventory, `A` Auction House) | `src/source/UI/NewUI/HUD/NewUIHotKey.cpp` |
| Nine-slice and the Auction House look | `src/source/UI/NewUI/Widgets/NineSlice.*`, `src/source/UI/NewUI/AuctionHouse/AuctionChrome.*` |
| Texture index enums | `src/source/Core/Globals/_TextureIndex.h` and each window's header |
| Asset copy to the build folder | `src/CMakeLists.txt` (`MU_COPY_RUNTIME_ASSETS`), `cmake/CopyRuntimeAssets.cmake` |
| Interface assets | `src/bin/Data/Interface/` (and sub-folders `partCharge1`, `InGameShop`, `MacroUI`) |
| Build presets | `CMakePresets.json` (`windows-x86`, `windows-x86-mueditor`, ...) |
| Rules and docs | `AGENTS.md`, `docs/CODING_RULES.md`, `docs/build/README.md`, `docs/control-socket.md`, `docs/in-game-tests.md`, `docs/options-window.md` |

When you are done with a step, report to the owner in plain words. If something blocks you, say exactly where you stopped and why.
