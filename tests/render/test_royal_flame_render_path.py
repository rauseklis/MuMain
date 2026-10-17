"""Catch an emitter added to body rendering instead of equipped weapon rendering."""
from pathlib import Path
import json
import sys

root = Path(sys.argv[1])
character = (root / 'src/source/Engine/Object/ZzzCharacter.cpp').read_text(encoding='utf-8')
body = (root / 'src/source/Engine/Object/ZzzObject.cpp').read_text(encoding='utf-8')
start = character.index('void RenderLinkObject(')
link = character[start:character.index('void ', start + len('void RenderLinkObject('))]
call = 'Render::Items::RoyalFlames::Emit(b, o, Type, Level, o->Alpha, 0, RenderType,'
assert call in link, 'Equipped weapons must reach the flame emitter with their character owner'
assert 'Translate, BoneTransform);' in link[link.index(call):link.index(call) + 180]
assert link.index(call) > link.index('b->Animation(BoneTransform'), 'Weapon bones must be animated first'
assert link.index(call) > link.index('b->Transform(BoneTransform'), 'Weapon transform must precede emission'
assert 'RoyalFlames::Emit' not in body, 'Body/preview rendering must not emit duplicate flames'
models = json.loads((root / 'src/bin/Data/Items/Models/Group02_Mace.json').read_text())
royal = next(model for model in models['models'] if model['number'] == 23)
assert 'glow' not in royal, 'Royal must retain the normal upgrade glow'
print('PASS: equipped render path, animated weapon bones, character owner, no body duplicates, default upgrade glow')
