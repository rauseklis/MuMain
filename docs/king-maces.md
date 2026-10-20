# King mace names

Mace of King (group 2, item 22) and Royal Mace of Emberion (group 2, item 23)
use orange name text in inventory tooltips and ground labels. Other item names
and the colors of stats, requirements and option descriptions are unchanged.
Excellent/ancient prefixes and option text remain visible where applicable.

The visual setup was restarted using only MaceOfKing.rar. Item 2:22 selects
Data/Item/MaceOfKing/Mace.bmd and the three matching OZ_nfm textures in that
folder. These are unmodified archive files. No old models, effect meshes or
placeholder textures are used. The archive's effect configuration snippets are
for another client and are not installed.

Royal (item 2:23) uses the same new three-mesh weapon shape, with the approved
Blood Emperor palette: dark steel, silver edges and deep red accents. Its files
are under Data/Item/RoyalMaceOfKing. Only the model's
texture references differ; geometry, UVs, bones and animations are unchanged.
The recolor preserves texture dimensions, shading layout and exact TGA alpha.
Unique Royal texture names allow both variants to load independently.

The old KingMaces asset folders remain removed. Archived copies are outside
the client under C:/mu/.runtime/backups. Server definitions, stats and existing
bank items were not changed by the visual work. Restart the client to load the
Royal variant; final in-game appearance still needs review.

Equipped Royal maces emit built-in fire particles around their head from +7.
Flames grow at every upgrade level, with one emitter at +7, two at +9/+11 and
three at +13. Scale is bounded at +15. Both equipped hands are supported; the
normal mace and other items are unaffected. No flames appear below +7 or in
inventory/ground previews. Shadow and selection outline passes do not emit.
Particles use the client's FPS-adjusted emitter in the linked equipped-weapon
render path, with that weapon's freshly calculated bone transforms and its
character owner. Royal keeps the normal upgrade chrome/shine on all meshes.
This is client-only visual behavior and changes no stats, damage or server rules.

Weapon tooltips use +4 minimum and maximum damage per upgrade level, matching
the server. Requirements and base stats match the staged server definitions.
Ordinary weapon upgrade calculations and colors are unchanged.

Rare mace requirements match server update 124: STR/AGI coefficients 480/334
for normal and 968/675 for Royal. Plain +0 requirements are 1200/841 and
2401/1680 respectively; upgrades follow the normal requirement formula.
Additional damage +4 requires 5 extra STR and no extra AGI. Both weapons
remain without a built-in weapon skill.
