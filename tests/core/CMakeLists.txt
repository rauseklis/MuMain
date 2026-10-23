mu_add_test(
    NAME test_mu_timer
    SOURCES
        test_mu_timer.cpp
    LINK_LIBS
        MuClient
)

mu_add_test(
    NAME test_frame_timing
    SOURCES
        test_frame_timing.cpp
    LINK_LIBS
        MuClient
)

mu_add_test(
    NAME test_ground_truth
    SOURCES
        test_ground_truth.cpp
    LINK_LIBS
        MuClient
)

mu_add_test(
    NAME test_windows_console
    SOURCES
        test_windows_console.cpp
    LINK_LIBS
        MuClient
)

mu_add_test(
    NAME test_server_port
    SOURCES
        test_server_port.cpp
    LINK_LIBS
        MuClient
)

mu_add_test(
    NAME test_narrow_path
    SOURCES
        test_narrow_path.cpp
    LINK_LIBS
        MuClient
)

mu_add_test(
    NAME test_scene_state
    SOURCES
        test_scene_state.cpp
    LINK_LIBS
        MuClient
)

mu_add_test(
    NAME test_audio_playback
    SOURCES
        test_audio_playback.cpp
        ${CMAKE_SOURCE_DIR}/src/source/App/Platform/Windows/Winmain.cpp
    LINK_LIBS
        MuClient
)

mu_add_test(
    NAME test_platform_crypto
    SOURCES
        test_platform_crypto.cpp
    LINK_LIBS
        MuClient
)

mu_add_test(
    NAME test_mu_logger
    SOURCES
        test_mu_logger_static_init.cpp
        test_mu_logger_doctest.cpp
    LINK_LIBS
        MuClient
)

# The local-socket transport exists only in builds with the control socket.
if(ENABLE_CONTROL_SOCKET)
    mu_add_test(
        NAME test_local_socket
        SOURCES
            test_local_socket.cpp
            ${CMAKE_SOURCE_DIR}/src/source/Core/Platform/LocalSocket.cpp
        # The transport and the test's own client side are Winsock calls on
        # Windows; on POSIX they come from libc.
        LINK_LIBS
            $<$<BOOL:${WIN32}>:ws2_32>
    )
endif()

mu_add_test(
    NAME test_automation_ranges
    SOURCES
        test_automation_ranges.cpp
)

mu_add_test(
    NAME test_skill_level_requirements
    SOURCES test_skill_level_requirements.cpp
)

mu_add_test(
    NAME test_potion_stacking
    SOURCES test_potion_stacking.cpp
    LINK_LIBS MuClient
)

mu_add_test(
    NAME test_two_handed_weapon_bonus
    SOURCES test_two_handed_weapon_bonus.cpp
    LINK_LIBS MuClient
)

mu_add_test(
    NAME test_buff_details
    SOURCES test_buff_details.cpp
        ${CMAKE_SOURCE_DIR}/src/source/GameLogic/Buffs/BuffDetails.cpp
        ${CMAKE_SOURCE_DIR}/src/source/UI/NewUI/HUD/BuffDuration.cpp
)

mu_add_test(
    NAME test_map_object_loading
    SOURCES
        test_map_object_loading.cpp
        ${CMAKE_SOURCE_DIR}/src/source/App/Platform/Windows/Winmain.cpp
    LINK_LIBS MuClient
)

# The injected input paths in Winmain exist only in builds with the control
# socket, like the transport above.
if(ENABLE_CONTROL_SOCKET)
    mu_add_test(
        NAME test_synthetic_input
        SOURCES
            test_synthetic_input.cpp
            ${CMAKE_SOURCE_DIR}/src/source/App/Platform/Windows/Winmain.cpp
        LINK_LIBS
            MuClient
    )
endif()

mu_add_test(
    NAME test_frame_profiler
    SOURCES
        test_frame_profiler.cpp
)

mu_add_test(
    NAME test_cpu_usage_math
    SOURCES
        test_cpu_usage_math.cpp
)

add_test(
    NAME msvc_runtime_dll_staging_contract
    COMMAND "${CMAKE_COMMAND}"
        "-DMU_CMAKE_SOURCE=${CMAKE_SOURCE_DIR}/src/CMakeLists.txt"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/test_msvc_runtime_dll_staging.cmake"
)

mu_add_test(
    NAME test_mu_helper_lifecycle
    SOURCES
        test_mu_helper_lifecycle.cpp
        ${CMAKE_SOURCE_DIR}/src/source/App/Platform/Windows/Winmain.cpp
    LINK_LIBS MuClient
)

mu_add_test(
    NAME test_legacy_window_message
    SOURCES test_legacy_window_message.cpp
)
