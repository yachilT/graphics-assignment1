#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>
#include <cmath>
#include <iostream>

// gray scale weights
#define RED_WEIGHT 0.2989
#define GREEN_WEIGHT 0.5870
#define BLUE_WEIGHT 0.1140

#define WHITE 255
#define BLACK 0
#define NON_RELEVANT 0
#define WEAK 1
#define STRONG 2
#define CANNY_SCALE 0.25
#define M_PI 3.14159265358979323846

// Floyed-Steinberg dithering
#define COMPRESSED_GS 16 // compressed grayscale
#define ALPHA 7/16.0
#define BETA 3/16.0
#define GAMMA 5/16.0
#define DELTA 1/16.0



unsigned char * convolution(unsigned char * buffer, unsigned char* newBuffer, int width, int height, float * kernel, int kwidth, int kheight, float norm);
unsigned char * greyscale(unsigned char * buffer, int length, float gw, float rw, float bw);
void applyKernel(unsigned char * buffer, unsigned char * newBuffer, int width, int x, int y, float * kernel, int kwidth, int kheight, float norm);
unsigned char * canny(unsigned char * buffer, int width, int height, float scale, float lower, float upper);
unsigned char * halftone(unsigned char * buffer, int width, int height);
float clipPixel(float p);
int doubleThreshhldingPixel(unsigned char p, int lower, int upper);

unsigned char * fsErrorDiffDithering(unsigned char * buffer, int width, int height, float a, float b, float c, float d);
unsigned char trunc(unsigned char p);
int main(void)
{
    std::string filepath = "res/textures/Lenna.png";
    int width, height, comps;
    int req_comps = 4;
    unsigned char * buffer = stbi_load(filepath.c_str(), &width, &height, &comps, req_comps);


    unsigned char *greyBuffer = greyscale(buffer, width * height, RED_WEIGHT, GREEN_WEIGHT, BLUE_WEIGHT);
    int result = stbi_write_png("res/textures/grey_Lenna.png", width, height, 1, greyBuffer, width);

    unsigned char *cannyBuffer = canny(greyBuffer, width, height, CANNY_SCALE, 0.05, 0.2);
    result = result + stbi_write_png("res/textures/canny_Lenna.png", width, height, 1, cannyBuffer, width);
    unsigned char * halfBuff = halftone(greyBuffer, width, height);
    result += stbi_write_png("res/textures/Halftone.png", width * 2, height * 2, 1, halfBuff, width * 2);

    unsigned char * fsBuffer = fsErrorDiffDithering(greyBuffer, width, height, ALPHA, BETA, GAMMA, DELTA);
    result += stbi_write_png("res/textures/FloyedSteinberg.png", width, height, 1, fsBuffer, width);
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

unsigned char * canny(unsigned char* buffer, int width, int height, float scale, float lower, float upper){
    float xSobel[] = {1,0,-1, 2,0,-2, 1,0,-1};
    float ySobel[] = {1,2,1, 0,0,0, -1,-2,-1};
    float gaussian[] = {1,2,1 ,2,4,2, 1,2,1};

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
    convolution(buffer, blurredImage, width, height, gaussian, kwidth, kheight, 1.0/16);

    //finding gradient and angels
    for(int i = 0; i < height; i++){
        for(int j = 0; j < width; j++){
            if(i < h || i > height - 1 - h || j < w || j > width - 1 - w){
                imageGradients[j + i * width] = BLACK;
                imageAngels[j + i * width] = BLACK;
                continue;
            }
            applyKernel(blurredImage, xConv,width, j, i, xSobel, kwidth, kheight, scale);
            applyKernel(blurredImage, yConv,width, j, i, ySobel, kwidth, kheight, scale);
            imageGradients[i * width + j] = clipPixel(std::sqrt((int)xConv[i * width + j] * xConv[i * width + j] + (int)yConv[i * width + j] * yConv[i * width + j]));
            imageAngels[j + i * width] = std::atan2(yConv[j + i * width], xConv[j + i * width]) * (180/M_PI); 
        }
    }

    //Non-max suppresion
    for(int i = 0; i < height; i++){
        for(int j = 0; j < width; j++){
            if(i < h || i > height - 1 - h || j < w || j > width - 1 - w){
                imageOutlines[j + i * width] = BLACK;
                continue;
            }
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
            }
            else{
                imageOutlines[j + i * width] = BLACK;
            }

            //Double threashholding
            pixelStrength[j + i * width] = doubleThreshhldingPixel(imageOutlines[j + i * width], lower * WHITE, upper * WHITE);
        }
    }
    
    //Hysteresis
    for(int i = h; i < height - h; i++){
        for(int j = w; j < width - w; j++){
            if(pixelStrength[j + i * width] == NON_RELEVANT) imageOutlines[j + i * width] = BLACK;
            if(pixelStrength[j + i * width] == STRONG) imageOutlines[j + i * width] = WHITE;
            if(pixelStrength[j + i * width] == WEAK){
                if(pixelStrength[j-1 + (i-1)*width] == STRONG ||
                    pixelStrength[j + (i-1)*width] == STRONG ||
                    pixelStrength[j+1 + (i-1)*width] == STRONG ||
                    pixelStrength[j-1 + (i)*width] == STRONG ||
                    pixelStrength[j+1 + (i)*width] == STRONG ||
                    pixelStrength[j-1 + (i+1)*width] == STRONG ||
                    pixelStrength[j + (i+1)*width] == STRONG ||
                    pixelStrength[j+1 + (i+1)*width] == STRONG) {  
                        imageOutlines[j + i * width] = WHITE;
                        pixelStrength[j + i * width] = STRONG;
                    }
                else imageOutlines[j + i * width] = BLACK;
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
    for(int i = 0; i < kheight; i++){
        for(int j = 0; j < width; j++){
            newBuffer[j + i * width] = BLACK;
            newBuffer[j + (height-1 - i) * width] = BLACK;
        }
    }
    for(int i = kheight; i < height; i++){
        for(int j = 0; j < kwidth; j++){
            newBuffer[j + i * width] = BLACK;
            newBuffer[width-1-j + i * width] = BLACK;
        }
    }

    for(int i = (kheight-1)/2; i < height - (kheight-1)/2; i++){
        for(int j = (kwidth-1)/2; j < width - (kheight-1)/2; j++){
            applyKernel(buffer, newBuffer,width, j, i, kernel, kwidth, kheight, norm);
        }
    }
    return newBuffer;
}

void applyKernel(unsigned char * buffer, unsigned char * newBuffer, int width, int x, int y, float * kernel, int kwidth, int kheight, float norm){
    float sum = 0;
    for(int i = 0; i < kheight; i++){
        for(int j = 0; j < kwidth; j++){
            sum += buffer[x-(kwidth-1)/2+j + (y-(kheight-1)/2+i) * width] * kernel[j + (i) * kwidth];
        }
    }
    newBuffer[x + y * width] = clipPixel(std::abs(sum) * norm);
}

float clipPixel(float p){
    if(p > WHITE) return WHITE;
    if(p < BLACK) return BLACK;
    return p;
}

unsigned char * halftone(unsigned char * buffer, int width, int height) {
    int length = width * height;
    unsigned char * result = new unsigned char[length * 4];
    for (int i = 0; i < length; i++) {
        int row = i / width;
        int col = i % width;
        if (buffer[i] < WHITE / 5) {
            result[row * 2 * 2 * width + col * 2] = BLACK;
            result[row * 2 * 2 * width + col * 2 + 1] = BLACK;
            result[(row * 2 + 1) * 2 * width + col * 2] = BLACK;
            result[(row * 2 + 1) * 2 * width + col * 2 + 1] = BLACK;
        } else if (buffer[i] >= WHITE / 5 && buffer[i] < WHITE / 5 * 2 ) {
            result[row * 2 * 2 * width + col * 2] = BLACK; 
            result[row * 2 * 2 * width + col * 2 + 1] = BLACK;
            result[(row * 2 + 1) * 2 * width + col * 2] = WHITE;
            result[(row * 2 + 1) * 2 * width + col * 2 + 1] = BLACK;
        } else if (buffer[i] >= WHITE / 5 * 2 && buffer[i] < WHITE / 5 * 3) {
            result[row * 2 * 2 * width + col * 2] = WHITE;
            result[row * 2 * 2 * width + col * 2 + 1] = BLACK;
            result[(row * 2 + 1) * 2 * width + col * 2] = WHITE;
            result[(row * 2 + 1) * 2 * width + col * 2 + 1] = BLACK;
        } else if (buffer[i] >= WHITE / 5 * 3 && buffer[i] < WHITE / 5 * 4) {
            result[row * 2 * 2 * width + col * 2] = BLACK;
            result[row * 2 * 2 * width + col * 2 + 1] = WHITE;
            result[(row * 2 + 1) * 2 * width + col * 2] = WHITE;
            result[(row * 2 + 1) * 2 * width + col * 2 + 1] = WHITE;
        } else if (buffer[i] >= WHITE / 5 * 4) {
            result[row * 2 * 2 * width + col * 2] = WHITE;
            result[row * 2 * 2 * width + col * 2 + 1] = WHITE;
            result[(row * 2 + 1) * 2 * width + col * 2] = WHITE;
            result[(row * 2 + 1) * 2 * width + col * 2 + 1] = WHITE;
        }
    }
    return result;
}

unsigned char * fsErrorDiffDithering(unsigned char * buffer, int width, int height, float a, float b, float c, float d) {
    unsigned char * diffused = new unsigned char[width * height]; // copy of image with diffused error
    unsigned char * result = new unsigned char[width * height];

    diffused[0] = buffer[0];
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            result[i * width + j] = trunc(diffused[i * width + j]);
            char error = buffer[i * width + j] - result[i * width + j];
            diffused[i * width + j + 1] = buffer[i * width + j + 1] + error * a;
            if (i < height - 1) {
                diffused[(i + 1) * width + j - 1] = buffer[(i + 1) * width + j - 1] + error * b; 
                diffused[(i + 1) * width + j] = buffer[(i + 1) * width + j] + error * c;
                diffused[(i + 1) * width + j + 1] = buffer[(i + 1) * width + j + 1] + error * d;
            }
        }
    }
    return result;
 }
 
unsigned char trunc(unsigned char p) {
    // maps 0-255 into one of COMPRESSED_GS values spaced
    return (unsigned char)(p / 255.0 * COMPRESSED_GS) / (float)COMPRESSED_GS * 255.0;
 }
