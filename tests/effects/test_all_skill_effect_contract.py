"""Guard one-shot and continuous timing rules found by the all-skill FPS audit."""

from pathlib import Path
import sys


root = Path(sys.argv[1])
move_handlers = (
    root / "src/source/Render/Effects/Behaviors/MoveHandlers.cpp"
).read_text(encoding="utf-8")
characters = (root / "src/source/Engine/Object/ZzzCharacter.cpp").read_text(
    encoding="utf-8"
)
effects = (root / "src/source/Render/Effects/ZzzEffect.cpp").read_text(
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


attack_stage = function_body(characters, "bool AttackStage")

for effect_source in (move_handlers, (root / "src/source/Render/Effects/ZzzEffectJoint.cpp").read_text(encoding="utf-8")):
    assert "pow(-1" not in effect_source and "powf(-1" not in effect_source, (
        "A negative sign must never be FPS-scaled; fractional exponents produce NaN and destroy effects"
    )
    assert "o->SubType >= 11 || o->SubType <= 13" not in effect_source, (
        "A bounded subtype range must not use an always-true disjunction"
    )

power_slash_start = attack_stage.index("case AT_SKILL_POWER_SLASH:")
power_slash_end = attack_stage.index("case AT_SKILL_NOVA:", power_slash_start)
power_slash = attack_stage[power_slash_start:power_slash_end]
assert "rand_fps_check" not in power_slash, (
    "Power Slash's only fan must be deterministic, not chance"
)

spiral_start = attack_stage.index("AT_SKILL_SPIRAL_SLASH:")
spiral_end = attack_stage.index("case AT_SKILL_SPACE_SPLIT:", spiral_start)
spiral_slash = attack_stage[spiral_start:spiral_end]
assert "rand_fps_check" not in spiral_slash, (
    "Spiral Slash's two animation milestones must not be randomly dropped"
)
assert spiral_slash.count("SkillTiming::CrossedUp") == 2

summoner_cast = function_body(move_handlers, "bool Move_MODEL_SUMMONER_CASTING_EFFECT1")
assert "FrameStepTowards" in summoner_cast, (
    "Summoner casting fades must take the same real time at every frame rate"
)

summon_neil = function_body(move_handlers, "bool Move_MODEL_SUMMONER_SUMMON_NEIL")
assert "CreateEffectFpsChecked" not in summon_neil, (
    "Neil's state-guarded summon stages are one-shot events and must not be dropped"
)

move_effect_start = effects.index("void MoveEffect")
lagul_start = effects.index("case MODEL_SUMMONER_EQUIP_HEAD_LAGUL:", move_effect_start)
lagul_end = effects.index("case BITMAP_FIRE + 1:", lagul_start)
lagul = effects[lagul_start:lagul_end]
assert "FrameStepTowards" in lagul, (
    "Summoner's Lagul casting light must fade in real time, not rendered frames"
)

print("PASS: all-skill audit preserves deterministic milestones and timed fades")
