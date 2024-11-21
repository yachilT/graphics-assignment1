#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image.h>
#include <stb/stb_image_write.h>

#include <iostream>

unsigned char * grayscale(unsigned char * buffer, unsigned char * newBuffer, int length, float gw, float rw, float bw);
int main(void)
{
    std::string filepath = "res/textures/Lenna.png";
    int width, height, comps;
    int req_comps = 4;
    unsigned char * buffer = stbi_load(filepath.c_str(), &width, &height, &comps, req_comps);
    unsigned char newBuffer[width * height];

    grayscale(buffer, newBuffer, width * height, 0.2989, 0.5870, 0.1140);
    int result = stbi_write_png("res/textures/new_Lenna.png", width, height, 1, newBuffer, width);
    std::cout << result << std::endl;
    return 0;
}

unsigned char * grayscale(unsigned char * buffer, unsigned char * newBuffer, int length, float rw, float gw, float bw) {
    for (int i = 0; i < length; i++) {
        newBuffer[i] = buffer[i * 4] * rw + buffer[i * 4 + 1] *gw + buffer[i * 4 + 2] * bw;
    }
    return newBuffer;
}