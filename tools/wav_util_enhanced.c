/*
 * Enhanced WAV file utilities implementation
 * Handles real-world WAV files with automatic conversion
 */

#include "wav_util_enhanced.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Helper function to read a 32-bit little-endian value
static uint32_t read_le32(FILE* file) {
    uint8_t bytes[4];
    if (fread(bytes, 1, 4, file) != 4) return 0;
    return bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);
}

// Helper function to read a 16-bit little-endian value
static uint16_t read_le16(FILE* file) {
    uint8_t bytes[2];
    if (fread(bytes, 1, 2, file) != 2) return 0;
    return bytes[0] | (bytes[1] << 8);
}

// Helper function to write a 32-bit little-endian value
static void write_le32(FILE* file, uint32_t value) {
    uint8_t bytes[4] = {
        value & 0xFF,
        (value >> 8) & 0xFF,
        (value >> 16) & 0xFF,
        (value >> 24) & 0xFF
    };
    fwrite(bytes, 1, 4, file);
}

// Helper function to write a 16-bit little-endian value
static void write_le16(FILE* file, uint16_t value) {
    uint8_t bytes[2] = {
        value & 0xFF,
        (value >> 8) & 0xFF
    };
    fwrite(bytes, 1, 2, file);
}

// Simple linear interpolation resampler
static int resample_to_8khz(wav_enhanced_t* wav, const short* input, int input_samples, 
                           short* output, int max_output_samples) {
    if (wav->original_sample_rate == 8000) {
        // No resampling needed
        int samples_to_copy = (input_samples < max_output_samples) ? input_samples : max_output_samples;
        memcpy(output, input, samples_to_copy * sizeof(short));
        return samples_to_copy;
    }
    
    int output_count = 0;
    double ratio = (double)wav->original_sample_rate / 8000.0;
    
    for (int i = 0; i < max_output_samples && output_count < max_output_samples; i++) {
        double src_pos = wav->resample_phase + i * ratio;
        int src_idx = (int)src_pos;
        
        if (src_idx >= input_samples - 1) break;
        
        // Linear interpolation
        double frac = src_pos - src_idx;
        short sample1 = input[src_idx];
        short sample2 = (src_idx + 1 < input_samples) ? input[src_idx + 1] : sample1;
        
        output[output_count++] = (short)(sample1 + frac * (sample2 - sample1));
    }
    
    // Update phase for next call
    wav->resample_phase += output_count * ratio;
    wav->resample_phase -= (int)wav->resample_phase; // Keep fractional part
    
    return output_count;
}

// Convert stereo to mono by averaging channels
static void stereo_to_mono(const short* stereo, short* mono, int samples) {
    for (int i = 0; i < samples; i++) {
        mono[i] = (short)((stereo[i * 2] + stereo[i * 2 + 1]) / 2);
    }
}

// Convert various bit depths to 16-bit
static void convert_to_16bit(const void* input, short* output, int samples, 
                            int input_bits, int channels) {
    int i, ch;
    
    switch (input_bits) {
        case 8: {
            const uint8_t* in8 = (const uint8_t*)input;
            for (i = 0; i < samples; i++) {
                int sum = 0;
                for (ch = 0; ch < channels; ch++) {
                    sum += (in8[i * channels + ch] - 128) * 256; // Convert 8-bit unsigned to 16-bit signed
                }
                output[i] = (short)(sum / channels); // Average if multichannel
            }
            break;
        }
        case 16: {
            const short* in16 = (const short*)input;
            for (i = 0; i < samples; i++) {
                int sum = 0;
                for (ch = 0; ch < channels; ch++) {
                    sum += in16[i * channels + ch];
                }
                output[i] = (short)(sum / channels); // Average if multichannel
            }
            break;
        }
        case 24: {
            const uint8_t* in24 = (const uint8_t*)input;
            for (i = 0; i < samples; i++) {
                int sum = 0;
                for (ch = 0; ch < channels; ch++) {
                    int base = (i * channels + ch) * 3;
                    int sample = (in24[base] | (in24[base + 1] << 8) | (in24[base + 2] << 16));
                    if (sample & 0x800000) sample |= 0xFF000000; // Sign extend
                    sum += sample >> 8; // Convert to 16-bit range
                }
                output[i] = (short)(sum / channels);
            }
            break;
        }
        case 32: {
            const int32_t* in32 = (const int32_t*)input;
            for (i = 0; i < samples; i++) {
                int sum = 0;
                for (ch = 0; ch < channels; ch++) {
                    sum += in32[i * channels + ch] >> 16; // Convert to 16-bit range
                }
                output[i] = (short)(sum / channels);
            }
            break;
        }
        default:
            // Unsupported bit depth, output silence
            memset(output, 0, samples * sizeof(short));
            break;
    }
}

wav_enhanced_t* wav_enhanced_open_read(const char* filename) {
    wav_enhanced_t* wav = malloc(sizeof(wav_enhanced_t));
    if (!wav) return NULL;
    
    memset(wav, 0, sizeof(wav_enhanced_t));
    
    wav->file = fopen(filename, "rb");
    if (!wav->file) {
        free(wav);
        return NULL;
    }
    
    // Read RIFF header
    uint32_t riff_id = read_le32(wav->file);
    if (riff_id != FOURCC_RIFF) {
        printf("Error: Not a RIFF file (got 0x%08X)\n", riff_id);
        fclose(wav->file);
        free(wav);
        return NULL;
    }
    
    uint32_t file_size = read_le32(wav->file);
    uint32_t wave_id = read_le32(wav->file);
    if (wave_id != FOURCC_WAVE) {
        printf("Error: Not a WAVE file (got 0x%08X)\n", wave_id);
        fclose(wav->file);
        free(wav);
        return NULL;
    }
    
    // Parse chunks
    int found_fmt = 0, found_data = 0;
    
    while (!feof(wav->file) && (!found_fmt || !found_data)) {
        uint32_t chunk_id = read_le32(wav->file);
        uint32_t chunk_size = read_le32(wav->file);
        
        if (chunk_id == FOURCC_FMT) {
            // Format chunk
            wav->original_format = read_le16(wav->file);
            wav->original_channels = read_le16(wav->file);
            wav->original_sample_rate = read_le32(wav->file);
            uint32_t byte_rate = read_le32(wav->file);
            uint16_t block_align = read_le16(wav->file);
            wav->original_bits_per_sample = read_le16(wav->file);
            
            // Skip any extra format data
            if (chunk_size > 16) {
                fseek(wav->file, chunk_size - 16, SEEK_CUR);
            }
            
            found_fmt = 1;
            
            printf("WAV Format: %s, %d Hz, %d channels, %d bits\n",
                   (wav->original_format == WAV_FORMAT_PCM) ? "PCM" :
                   (wav->original_format == WAV_FORMAT_IEEE_FLOAT) ? "Float" :
                   (wav->original_format == WAV_FORMAT_EXTENSIBLE) ? "Extensible" : "Unknown",
                   wav->original_sample_rate, wav->original_channels, wav->original_bits_per_sample);
            
        } else if (chunk_id == FOURCC_DATA) {
            // Data chunk
            wav->data_start_pos = ftell(wav->file);
            wav->data_size = chunk_size;
            wav->total_samples = chunk_size / (wav->original_channels * wav->original_bits_per_sample / 8);
            found_data = 1;
            
            printf("Data chunk: %d bytes, %d samples\n", chunk_size, wav->total_samples);
            break; // Don't skip the data
            
        } else {
            // Skip unknown chunk
            printf("Skipping chunk '%.4s' (%d bytes)\n", (char*)&chunk_id, chunk_size);
            if (chunk_size > 0) {
                fseek(wav->file, chunk_size, SEEK_CUR);
            }
        }
    }
    
    if (!found_fmt || !found_data) {
        printf("Error: Missing fmt or data chunk\n");
        fclose(wav->file);
        free(wav);
        return NULL;
    }
    
    // Set target format (codec2 requirements)
    wav->target_sample_rate = 8000;
    wav->target_channels = 1;
    wav->target_bits_per_sample = 16;
    
    // Calculate resampling ratio
    wav->resample_ratio = (double)wav->original_sample_rate / wav->target_sample_rate;
    wav->resample_phase = 0.0;
    
    // Allocate resampling buffer
    wav->resample_buffer_size = 4096; // Reasonable chunk size
    wav->resample_buffer = malloc(wav->resample_buffer_size * wav->original_channels * sizeof(short));
    
    return wav;
}

wav_enhanced_t* wav_enhanced_open_write(const char* filename) {
    wav_enhanced_t* wav = malloc(sizeof(wav_enhanced_t));
    if (!wav) return NULL;
    
    memset(wav, 0, sizeof(wav_enhanced_t));
    
    wav->file = fopen(filename, "wb");
    if (!wav->file) {
        free(wav);
        return NULL;
    }
    
    // Set codec2 format
    wav->target_sample_rate = 8000;
    wav->target_channels = 1;
    wav->target_bits_per_sample = 16;
    
    // Write WAV header (will be updated when closing)
    fwrite("RIFF", 1, 4, wav->file);
    write_le32(wav->file, 36); // Will be updated
    fwrite("WAVE", 1, 4, wav->file);
    
    // Format chunk
    fwrite("fmt ", 1, 4, wav->file);
    write_le32(wav->file, 16); // fmt chunk size
    write_le16(wav->file, WAV_FORMAT_PCM);
    write_le16(wav->file, wav->target_channels);
    write_le32(wav->file, wav->target_sample_rate);
    write_le32(wav->file, wav->target_sample_rate * wav->target_channels * wav->target_bits_per_sample / 8);
    write_le16(wav->file, wav->target_channels * wav->target_bits_per_sample / 8);
    write_le16(wav->file, wav->target_bits_per_sample);
    
    // Data chunk header
    fwrite("data", 1, 4, wav->file);
    write_le32(wav->file, 0); // Will be updated
    
    return wav;
}

int wav_enhanced_read_samples_16bit_mono_8khz(wav_enhanced_t* wav, short* samples, uint32_t num_samples) {
    if (!wav || !wav->file || !samples) return -1;
    
    int bytes_per_sample = wav->original_bits_per_sample / 8;
    int samples_to_read = (int)(num_samples * wav->resample_ratio) + 2; // Extra for interpolation
    
    if (samples_to_read > wav->resample_buffer_size) {
        samples_to_read = wav->resample_buffer_size;
    }
    
    // Read raw samples
    void* raw_buffer = malloc(samples_to_read * wav->original_channels * bytes_per_sample);
    if (!raw_buffer) return -1;
    
    size_t bytes_read = fread(raw_buffer, 1, samples_to_read * wav->original_channels * bytes_per_sample, wav->file);
    int actual_samples_read = bytes_read / (wav->original_channels * bytes_per_sample);
    
    if (actual_samples_read <= 0) {
        free(raw_buffer);
        return 0;
    }
    
    // Convert to 16-bit mono
    convert_to_16bit(raw_buffer, wav->resample_buffer, actual_samples_read, 
                     wav->original_bits_per_sample, wav->original_channels);
    
    free(raw_buffer);
    
    // Resample to 8kHz
    int output_samples = resample_to_8khz(wav, wav->resample_buffer, actual_samples_read, 
                                         samples, num_samples);
    
    wav->samples_read += output_samples;
    return output_samples;
}

int wav_enhanced_write_samples(wav_enhanced_t* wav, const short* samples, uint32_t num_samples) {
    if (!wav || !wav->file || !samples) return -1;
    
    size_t bytes_written = fwrite(samples, sizeof(short), num_samples, wav->file);
    wav->samples_written += bytes_written;
    wav->bytes_written += bytes_written * sizeof(short);
    
    return bytes_written;
}

void wav_enhanced_close(wav_enhanced_t* wav) {
    if (!wav) return;
    
    if (wav->file) {
        // Update header for write files
        if (wav->samples_written > 0) {
            uint32_t data_size = wav->bytes_written;
            uint32_t file_size = 36 + data_size;
            
            // Update file size
            fseek(wav->file, 4, SEEK_SET);
            write_le32(wav->file, file_size);
            
            // Update data size
            fseek(wav->file, 40, SEEK_SET);
            write_le32(wav->file, data_size);
        }
        
        fclose(wav->file);
    }
    
    if (wav->resample_buffer) {
        free(wav->resample_buffer);
    }
    
    free(wav);
}

uint32_t wav_enhanced_get_original_sample_rate(wav_enhanced_t* wav) {
    return wav ? wav->original_sample_rate : 0;
}

uint16_t wav_enhanced_get_original_channels(wav_enhanced_t* wav) {
    return wav ? wav->original_channels : 0;
}

uint16_t wav_enhanced_get_original_bits_per_sample(wav_enhanced_t* wav) {
    return wav ? wav->original_bits_per_sample : 0;
}

uint32_t wav_enhanced_get_total_samples_8khz_mono(wav_enhanced_t* wav) {
    if (!wav) return 0;
    return (uint32_t)(wav->total_samples / wav->resample_ratio);
}

void wav_enhanced_print_info(wav_enhanced_t* wav) {
    if (!wav) return;
    
    printf("Original format: %d Hz, %d channels, %d bits\n",
           wav->original_sample_rate, wav->original_channels, wav->original_bits_per_sample);
    printf("Target format: %d Hz, %d channels, %d bits\n",
           wav->target_sample_rate, wav->target_channels, wav->target_bits_per_sample);
    printf("Original duration: %.2f seconds\n", 
           (float)wav->total_samples / wav->original_sample_rate);
    printf("Resampling ratio: %.3f\n", wav->resample_ratio);
}