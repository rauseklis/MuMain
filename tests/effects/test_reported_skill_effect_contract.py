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
effect_joints = (root / "src/source/Render/Effects/ZzzEffectJoint.cpp").read_text(
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
assert "AngleMatrix(OrbitAngle, Matrix)" in twisting_orbit
assert "o->Timer -= 18 * FPS_ANIMATION_FACTOR" in twisting_orbit, (
    "Twisting Slash's orbit must advance at the canonical rate"
)
assert "o->Angle[2] -= 18 * FPS_ANIMATION_FACTOR" in twisting_orbit, (
    "Twisting Slash's rendered weapon must retain its canonical rotation"
)
assert "SkillTiming::TwistingSlashRadius" in twisting_orbit, (
    "Twisting Slash must use the reviewed, bounded visual radius"
)
assert "OrbitAngle[2] = o->Timer" in twisting_orbit, (
    "Twisting Slash's orbit phase must not be polluted by render-time weapon spin"
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
assert "RenderTerrainAlphaBitmap(BITMAP_LIGHT" in flame_render, (
    "The violet Flame ground footprint must use a bounded radial texture"
)
assert "RenderTerrainAlphaBitmap(o->Type" not in flame_render, (
    "The clamped void-flame JPEG must not be stretched across terrain tiles"
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

fire_slash_effect = function_body(move_handlers, "bool Move_BITMAP_SWORD_FORCE")
assert "CreateJointFpsChecked" not in fire_slash_effect, (
    "Fire Slash's only sword-force joint is a one-shot event and must never be randomly discarded"
)

fire_burst = function_body(move_handlers, "bool Move_MODEL_PIER_PART")
assert "SkillTiming::ReferenceEventsDue" in fire_burst, (
    "Fire Burst must emit trail segments on deterministic legacy ticks"
)
assert "CreateEffectFpsChecked(MODEL_PIER_PART" not in fire_burst, (
    "Fire Burst's canonical trail must not contain random high-FPS gaps"
)
assert "CreateEffect(MODEL_PIER_PART" in fire_burst, (
    "Fire Burst must retain its canonical trail segment"
)
homing_loop_start = fire_burst.index("for (int i = 1; i < o->Gravity; ++i)")
homing_loop_brace = fire_burst.index("{", homing_loop_start)
depth = 0
homing_loop_end = None
for index in range(homing_loop_brace, len(fire_burst)):
    if fire_burst[index] == "{":
        depth += 1
    elif fire_burst[index] == "}":
        depth -= 1
        if depth == 0:
            homing_loop_end = index + 1
            break
assert homing_loop_end is not None
homing_loop = fire_burst[homing_loop_start:homing_loop_end]
assert "CreateEffect(MODEL_PIER_PART" not in homing_loop, (
    "Fire Burst must emit one trail child per reference event, not one per growing homing sub-step"
)
assert "SkillTiming::FireBurstMaximumGravity" in fire_burst, (
    "Fire Burst must cap its growing homing sub-steps so the projectile cannot accelerate without bound"
)

rageful_blow = function_body(move_handlers, "bool Move_MODEL_SKILL_FURY_STRIKE")
assert "pow(-1" not in rageful_blow, (
    "Rageful Blow must not raise a negative value to a fractional FPS exponent"
)
assert "SkillTiming::CrossedDown" in rageful_blow, (
    "Rageful Blow's rise, return and impact milestones must be crossed exactly once"
)

evil_spirit_start = characters.index("case AT_SKILL_EVIL_SPIRIT:")
evil_spirit_end = characters.index("case AT_SKILL_PLASMA_STORM_FENRIR:", evil_spirit_start)
evil_spirit = characters[evil_spirit_start:evil_spirit_end]
assert "SkillTiming::CardinalAngle" in evil_spirit, (
    "Evil Spirit must generate four exact cardinal launch angles from one tested definition"
)

evil_spirit_move_start = effect_joints.rindex("case BITMAP_JOINT_SPIRIT:")
evil_spirit_move = effect_joints[evil_spirit_move_start:evil_spirit_move_start + 10000]
assert "sqrtf(FPS_ANIMATION_FACTOR)" in evil_spirit_move
assert "powf(0.6f, FPS_ANIMATION_FACTOR)" in evil_spirit_move
assert "powf(0.8f, FPS_ANIMATION_FACTOR)" in evil_spirit_move
assert evil_spirit_move.count("rand() % 33 - 16") >= 2, (
    "Evil Spirit steering noise must be centered on zero instead of carrying a directional bias"
)

print("PASS: reported skill production paths preserve canonical staged and one-shot effects")
