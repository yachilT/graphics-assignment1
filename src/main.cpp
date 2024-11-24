#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image.h>
#include <stb/stb_image_write.h>

#include <iostream>


unsigned char * convolution(unsigned char * buffer, int width, int height, float * kernel, int kwidth, int kheight, float norm);
unsigned char * grayscale(unsigned char * buffer, int length, float gw, float rw, float bw);
void applyKernel(unsigned char * buffer,unsigned char * newBuffer, int width, int height, int x, int y, float * kernel, int kwidth, int kheight, float norm);
unsigned char * canny(unsigned char * buffer, int width, int height, float scale);
unsigned char * halftone(unsigned char * buffer, int width, int height);
float capPixel(float p);

int main(void)
{
    std::string filepath = "res/textures/Lenna.png";
    int width, height, comps;
    int req_comps = 4;
    unsigned char * buffer = stbi_load(filepath.c_str(), &width, &height, &comps, req_comps);


    unsigned char *greyBuffer = grayscale(buffer, width * height, 0.2989, 0.5870, 0.1140);
    int result = stbi_write_png("res/textures/grey_Lenna.png", width, height, 1, greyBuffer, width);
    unsigned char *cannyBuffer = canny(greyBuffer, width, height, 5);
    result = result + stbi_write_png("res/textures/canny_Lenna.png", width, height, 1, cannyBuffer, width);
    unsigned char * resBuff = halftone(greyBuffer, width, height);
    result += stbi_write_png("res/textures/Halftone.png", width * 2, height * 2, 1, resBuff, width * 2);
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

unsigned char * canny(unsigned char* buffer, int width, int height, float scale){
    float xSobel[] = {1,0,-1, 2,0,-2, 1,0,-1};
    float ySobel[] = {1,2,1, 0,0,0, -1,-2,-1};

    unsigned char* xConv = convolution(buffer, width, height, xSobel, 3, 3, 9);
    unsigned char* yConv = convolution(buffer, width, height, ySobel, 3, 3, 9);
    unsigned char* imageGradients = new unsigned char[width * height];
    float* imageAngels = new float[width * height];

    for(int i = 1; i < height - 1; i++){
        for(int j = 1; j < width - 1; j++){
            imageGradients[i * width + j] = scale * std::sqrt(std::pow((float)(int)xConv[i * width + j], 2) + std::pow((float)(int)yConv[i * width + j], 2));
        }
    }

    return imageGradients;
}

unsigned char * convolution(unsigned char * buffer, int width, int height, float * kernel, int kwidth, int kheight, float norm){
    unsigned char * newBuffer = new unsigned char[width*height];
    int h = (kheight - 1)/2;
    int w = (kwidth - 1)/2;
    for(int i = h; i < height - h; i++){
        for(int j = w; j < width - w; j++){
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
    newBuffer[x + y * width] = capPixel(sum);
}

float capPixel(float p){
    p = p > 255 ? 255 : p;
    p = p < 0 ? 0 : p;  
    return p;
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