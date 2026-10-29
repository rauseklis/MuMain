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
