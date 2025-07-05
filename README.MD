# Codec2 - Low Bitrate Speech Codec

This is a standalone, enhanced implementation of the Codec2 low-bitrate speech codec, originally developed by David Rowe. This version has been organized into a proper CMake-based project with enhanced tools for real-world audio file processing.

## Credits and Acknowledgments

### Original Codec2 Development
- **Original Author**: David Rowe (VK2DLZ)
- **Original Project**: https://github.com/drowe67/codec2
- **License**: LGPL-2.1 (GNU Lesser General Public License version 2.1)

### Source Implementation Base
- **Derived From**: https://github.com/onemikedelta/M17-ESP32/tree/master
- **M17 Project**: Open-source digital voice protocol implementation
- **Contributors**: OneMillionDelta and M17 project contributors

### This Enhanced Version
- **Enhancements**: 
  - Modern CMake build system
  - Microcontroller/embedded support
  - Enhanced WAV file processing with automatic format conversion
  - Comprehensive command-line tools
  - Real-world audio file compatibility
- **Organization**: Restructured for professional development and distribution
- **Documentation**: Comprehensive development and usage documentation

### Technical Foundation
Codec2 is a revolutionary low-bitrate speech codec that achieves remarkable compression ratios while maintaining intelligible speech quality. The algorithm uses sinusoidal modeling, linear predictive coding (LPC), and vector quantization to achieve bitrates from 700 to 3200 bits per second.

## License

This project maintains the original LGPL-2.1 license as required. See the LICENSE file for complete terms.

## Features

- Low bitrate speech compression (700 bps to 3200 bps)
- Multiple codec modes: 3200, 2400, 1600, 1400, 1300, 1200, 700, 700B bps
- C library with C++ utility components
- CMake build system for cross-platform compilation
- Command-line tools for encoding/decoding WAV files
- Microcontroller/embedded-friendly build options
- Example programs demonstrating usage

## Building

### Prerequisites

- CMake 3.10 or later
- C99-compatible compiler (GCC, Clang, MSVC)
- C++11-compatible compiler for utility components
- Math library (libm on Unix systems)

### Build Instructions

```bash
# Clone or extract the source code
# Navigate to the project directory

# Create build directory
mkdir build
cd build

# Configure with CMake
cmake ..

# Build the project
make -j4

# Optionally run the example
./examples/codec2_example
```

### Build Options

- `BUILD_EXAMPLES`: Build example programs (default: ON)
- `BUILD_TESTING`: Enable testing (default: OFF)
- `CMAKE_BUILD_TYPE`: Debug or Release (default: Release)
- `CODEC2_BUILD_TOOLS`: Build command-line tools (default: ON)
- `CODEC2_BUILD_EMBEDDED`: Build for embedded/microcontroller targets (default: OFF)
- `CODEC2_ENABLE_CORTEX_M4`: Enable Cortex-M4 optimizations (default: OFF)

### Cross-Platform Building

For different build systems:

```bash
# Unix Makefiles (default)
cmake ..

# Ninja
cmake -G Ninja ..

# Xcode (macOS)
cmake -G Xcode ..

# Visual Studio (Windows)
cmake -G "Visual Studio 16 2019" ..

# Embedded/microcontroller build
cmake -DCODEC2_BUILD_EMBEDDED=ON -DCODEC2_ENABLE_CORTEX_M4=ON ..

# Library-only build (no tools or examples)
cmake -DCODEC2_BUILD_TOOLS=OFF -DBUILD_EXAMPLES=OFF ..
```

### Using as a Git Submodule

To use this project as a submodule in a microcontroller project:

```bash
# Add as submodule
git submodule add https://github.com/your-repo/codec2.git external/codec2

# In your CMakeLists.txt
add_subdirectory(external/codec2)

# Configure for embedded use
set(CODEC2_BUILD_EMBEDDED ON CACHE BOOL "Build for embedded targets")
set(CODEC2_BUILD_TOOLS OFF CACHE BOOL "Don't build command-line tools")
set(BUILD_EXAMPLES OFF CACHE BOOL "Don't build examples")

# Link against the codec2 library
target_link_libraries(your_target codec2)
```

## Usage

### Basic Usage

```c
#include <codec2/codec2.h>

// Create codec instance
struct CODEC2 *codec2 = codec2_create(CODEC2_MODE_3200);

// Encode speech samples
unsigned char bits[8];  // 64 bits for 3200 bps mode
short speech_in[160];   // 160 samples for 3200 bps mode
codec2_encode(codec2, bits, speech_in);

// Decode speech samples
short speech_out[160];
codec2_decode(codec2, speech_out, bits);

// Clean up
codec2_destroy(codec2);
```

### Codec Modes

| Mode | Bit Rate | Samples/Frame | Bits/Frame | Frame Duration |
|------|----------|---------------|------------|----------------|
| 3200 | 3200 bps | 160           | 64         | 20ms           |
| 2400 | 2400 bps | 160           | 48         | 20ms           |
| 1600 | 1600 bps | 320           | 64         | 40ms           |
| 1400 | 1400 bps | 320           | 56         | 40ms           |
| 1300 | 1300 bps | 320           | 52         | 40ms           |
| 1200 | 1200 bps | 320           | 48         | 40ms           |
| 700  | 700 bps  | 320           | 28         | 40ms           |
| 700B | 700 bps  | 320           | 28         | 40ms           |

### Command-Line Tools

The project includes command-line tools for file processing:

#### codec2_encode (Basic)

Encode properly formatted WAV files to Codec2 format:

```bash
# Encode with default 3200 bps mode  
./tools/codec2_encode input.wav output.c2

# Encode with specific mode
./tools/codec2_encode -m 1200 input.wav output.c2
```

**Requirements:**
- Input: Exactly 8000 Hz, mono, 16-bit PCM WAV files
- Output: Binary Codec2 frames with header

#### codec2_encode_enhanced (Recommended)

Encode any WAV file to Codec2 format with automatic conversion:

```bash
# Encode any WAV file with automatic conversion
./tools/codec2_encode_enhanced input.wav output.c2

# Encode with specific mode and verbose output
./tools/codec2_encode_enhanced -m 1200 -v music.wav output.c2

# Show help
./tools/codec2_encode_enhanced -h
```

**Supported Input Formats:**
- Any sample rate (automatically resampled to 8000 Hz)
- Mono or stereo (stereo mixed to mono)
- 8, 16, 24, or 32-bit PCM WAV files
- Standard WAV chunk ordering
- Output: Binary Codec2 frames with header

#### codec2_decode & codec2_decode_enhanced

Decode Codec2 files to WAV format:

```bash
# Basic decoder
./tools/codec2_decode input.c2 output.wav

# Enhanced decoder with verbose output
./tools/codec2_decode_enhanced -v input.c2 output.wav

# Show help
./tools/codec2_decode_enhanced -h
```

**Output:** 8000 Hz, mono, 16-bit PCM WAV files

Both decoders produce identical output, but the enhanced version provides detailed information about the decoding process.

#### Example Usage

```bash
# Create test WAV files with different formats
./create_test_files

# Enhanced encoding (handles any WAV format)
./tools/codec2_encode_enhanced test_44khz_stereo_16bit.wav music.c2
./tools/codec2_encode_enhanced -m 1200 test_22khz_mono_8bit.wav speech.c2
./tools/codec2_encode_enhanced -m 700 -v test_48khz_mono_24bit.wav ultra_compressed.c2

# Basic encoding (requires exact format)
./create_test_wav  # Creates 8kHz mono 16-bit file
./tools/codec2_encode -m 3200 test_8khz_mono.wav basic.c2

# Decode back to WAV
./tools/codec2_decode_enhanced -v music.c2 decoded_music.wav
./tools/codec2_decode speech.c2 decoded_speech.wav

# Compare compression ratios
ls -la *.c2 *.wav
```

#### Real-World Example

```bash
# Convert any music file to ultra-low bitrate
./tools/codec2_encode_enhanced -m 700 -v your_music.wav compressed.c2

# Results in approximately 100:1 compression ratio
# 44100 Hz stereo 16-bit → 8000 Hz mono at 700 bps
```

## Project Structure

```
├── src/                    # Source files
│   ├── codec2.c           # Main codec implementation
│   ├── kiss_fft.c         # FFT implementation
│   ├── *.c                # Other codec modules
│   └── *.cpp              # C++ utility classes
├── include/codec2/         # Header files
│   ├── codec2.h           # Main API header
│   └── *.h                # Other headers
├── tools/                  # Command-line tools
│   ├── codec2_encode.c    # WAV to Codec2 encoder
│   ├── codec2_decode.c    # Codec2 to WAV decoder
│   ├── wav_util.c         # WAV file I/O utilities
│   └── wav_util.h         # WAV utility headers
├── examples/               # Example programs
│   └── codec2_example.c   # Basic usage example
├── CMakeLists.txt         # Main CMake configuration
├── CLAUDE.md              # Development guidance
└── README.MD              # This file
```

## Installation

After building, install system-wide:

```bash
# From build directory
sudo make install
```

This installs:
- Libraries: `/usr/local/lib/libcodec2.a`, `/usr/local/lib/libcodec2_utils.a`, `/usr/local/lib/libwav_util.a`
- Headers: `/usr/local/include/codec2/`
- Tools: `/usr/local/bin/codec2_encode`, `/usr/local/bin/codec2_decode`
- Examples: `/usr/local/bin/codec2_example`

## Contributing

This project maintains the structure and API of the original Codec2 implementation while providing modern build tooling and organization.

### Contribution Guidelines
- All contributions must maintain LGPL-2.1 compatibility
- Preserve original Codec2 algorithm integrity
- Follow existing code style and conventions
- Add appropriate tests for new functionality
- Update documentation for user-facing changes

### Attribution Requirements
When using or distributing this code:
1. Maintain original copyright notices
2. Credit David Rowe and the Codec2 project
3. Include the LGPL-2.1 license text
4. Acknowledge the M17 project source implementation

## Support and Development

### Related Projects
- **Original Codec2**: https://github.com/drowe67/codec2
- **M17 Protocol**: https://github.com/M17-Project
- **FreeDV**: https://freedv.org/ (Digital voice mode using Codec2)

### Research Papers
- Rowe, D. "Sinusoidal Speech Coding at 2400 bit/s" (IEEE 1997)
- Rowe, D. "Low Bit Rate Speech Coding with VQ and Phase Prediction" (IEEE 2006)

For technical questions about the Codec2 algorithm, refer to the original project documentation and research papers.
