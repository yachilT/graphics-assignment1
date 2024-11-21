#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image.h>
#include <stb/stb_image_write.h>

#include <iostream>


unsigned char * convolution(unsigned char * buffer, int width, int hight, float ** kernel, int kwidth, int klength);
unsigned char * grayscale(unsigned char * buffer, int length, float gw, float rw, float bw);
void applyKernel(unsigned char * buffer,unsigned char * newBuffer, int width, int height, int x, int y, float * kernel, int kwidth, int kheight);
unsigned char * canny(unsigned char * buffer, int width, int height);

int main(void)
{
    std::string filepath = "res/textures/Lenna.png";
    int width, height, comps;
    int req_comps = 4;
    unsigned char * buffer = stbi_load(filepath.c_str(), &width, &height, &comps, req_comps);

    unsigned char *greyBuffer = grayscale(buffer, width * height, 0.2989, 0.5870, 0.1140);
    int result = stbi_write_png("res/textures/grey_Lenna.png", width, height, 1, greyBuffer, width);

    unsigned char *cannyBuffer = canny(buffer, width, height);


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
    return convolution(buffer, width, height, xSobel, 3, 3);
}

unsigned char * convolution(unsigned char * buffer, int width, int height, float * kernel, int kwidth, int kheight){
    unsigned char * newBuffer = new unsigned char[width*height];
    for(int i = kheight/2; i < height - kheight/2; i++){
        for(int j = kwidth/2; j < width - kwidth/2; j++){
            applyKernel(buffer, newBuffer,width, height, j, i, kernel, kwidth, kheight);
        }
    }
    return newBuffer;
}

void applyKernel(unsigned char * buffer, unsigned char * newBuffer, int width, int length, int x, int y, float ** kernel, int kwidth, int klength){
    for(int i = 0; i < klength; i++){
        for(int j = 0; j < kwidth; j++){
            newBuffer[(x + j) + (y + i) * width] = buffer[(x + j) + (y + i) * width] * kernel[i][j];
        }
    }
}