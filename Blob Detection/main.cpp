//
//  main.cpp
//  Image Processing
//
//  Created by Idean Labib on 6/20/15.
//  Copyright (c) 2015 Idean Labib. All rights reserved.
//


//STEP 1: THRESHOLDING

#include <iostream>
#include <stdlib.h>
#include <math.h>
#include <vector>
#include "CImg.h"
#define cimg_use_magick

using namespace cimg_library;

int y_arr[256];
int u_arr[256];
int v_arr[256];

struct color {
    std::string name = "";
    int r;
    int g;
    int b;
};

struct region {
    int minX;
    int maxX;
    int minY;
    int maxY;
    
    int mass = 1;
    
    color color;
    int colorBits;
};

std::vector<int> map;

color colors[8];

std::vector<region> blobs;
int blobsCount;

inline int get_max(int x, int y) {
    return x > y ? x : y;
}

inline int get_min(int x, int y) {
    return x < y ? x : y;
}

//Returns color for a pixel's value from the bitwise operations.
inline color colorFromBits(int colorBits) {
    return colors[(int) (log(colorBits) / log(2))];
}

//load the colors
bool loadColors() {
    for (int i = 0; i < 256; i++) {
        y_arr[i] = 0;
        u_arr[i] = 0;
        v_arr[i] = 0;
    }
    
    FILE *in;
    
    in = fopen("colors.txt", "r");
    if (!in) {
        std::cout << "FILE NOT FOUND" << std::endl;
        return false;
    }
    
    char buf[256];
    
    int i = 0;
    while(fgets(buf, 256, in)) {
        int y1, y2, u1, u2, v1, v2;
        int n = sscanf(buf,"(%d:%d,%d:%d,%d:%d)",&y1,&y2,&u1,&u2,&v1,&v2);
        
        if (n == 6) {
            y1 = get_max(y1, 0);
            y2 = get_min(y2, 256);
            for (int j = y1; j < y2; j++) {
                y_arr[j] |= (1 << i);
            }
            u1 = get_max(u1, 0);
            u2 = get_min(u2, 256);
            for (int j = u1; j < u2; j++) {
                u_arr[j] |= (1 << i);
            }
            v1 = get_max(v1, 0);
            v2 = get_min(v2, 256);
            for (int j = v1; j < v2; j++) {
                v_arr[j] |= (1 << i);
            }
        }
        i++;
    }
    
    colors[0].r = 255;
    colors[0].g = 0;
    colors[0].b = 0;
    colors[0].name = "Red";

    
    return true;
}

template<typename T> CImg<T> RGBtoYUV(CImg<T> image) {
    CImg<T> changed(image.width(), image.height(), 1, 3, 0);
    for (int x = 0; x < image.width();x++) {
        for (int y = 0; y < image.height(); y++) {
            T r = image(0, 0, 0, 0);
            T g = image(0, 0, 0, 1);
            T b = image(0, 0, 0, 2);

            T u = (r + 1.96*g - 2.96*b) / (-3.333);
            T y_col = b - u;
            T v = r - y_col;
            
            changed(x, y, 0, 0) = y_col;
            changed(x, y, 0, 1) = u;
            changed(x, y, 0, 2) = v;
        }
    }
    return changed;
}


/*Find groups
 BUFFER:
 _____________
 | A | B | C |
 -------------
 | D | X |
 */
template<typename T> void findGroups(CImg<T> image) {

    int width = image.width();
    int height = image.height();
    
    std::vector<int> labelTable = {0}; //values correspond to indeces of regionsTable array
    std::vector<region> regionsTable;

    int labelBuffer[width * height];
    
    int currLabel = 1;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int i = y * image.width() + x;
            int aLabel = (x > 0 && y > 0) ? labelBuffer[i - width - 1] : 0;
            int bLabel = (y > 0) ? labelBuffer[i - width] : 0;
            int cLabel = (x < width - 1 && y > 0) ? labelBuffer[i - width + 1] : 0;
            int dLabel = (x > 0) ? labelBuffer[i - 1] : 0;
            
            if (map[i] != 0) {
                int min = width * height + 1;
                if (aLabel != 0 && regionsTable[labelTable[aLabel]].colorBits == map[i] && aLabel < min) min = aLabel;
                if (bLabel != 0 && regionsTable[labelTable[bLabel]].colorBits == map[i] && bLabel < min) min = bLabel;
                if (cLabel != 0 && regionsTable[labelTable[cLabel]].colorBits == map[i] && cLabel < min) min = cLabel;
                if (dLabel != 0 && regionsTable[labelTable[dLabel]].colorBits == map[i] && dLabel < min) min = dLabel;
                
                //A, B, C, and D must be unlabeled
                if (min == width * height + 1) {
                    labelBuffer[i] = currLabel;
                    labelTable.push_back((int) labelTable.size());
                    region r;
                    
                    //Initialize values for region/labels
                    r.minX = x;
                    r.maxX = x;
                    r.minY = y;
                    r.maxY = y;
                    r.colorBits = map[i];
                    regionsTable.push_back(r);
                    
                    currLabel++;
                } else {
                    labelBuffer[i] = min;
                    
                    regionsTable[min].maxY = y;
                    
                    regionsTable[min].mass++;
                    
                    if (x > regionsTable[min].maxX) regionsTable[min].maxX = x;
                    if (x < regionsTable[min].minX) regionsTable[min].minX = x;
                    
                    if (aLabel != 0 && regionsTable[min].colorBits == regionsTable[labelTable[aLabel]].colorBits) labelTable[aLabel] = min;
                    if (bLabel != 0 && regionsTable[min].colorBits == regionsTable[labelTable[bLabel]].colorBits) labelTable[bLabel] = min;
                    if (cLabel != 0 && regionsTable[min].colorBits == regionsTable[labelTable[cLabel]].colorBits) labelTable[cLabel] = min;
                    if (dLabel != 0 && regionsTable[min].colorBits == regionsTable[labelTable[dLabel]].colorBits) labelTable[dLabel] = min;
                }
            }
            else {
                labelBuffer[i] = 0;
            }
        }
    }
    
    for (int i = 1; i < regionsTable.size() - 1; i++) {
        if (labelTable[i] != i) {
            if (regionsTable[i].maxX > regionsTable[labelTable[i]].maxX) regionsTable[labelTable[i]].maxX = regionsTable[i].maxX;
            if (regionsTable[i].minX < regionsTable[labelTable[i]].minX) regionsTable[labelTable[i]].minX = regionsTable[i].minX;
            if (regionsTable[i].maxY > regionsTable[labelTable[i]].maxY) regionsTable[labelTable[i]].maxY = regionsTable[i].maxY;
            if (regionsTable[i].minY < regionsTable[labelTable[i]].minY) regionsTable[labelTable[i]].minY = regionsTable[i].minY;
        } else {
            if (regionsTable[i].mass > 50) blobs.push_back(regionsTable[i]);
        }
    }
}

void setColors() {
    for (int i = 0; i < blobs.size();i++) {
        blobs[i].color = colorFromBits(blobs[i].colorBits);
        std::cout << "X: " << blobs[i].minX << " -> " << blobs[i].maxX << ", Y: " << blobs[i].minY << " -> " << blobs[i].maxY << ", Color: " << blobs[i].color.name << " (" << blobs[i].color.r << ", " << blobs[i].color.g << ", " << blobs[i].color.b << ")" << ", Mass: " << blobs[i].mass << std::endl;
    }
}

region mergeBlobs(region first, region second) {
    region merged;
    
    merged.colorBits = first.colorBits;
    merged.minX = get_min(first.minX, second.minX);
    merged.minY = get_min(first.minY, second.minY);
    merged.maxX = get_max(first.maxX, second.maxX);
    merged.maxY = get_max(first.maxY, second.maxY);
    merged.mass = first.mass + second.mass;
    
    return merged;
}

bool checkDensity(region first, region second, int threshold) {
    double originalDensity = first.mass / ((first.maxX - first.minX) * (first.maxY - first.minY));
    double newDensity = (double) (first.mass + second.mass) / ((get_max(first.maxX, second.maxX) - get_min(first.minX, second.minX)) * (get_max(first.maxY, second.maxY) - get_min(first.minY, second.minY)));
    return newDensity - originalDensity >= 0 || originalDensity - newDensity < threshold;
}

void mergeDensities() {
    for (int i = 0; i < blobs.size() - 1;i++) {
        for (int j = i + 1; j < blobs.size(); j++) {
            int first_x = (blobs[i].minX + blobs[i].maxX) / 2;
            int first_y = (blobs[i].minY + blobs[i].maxY) / 2;
            int second_x = (blobs[j].minX + blobs[j].maxX) / 2;
            int second_y = (blobs[j].minY + blobs[j].maxY) / 2;
            int maxDistanceX = first_x - blobs[i].minX + second_x - blobs[j].minX + 10;
            int maxDistanceY = first_y - blobs[i].minY + second_y - blobs[j].minY + 10;
            
            //TODO: Check Distance!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Overlapping won't be same item........
            if (std::abs(first_x - second_x) < maxDistanceX && std::abs(first_y - second_y) < maxDistanceY && blobs[i].colorBits == blobs[j].colorBits && checkDensity(blobs[i], blobs[j], 1)) {
                blobs[i] = mergeBlobs(blobs[i], blobs[j]);
                blobs.erase(blobs.begin() + j);
            }
        }
    }
}

//detect the colors of the pixels using bitwise and store in map array
template<typename T> CImg<T> detectColors(CImg<T> image) {
    CImg<T> changed(image.width(), image.height(), 1, 3, 0);
    CImg<T> converted = image.get_RGBtoYCbCr();
    changed = image;
    for (int y = 0; y < image.height(); y++) {
        for (int x = 0; x < image.width(); x++) {
            
            int color_y = converted(x, y, 0);
            int color_u = converted(x, y, 1);
            int color_v = converted(x, y, 2);
            
            int colorBits = (y_arr[color_y] & u_arr[color_u]) & v_arr[color_v];
            
            map.push_back(colorBits);
        }
    }
    
    findGroups(image);
    
    mergeDensities();
    
    setColors();
    
    for (int i = 0; i < blobs.size();i++) {
        int minX = blobs[i].minX;
        int maxX = blobs[i].maxX;
        int minY = blobs[i].minY;
        int maxY = blobs[i].maxY;
        
        for (int x = minX; x <= maxX;x++) {
            changed(x, minY, 0) = blobs[i].color.r;
            changed(x, minY, 1) = blobs[i].color.g;
            changed(x, minY, 2) = blobs[i].color.b;
            
            changed(x, maxY, 0) = blobs[i].color.r;
            changed(x, maxY, 1) = blobs[i].color.g;
            changed(x, maxY, 2) = blobs[i].color.b;
            
            for (int y = minY; y <= maxY; y++) {
                changed(minX, y, 0) = blobs[i].color.r;
                changed(minX, y, 1) = blobs[i].color.g;
                changed(minX, y, 2) = blobs[i].color.b;
                
                changed(maxX, y, 0) = blobs[i].color.r;
                changed(maxX, y, 1) = blobs[i].color.g;
                changed(maxX, y, 2) = blobs[i].color.b;
            }
        }
    }
    
    return changed;
}

int main(int argc, const char * argv[]) {
    cimg::imagemagick_path("/usr/local/bin/convert");
    
    if (!loadColors())
        return 1;
    
    CImg<double> image("hydrant.jpg");
    CImgDisplay main_disp(image,"Original",0);
    
    CImg<double> colors = detectColors(image);
    CImgDisplay colors_disp(colors,"Original",0);
    
    while (!main_disp.is_closed()) {
        main_disp.wait();
    }
    
    return 0;
}
