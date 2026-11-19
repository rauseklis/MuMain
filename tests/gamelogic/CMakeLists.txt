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
