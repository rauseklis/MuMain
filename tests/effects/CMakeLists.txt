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
