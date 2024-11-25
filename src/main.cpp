#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>
#include <cmath>

#include <iostream>
#define NON_RELEVANT 0
#define WEAK 1
#define STRONG 2
#define SCALE_FACTOR 4

unsigned char * convolution(unsigned char * buffer, unsigned char* newBuffer, int width, int height, float * kernel, int kwidth, int kheight, float norm);
unsigned char * greyscale(unsigned char * buffer, int length, float gw, float rw, float bw);
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


    unsigned char *greyBuffer = greyscale(buffer, width * height, 0.2989, 0.5870, 0.1140);
    int result = stbi_write_png("res/textures/grey_Lenna.png", width, height, 1, greyBuffer, width);

    unsigned char *cannyBuffer = canny(greyBuffer, width, height, 1);
    result = result + stbi_write_png("res/textures/canny_Lenna.png", width, height, 1, cannyBuffer, width);
    unsigned char * halfBuff = halftone(greyBuffer, width, height);
    result += stbi_write_png("res/textures/Halftone.png", width * 2, height * 2, 1, halfBuff, width * 2);
    std::cout << result << std::endl;

    delete [] buffer;
    delete [] greyBuffer;
    delete [] cannyBuffer;
    delete [] halfBuff;
    return 0;
}

unsigned char * greyscale(unsigned char * buffer, int length, float rw, float gw, float bw) {
    unsigned char * newBuffer = new unsigned char[length];
    for (int i = 0; i < length; i++) {
        newBuffer[i] = buffer[i * 4] * rw + buffer[i * 4 + 1] *gw + buffer[i * 4 + 2] * bw;
    }
    return newBuffer;
}

unsigned char * canny(unsigned char* buffer, int width, int height, float scale){
    float xSobel[] = {1,0,-1, 2,0,-2, 1,0,-1};
    float ySobel[] = {1,2,1, 0,0,0, -1,-2,-1};
    float gausian[] = {1,2,1 ,2,4,2, 1,2,1};

    float xSobelGaussian[] = {}; // sobel derivative of gaussian
    float ySobelGaussian[] = {}
    int kheight = 3;
    int kwidth = 3;
    int h = (kheight - 1)/2;
    int w = (kwidth - 1)/2;
    int *pixelStrength = new int[width * height];
    float* imageAngels = new float[width * height];
    unsigned char* blurredImage = new unsigned char[width * height];
    unsigned char* xConv = new unsigned char[width * height];
    unsigned char* yConv = new unsigned char[width * height];
    unsigned char* imageGradients = new unsigned char[width * height];
    unsigned char* imageOutlines = new unsigned char[width * height];
    float currAngel = 0;
    int vecXSign = 0;    
    int vecYSign = 0;
    unsigned char currPixel = 0;
    unsigned char posPixel = 0;
    unsigned char negPixel = 0;

    //reducing noise
    blurredImage = convolution(buffer, blurredImage, width, height, gausian, kwidth, kheight, 16);

    //finding gradient and angels
    for(int i = h; i < height - h; i++){
        for(int j = w; j < width - w; j++){
            applyKernel(blurredImage, xConv,width, j, i, xSobel, kwidth, w, h, 1/scale);
            applyKernel(blurredImage, yConv,width, j, i, ySobel, kwidth, w, h, 1/scale);
            imageGradients[i * width + j] = std::sqrt(xConv[i * width + j] * xConv[i * width + j] + yConv[i * width + j] * yConv[i * width + j]);
            imageAngels[j + i * width] = std::atan2(yConv[j + i * width], xConv[j + i * width]); // i think it should be reversed?
        }
    }

    stbi_write_png("res/textures/grad_Lenna.png", width, height, 1, imageGradients, width);

    //Non-max suppresion
    for(int i = 1; i < height - 1; i++){
        for(int j = 1; j < width - 1; j++){
            currAngel = imageAngels[j + i * width] < 0 ? imageAngels[j + i * width] + 360 : imageAngels[j + i * width];

            //0 degrees
            if((currAngel > 337.5 ||  currAngel <= 22.5) || (currAngel > 157.5 && currAngel <= 202.5)){
                vecXSign = 1;
                vecYSign = 0;
            }
            //45 degrees
            else if((currAngel > 22.5 ||  currAngel <= 67.5) || (currAngel > 202.5 && currAngel <= 247.5)){
                vecXSign = 1;
                vecYSign = -1;
            }
            //90 degrees
            else if((currAngel > 67.5 ||  currAngel <= 112.5) || (currAngel > 247.5 && currAngel <= 292.5)){
                vecXSign = 0;
                vecYSign = 1;
            }
            //135 degrees
            else if((currAngel > 112.5 ||  currAngel <= 157.5) || (currAngel > 292.5 && currAngel <= 337.5)){
                vecXSign = 1;
                vecYSign = 1;
            }

            currPixel = imageGradients[j + i * width];
            posPixel = imageGradients[j+vecXSign + (i+vecYSign) * width];
            negPixel = imageGradients[j-vecXSign + (i-vecYSign) * width];
            
            if(posPixel < currPixel && negPixel < currPixel){
                imageOutlines[j + (i) * width] = currPixel;
                //imageOutlines[j+vecXSign + (i+vecYSign) * width] = 0;
                //imageOutlines[j-vecXSign + (i-vecYSign) * width] = 0;
            }
            else{
                imageOutlines[j + i * width] = 0;
            }
            /*else if(currPixel < posPixel && negPixel < posPixel){
                imageOutlines[j + (i) * width] = 0;
                imageOutlines[j+vecXSign + (i+vecYSign) * width] = posPixel;
                imageOutlines[j-vecXSign + (i-vecYSign) * width] = 0;
            }
            else if(currPixel < negPixel && posPixel < negPixel){
                imageOutlines[j + (i) * width] = 0;
                imageOutlines[j+vecXSign + (i+vecYSign) * width] = 0;
                imageOutlines[j-vecXSign + (i-vecYSign) * width] = negPixel;
            }*/

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
                    pixelStrength[j+1 + (i+1)*width] == STRONG) {  
                        imageOutlines[j + i * width] = 255;
                        pixelStrength[j + i * width] = STRONG;
                    }
                else imageOutlines[j + i * width] = 0;
            }
        }
    }

    delete [] xConv;
    delete [] yConv;
    delete [] imageGradients;
    delete [] pixelStrength;
    delete [] imageAngels;
    delete [] blurredImage;
    return imageOutlines;
}

// 0 - non relevant, 1 - weak, 2 - strong
int doubleThreshhldingPixel(unsigned char p, int lower, int upper){
    if(p < lower) return NON_RELEVANT;
    else if(p >= lower && p <= upper) return WEAK;
    else return STRONG;
}
 
//working but only give convolution
unsigned char * convolution(unsigned char * buffer, unsigned char* newBuffer, int width, int height, float * kernel, int kwidth, int kheight, float norm){
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