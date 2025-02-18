#pragma once

#define VIDEO_FLAG          1

#define TESTING_MODE        1          // using a testing mode

#include <opencv2/opencv.hpp>
#include <vector>

/*===    file include        ===*/

/*===    user definition        ===*/

/*===    external variable    ===*/

/*===    external function    ===*/

/*===    public function prototypes    ===*/
void TestColorBand(cv::Mat& frame);
void getimageComparator(cv::Mat& frame1, cv::Mat& frame2);
void maskingDifferenceImage(const cv::Mat& diffImage);
// this section is used for sharping the edge of color edge by 8 point left and 8 point right 
void PartitionOfImage(const cv::Mat& frame, std::vector<cv::Point2f> &topEdgePoints, std::vector<cv::Point2f> &bottomEdgePoints);

/*===    static function prototypes    ===*/

/*===    static typedef variable        ===*/

/*===    static variable        ===*/

/*===    constant variable    ===*/

/*===    global variable        ===*/

/*===    implementation        ===*/

/***********************************************************************************************************************
Global functions
***********************************************************************************************************************/
struct Rectanglepoint{
    cv::Point topLeft;
    cv::Point buttomRight;
};

struct Point{
    int max; // maxmimum
    int min; // minimum 
    // using a constructor to limit the value upper band and lower band
    Point(int x, int y): max(std::clamp(x, 0, 255)),
                         min(std::clamp(y, 0, 255)){} 
};

struct ThresholdColor{
    Point blue;
    Point green;
    Point red; 
};