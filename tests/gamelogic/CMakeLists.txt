mu_add_test(
    NAME test_chaos_upgrade_rates
    SOURCES test_chaos_upgrade_rates.cpp
)

mu_add_test(
    NAME test_direction_turning
    SOURCES
        test_direction_turning.cpp
    LINK_LIBS
        MuClient
)

mu_add_test(
    NAME test_muhelper_extra_items
    SOURCES
        test_muhelper_extra_items.cpp
        ${CMAKE_SOURCE_DIR}/src/source/MUHelper/ExtraItemFilter.cpp
)

mu_add_test(
    NAME test_muhelper_hunting_area
    SOURCES
        test_muhelper_hunting_area.cpp
        ${CMAKE_SOURCE_DIR}/src/source/MUHelper/HuntingArea.cpp
)

mu_add_test(
    NAME test_muhelper_clock
    SOURCES
        test_muhelper_clock.cpp
        ${CMAKE_SOURCE_DIR}/src/source/MUHelper/HelperClock.cpp
        ${CMAKE_SOURCE_DIR}/src/source/MUHelper/AttackFollowUp.cpp
)

mu_add_test(
    NAME test_invisible_equipment
    SOURCES test_invisible_equipment.cpp
)
