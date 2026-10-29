# Skill requirements

All classes can cast already learned skills without a character-level minimum.
This also applies to Master replacements and after a reset. Skill tooltips no
longer show a minimum casting level, including previews without a character.

Learning a scroll, orb, crystal or parchment still requires its configured
learning level and stats. Dark Horse keeps its equipment level requirement.
Class advancement, equipment, master-tree prerequisites, cooldowns, mana and AG
costs still apply.

Required stats are strict: a spell requiring 220 energy cannot be cast with 219;
220 meets that energy requirement, provided every other requirement is met.
Strength, agility and leadership requirements behave the same way. Skill data
retains its original values, including the level value used to calculate energy
requirements; removing the casting-level check does not lower the energy cost.

Online casting, offline combat and offline buffs follow the same server rule.
Stat checks use the current values, so spending or removing stats updates whether
a skill can be used.

After installation, check Evil Spirit on a low-level Soul Master: it should have
no minimum casting-level line, should fail below the required energy, and should
cast once energy and all other requirements are sufficient. Also check a learned
Knight skill after reset, a Master replacement, an orb below its learning level,
and Dark Horse below its equipment level. Automated checks do not verify gameplay.
