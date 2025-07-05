/*
 * WAV file utilities for codec2 tools
 * Simple WAV file reading and writing
 */

#ifndef WAV_UTIL_H
#define WAV_UTIL_H

#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

// WAV file header structure
typedef struct {
    char     riff[4];           // "RIFF"
    uint32_t chunk_size;        // File size - 8
    char     wave[4];           // "WAVE"
    char     fmt[4];            // "fmt "
    uint32_t fmt_chunk_size;    // Usually 16 for PCM
    uint16_t audio_format;      // 1 for PCM
    uint16_t num_channels;      // 1 for mono, 2 for stereo
    uint32_t sample_rate;       // 8000, 44100, etc.
    uint32_t byte_rate;         // sample_rate * num_channels * bits_per_sample/8
    uint16_t block_align;       // num_channels * bits_per_sample/8
    uint16_t bits_per_sample;   // 8, 16, 24, 32
    char     data[4];           // "data"
    uint32_t data_size;         // Number of bytes in data
} wav_header_t;

// WAV file context
typedef struct {
    FILE *file;
    wav_header_t header;
    uint32_t samples_read;
    uint32_t samples_written;
    uint32_t total_samples;
} wav_file_t;

// Function prototypes
wav_file_t* wav_open_read(const char* filename);
wav_file_t* wav_open_write(const char* filename, uint32_t sample_rate, uint16_t channels, uint16_t bits_per_sample);
int wav_read_samples(wav_file_t* wav, short* samples, uint32_t num_samples);
int wav_write_samples(wav_file_t* wav, const short* samples, uint32_t num_samples);
void wav_close(wav_file_t* wav);
uint32_t wav_get_sample_rate(wav_file_t* wav);
uint16_t wav_get_channels(wav_file_t* wav);
uint16_t wav_get_bits_per_sample(wav_file_t* wav);
uint32_t wav_get_total_samples(wav_file_t* wav);

#ifdef __cplusplus
}
#endif

#endif // WAV_UTIL_H