# Skill requirements

## 2026-10-15: Summoner reset-friendly skills

Summoner, Bloody Summoner and Dimension Master do not need to regain a character
level before casting their learned spells after a reset. This includes
Summoner-compatible shared spells and the base spells behind master upgrades.
The spell tooltips follow the same rule as their use checks.

Energy, strength, mana, AG, cooldowns, class advancement, master-tree prerequisites
and equipment requirements are unchanged. Learning parchments is unchanged.
Books equipped as weapons and wings still have their normal equipment
requirements. Other classes are unchanged.

The original skill-data level remains intact because it also scales the energy
requirement. Only the character-level gate is removed, not that energy scaling.

The client policy is in
`src/source/GameLogic/Skills/SkillLevelRequirements.cpp`. The small resolver and
its tests are in `SkillLevelRequirements.h` and
`tests/core/test_skill_level_requirements.cpp`. The current Season 6 server
configuration already has no character-level requirement on the base Summoner
spells and parchments; no database-only override is needed.

Validation: the Windows x86 Release client builds successfully and all 500 CTest
tests pass, including the two reset-level regression cases (18 assertions).
The owner still needs to confirm casting the affected spells in the running game.

## Live check

With a low-level Summoner and enough energy, cast a learned
spell whose original client data had a higher level requirement. Check that its
tooltip no longer lists that level. Repeat below the energy requirement: the
spell must still be unavailable. Check a different class and equipment to confirm
their level requirements still apply.
