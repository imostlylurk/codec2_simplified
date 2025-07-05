/*
 * Create a simple test WAV file for testing codec2 tools
 */

#include <stdio.h>
#include <math.h>
#include "wav_util.h"

int main() {
    const char* filename = "test_8khz_mono.wav";
    const int sample_rate = 8000;
    const int duration_seconds = 2;
    const int total_samples = sample_rate * duration_seconds;
    
    printf("Creating test WAV file: %s\n", filename);
    printf("Format: %d Hz, mono, 16-bit, %d seconds\n", sample_rate, duration_seconds);
    
    wav_file_t* wav = wav_open_write(filename, sample_rate, 1, 16);
    if (!wav) {
        fprintf(stderr, "Error: Cannot create WAV file\n");
        return 1;
    }
    
    // Generate a simple test signal (440 Hz sine wave)
    for (int i = 0; i < total_samples; i++) {
        double t = (double)i / sample_rate;
        short sample = (short)(16000.0 * sin(2.0 * M_PI * 440.0 * t));
        wav_write_samples(wav, &sample, 1);
    }
    
    wav_close(wav);
    printf("Test WAV file created successfully!\n");
    return 0;
}