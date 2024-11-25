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
#define CANNY_SCALE 1
#define PIXEL_BITRATE 16

unsigned char * convolution(unsigned char * buffer, unsigned char* newBuffer, int width, int height, float * kernel, int kwidth, int kheight, float norm);
unsigned char * greyscale(unsigned char * buffer, int length, float gw, float rw, float bw);
void applyKernel(unsigned char * buffer, unsigned char * newBuffer, int width, int x, int y, float * kernel, int kwidth, int kheight, float norm);
unsigned char * canny(unsigned char * buffer, int width, int height, float scale);
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


    unsigned char *greyBuffer = greyscale(buffer, width * height, 0.2989, 0.5870, 0.1140);
    int result = stbi_write_png("res/textures/grey_Lenna.png", width, height, 1, greyBuffer, width);

    unsigned char *cannyBuffer = canny(greyBuffer, width, height, CANNY_SCALE);
    result = result + stbi_write_png("res/textures/canny_Lenna.png", width, height, 1, cannyBuffer, width);
    unsigned char * halfBuff = halftone(greyBuffer, width, height);
    result += stbi_write_png("res/textures/Halftone.png", width * 2, height * 2, 1, halfBuff, width * 2);

    unsigned char * fsBuffer = fsErrorDiffDithering(greyBuffer, width, height, 7/16.0, 3/ 16.0, 5/16.0, 1/16.0);
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

unsigned char * canny(unsigned char* buffer, int width, int height, float scale){
    float xSobel[] = {1,0,-1, 2,0,-2, 1,0,-1};
    float ySobel[] = {1,2,1, 0,0,0, -1,-2,-1};
    float gaussian[] = {1,2,1 ,2,4,2, 1,2,1};
    float xDirv[] = {0,-1,1};
    float yDirv[] = {0,-1,1};

    int kheight = 3;
    int kwidth = 3;
    int h = (kheight - 1)/2;
    int w = (kwidth - 1)/2;
    float norm = 1.0;
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
    //blurredImage = convolution(buffer, blurredImage, width, height, gaussian, kwidth, kheight, 16);
    
    //finding gradient and angels
    for(int i = 0; i < height; i++){
        for(int j = 0; j < width; j++){
            if(i < h || i > height - 1 - h || j < w || j > width - 1 - w){
                imageGradients[j + i * width] = 0;
                imageAngels[j + i * width] = 0;
                continue;
            }
            applyKernel(buffer, xConv,width, j, i, xSobel, kwidth, kheight, scale);
            applyKernel(buffer, yConv,width, j, i, ySobel, kwidth, kheight, scale);
            imageGradients[i * width + j] = clipPixel(std::sqrt((int)xConv[i * width + j] * xConv[i * width + j] + (int)yConv[i * width + j] * yConv[i * width + j]));
            imageAngels[j + i * width] = std::atan2(yConv[j + i * width], xConv[j + i * width]); 
        }
    }
    stbi_write_png("res/textures/grad_Lenna.png", width, height, 1, imageGradients, width);

    //Non-max suppresion
    for(int i = 0; i < height; i++){
        for(int j = 0; j < width; j++){
            if(i < h || i > height - 1 - h || j < w || j > width - 1 - w){
                imageOutlines[j + i * width] = 0;
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
                imageOutlines[j + i * width] = 0;
            }

            //Double threashholding
            pixelStrength[j + i * width] = doubleThreshhldingPixel(imageOutlines[j + i * width], 0.1 * 255, 0.7 * 255);
        }
    }
    
    stbi_write_png("res/textures/nonemax_Lenna.png", width, height, 1, imageOutlines, width);

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
            sum += (buffer[x - (kwidth-1)/2 + (y-(kheight-1)/2) * width]) * kernel[j + (i) * kwidth];
        }
    }
    newBuffer[x + y * width] = clipPixel(sum * norm);
}

float clipPixel(float p){
    if(p > 255) p = 255;
    if(p < 0) p = 0;
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

unsigned char * fsErrorDiffDithering(unsigned char * buffer, int width, int height, float a, float b, float c, float d) {
    unsigned char * diffused = new unsigned char[width * height];
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
    unsigned char newP = (unsigned char)(p / 255.0 * PIXEL_BITRATE) / 16.0 * 255.0;
    return newP;
 }