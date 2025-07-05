/*
 * codec2_encode - Encode WAV files using Codec2
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <codec2/codec2.h>
#include "wav_util.h"

void print_usage(const char* prog_name) {
    printf("Usage: %s [options] input.wav output.c2\n", prog_name);
    printf("\nOptions:\n");
    printf("  -m MODE    Codec2 mode (3200, 2400, 1600, 1400, 1300, 1200, 700, 700B)\n");
    printf("             Default: 3200\n");
    printf("  -h         Show this help\n");
    printf("\nSupported input formats:\n");
    printf("  - 8000 Hz sample rate\n");
    printf("  - Mono (1 channel)\n");
    printf("  - 16-bit PCM WAV files\n");
    printf("\nOutput format:\n");
    printf("  - Binary codec2 frames\n");
}

int mode_from_string(const char* mode_str) {
    if (strcmp(mode_str, "3200") == 0) return CODEC2_MODE_3200;
    if (strcmp(mode_str, "2400") == 0) return CODEC2_MODE_2400;
    if (strcmp(mode_str, "1600") == 0) return CODEC2_MODE_1600;
    if (strcmp(mode_str, "1400") == 0) return CODEC2_MODE_1400;
    if (strcmp(mode_str, "1300") == 0) return CODEC2_MODE_1300;
    if (strcmp(mode_str, "1200") == 0) return CODEC2_MODE_1200;
    if (strcmp(mode_str, "700") == 0) return CODEC2_MODE_700;
    if (strcmp(mode_str, "700B") == 0) return CODEC2_MODE_700B;
    return -1;
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
    int mode = CODEC2_MODE_3200;
    char* input_file = NULL;
    char* output_file = NULL;
    
    // Parse command line arguments
    while ((opt = getopt(argc, argv, "m:h")) != -1) {
        switch (opt) {
            case 'm':
                mode = mode_from_string(optarg);
                if (mode == -1) {
                    fprintf(stderr, "Error: Invalid mode '%s'\n", optarg);
                    print_usage(argv[0]);
                    return 1;
                }
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
    
    printf("Codec2 Encoder\n");
    printf("==============\n");
    printf("Input file:  %s\n", input_file);
    printf("Output file: %s\n", output_file);
    printf("Mode:        %s bps\n", mode_to_string(mode));
    
    // Open input WAV file
    wav_file_t* wav_in = wav_open_read(input_file);
    if (!wav_in) {
        fprintf(stderr, "Error: Cannot open input file '%s'\n", input_file);
        return 1;
    }
    
    // Validate input format
    if (wav_get_sample_rate(wav_in) != 8000) {
        fprintf(stderr, "Error: Input must be 8000 Hz (got %d Hz)\n", wav_get_sample_rate(wav_in));
        wav_close(wav_in);
        return 1;
    }
    
    if (wav_get_channels(wav_in) != 1) {
        fprintf(stderr, "Error: Input must be mono (got %d channels)\n", wav_get_channels(wav_in));
        wav_close(wav_in);
        return 1;
    }
    
    if (wav_get_bits_per_sample(wav_in) != 16) {
        fprintf(stderr, "Error: Input must be 16-bit (got %d bits)\n", wav_get_bits_per_sample(wav_in));
        wav_close(wav_in);
        return 1;
    }
    
    printf("Input format: %d Hz, %d channels, %d bits\n", 
           wav_get_sample_rate(wav_in), wav_get_channels(wav_in), wav_get_bits_per_sample(wav_in));
    printf("Total samples: %d (%.2f seconds)\n", 
           wav_get_total_samples(wav_in), (float)wav_get_total_samples(wav_in) / 8000.0);
    
    // Create codec2 instance
    struct CODEC2* codec2 = codec2_create(mode);
    if (!codec2) {
        fprintf(stderr, "Error: Cannot create codec2 instance\n");
        wav_close(wav_in);
        return 1;
    }
    
    int samples_per_frame = codec2_samples_per_frame(codec2);
    int bits_per_frame = codec2_bits_per_frame(codec2);
    int bytes_per_frame = (bits_per_frame + 7) / 8;
    
    printf("Codec2 parameters:\n");
    printf("  Samples per frame: %d\n", samples_per_frame);
    printf("  Bits per frame: %d\n", bits_per_frame);
    printf("  Bytes per frame: %d\n", bytes_per_frame);
    
    // Open output file
    FILE* output = fopen(output_file, "wb");
    if (!output) {
        fprintf(stderr, "Error: Cannot create output file '%s'\n", output_file);
        codec2_destroy(codec2);
        wav_close(wav_in);
        return 1;
    }
    
    // Write a simple header to the output file
    uint32_t header[4];
    header[0] = 0x43324332; // "C2C2" magic
    header[1] = mode;        // Mode
    header[2] = samples_per_frame; // Samples per frame
    header[3] = bits_per_frame;    // Bits per frame
    fwrite(header, sizeof(uint32_t), 4, output);
    
    // Allocate buffers
    short* speech_samples = malloc(samples_per_frame * sizeof(short));
    unsigned char* codec2_bits = malloc(bytes_per_frame);
    
    if (!speech_samples || !codec2_bits) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        codec2_destroy(codec2);
        wav_close(wav_in);
        fclose(output);
        return 1;
    }
    
    // Encode loop
    int frames_encoded = 0;
    int samples_read;
    
    printf("\nEncoding...\n");
    
    while ((samples_read = wav_read_samples(wav_in, speech_samples, samples_per_frame)) > 0) {
        // Pad with zeros if incomplete frame
        if (samples_read < samples_per_frame) {
            memset(&speech_samples[samples_read], 0, (samples_per_frame - samples_read) * sizeof(short));
        }
        
        // Encode frame
        codec2_encode(codec2, codec2_bits, speech_samples);
        
        // Write encoded frame
        fwrite(codec2_bits, 1, bytes_per_frame, output);
        
        frames_encoded++;
        if (frames_encoded % 100 == 0) {
            printf("  Encoded %d frames (%.1f seconds)\n", 
                   frames_encoded, (float)(frames_encoded * samples_per_frame) / 8000.0);
        }
    }
    
    printf("Encoding complete!\n");
    printf("Total frames encoded: %d\n", frames_encoded);
    printf("Total time: %.2f seconds\n", (float)(frames_encoded * samples_per_frame) / 8000.0);
    printf("Compression ratio: %.1f:1\n", 
           (float)(frames_encoded * samples_per_frame * 2) / (float)(frames_encoded * bytes_per_frame));
    
    // Cleanup
    free(speech_samples);
    free(codec2_bits);
    codec2_destroy(codec2);
    wav_close(wav_in);
    fclose(output);
    
    return 0;
}