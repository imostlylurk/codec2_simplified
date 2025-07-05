/*
 * Create various test WAV files to demonstrate enhanced decoder capabilities
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include <string.h>

// Simple WAV header structure
typedef struct {
    char     riff[4];
    uint32_t chunk_size;
    char     wave[4];
    char     fmt[4];
    uint32_t fmt_chunk_size;
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
    char     data[4];
    uint32_t data_size;
} __attribute__((packed)) wav_header_t;

void write_wav_header(FILE* f, uint32_t sample_rate, uint16_t channels, uint16_t bits_per_sample, uint32_t data_size) {
    wav_header_t header;
    
    memcpy(header.riff, "RIFF", 4);
    header.chunk_size = 36 + data_size;
    memcpy(header.wave, "WAVE", 4);
    memcpy(header.fmt, "fmt ", 4);
    header.fmt_chunk_size = 16;
    header.audio_format = 1; // PCM
    header.num_channels = channels;
    header.sample_rate = sample_rate;
    header.bits_per_sample = bits_per_sample;
    header.byte_rate = sample_rate * channels * bits_per_sample / 8;
    header.block_align = channels * bits_per_sample / 8;
    memcpy(header.data, "data", 4);
    header.data_size = data_size;
    
    fwrite(&header, sizeof(wav_header_t), 1, f);
}

void create_44khz_stereo_16bit(const char* filename) {
    printf("Creating %s: 44100 Hz, stereo, 16-bit\n", filename);
    
    FILE* f = fopen(filename, "wb");
    if (!f) return;
    
    const int sample_rate = 44100;
    const int channels = 2;
    const int duration = 2; // seconds
    const int total_samples = sample_rate * duration;
    const int data_size = total_samples * channels * 2;
    
    write_wav_header(f, sample_rate, channels, 16, data_size);
    
    // Generate stereo sine waves (440 Hz left, 880 Hz right)
    for (int i = 0; i < total_samples; i++) {
        double t = (double)i / sample_rate;
        int16_t left = (int16_t)(16000.0 * sin(2.0 * M_PI * 440.0 * t));
        int16_t right = (int16_t)(16000.0 * sin(2.0 * M_PI * 880.0 * t));
        fwrite(&left, 2, 1, f);
        fwrite(&right, 2, 1, f);
    }
    
    fclose(f);
}

void create_22khz_mono_8bit(const char* filename) {
    printf("Creating %s: 22050 Hz, mono, 8-bit\n", filename);
    
    FILE* f = fopen(filename, "wb");
    if (!f) return;
    
    const int sample_rate = 22050;
    const int channels = 1;
    const int duration = 2; // seconds
    const int total_samples = sample_rate * duration;
    const int data_size = total_samples * channels * 1;
    
    write_wav_header(f, sample_rate, channels, 8, data_size);
    
    // Generate 8-bit sine wave (440 Hz)
    for (int i = 0; i < total_samples; i++) {
        double t = (double)i / sample_rate;
        uint8_t sample = (uint8_t)(128 + 100.0 * sin(2.0 * M_PI * 440.0 * t));
        fwrite(&sample, 1, 1, f);
    }
    
    fclose(f);
}

void create_48khz_mono_24bit(const char* filename) {
    printf("Creating %s: 48000 Hz, mono, 24-bit\n", filename);
    
    FILE* f = fopen(filename, "wb");
    if (!f) return;
    
    const int sample_rate = 48000;
    const int channels = 1;
    const int duration = 2; // seconds
    const int total_samples = sample_rate * duration;
    const int data_size = total_samples * channels * 3;
    
    write_wav_header(f, sample_rate, channels, 24, data_size);
    
    // Generate 24-bit sine wave (440 Hz)
    for (int i = 0; i < total_samples; i++) {
        double t = (double)i / sample_rate;
        int32_t sample32 = (int32_t)(8000000.0 * sin(2.0 * M_PI * 440.0 * t));
        
        // Write 24-bit little-endian
        uint8_t bytes[3];
        bytes[0] = sample32 & 0xFF;
        bytes[1] = (sample32 >> 8) & 0xFF;
        bytes[2] = (sample32 >> 16) & 0xFF;
        fwrite(bytes, 3, 1, f);
    }
    
    fclose(f);
}

int main() {
    printf("Creating test WAV files with various formats...\n\n");
    
    create_44khz_stereo_16bit("test_44khz_stereo_16bit.wav");
    create_22khz_mono_8bit("test_22khz_mono_8bit.wav");
    create_48khz_mono_24bit("test_48khz_mono_24bit.wav");
    
    printf("\nTest files created! Try encoding them with:\n");
    printf("  ./tools/codec2_encode_enhanced -v test_44khz_stereo_16bit.wav test1.c2\n");
    printf("  ./tools/codec2_encode_enhanced -v test_22khz_mono_8bit.wav test2.c2\n");
    printf("  ./tools/codec2_encode_enhanced -v test_48khz_mono_24bit.wav test3.c2\n");
    
    return 0;
}