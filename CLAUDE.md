# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a Codec2 audio compression library implementation, based on David Rowe's Codec2 project. Codec2 is a low-bitrate speech codec designed for digital voice applications, particularly amateur radio and other communications systems.

## Architecture

### Core Components

- **codec2.c/h**: Main encoder/decoder API with functions like `codec2_create()`, `codec2_encode()`, `codec2_decode()`
- **codec2_internal.h**: Internal state structures and configuration for the codec
- **defines.h**: System constants including sample rates (8kHz), frame sizes (N=80), FFT sizes (512)

### Signal Processing Modules

- **sine.c/h**: Sinusoidal model for speech synthesis
- **nlp.c/h**: Non-linear pitch predictor for fundamental frequency estimation
- **lpc.c/h**: Linear predictive coding for spectral envelope modeling
- **lsp.c/h**: Line spectral pairs for robust quantization of LPC parameters
- **quantise.c/h**: Vector quantization of speech parameters
- **phase.c/h**: Phase modeling and synthesis
- **interp.c/h**: Interpolation between frames
- **postfilter.c/h**: Post-filtering for quality enhancement

### Utility Components

- **ButterworthFilter.h/cpp**: C++ Butterworth filter implementation (highpass/lowpass)
- **FastAudioFIFO.h**: Lock-free circular buffer for audio data (2048 samples)
- **SampleFilter.h/cpp**: Audio sample filtering utilities
- **fifo.c**: Generic FIFO implementation
- **kiss_fft.c/h**: Fast Fourier Transform implementation
- **dump.c/h**: Debug output utilities

### Codec Modes

The codec supports multiple bitrate modes defined in codec2.h:
- MODE_3200: 3200 bps
- MODE_2400: 2400 bps  
- MODE_1600: 1600 bps
- MODE_1400: 1400 bps
- MODE_1300: 1300 bps
- MODE_1200: 1200 bps
- MODE_700: 700 bps
- MODE_700B: 700B bps

## Key Constants

- Sample rate: 8000 Hz (FS in defines.h)
- Frame size: 80 samples (N in defines.h)
- FFT size: 512 samples for both encoder and decoder
- Maximum harmonics: 80 (MAX_AMP in defines.h)

## Development Notes

### Build System

The project uses CMake for cross-platform building:

```bash
mkdir build && cd build
cmake ..
make -j4
```

Build outputs:
- `libcodec2.a`: Main codec library (C code)
- `libcodec2_utils.a`: Utility library (C++ code)
- `codec2_example`: Example program

### CMake Targets

- `codec2`: Static library with core codec functionality
- `codec2_utils`: Static library with C++ utility classes
- `wav_util`: Static library for WAV file I/O
- `codec2_encode`: Command-line tool for encoding WAV files to Codec2
- `codec2_decode`: Command-line tool for decoding Codec2 files to WAV
- `codec2_example`: Example program demonstrating usage

### Build Options for Embedded/Microcontroller Use

```bash
# Embedded build with size optimizations
cmake -DCODEC2_BUILD_EMBEDDED=ON -DCODEC2_BUILD_TOOLS=OFF -DBUILD_EXAMPLES=OFF ..

# Cortex-M4 specific build
cmake -DCODEC2_BUILD_EMBEDDED=ON -DCODEC2_ENABLE_CORTEX_M4=ON ..
```

When `CODEC2_BUILD_EMBEDDED=ON`:
- Enables size optimizations (-Os)
- Adds function and data section separation for better dead code elimination
- Disables tools and examples by default
- Adds `CODEC2_EMBEDDED` preprocessor definition

### File Organization

- Pure C implementation for core codec functionality
- C++ components for utility classes (filters, FIFO)
- Extensive use of codebooks (codebook*.c files) for vector quantization
- Coefficient files (*_coeff.h) contain pre-computed filter coefficients

### Testing and Debugging

- dump.c/h provides debugging output capabilities
- codec2_internal.h exposes internal states for testing
- Various test coefficient files (test_bits*.h) for validation

## Command-Line Tools

### WAV File Requirements

All tools expect 8000 Hz, mono, 16-bit PCM WAV files as input. This matches the native sample rate of Codec2.

### codec2_encode

```bash
./tools/codec2_encode [options] input.wav output.c2
```

Options:
- `-m MODE`: Codec2 mode (3200, 2400, 1600, 1400, 1300, 1200, 700, 700B)

Creates binary files with a simple header:
- 4 bytes: Magic number ("C2C2")
- 4 bytes: Mode 
- 4 bytes: Samples per frame
- 4 bytes: Bits per frame
- Followed by raw codec2 frames

### codec2_decode

```bash
./tools/codec2_decode input.c2 output.wav
```

Reads the header to automatically determine codec parameters and decodes to WAV format.

### File Format

The .c2 files use a simple custom format with a 16-byte header followed by raw codec2 frames. This is not compatible with other codec2 implementations but provides a complete self-contained format for testing.

## License

LGPL-2.1 (GNU Lesser General Public License version 2.1)