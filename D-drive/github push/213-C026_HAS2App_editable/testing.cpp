#include <iostream>
#include <vector>
#include <opencv2/opencv.hpp>
#include "testing.hpp"
class ImageComparator
{
    private: 
    cv::Mat diffMask;
    std::vector<cv::Point> diffLocations;

    public:
    // comparing two images and creating a frame of pixel difference
    void compareImages(const cv::Mat& frame1, const cv::Mat& frame2);
    void findDifferenceLocations();
    cv::Mat markDifferences(cv::Mat& targetFrame);
    void markingPixellyByThreshold(const cv::Mat& targetFrame);
    cv::Scalar getRGBAtDiffPoints(const cv::Mat& frame, size_t index);
    size_t getDifferenceCount()const;
};

/*************************************************************************************
 * @brief compare two frame and create a new frame out of it called diffMask
 * @param frame1,frame2 Two frame which will need to be compared
 * @return Nonei
 * @note This function does modify the diffMask private object.
 * @warning The function assumes the both frame matrix have same size and type.
 **************************************************************************************/
void ImageComparator::compareImages(const cv::Mat& frame1, const cv::Mat& frame2){
    if(frame1.size() != frame2.size() || frame1.type() != frame2.type())
    {
        throw std::runtime_error("Image must be same size and type !");
    }
    cv:: Mat diffMaskOutput;
    // for reference
    // cv::imwrite("Display_frame1.bmp", frame1);
    // cv::imwrite("Capture_output.bmp", frame2);
    //creating a different masking frame showing the difference
    cv::absdiff(frame1, frame2, diffMask);
    cv::imwrite("before_Diff_mask.bmp", diffMask);
    cv::cvtColor(diffMask, diffMask, cv::COLOR_BGR2GRAY);
    cv::threshold(diffMask, diffMaskOutput, 10, 255, cv::THRESH_BINARY);
    cv::imwrite("after_Diff_mask.bmp", diffMaskOutput);
    findDifferenceLocations();
    //maskingDifferenceImage(diffMaskOutput);
    // diffMask = cv::Mat()
    // cv::adaptiveThreshold(diffMask,             //INPUT image
    //     diffMask,                               // output image
    //     255,                                    // maximum value to use for bonary threshold(white)  -> i.e. image converted to GRAY 
    //     cv::ADAPTIVE_THRESH_GAUSSIAN_C,         // adaptive method: use gussian-weighted sum to compute the threshold
    //     cv::THRESH_BINARY,                      // threshold tye : pixel above this threshold are set to 255 i.e. white color
    //     5,                                      // the size of neighborhood
    //     2);                                     // value subratcted from wighted sum to fine tune the threshold
    //finf difference location on frame 
    
}

/*************************************************************************************
 * @brief findDifferenceLocations() compare two frame and create a new frame out of it called diffMask
 * @note  This function does modify the diffMask private object.
 **************************************************************************************/
void ImageComparator::findDifferenceLocations(){
    // diffLocations is 2 points vector 
    diffLocations.clear();
    for (int y = 0; y < diffMask.rows; y++) {
        for (int x = 0; x < diffMask.cols; x++){
            if (diffMask.at<uchar>(y,x) > 0){
                diffLocations.push_back(cv::Point(x,y));
            }
        }
    }
}

/*************************************************************************************
 * @brief markDifferences(cv::Mat& targetFrame) draw a circle on difference point on frame
 * @param targetFrame frame where circle is drawn
 * @return return a new frame with circle added on it 
 **************************************************************************************/
cv::Mat ImageComparator::markDifferences(cv::Mat& targetFrame){
    // creating a clone 
    cv::Mat circeMark = targetFrame.clone();
    for (const auto& x0_y0 : diffLocations){
        cv::circle(circeMark, x0_y0, 3, cv::Scalar(255, 255, 255), -1);
    }
    return circeMark;
}

/*************************************************************************************
 * @brief markingPixellyByThreshold(const cv::Mat& frame)  read a each pixel column wise
 *        using airthematic pointer 
 * @param frame1,index Both are related and index is a size value
 * @return 3D vector data set conatining location of difference between the frame
 **************************************************************************************/
void ImageComparator::markingPixellyByThreshold(const cv::Mat& frame){
    cv::imwrite("beforepixelmarking.bmp", frame);

       std::vector<ThresholdColor> threshold_mat = {
        {Point(255, 238 ),  Point(19, 0),      Point(50, 0)},         // 0 -> blue color 
        {Point(255, 228),   Point(21, 0),      Point(255, 227)},      // 1 -> magenta
        {Point(36, 0),      Point(255, 234),    Point(39, 0)},         // 2 -> green
        {Point(255, 244),   Point(255, 253),    Point(255, 244)},      // 3 -> white
        {Point(31, 0),      Point(255, 245),    Point(255, 220)},         // 5 -> yellow 
        {Point(255, 245),   Point(255, 251),    Point(44, 0)},         // 4 -> cyan
        {Point(8, 0),       Point(10, 0),        Point(14, 0)},         // 6 -> black
        {Point(15, 0),     Point(7, 0),      Point(255, 231)}       // 7 -> red
    };
    
    //to store the image that spot thresholder ommiter
    cv::Mat thresholder = frame.clone();
    if(thresholder.type() == CV_8UC3){

        // definig a point for reading topedge and buttom edge of rectangle
        std::vector<cv::Point2f> topEdgePoints;
        std::vector<cv::Point2f> bottomEdgePoints;

        // calling a fucntion for refrencing a area under a rectangle
        PartitionOfImage(thresholder, topEdgePoints, bottomEdgePoints);
        for(int k = 0; k < 8; ++k){
            printf("top  %d -> (%f, %f)\n",k,topEdgePoints[k].x,topEdgePoints[k].y);
            printf("down %d -> (%f, %f)\n",k,bottomEdgePoints[k].x,bottomEdgePoints[k].y);
            // using arithematic pointer columnn wise data
            for (int i = topEdgePoints[k].y; i <= bottomEdgePoints[k].y; i++){
                for (int j = topEdgePoints[k].x; j <= bottomEdgePoints[k].x; j++){
                    // getting a reference value to a pixel at (x,y)
                    // cv::Vec3b& pixel = *(frame.ptr<cv::Vec3b>(j) + i);
                   
                    cv::Vec3b& pixel = thresholder.at<cv::Vec3b>(cv::Point(j,i));
                    if ( j == 1200 && i == 45){
                        printf("pixel value at (%d, %d) = (%d, %d, %d) \n", j,i,pixel[0],pixel[1], pixel[2]);
                        printf("threshold value at BGR (%d, %d) = (%d, %d, %d) \n", j,i,threshold_mat[k].blue.min, threshold_mat[k].green.min, threshold_mat[k].red.min);
                    }        
                    if ((pixel[0] < threshold_mat[k].blue.min       ||  pixel[0] > threshold_mat[k].blue.max )    ||
                        (pixel[1] < threshold_mat[k].green.min      ||  pixel[1] > threshold_mat[k].green.max)    ||
                        (pixel[2] < threshold_mat[k].red.min        ||  pixel[2] > threshold_mat[k].red.max )){

                        std::cout << "point : " << j << ", " << i << std::endl;
                        //cv::Point center(j, i);
                        //cv::circle(thresholder, center, 1, cv::Scalar(255, 255, 255), -1);
                        //thresholder.at<cv::Vec3b>(j, i) = cv::Vec3b(0, 0, 0);
                        try {
                            thresholder.at<cv::Vec3b>(j, i) = {255,255,255};
                        }
                        catch (const std::exception& e){
                            std::cerr << "point : " << j << ", " << i <<  "Error: " << e.what() << std::endl;
                        }
                    }
                }
            }
        }
        cv::imwrite("Threshold_check.bmp", thresholder);
    }
}

/*************************************************************************************
 * @brief getRGBAtDiffPoints(const cv::Mat& frame, size_t index)  get the difference point
 *        on a frame andd its index number  and return its value in vector 3D
 * @param frame1,index Both are related and index is a size value
 * @return 3D vector data set conatining location of difference between the frame
 **************************************************************************************/
cv::Scalar ImageComparator::getRGBAtDiffPoints(const cv::Mat& frame, size_t index){
    if (index >= diffLocations.size()) {
        throw std::out_of_range("Difference point index out of range");
    }

    cv::Point diff_pt = diffLocations[index];
    return frame.at<cv::Vec3b>(diff_pt);
}

/*************************************************************************************
 * @brief  getDifferenceCount()  count the number of different pixel in a frame
 *         on a frame andd its index number  and return its value in vector 3D
 * @param  frame1,index Both are related and index is a size value
 * @return 3D vector data set conatining location of difference between the frame
 **************************************************************************************/
size_t ImageComparator::getDifferenceCount() const{
    return diffLocations.size();
}

/*************************************************************************************
 * @brief  getimageComparator()  call a class ImageComparator for comparing, displying
 *         and conting difference point        
 * @param  frame1,frame2 Both frame are read from disp0_img and frame matrix variable
 * @return void
 **************************************************************************************/
void getimageComparator(cv::Mat& frame1, cv::Mat& frame2)
{
    ImageComparator comp_obj;
    
    try {
        comp_obj.markingPixellyByThreshold(frame2);
        // comaparator images
        comp_obj.compareImages(frame1, frame2);

        // mark difference on frame1 
        cv::Mat markedFrame = comp_obj.markDifferences(frame1);

        // print difference count
        //std::cout << "Difference found: " << comp_obj.getDifferenceCount() << std::endl;

        // Example of getting RGB at first difference
        // if (comp_obj.getDifferenceCount() > 0){
        //     cv::Scalar rgbValue = comp_obj.getRGBAtDiffPoints(frame2, 0);
        //     std::cout << "RBG at firdt diff point: "
        //               << "(R: "  << rgbValue[0]
        //               << ", G: " << rgbValue[1] 
        //               << ", B: " << rgbValue[2] << " )" << std::endl;
        // }
        // //display result
        cv::imwrite("circeMark Difference.bmp", markedFrame);
    }
    catch (const std::exception& e){
        std::cerr << "Error: " << e.what() << std::endl;
    }
}
# if 0
/*************************************************************************************
 * @brief  maskingDifferenceImage()  mask the difference point which remain after
 *         comparing monitor output and monitor 2 frame . 
 * @param  diffImage this cv::Mat is a difference between the two monitor
 * @return void
 **************************************************************************************/
void maskingDifferenceImage(const cv::Mat& diffImage)
{
    // clonig a image matrix for modification
    cv::Mat maskingImage = diffImage.clone();
    // reading a frame size
    int frame_width  = diffImage.cols;
    int frame_height = diffImage.rows;
    float stepX = frame_width / 8;                              // dividing a frame into 8 section
    
    // define a rectangle parameters
    std::vector<cv::Point2f> topEdgePoints;
    std::vector<cv::Point2f> bottomEdgePoints;
    
    cv::Point topLeft(0, 0);
    cv::Point buttomRight(frame_width, frame_height);
    
    // storing a  top vertex inside a vector
    for (int i = 0; i < 9; ++i) {
        int x = topLeft.x + i * stepX;
        int y = topLeft.y;
        if (i == 0)            topEdgePoints.push_back(cv::Point2f(x , y)); 
        else if( i == 8)       topEdgePoints.push_back(cv::Point2f(x - 6, y));
        else                   topEdgePoints.push_back(cv::Point2f(x - 4, y)); 
    }

    // storing buttom vertex inside a vector
    for( int i = 0; i < 9; ++i){
        int x = topLeft.x + i * stepX;
        int y = buttomRight.y;
        if (i == 8)             bottomEdgePoints.push_back(cv::Point2f(x, y));
        else                    bottomEdgePoints.push_back(cv::Point2f(x + 6, y)); 
    }

    // draw a filled black rectangles
    if (topEdgePoints.size() == bottomEdgePoints.size()){
        
        for(size_t i = 0; i < topEdgePoints.size(); i++){
            // -1 is used for body fill and all fill color is black color
            cv::Point2f Edge1 = topEdgePoints[i];
            cv::Point2f Edge2 = bottomEdgePoints[i];
            cv::rectangle(maskingImage, Edge1, Edge2, cv::Scalar(0, 0, 0) ,-1);
        }
    }
    printf("--------------------------------------------------------");
    cv::imwrite("masking_Image.bmp", maskingImage);
}
# else

/*************************************************************************************
 * @brief  PartitionOfImage()  divide the working section inside a two vector  . 
 * @param  frame,topEdgePoints,bottomEdgePoints frame is to read the image detail which is
 *                                              clone inorder to protect original data
 *          -> topEdgePoints is to read upper section of screen , this also return value
 *          -> bottomEdgePoints is to read buttom section of screen which return this value
 * @return void
 **************************************************************************************/
void PartitionOfImage(const cv::Mat& frame, std::vector<cv::Point2f>& topEdgePoints, std::vector<cv::Point2f>& bottomEdgePoints)
{
    // clonig a image matrix for modification
    cv::Mat maskingImage = frame.clone();
    // reading a frame size
    int frame_width  = maskingImage.cols;
    int frame_height = maskingImage.rows;
    float stepX = frame_width / 8;                              // dividing a frame into 8 section
    
    cv::Point topLeft(0, 0);
    cv::Point buttomRight(frame_width - 1, frame_height - 1);

    // storing a  top vertex inside a vector
    for (int i = 0; i < 8; ++i) {
        int x = topLeft.x + i * stepX;
        int y = topLeft.y;
        if (i == 0)            topEdgePoints.push_back(cv::Point2f(x + 8 , y)); 
        else if( i == 8)       topEdgePoints.push_back(cv::Point2f(x + 8, y));
        else                   topEdgePoints.push_back(cv::Point2f(x + 8, y)); 
    }

    // storing buttom vertex inside a vector
    for( int i = 0; i < 8; ++i){
        int x = stepX + i * stepX;
        int y = buttomRight.y;
        if (i == 8)             bottomEdgePoints.push_back(cv::Point2f(x - 8, y));
        else                    bottomEdgePoints.push_back(cv::Point2f(x - 8, y)); 
    }
}
#endif

