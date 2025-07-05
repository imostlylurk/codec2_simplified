/*
 * Simple codec2 example program
 * Demonstrates basic usage of the codec2 library
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <codec2/codec2.h>

#define SAMPLES_PER_FRAME 160  // for 3200 bps mode
#define BITS_SIZE ((64 + 7) / 8)  // 64 bits = 8 bytes for 3200 bps mode

int main() {
    struct CODEC2 *codec2;
    short speech_in[SAMPLES_PER_FRAME];
    short speech_out[SAMPLES_PER_FRAME];
    unsigned char bits[BITS_SIZE];
    int i;
    
    printf("Codec2 Example Program\n");
    printf("======================\n");
    
    // Create codec2 instance
    codec2 = codec2_create(CODEC2_MODE_3200);
    if (codec2 == NULL) {
        fprintf(stderr, "Error: Could not create codec2 instance\n");
        return 1;
    }
    
    printf("Codec2 instance created successfully\n");
    printf("Mode: 3200 bps\n");
    printf("Samples per frame: %d\n", codec2_samples_per_frame(codec2));
    printf("Bits per frame: %d\n", codec2_bits_per_frame(codec2));
    
    // Generate a simple test signal (sine wave)
    for (i = 0; i < SAMPLES_PER_FRAME; i++) {
        speech_in[i] = (short)(16000.0 * sin(2.0 * M_PI * 440.0 * i / 8000.0));
    }
    
    printf("\nEncoding test signal...\n");
    
    // Encode the speech
    codec2_encode(codec2, bits, speech_in);
    
    printf("Encoding completed. Compressed to %d bytes\n", BITS_SIZE);
    
    // Decode the speech
    printf("Decoding...\n");
    codec2_decode(codec2, speech_out, bits);
    
    printf("Decoding completed\n");
    
    // Simple comparison (just show first few samples)
    printf("\nFirst 10 samples comparison:\n");
    printf("Original -> Decoded\n");
    for (i = 0; i < 10; i++) {
        printf("%6d -> %6d\n", speech_in[i], speech_out[i]);
    }
    
    // Clean up
    codec2_destroy(codec2);
    
    printf("\nCodec2 example completed successfully!\n");
    
    return 0;
}