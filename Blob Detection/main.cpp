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
#include "CImg.h"
#define cimg_use_magick

using namespace cimg_library;

int y_arr[256];
int u_arr[256];
int v_arr[256];

struct color {
    int r;
    int g;
    int b;
};

unsigned *map;

color colors[8];


inline int get_max(int x, int y) {
    return x > y ? x : y;
}

inline int get_min(int x, int y) {
    return x < y ? x : y;
}

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
            for (int j = u1; j < u2; j++) {
                u_arr[j] |= (1 << i);
            }
            for (int j = v1; j < v2; j++) {
                v_arr[j] |= (1 << i);
            }
        }
        i++;
    }
    
    colors[0].r = 255;
    colors[0].g = 0;
    colors[0].b = 0;
    
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

template<typename T> CImg<T> detectColors(CImg<T> image) {
    CImg<T> changed(image.width(), image.height(), 1, 3, 0);
    image.RGBtoYCbCr();

    for (int y = 0; y < image.height(); y++) {
        for (int x = 0; x < image.width(); x++) {

            int color_y = image(x, y, 0);
            int color_u = image(x, y, 1);
            int color_v = image(x, y, 2);
            
            int colorBits = (y_arr[color_y] & u_arr[color_u]) & v_arr[color_v];
            
            map[y * image.width() + x] = colorBits;
        }
    }
    
    return changed;
}

inline color colorfromBits(int colorBits) {
    return colors[(int) (log(colorBits) / log(2))];
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
