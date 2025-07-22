/*
 * codec2_decode - Decode Codec2 files to WAV
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <codec2.h>
#include "wav_util.h"

void print_usage(const char* prog_name) {
    printf("Usage: %s [options] input.c2 output.wav\n", prog_name);
    printf("\nOptions:\n");
    printf("  -h         Show this help\n");
    printf("\nInput format:\n");
    printf("  - Binary codec2 frames with header\n");
    printf("\nOutput format:\n");
    printf("  - 8000 Hz, mono, 16-bit PCM WAV file\n");
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
    char* input_file = NULL;
    char* output_file = NULL;
    
    // Parse command line arguments
    while ((opt = getopt(argc, argv, "h")) != -1) {
        switch (opt) {
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
    
    printf("Codec2 Decoder\n");
    printf("==============\n");
    printf("Input file:  %s\n", input_file);
    printf("Output file: %s\n", output_file);
    
    // Open input file
    FILE* input = fopen(input_file, "rb");
    if (!input) {
        fprintf(stderr, "Error: Cannot open input file '%s'\n", input_file);
        return 1;
    }
    
    // Read header
    uint32_t header[4];
    if (fread(header, sizeof(uint32_t), 4, input) != 4) {
        fprintf(stderr, "Error: Cannot read header from input file\n");
        fclose(input);
        return 1;
    }
    
    // Validate header
    if (header[0] != 0x43324332) { // "C2C2" magic
        fprintf(stderr, "Error: Invalid codec2 file format\n");
        fclose(input);
        return 1;
    }
    
    int mode = header[1];
    int expected_samples_per_frame = header[2];
    int expected_bits_per_frame = header[3];
    
    printf("Mode:        %s bps\n", mode_to_string(mode));
    
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
    
    printf("Codec2 parameters:\n");
    printf("  Samples per frame: %d\n", samples_per_frame);
    printf("  Bits per frame: %d\n", bits_per_frame);
    printf("  Bytes per frame: %d\n", bytes_per_frame);
    
    // Count frames in file
    long current_pos = ftell(input);
    fseek(input, 0, SEEK_END);
    long file_size = ftell(input);
    fseek(input, current_pos, SEEK_SET);
    
    int total_frames = (file_size - current_pos) / bytes_per_frame;
    int total_samples = total_frames * samples_per_frame;
    float total_time = (float)total_samples / 8000.0;
    
    printf("Input file contains %d frames (%.2f seconds)\n", total_frames, total_time);
    
    // Open output WAV file
    wav_file_t* wav_out = wav_open_write(output_file, 8000, 1, 16);
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
        wav_close(wav_out);
        fclose(input);
        return 1;
    }
    
    // Decode loop
    int frames_decoded = 0;
    
    printf("\nDecoding...\n");
    
    while (fread(codec2_bits, 1, bytes_per_frame, input) == bytes_per_frame) {
        // Decode frame
        codec2_decode(codec2, speech_samples, codec2_bits);
        
        // Write decoded samples
        wav_write_samples(wav_out, speech_samples, samples_per_frame);
        
        frames_decoded++;
        if (frames_decoded % 100 == 0) {
            printf("  Decoded %d frames (%.1f seconds)\n", 
                   frames_decoded, (float)(frames_decoded * samples_per_frame) / 8000.0);
        }
    }
    
    printf("Decoding complete!\n");
    printf("Total frames decoded: %d\n", frames_decoded);
    printf("Total time: %.2f seconds\n", (float)(frames_decoded * samples_per_frame) / 8000.0);
    
    // Cleanup
    free(codec2_bits);
    free(speech_samples);
    codec2_destroy(codec2);
    wav_close(wav_out);
    fclose(input);
    
    return 0;
}
