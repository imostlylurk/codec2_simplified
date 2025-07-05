/*
 * codec2_decode_enhanced - Decode Codec2 files to WAV with enhanced output
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <codec2/codec2.h>
#include "wav_util_enhanced.h"

void print_usage(const char* prog_name) {
    printf("Usage: %s [options] input.c2 output.wav\n", prog_name);
    printf("\nOptions:\n");
    printf("  -v         Verbose output\n");
    printf("  -h         Show this help\n");
    printf("\nInput format:\n");
    printf("  - Binary codec2 frames with header\n");
    printf("\nOutput format:\n");
    printf("  - 8000 Hz, mono, 16-bit PCM WAV file\n");
    printf("\nExamples:\n");
    printf("  %s compressed.c2 speech.wav\n", prog_name);
    printf("  %s -v ultra_compressed.c2 decoded.wav\n", prog_name);
}

const char* mode_to_string(int mode) {
    switch (mode) {
        case CODEC2_MODE_3200: return "3200";
        case CODEC2_MODE_2400: return "2400";
        case CODEC2_MODE_1600: return "1600";
        case CODEC2_MODE_1400: return "1400";
        case CODEC2_MODE_1300: return "1300";
        case CODEC2_MODE_1200: return "1200";
        case CODEC2_MODE_700: return "700";
        case CODEC2_MODE_700B: return "700B";
        default: return "unknown";
    }
}

int main(int argc, char* argv[]) {
    int opt;
    int verbose = 0;
    char* input_file = NULL;
    char* output_file = NULL;
    
    // Parse command line arguments
    while ((opt = getopt(argc, argv, "vh")) != -1) {
        switch (opt) {
            case 'v':
                verbose = 1;
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }
    
    // Get input and output filenames
    if (optind + 2 != argc) {
        fprintf(stderr, "Error: Input and output files required\n");
        print_usage(argv[0]);
        return 1;
    }
    
    input_file = argv[optind];
    output_file = argv[optind + 1];
    
    printf("Enhanced Codec2 Decoder\n");
    printf("=======================\n");
    printf("Input file:  %s\n", input_file);
    printf("Output file: %s\n", output_file);
    if (verbose) printf("Verbose mode enabled\n");
    printf("\n");
    
    // Open input file
    FILE* input = fopen(input_file, "rb");
    if (!input) {
        fprintf(stderr, "Error: Cannot open input file '%s'\n", input_file);
        return 1;
    }
    
    // Read and validate header
    uint32_t header[4];
    if (fread(header, sizeof(uint32_t), 4, input) != 4) {
        fprintf(stderr, "Error: Cannot read header from input file\n");
        fclose(input);
        return 1;
    }
    
    if (header[0] != 0x43324332) { // "C2C2" magic
        fprintf(stderr, "Error: Invalid codec2 file format (magic = 0x%08X)\n", header[0]);
        fclose(input);
        return 1;
    }
    
    int mode = header[1];
    int expected_samples_per_frame = header[2];
    int expected_bits_per_frame = header[3];
    
    printf("Codec2 file format detected\n");
    printf("Mode: %s bps\n", mode_to_string(mode));
    if (verbose) {
        printf("Expected samples per frame: %d\n", expected_samples_per_frame);
        printf("Expected bits per frame: %d\n", expected_bits_per_frame);
    }
    
    // Create codec2 instance
    struct CODEC2* codec2 = codec2_create(mode);
    if (!codec2) {
        fprintf(stderr, "Error: Cannot create codec2 instance for mode %d\n", mode);
        fclose(input);
        return 1;
    }
    
    int samples_per_frame = codec2_samples_per_frame(codec2);
    int bits_per_frame = codec2_bits_per_frame(codec2);
    int bytes_per_frame = (bits_per_frame + 7) / 8;
    
    // Validate parameters
    if (samples_per_frame != expected_samples_per_frame || bits_per_frame != expected_bits_per_frame) {
        fprintf(stderr, "Error: Codec2 parameters don't match file header\n");
        fprintf(stderr, "Expected: %d samples, %d bits per frame\n", expected_samples_per_frame, expected_bits_per_frame);
        fprintf(stderr, "Got:      %d samples, %d bits per frame\n", samples_per_frame, bits_per_frame);
        codec2_destroy(codec2);
        fclose(input);
        return 1;
    }
    
    if (verbose) {
        printf("Codec2 parameters validated:\n");
        printf("  Samples per frame: %d\n", samples_per_frame);
        printf("  Bits per frame: %d\n", bits_per_frame);
        printf("  Bytes per frame: %d\n", bytes_per_frame);
    }
    
    // Count frames in file
    long current_pos = ftell(input);
    fseek(input, 0, SEEK_END);
    long file_size = ftell(input);
    fseek(input, current_pos, SEEK_SET);
    
    int total_frames = (file_size - current_pos) / bytes_per_frame;
    int total_samples = total_frames * samples_per_frame;
    float total_time = (float)total_samples / 8000.0;
    
    printf("Input file analysis:\n");
    printf("  File size: %ld bytes\n", file_size);
    printf("  Data size: %ld bytes\n", file_size - current_pos);
    printf("  Total frames: %d\n", total_frames);
    printf("  Total samples: %d\n", total_samples);
    printf("  Duration: %.2f seconds\n", total_time);
    
    if (total_frames == 0) {
        fprintf(stderr, "Error: No valid frames found in input file\n");
        codec2_destroy(codec2);
        fclose(input);
        return 1;
    }
    
    // Open output WAV file using enhanced utilities
    wav_enhanced_t* wav_out = wav_enhanced_open_write(output_file);
    if (!wav_out) {
        fprintf(stderr, "Error: Cannot create output file '%s'\n", output_file);
        codec2_destroy(codec2);
        fclose(input);
        return 1;
    }
    
    // Allocate buffers
    unsigned char* codec2_bits = malloc(bytes_per_frame);
    short* speech_samples = malloc(samples_per_frame * sizeof(short));
    
    if (!codec2_bits || !speech_samples) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        codec2_destroy(codec2);
        wav_enhanced_close(wav_out);
        fclose(input);
        return 1;
    }
    
    // Decode loop
    int frames_decoded = 0;
    int samples_decoded = 0;
    
    printf("\nDecoding");
    if (!verbose) printf("...");
    printf("\n");
    
    while (fread(codec2_bits, 1, bytes_per_frame, input) == bytes_per_frame) {
        // Decode frame
        codec2_decode(codec2, speech_samples, codec2_bits);
        
        // Write decoded samples
        int written = wav_enhanced_write_samples(wav_out, speech_samples, samples_per_frame);
        if (written != samples_per_frame) {
            fprintf(stderr, "Warning: Could only write %d of %d samples\n", written, samples_per_frame);
        }
        
        frames_decoded++;
        samples_decoded += samples_per_frame;
        
        if (verbose && (frames_decoded % 50 == 0)) {
            printf("  Decoded %d frames (%.1f seconds)\n", 
                   frames_decoded, (float)samples_decoded / 8000.0);
        } else if (!verbose && (frames_decoded % 100 == 0)) {
            printf(".");
            fflush(stdout);
        }
    }
    
    if (!verbose) printf("\n");
    
    printf("\nDecoding complete!\n");
    printf("Total frames decoded: %d\n", frames_decoded);
    printf("Total samples written: %d\n", samples_decoded);
    printf("Total time: %.2f seconds\n", (float)samples_decoded / 8000.0);
    
    if (frames_decoded != total_frames) {
        printf("Warning: Expected %d frames but decoded %d frames\n", total_frames, frames_decoded);
    }
    
    printf("Output: 8000 Hz, mono, 16-bit PCM WAV\n");
    
    // Cleanup
    free(codec2_bits);
    free(speech_samples);
    codec2_destroy(codec2);
    wav_enhanced_close(wav_out);
    fclose(input);
    
    return 0;
}