# Tools CMakeLists.txt

# WAV utility libraries
add_library(wav_util STATIC wav_util.c wav_util.h)
add_library(wav_util_enhanced STATIC wav_util_enhanced.c wav_util_enhanced.h)

# Basic codec2 tools (original, simple format requirements)
add_executable(codec2_encode codec2_encode.c)
target_link_libraries(codec2_encode codec2 wav_util)
if(MATH_LIBRARY)
    target_link_libraries(codec2_encode ${MATH_LIBRARY})
endif()

add_executable(codec2_decode codec2_decode.c)
target_link_libraries(codec2_decode codec2 wav_util)
if(MATH_LIBRARY)
    target_link_libraries(codec2_decode ${MATH_LIBRARY})
endif()

# Enhanced codec2 tools (handles real-world WAV files)
add_executable(codec2_encode_enhanced codec2_encode_enhanced.c)
target_link_libraries(codec2_encode_enhanced codec2 wav_util_enhanced)
if(MATH_LIBRARY)
    target_link_libraries(codec2_encode_enhanced ${MATH_LIBRARY})
endif()

add_executable(codec2_decode_enhanced codec2_decode_enhanced.c)
target_link_libraries(codec2_decode_enhanced codec2 wav_util_enhanced)
if(MATH_LIBRARY)
    target_link_libraries(codec2_decode_enhanced ${MATH_LIBRARY})
endif()

# Install tools
install(TARGETS 
    codec2_encode codec2_decode 
    codec2_encode_enhanced codec2_decode_enhanced 
    wav_util wav_util_enhanced
    RUNTIME DESTINATION bin
    ARCHIVE DESTINATION lib
)

# Install headers for wav utilities (useful for other projects)
install(FILES wav_util.h wav_util_enhanced.h
    DESTINATION include/codec2
)