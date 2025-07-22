cmake_minimum_required(VERSION 3.10)
project(codec2 VERSION 1.0.0 LANGUAGES C CXX)

# Set C and C++ standards
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set default build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Options for different build configurations
option(CODEC2_BUILD_TOOLS "Build command-line tools" ON)
option(CODEC2_BUILD_EMBEDDED "Build for embedded/microcontroller targets" OFF)
option(CODEC2_ENABLE_CORTEX_M4 "Enable Cortex-M4 optimizations" OFF)

# Embedded/microcontroller specific settings
if(CODEC2_BUILD_EMBEDDED)
    add_definitions(-DCODEC2_EMBEDDED)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Os -ffunction-sections -fdata-sections")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Os -ffunction-sections -fdata-sections")
    
    # Disable tools and examples for embedded builds
    set(CODEC2_BUILD_TOOLS OFF)
    set(BUILD_EXAMPLES OFF)
endif()

# Cortex-M4 specific optimizations
if(CODEC2_ENABLE_CORTEX_M4)
    add_definitions(-DCORTEX_M4)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16")
endif()

# Compiler flags
if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")
    set(CMAKE_C_FLAGS_DEBUG "-g -O0")
    set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
endif()

# Find required libraries
find_library(MATH_LIBRARY m)

# Include directories
include_directories(include)

# Core codec2 library sources
set(CODEC2_SOURCES
    src/codec2.c
    src/kiss_fft.c
    src/lpc.c
    src/nlp.c
    src/postfilter.c
    src/sine.c
    src/lsp.c
    src/quantise.c
    src/phase.c
    src/interp.c
    src/dump.c
    src/fifo.c
    src/pack.c
    src/phaseexp.c
    src/codebook.c
    src/codebookd.c
    src/codebookdt.c
    src/codebookge.c
    src/codebookjnd.c
    src/codebookjvm.c
    src/codebooklspmelvq.c
    src/codebookmel.c
    src/codebookres.c
    src/codebookvq.c
    src/codebookvqanssi.c
)

# C++ utility sources
set(CODEC2_CXX_SOURCES
    src/ButterworthFilter.cpp
    src/SampleFilter.cpp
)

# Create the main codec2 library
add_library(codec2 STATIC ${CODEC2_SOURCES})

# Create the C++ utilities library
add_library(codec2_utils STATIC ${CODEC2_CXX_SOURCES})

# Link math library if found
if(MATH_LIBRARY)
    target_link_libraries(codec2 ${MATH_LIBRARY})
endif()

# Set target properties
set_target_properties(codec2 PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER "include/codec2.h"
)

# Install rules
install(TARGETS codec2 codec2_utils
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
#    PUBLIC_HEADER DESTINATION include/codec2
)

# Install all headers
install(DIRECTORY include/codec2/
    DESTINATION include/codec2
    FILES_MATCHING PATTERN "*.h"
)

# Option to build examples
option(BUILD_EXAMPLES "Build example programs" ON)

if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Build command-line tools if enabled
if(CODEC2_BUILD_TOOLS)
    add_subdirectory(tools)
endif()

# Option to enable testing
option(BUILD_TESTING "Build tests" OFF)

if(BUILD_TESTING)
    enable_testing()
    # Add test subdirectory if it exists
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests")
        add_subdirectory(tests)
    endif()
endif()

# Packaging
set(CPACK_PACKAGE_NAME "codec2")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Codec2 - Low bitrate speech codec")
set(CPACK_PACKAGE_VENDOR "David Rowe")
set(CPACK_PACKAGE_CONTACT "https://github.com/drowe67/codec2")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.MD")

include(CPack)
