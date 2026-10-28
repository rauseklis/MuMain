"""Guard the production paths of the five initially reported skill regressions."""

from pathlib import Path
import sys


root = Path(sys.argv[1])
move_handlers = (
    root / "src/source/Render/Effects/Behaviors/MoveHandlers.cpp"
).read_text(encoding="utf-8")
effects = (root / "src/source/Render/Effects/ZzzEffect.cpp").read_text(
    encoding="utf-8"
)
characters = (root / "src/source/Engine/Object/ZzzCharacter.cpp").read_text(
    encoding="utf-8"
)


def function_body(source: str, signature: str) -> str:
    start = source.index(signature)
    brace = source.index("{", start)
    depth = 0
    for index in range(brace, len(source)):
        if source[index] == "{":
            depth += 1
        elif source[index] == "}":
            depth -= 1
            if depth == 0:
                return source[brace : index + 1]
    raise AssertionError(f"unterminated function: {signature}")


twisting_driver = function_body(move_handlers, "bool Move_MODEL_SKILL_WHEEL1")
assert "CreateEffect(MODEL_SKILL_WHEEL2" in twisting_driver, (
    "Twisting Slash must retain its staged orbit trail instead of one opaque blade"
)
assert "SkillTiming::TwistingSlashStage" in twisting_driver, (
    "Twisting Slash must emit the canonical staged wheel subtypes"
)
assert "o->LifeTime = 0" not in twisting_driver, (
    "Twisting Slash's trail driver must live long enough to emit every stage"
)

twisting_orbit = function_body(move_handlers, "bool Move_MODEL_SKILL_WHEEL2")
assert "AngleMatrix(o->Angle, Matrix)" in twisting_orbit
assert "o->Angle[2] -= 18 * FPS_ANIMATION_FACTOR" in twisting_orbit, (
    "Twisting Slash's rendered blade must follow the canonical orbit angle"
)
assert "DeleteEffect(MODEL_SKILL_WHEEL2" not in characters, (
    "A new Twisting Slash cast must not erase the staged trail of the previous cast"
)

flame_render_start = effects.index("case BITMAP_FLAME:", effects.index("void RenderEffect"))
flame_render_end = effects.index("case BITMAP_LIGHTNING + 1:", flame_render_start)
flame_render = effects[flame_render_start:flame_render_end]
assert "case BITMAP_FLAME_VOID:" in flame_render, (
    "Scroll of Flame's violet replacement must retain the original ground footprint"
)
assert "RenderTerrainAlphaBitmap(o->Type" in flame_render, (
    "The Flame ground footprint must render the active flame texture"
)

ice_storm = function_body(move_handlers, "bool Move_MODEL_BLIZZARD")
assert "CreateEffectFpsChecked(MODEL_BLIZZARD" not in ice_storm, (
    "Ice Storm impact children are one-shot events and must not be randomly dropped at high FPS"
)
blizzard_create_start = effects.index("case MODEL_BLIZZARD:")
blizzard_create_end = effects.index("case MODEL_ARROW_DRILL:", blizzard_create_start)
blizzard_create = effects[blizzard_create_start:blizzard_create_end]
assert "(rand() % 30 + 10) * FPS_ANIMATION_FACTOR" not in blizzard_create, (
    "Ice Storm's one-time initial fall speed must not depend on the creation-frame FPS"
)
assert "(100.f) * FPS_ANIMATION_FACTOR" not in blizzard_create, (
    "Ice Storm's one-time spawn offset must not shrink as FPS rises"
)

fire_slash_start = characters.index("case AT_SKILL_FIRE_SLASH:")
fire_slash_end = characters.index("case AT_SKILL_POWER_SLASH:", fire_slash_start)
fire_slash = characters[fire_slash_start:fire_slash_end]
assert "o->AnimationFrame >= 3.f && rand_fps_check(1)" not in fire_slash, (
    "Fire Slash's one-shot arc must use an animation crossing, not a random high-FPS gate"
)

fire_burst = function_body(move_handlers, "bool Move_MODEL_PIER_PART")
assert "SkillTiming::ReferenceEventsDue" in fire_burst
assert "CreateEffectFpsChecked(MODEL_PIER_PART" not in fire_burst, (
    "Fire Burst's continuous trail must not use random per-frame emission"
)

print("PASS: reported skill production paths preserve canonical staged and one-shot effects")
