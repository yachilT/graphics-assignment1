#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image.h>
#include <stb/stb_image_write.h>

#include <iostream>


unsigned char * convolution(unsigned char * buffer, int width, int height, float * kernel, int kwidth, int kheight, float norm);
unsigned char * grayscale(unsigned char * buffer, int length, float gw, float rw, float bw);
void applyKernel(unsigned char * buffer,unsigned char * newBuffer, int width, int height, int x, int y, float * kernel, int kwidth, int kheight, float norm);
unsigned char * canny(unsigned char * buffer, int width, int height);
unsigned char * halftone(unsigned char * buffer, int width, int height);

int main(void)
{
    std::string filepath = "res/textures/Lenna.png";
    int width, height, comps;
    int req_comps = 4;
    unsigned char * buffer = stbi_load(filepath.c_str(), &width, &height, &comps, req_comps);

    unsigned char *greyBuffer = grayscale(buffer, width * height, 0.2989, 0.5870, 0.1140);
    int result = stbi_write_png("res/textures/grey_Lenna.png", width, height, 1, greyBuffer, width);

    unsigned char *cannyBuffer = canny(greyBuffer, width, height);
    result = result + stbi_write_png("res/textures/canny_Lenna.png", width, height, 1, cannyBuffer, width);
    std::cout << result << std::endl;
    return 0;
}

unsigned char * grayscale(unsigned char * buffer, int length, float rw, float gw, float bw) {
    unsigned char * newBuffer = new unsigned char[length];
    for (int i = 0; i < length; i++) {
        newBuffer[i] = buffer[i * 4] * rw + buffer[i * 4 + 1] *gw + buffer[i * 4 + 2] * bw;
    }
    return newBuffer;
}

unsigned char * canny(unsigned char* buffer, int width, int height){
    float xSobel[] = {1,0,-1, 2,0,-2, 1,0,-1};
    return convolution(buffer, width, height, xSobel, 3, 3, 9);
}

unsigned char * convolution(unsigned char * buffer, int width, int height, float * kernel, int kwidth, int kheight, float norm){
    unsigned char * newBuffer = new unsigned char[width*height];
    for(int i = kheight/2; i < height - kheight/2; i++){
        for(int j = kwidth/2; j < width - kwidth/2; j++){
            applyKernel(buffer, newBuffer,width, height, j, i, kernel, kwidth, kheight, norm);
        }
    }

    return newBuffer;
}

void applyKernel(unsigned char * buffer, unsigned char * newBuffer, int width, int height, int x, int y, float * kernel, int kwidth, int kheight, float norm){
    float sum = 0;
    for(int i = 0; i < kheight; i++){
        for(int j = 0; j < kwidth; j++){
            sum += (buffer[(x + j - kwidth/2) + (y + i - kheight) * width]) * kernel[j + i * kwidth];
        }
    }
    sum = sum / norm;
    sum = sum > 255 ? 255 : sum;
    sum = sum < 0 ? 0 : sum;  
    newBuffer[x + y * width] = sum;
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