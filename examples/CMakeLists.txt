# Examples CMakeLists.txt

# Simple codec2 example
add_executable(codec2_example codec2_example.c)
target_link_libraries(codec2_example codec2)
if(MATH_LIBRARY)
    target_link_libraries(codec2_example ${MATH_LIBRARY})
endif()

# Install examples
install(TARGETS codec2_example
    RUNTIME DESTINATION bin
)