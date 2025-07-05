/*
 * WAV file utilities implementation
 */

#include "wav_util.h"
#include <stdlib.h>
#include <string.h>

wav_file_t* wav_open_read(const char* filename) {
    wav_file_t* wav = malloc(sizeof(wav_file_t));
    if (!wav) return NULL;
    
    memset(wav, 0, sizeof(wav_file_t));
    
    wav->file = fopen(filename, "rb");
    if (!wav->file) {
        free(wav);
        return NULL;
    }
    
    // Read WAV header
    if (fread(&wav->header, sizeof(wav_header_t), 1, wav->file) != 1) {
        fclose(wav->file);
        free(wav);
        return NULL;
    }
    
    // Validate WAV header
    if (strncmp(wav->header.riff, "RIFF", 4) != 0 ||
        strncmp(wav->header.wave, "WAVE", 4) != 0 ||
        strncmp(wav->header.fmt, "fmt ", 4) != 0 ||
        strncmp(wav->header.data, "data", 4) != 0) {
        fclose(wav->file);
        free(wav);
        return NULL;
    }
    
    // Calculate total samples
    wav->total_samples = wav->header.data_size / (wav->header.num_channels * wav->header.bits_per_sample / 8);
    
    return wav;
}

wav_file_t* wav_open_write(const char* filename, uint32_t sample_rate, uint16_t channels, uint16_t bits_per_sample) {
    wav_file_t* wav = malloc(sizeof(wav_file_t));
    if (!wav) return NULL;
    
    memset(wav, 0, sizeof(wav_file_t));
    
    wav->file = fopen(filename, "wb");
    if (!wav->file) {
        free(wav);
        return NULL;
    }
    
    // Initialize WAV header
    strncpy(wav->header.riff, "RIFF", 4);
    wav->header.chunk_size = 36; // Will be updated when closing
    strncpy(wav->header.wave, "WAVE", 4);
    strncpy(wav->header.fmt, "fmt ", 4);
    wav->header.fmt_chunk_size = 16;
    wav->header.audio_format = 1; // PCM
    wav->header.num_channels = channels;
    wav->header.sample_rate = sample_rate;
    wav->header.bits_per_sample = bits_per_sample;
    wav->header.byte_rate = sample_rate * channels * bits_per_sample / 8;
    wav->header.block_align = channels * bits_per_sample / 8;
    strncpy(wav->header.data, "data", 4);
    wav->header.data_size = 0; // Will be updated when closing
    
    // Write header (will be updated later)
    fwrite(&wav->header, sizeof(wav_header_t), 1, wav->file);
    
    return wav;
}

int wav_read_samples(wav_file_t* wav, short* samples, uint32_t num_samples) {
    if (!wav || !wav->file || !samples) return -1;
    
    uint32_t samples_to_read = num_samples;
    if (wav->samples_read + samples_to_read > wav->total_samples) {
        samples_to_read = wav->total_samples - wav->samples_read;
    }
    
    if (samples_to_read == 0) return 0;
    
    size_t bytes_per_sample = wav->header.bits_per_sample / 8;
    size_t samples_read;
    
    if (wav->header.bits_per_sample == 16) {
        // Direct read for 16-bit samples
        samples_read = fread(samples, bytes_per_sample * wav->header.num_channels, samples_to_read, wav->file);
    } else if (wav->header.bits_per_sample == 8) {
        // Convert 8-bit to 16-bit
        uint8_t* temp_buffer = malloc(samples_to_read * wav->header.num_channels);
        if (!temp_buffer) return -1;
        
        samples_read = fread(temp_buffer, wav->header.num_channels, samples_to_read, wav->file);
        for (uint32_t i = 0; i < samples_read * wav->header.num_channels; i++) {
            samples[i] = (short)((temp_buffer[i] - 128) * 256);
        }
        free(temp_buffer);
    } else {
        return -1; // Unsupported bit depth
    }
    
    wav->samples_read += samples_read;
    return samples_read;
}

int wav_write_samples(wav_file_t* wav, const short* samples, uint32_t num_samples) {
    if (!wav || !wav->file || !samples) return -1;
    
    size_t samples_written = fwrite(samples, sizeof(short) * wav->header.num_channels, num_samples, wav->file);
    wav->samples_written += samples_written;
    
    return samples_written;
}

void wav_close(wav_file_t* wav) {
    if (!wav) return;
    
    if (wav->file) {
        // Update header for write files
        if (wav->samples_written > 0) {
            wav->header.data_size = wav->samples_written * wav->header.num_channels * wav->header.bits_per_sample / 8;
            wav->header.chunk_size = 36 + wav->header.data_size;
            
            // Seek to beginning and rewrite header
            fseek(wav->file, 0, SEEK_SET);
            fwrite(&wav->header, sizeof(wav_header_t), 1, wav->file);
        }
        
        fclose(wav->file);
    }
    
    free(wav);
}

uint32_t wav_get_sample_rate(wav_file_t* wav) {
    return wav ? wav->header.sample_rate : 0;
}

uint16_t wav_get_channels(wav_file_t* wav) {
    return wav ? wav->header.num_channels : 0;
}

uint16_t wav_get_bits_per_sample(wav_file_t* wav) {
    return wav ? wav->header.bits_per_sample : 0;
}

uint32_t wav_get_total_samples(wav_file_t* wav) {
    return wav ? wav->total_samples : 0;
}