#include <stb/stb_image.h>
#include <stb/stb_image_write.h>
#include <cmath>

#include <iostream>
#define NON_RELEVANT 0
#define WEAK 1
#define STRONG 2

unsigned char * convolution(unsigned char * buffer, int width, int height, float * kernel, int kwidth, int kheight, float norm);
unsigned char * grayscale(unsigned char * buffer, int length, float gw, float rw, float bw);
void applyKernel(unsigned char * buffer, unsigned char * newBuffer, int width, int x, int y, float * kernel, int kwidth, int w, int h, float norm);
unsigned char * canny(unsigned char * buffer, int width, int height, float scale);
unsigned char * halftone(unsigned char * buffer, int width, int height);
float clipPixel(float p);
int doubleThreshhldingPixel(unsigned char p, int lower, int upper);

int main(void)
{
    std::string filepath = "res/textures/Lenna.png";
    int width, height, comps;
    int req_comps = 4;
    unsigned char * buffer = stbi_load(filepath.c_str(), &width, &height, &comps, req_comps);


    unsigned char *greyBuffer = grayscale(buffer, width * height, 0.2989, 0.5870, 0.1140);
    int result = stbi_write_png("res/textures/grey_Lenna.png", width, height, 1, greyBuffer, width);

    unsigned char *cannyBuffer = canny(greyBuffer, width, height, 1);
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
    int kheight = 3;
    int kwidth = 3;
    int h = (kheight - 1)/2;
    int w = (kwidth - 1)/2;
    unsigned char* xConv = new unsigned char[width * height];
    unsigned char* yConv = new unsigned char[width * height];
    unsigned char* imageGradients = new unsigned char[width * height];
    unsigned char* imageOutlines = new unsigned char[width * height];
    int *pixelStrength = new int[width * height];
    float* imageAngels = new float[width * height];
    int vecXSign = 0;    
    int vecYSign = 0;
    unsigned char currPixel = 0;
    unsigned char posPixel = 0;
    unsigned char negPixel = 0;

    //finding gradient and angels
    for(int i = h; i < height - h; i++){
        for(int j = w; j < width - w; j++){
            applyKernel(buffer, xConv,width, j, i, xSobel, kwidth, w, h, 1/scale);
            applyKernel(buffer, yConv,width, j, i, ySobel, kwidth, w, h, 1/scale);
            imageGradients[i * width + j] = std::sqrt(xConv[i * width + j] * xConv[i * width + j] + yConv[i * width + j] * yConv[i * width + j]);
            imageAngels[j + i * width] = std::atan2(xConv[j + i * width], yConv[j + i * width]);
        }
    }

    //Non-max suppresion
    for(int i = 1; i < height - 1; i++){
        for(int j = 1; j < width - 1; j++){
            //TODO I don't think the angels are correct
            vecXSign = std::signbit(std::sin(imageAngels[j + i * width]) * std::sqrt(2)) ? -1 : 1;
            vecYSign = std::signbit(std::cos(imageAngels[j + i * width]) * std::sqrt(2)) ? -1 : 1;

            currPixel = imageGradients[j + i * width];
            posPixel = imageGradients[j+vecXSign + (i+vecYSign) * width];
            negPixel = imageGradients[j-vecXSign + (i-vecYSign) * width];

            if(posPixel < currPixel && negPixel < currPixel){
                imageOutlines[j + (i) * width] = currPixel;
                imageOutlines[j+vecXSign + (i+vecYSign) * width] = 0;
                imageOutlines[j-vecXSign + (i-vecYSign) * width] = 0;
            }
            else if(currPixel < posPixel && negPixel < posPixel){
                imageOutlines[j + (i) * width] = 0;
                imageOutlines[j+vecXSign + (i+vecYSign) * width] = posPixel;
                imageOutlines[j-vecXSign + (i-vecYSign) * width] = 0;
            }
            else if(currPixel < negPixel && posPixel < negPixel){
                imageOutlines[j + (i) * width] = 0;
                imageOutlines[j+vecXSign + (i+vecYSign) * width] = 0;
                imageOutlines[j-vecXSign + (i-vecYSign) * width] = negPixel;
            }

            //Double threashholding
            pixelStrength[j + i * width] = doubleThreshhldingPixel(imageOutlines[j + i * width], 0.1 * 255, 0.7 * 255);
        }
    }

    //Hysteresis
    for(int i = 1; i < height - 1; i++){
        for(int j = 1; j < width - 1; j++){
            if(pixelStrength[j + i * width] == NON_RELEVANT) imageOutlines[j + i * width] = 0;
            if(pixelStrength[j + i * width] == STRONG) imageOutlines[j + i * width] = 255;
            if(pixelStrength[j + i * width] == WEAK){
                if(pixelStrength[j-1 + (i-1)*width] == STRONG ||
                    pixelStrength[j + (i-1)*width] == STRONG ||
                    pixelStrength[j+1 + (i-1)*width] == STRONG ||
                    pixelStrength[j-1 + (i)*width] == STRONG ||
                    pixelStrength[j+1 + (i)*width] == STRONG ||
                    pixelStrength[j-1 + (i+1)*width] == STRONG ||
                    pixelStrength[j + (i+1)*width] == STRONG ||
                    pixelStrength[j+1 + (i+1)*width] == STRONG)  imageOutlines[j + i * width] = 255;
                else imageOutlines[j + i * width] = 0;
            }
        }
    }

    return imageOutlines;
}

// 0 - non relevant, 1 - weak, 2 - strong
int doubleThreshhldingPixel(unsigned char p, int lower, int upper){
    if(p < lower) return NON_RELEVANT;
    else if(p >= lower && p <= upper) return WEAK;
    else return STRONG;
}
 
//working but only give convolution
unsigned char * convolution(unsigned char * buffer, int width, int height, float * kernel, int kwidth, int kheight, float norm){
    unsigned char * newBuffer = new unsigned char[width*height];
    int h = (kheight - 1)/2;
    int w = (kwidth - 1)/2;
    for(int i = h; i < height - h; i++){
        for(int j = w; j < width - w; j++){
            applyKernel(buffer, newBuffer,width, j, i, kernel, kwidth, w, h, norm);
        }
    }

    return newBuffer;
}

void applyKernel(unsigned char * buffer, unsigned char * newBuffer, int width, int x, int y, float * kernel, int kwidth, int w, int h, float norm){
    float sum = 0;
    for(int i = y-h; i <= y+h; i++){
        for(int j = x-w; j <= x+w; j++){
            sum += (buffer[j + (i) * width]) * kernel[j+w-x + (i+h-y) * kwidth];
        }
    }
    newBuffer[x + y * width] = clipPixel(sum / norm);
}

float clipPixel(float p){
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