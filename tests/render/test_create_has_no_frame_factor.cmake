# FPS_ANIMATION_FACTOR scales by the frame time, so it belongs on work done every frame. CreateEffect,
# CreateJoint and CreateParticle run once per spawn: the factor in them shrinks spawn offsets, start angles
# and start rates above 25 FPS (a wizard's Energy Ball starts at the feet instead of 100 units up).
if(NOT DEFINED MU_EFFECT_SOURCES)
    message(FATAL_ERROR "MU_EFFECT_SOURCES is required")
endif()

foreach(entry IN LISTS MU_EFFECT_SOURCES)
    string(REPLACE "=" ";" parts "${entry}")
    list(GET parts 0 function)
    list(GET parts 1 path)
    file(READ "${path}" source)

    string(REGEX MATCH "\n[A-Za-z][^\n;]* ${function}\\([^;]*\n{" header "${source}")
    if(NOT header)
        message(FATAL_ERROR "${function} is missing from ${path}")
    endif()
    string(FIND "${source}" "${header}" begin)
    string(SUBSTRING "${source}" ${begin} -1 rest)
    string(FIND "${rest}" "\n{" open)
    string(SUBSTRING "${rest}" ${open} -1 rest)
    string(FIND "${rest}" "\n}" end)
    string(SUBSTRING "${rest}" 0 ${end} body)

    # The trail length still makes up for tails laid once per move.
    string(REPLACE "FPS, FPS_ANIMATION_FACTOR" "" scaled "${body}")
    string(FIND "${scaled}" "FPS_ANIMATION_FACTOR" factor)
    if(NOT factor EQUAL -1)
        message(FATAL_ERROR "${function} uses FPS_ANIMATION_FACTOR; it runs once per spawn, so its values take none")
    endif()
endforeach()
