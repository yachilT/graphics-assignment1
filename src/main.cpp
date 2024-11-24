#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image.h>
#include <stb/stb_image_write.h>

#include <iostream>

unsigned char * halftone(unsigned char * buffer, int width, int height);
unsigned char * grayscale(unsigned char * buffer, unsigned char * newBuffer, int length, float gw, float rw, float bw);
int main(void)
{
    std::string filepath = "res/textures/Lenna.png";
    int width, height, comps;
    int req_comps = 4;
    unsigned char * buffer = stbi_load(filepath.c_str(), &width, &height, &comps, req_comps);
    unsigned char newBuffer[width * height];

    grayscale(buffer, newBuffer, width * height, 0.2989, 0.5870, 0.1140);
    stbi_write_png("res/textures/grayscale.png", width, height, 1, newBuffer, width);
    unsigned char * resBuff = halftone(newBuffer, width, height);
    int result = stbi_write_png("res/textures/new_Lenna.png", width * 2, height * 2, 1, resBuff, width * 2);
    std::cout << result << std::endl;
    return 0;
}

unsigned char * grayscale(unsigned char * buffer, unsigned char * newBuffer, int length, float rw, float gw, float bw) {
    for (int i = 0; i < length; i++) {
        newBuffer[i] = buffer[i * 4] * rw + buffer[i * 4 + 1] *gw + buffer[i * 4 + 2] * bw;
    }
    return newBuffer;
}

unsigned char * halftone(unsigned char * buffer, int width, int height) {
    int length = width * height;
    unsigned char * result = new unsigned char[length * 4];
    for (int i = 0; i < length; i++) {
        int row = i / width;
        int col = i % width;
        if (buffer[i] < 255.0 / 5) {
            result[row * 2 * 2 * width + col * 2] = 0;
            result[row * 2 * 2 * width + col * 2 + 1] = 0;
            result[(row * 2 + 1) * 2 * width + col * 2] = 0;
            result[(row * 2 + 1) * 2 * width + col * 2 + 1] = 0;
        } else if (buffer[i] >= 255.0 / 5 && buffer[i] < 255.0 / 5 * 2 ) {
            result[row * 2 * 2 * width + col * 2] = 0;
            result[row * 2 * 2 * width + col * 2 + 1] = 0;
            result[(row * 2 + 1) * 2 * width + col * 2] = 255;
            result[(row * 2 + 1) * 2 * width + col * 2 + 1] = 0;
        } else if (buffer[i] >= 255.0 / 5 * 2 && buffer[i] < 255.0 / 5 * 3) {
            result[row * 2 * 2 * width + col * 2] = 255;
            result[row * 2 * 2 * width + col * 2 + 1] = 0;
            result[(row * 2 + 1) * 2 * width + col * 2] = 255;
            result[(row * 2 + 1) * 2 * width + col * 2 + 1] = 0;
        } else if (buffer[i] >= 255.0 / 5 * 3 && buffer[i] < 255.0 / 5 * 4) {
            result[row * 2 * 2 * width + col * 2] = 0;
            result[row * 2 * 2 * width + col * 2 + 1] = 255;
            result[(row * 2 + 1) * 2 * width + col * 2] = 255;
            result[(row * 2 + 1) * 2 * width + col * 2 + 1] = 255;
        } else if (buffer[i] >= 255.0 / 5 * 4) {
            result[row * 2 * 2 * width + col * 2] = 255;
            result[row * 2 * 2 * width + col * 2 + 1] = 255;
            result[(row * 2 + 1) * 2 * width + col * 2] = 255;
            result[(row * 2 + 1) * 2 * width + col * 2 + 1] = 255;
        }
    }
    return result;
}