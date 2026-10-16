cmake_minimum_required(VERSION 3.25)
set(test_root "${CMAKE_CURRENT_LIST_DIR}/.tmp-stage-arena-assets")
set(source "${test_root}/source")
set(output "${test_root}/output")
file(MAKE_DIRECTORY "${source}/World7" "${output}/World7")
file(WRITE "${source}/World7/EncTerrain7.map" "full Arena layout")
file(WRITE "${output}/World7/EncTerrain7.map" "old Arena layout")
file(SHA256 "${source}/World7/EncTerrain7.map" expected)
string(TOUPPER "${expected}" expected)
set(manifest "${test_root}/manifest.json")
file(WRITE "${manifest}" "[{\"Path\":\"World7/EncTerrain7.map\",\"SHA256\":\"${expected}\"}]")

foreach(pass RANGE 1 2)
    execute_process(COMMAND "${CMAKE_COMMAND}"
        "-DMU_ARENA_SOURCE=${source}" "-DMU_ARENA_OUTPUT=${output}"
        "-DMU_ARENA_MANIFEST=${manifest}"
        -P "${CMAKE_CURRENT_LIST_DIR}/../cmake/StageArenaAssets.cmake"
        RESULT_VARIABLE result)
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "Arena staging failed on pass ${pass}")
    endif()
    file(SHA256 "${output}/World7/EncTerrain7.map" actual)
    string(TOUPPER "${actual}" actual)
    if(NOT actual STREQUAL expected)
        message(FATAL_ERROR "Old Arena survived staging")
    endif()
endforeach()

file(WRITE "${source}/World7/EncTerrain7.map" "unreviewed data")
execute_process(COMMAND "${CMAKE_COMMAND}"
    "-DMU_ARENA_SOURCE=${source}" "-DMU_ARENA_OUTPUT=${output}"
    "-DMU_ARENA_MANIFEST=${manifest}"
    -P "${CMAKE_CURRENT_LIST_DIR}/../cmake/StageArenaAssets.cmake"
    RESULT_VARIABLE corrupt_result OUTPUT_QUIET ERROR_QUIET)
if(corrupt_result EQUAL 0)
    message(FATAL_ERROR "Arena staging accepted a corrupt source asset")
endif()
file(REMOVE_RECURSE "${test_root}")
message(STATUS "Arena stale-asset repair, repeat-build and corruption checks passed")
