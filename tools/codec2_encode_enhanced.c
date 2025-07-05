/*
 * codec2_encode_enhanced - Encode any WAV file using Codec2 with automatic conversion
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <codec2/codec2.h>
#include "wav_util_enhanced.h"

void print_usage(const char* prog_name) {
    printf("Usage: %s [options] input.wav output.c2\n", prog_name);
    printf("\nOptions:\n");
    printf("  -m MODE    Codec2 mode (3200, 2400, 1600, 1400, 1300, 1200, 700, 700B)\n");
    printf("             Default: 3200\n");
    printf("  -v         Verbose output\n");
    printf("  -h         Show this help\n");
    printf("\nSupported input formats:\n");
    printf("  - Any sample rate (automatically resampled to 8000 Hz)\n");
    printf("  - Mono or stereo (stereo converted to mono)\n");
    printf("  - 8, 16, 24, or 32-bit PCM WAV files\n");
    printf("  - Standard WAV file chunk ordering\n");
    printf("\nOutput format:\n");
    printf("  - Binary codec2 frames with header\n");
    printf("\nExamples:\n");
    printf("  %s speech.wav compressed.c2\n", prog_name);
    printf("  %s -m 1200 -v music.wav ultra_compressed.c2\n", prog_name);
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
    int verbose = 0;
    char* input_file = NULL;
    char* output_file = NULL;
    
    // Parse command line arguments
    while ((opt = getopt(argc, argv, "m:vh")) != -1) {
        switch (opt) {
            case 'm':
                mode = mode_from_string(optarg);
                if (mode == -1) {
                    fprintf(stderr, "Error: Invalid mode '%s'\n", optarg);
                    print_usage(argv[0]);
                    return 1;
                }
                break;
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
    
    printf("Enhanced Codec2 Encoder\n");
    printf("=======================\n");
    printf("Input file:  %s\n", input_file);
    printf("Output file: %s\n", output_file);
    printf("Mode:        %s bps\n", mode_to_string(mode));
    if (verbose) printf("Verbose mode enabled\n");
    printf("\n");
    
    // Open input WAV file
    wav_enhanced_t* wav_in = wav_enhanced_open_read(input_file);
    if (!wav_in) {
        fprintf(stderr, "Error: Cannot open or parse input file '%s'\n", input_file);
        return 1;
    }
    
    if (verbose) {
        wav_enhanced_print_info(wav_in);
        printf("\n");
    }
    
    uint32_t total_samples_8khz = wav_enhanced_get_total_samples_8khz_mono(wav_in);
    float total_time = (float)total_samples_8khz / 8000.0;
    
    printf("After conversion: 8000 Hz, mono, 16-bit\n");
    printf("Total samples: %d (%.2f seconds)\n", total_samples_8khz, total_time);
    
    // Check if conversion is needed
    if (wav_enhanced_get_original_sample_rate(wav_in) != 8000) {
        printf("Sample rate conversion: %d Hz -> 8000 Hz\n", wav_enhanced_get_original_sample_rate(wav_in));
    }
    if (wav_enhanced_get_original_channels(wav_in) != 1) {
        printf("Channel conversion: %d channels -> mono\n", wav_enhanced_get_original_channels(wav_in));
    }
    if (wav_enhanced_get_original_bits_per_sample(wav_in) != 16) {
        printf("Bit depth conversion: %d bit -> 16 bit\n", wav_enhanced_get_original_bits_per_sample(wav_in));
    }
    
    // Create codec2 instance
    struct CODEC2* codec2 = codec2_create(mode);
    if (!codec2) {
        fprintf(stderr, "Error: Cannot create codec2 instance\n");
        wav_enhanced_close(wav_in);
        return 1;
    }
    
    int samples_per_frame = codec2_samples_per_frame(codec2);
    int bits_per_frame = codec2_bits_per_frame(codec2);
    int bytes_per_frame = (bits_per_frame + 7) / 8;
    
    printf("\nCodec2 parameters:\n");
    printf("  Samples per frame: %d\n", samples_per_frame);
    printf("  Bits per frame: %d\n", bits_per_frame);
    printf("  Bytes per frame: %d\n", bytes_per_frame);
    
    int estimated_frames = (total_samples_8khz + samples_per_frame - 1) / samples_per_frame;
    printf("  Estimated frames: %d\n", estimated_frames);
    
    // Open output file
    FILE* output = fopen(output_file, "wb");
    if (!output) {
        fprintf(stderr, "Error: Cannot create output file '%s'\n", output_file);
        codec2_destroy(codec2);
        wav_enhanced_close(wav_in);
        return 1;
    }
    
    // Write header to output file
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
        wav_enhanced_close(wav_in);
        fclose(output);
        return 1;
    }
    
    // Encode loop
    int frames_encoded = 0;
    int samples_read;
    int total_samples_processed = 0;
    
    printf("\nEncoding");
    if (!verbose) printf("...");
    printf("\n");
    
    while ((samples_read = wav_enhanced_read_samples_16bit_mono_8khz(wav_in, speech_samples, samples_per_frame)) > 0) {
        // Pad with zeros if incomplete frame
        if (samples_read < samples_per_frame) {
            memset(&speech_samples[samples_read], 0, (samples_per_frame - samples_read) * sizeof(short));
            if (verbose) {
                printf("  Final frame padded: %d samples -> %d samples\n", samples_read, samples_per_frame);
            }
        }
        
        // Encode frame
        codec2_encode(codec2, codec2_bits, speech_samples);
        
        // Write encoded frame
        fwrite(codec2_bits, 1, bytes_per_frame, output);
        
        frames_encoded++;
        total_samples_processed += samples_read;
        
        if (verbose && (frames_encoded % 50 == 0)) {
            printf("  Encoded %d frames (%.1f seconds)\n", 
                   frames_encoded, (float)total_samples_processed / 8000.0);
        } else if (!verbose && (frames_encoded % 100 == 0)) {
            printf(".");
            fflush(stdout);
        }
    }
    
    if (!verbose) printf("\n");
    
    printf("\nEncoding complete!\n");
    printf("Total frames encoded: %d\n", frames_encoded);
    printf("Total time processed: %.2f seconds\n", (float)total_samples_processed / 8000.0);
    
    // Calculate compression ratio
    long input_size = total_samples_processed * 2; // 16-bit samples
    long output_size = frames_encoded * bytes_per_frame;
    printf("Compression ratio: %.1f:1\n", (float)input_size / (float)output_size);
    printf("Output size: %ld bytes (%.1f KB)\n", output_size, (float)output_size / 1024.0);
    
    // Cleanup
    free(speech_samples);
    free(codec2_bits);
    codec2_destroy(codec2);
    wav_enhanced_close(wav_in);
    fclose(output);
    
    return 0;
}