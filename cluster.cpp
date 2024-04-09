#include "cluster.h"
#include <iostream>
#include <cstring>
#include <cmath>
#include <sstream>
#include <random>
#include <algorithm>


float rand_norm() {
    return (rand()%RAND_MAX)/((float)RAND_MAX);
}


Cluster generate_cluster(Point center, size_t count, float spread) {
    Cluster result;
    for(size_t i=0; i<count; i++){
        float angle = M_PI*2*rand_norm();
        float distance = 0;
        for(int i=0; i<10; i++) {
            distance += (rand_norm()-0.5)*spread;
        }
        float x = center.x + cos(angle)*distance;
        float y = center.y + sin(angle)*distance;
        result.push_back({x,y});
    }
    return result;
};

ClusterList generate_cluster_list(Point lower_bounds, Point upper_bounds, size_t cluster_count, size_t point_count) {
    float width = (upper_bounds.x-lower_bounds.x);
    float height = (upper_bounds.y-lower_bounds.y);
    float area = width * height;
    ClusterList result;
    for(size_t i=0; i<cluster_count; i++){
        Point center = {rand_norm()*width,rand_norm()*height};
        float spread = (rand_norm()*0.7+0.3)*sqrt(area)/cluster_count/4;
        result.push_back(generate_cluster(center,point_count,spread));
    }
    return result;
}

Cluster collapse_cluster_list(ClusterList list) {
    Cluster result;
    for (Cluster& cluster : list) {
        for (Point& point : cluster) {
            result.push_back(point);
        }
    }

    std::random_device random_dev;
    std::mt19937 rng(random_dev());

    // shuffle the sequence for good measure
    std::shuffle(result.begin(),result.end(),rng);
    return result;
}





struct Color {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
};

Color hue_to_rgb(float hue) {
    hue = fmod(hue,360);
    int hue_segment = hue/60;
    uint8_t c = 255;
    uint8_t x = (1-fabs(fmod(hue/60,2)-1))*255;
    switch(hue_segment) {
        case 0:  return {c,x,0};
        case 1:  return {x,c,0};
        case 2:  return {0,c,x};
        case 3:  return {0,x,c};
        case 4:  return {x,0,c};
        case 5:
        default: return {c,0,x};
    }
}

void display_clusters(ClusterList list, Point lower_bounds, Point upper_bounds, float scale, bool color) {
    float width  = upper_bounds.x-lower_bounds.x;
    float height = upper_bounds.y-lower_bounds.y;
    if( (width<=0) || (height<=0) ){
        std::cerr << "Invalid display bounds given to display_clusters\n";
        return;
    }

    int view_width  = ceil(width*scale);
    int view_height = ceil(height*scale);
    char canvas[view_width*view_height];
    memset(canvas,' ',sizeof(canvas));
    char cluster_char = 'A';
    for(Cluster& cluster : list) {
        for(Point& point : cluster) {
            int x = (point.x - lower_bounds.x)*scale;
            int y = (point.y - lower_bounds.y)*scale;
            if( (x<0) || (x>=view_width) || (y<0) || (y>=view_height) ){
                continue;
            }
            canvas[y*view_width+x] = cluster_char;
        }
        cluster_char++;
    }

    for(int i=0; i<view_width; i++){
        std::cout << "--";
    }
    std::cout << '\n';

    std::stringstream display;
    float const GOLDEN_ANGLE = 137.5077640500378546463487;
    for(int y=0; y<view_height; y++){
        for(int x=0; x<view_width; x++){
            char symbol = canvas[y*view_width+x];
            if(symbol == ' '){
                display << "  ";
                continue;
            }
            char cluster_id = symbol-'A';
            float hue = cluster_id * GOLDEN_ANGLE;
            Color rgb = hue_to_rgb(hue);
            if(color){
                display << "\033[48;2;"
                        << (int)rgb.red << ';'
                        << (int)rgb.green << ';'
                        << (int)rgb.blue << 'm'
                        << symbol << symbol
                        << "\033[31;0m";
            } else {
                display << symbol << symbol;
            }
        }
        display << '\n';
    }
    std::cout << display.str();

    for(int i=0; i<view_width; i++){
        std::cout << "--";
    }
    std::cout << '\n';

}

