mu_add_test(
    NAME test_particle_draw_order
    SOURCES
        test_particle_draw_order.cpp
        ${CMAKE_SOURCE_DIR}/src/source/Render/Effects/ParticleDrawOrder.cpp
)

mu_add_test(
    NAME test_cloth_simulation_timing
    SOURCES
        test_cloth_simulation_timing.cpp
)

mu_add_test(
    NAME test_tail_scaling
    SOURCES
        test_tail_scaling.cpp
)

mu_add_test(
    NAME test_weapon_trail_timing
    SOURCES
        test_weapon_trail_timing.cpp
)

mu_add_test(
    NAME test_monster_dash
    SOURCES
        test_monster_dash.cpp
)

mu_add_test(
    NAME test_skill_effect_timing
    SOURCES
        test_skill_effect_timing.cpp
)

add_test(
    NAME reported_skill_effect_contract
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/test_reported_skill_effect_contract.py"
        "${CMAKE_SOURCE_DIR}"
)

add_test(
    NAME all_skill_effect_contract
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/test_all_skill_effect_contract.py"
        "${CMAKE_SOURCE_DIR}"
)
