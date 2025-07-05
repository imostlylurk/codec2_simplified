/*
 * Enhanced WAV file utilities for codec2 tools
 * Supports real-world WAV files with automatic conversion to codec2 format
 */

#ifndef WAV_UTIL_ENHANCED_H
#define WAV_UTIL_ENHANCED_H

#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

// WAV format constants
#define WAV_FORMAT_PCM          1
#define WAV_FORMAT_IEEE_FLOAT   3
#define WAV_FORMAT_EXTENSIBLE   0xFFFE

// Chunk identifiers
#define FOURCC_RIFF 0x46464952  // "RIFF"
#define FOURCC_WAVE 0x45564157  // "WAVE"
#define FOURCC_FMT  0x20746D66  // "fmt "
#define FOURCC_DATA 0x61746164  // "data"
#define FOURCC_FACT 0x74636166  // "fact"
#define FOURCC_LIST 0x5453494C  // "LIST"

// Enhanced WAV file context
typedef struct {
    FILE *file;
    
    // Original file format
    uint32_t original_sample_rate;
    uint16_t original_channels;
    uint16_t original_bits_per_sample;
    uint16_t original_format;
    
    // Target codec2 format (always 8000 Hz, mono, 16-bit)
    uint32_t target_sample_rate;
    uint16_t target_channels;
    uint16_t target_bits_per_sample;
    
    // File info
    uint32_t total_samples;        // In original format
    uint32_t samples_read;
    uint32_t samples_written;
    
    // Data chunk info
    long data_start_pos;
    uint32_t data_size;
    
    // Conversion state
    double resample_ratio;
    double resample_phase;
    short* resample_buffer;
    int resample_buffer_size;
    
    // For writing
    uint32_t bytes_written;
} wav_enhanced_t;

// Function prototypes
wav_enhanced_t* wav_enhanced_open_read(const char* filename);
wav_enhanced_t* wav_enhanced_open_write(const char* filename);

int wav_enhanced_read_samples_16bit_mono_8khz(wav_enhanced_t* wav, short* samples, uint32_t num_samples);
int wav_enhanced_write_samples(wav_enhanced_t* wav, const short* samples, uint32_t num_samples);

void wav_enhanced_close(wav_enhanced_t* wav);

// Info functions
uint32_t wav_enhanced_get_original_sample_rate(wav_enhanced_t* wav);
uint16_t wav_enhanced_get_original_channels(wav_enhanced_t* wav);
uint16_t wav_enhanced_get_original_bits_per_sample(wav_enhanced_t* wav);
uint32_t wav_enhanced_get_total_samples_8khz_mono(wav_enhanced_t* wav);

// Utility functions
void wav_enhanced_print_info(wav_enhanced_t* wav);

#ifdef __cplusplus
}
#endif

#endif // WAV_UTIL_ENHANCED_H