#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define BYTE_SIZE 8

void compare_images(const char *file1, const char *file2) {
    FILE *fp1 = fopen(file1, "rb");
    FILE *fp2 = fopen(file2, "rb");
    
    if (!fp1 || !fp2) {
        printf("Error opening files.\n");
        return;
    }
    
    uint8_t byte1, byte2;
    size_t total_bits = 0, matching_bits = 0, differing_bytes = 0;
    
    while (fread(&byte1, 1, 1, fp1) && fread(&byte2, 1, 1, fp2)) {
        if (byte1 != byte2) {
            differing_bytes++;
        }
        for (int i = 0; i < BYTE_SIZE; i++) {
            total_bits++;
            if (((byte1 >> i) & 1) == ((byte2 >> i) & 1)) {
                matching_bits++;
            }
        }
    }
    
    fclose(fp1);
    fclose(fp2);
    
    if (total_bits == 0) {
        printf("No data compared.\n");
        return;
    }
    
    double accuracy = (matching_bits * 100.0) / total_bits;
    printf("Total bits compared: %zu\n", total_bits);
    printf("Matching bits: %zu\n", matching_bits);
    printf("Bit-wise Accuracy: %.2f%%\n", accuracy);
    printf("Differing bytes: %zu\n", differing_bytes);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <image1.jpg> <image2.jpg>\n", argv[0]);
        return 1;
    }
    compare_images(argv[1], argv[2]);
    return 0;
}
