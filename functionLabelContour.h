//
//  functionLabelContour.h
//  popupImage
//
//  Created by jollytrees on 11/15/15.
//
//

#ifndef functionLabelContour_h
#define functionLabelContour_h

using namespace cv;
static void LabelContour(paths_type &contours, int index, Mat &drawing){
    
    static RNG rng(12345);
    /// Get the moments
    vector<Moments> mu(contours.size() );
    for( size_t i = 0; i < contours.size(); i++ )
    { mu[i] = moments( contours[i], false ); }
    
    ///  Get the mass centers:
    vector<Point2f> mc( contours.size() );
    for( size_t i = 0; i < contours.size(); i++ )
    { mc[i] = Point2f( mu[i].m10/mu[i].m00 , mu[i].m01/mu[i].m00 ); }

    std::string str = std::to_string(index);
    
    /// Draw contours
    for( size_t i = 0; i< contours.size(); i++ )
    {
        Scalar color = Scalar( rng.uniform(100, 255), rng.uniform(100,255), rng.uniform(100,255) );
        //drawContours( drawing, contours, i, color, 2, 8 );
        if(i==0){
            putText(drawing, str, mc[i], FONT_HERSHEY_SIMPLEX, 0.5, color, 1);
            circle( drawing, mc[i], 2, color, -1, 8, 0 );
        }
    }
}

static void LabelPatchContour(std::vector< struct patches* > &input_patches, cv::Size &matSize, cv::Mat &canvas){
    for(int i =0 ;i < input_patches.size(); i++){
        LabelContour(input_patches[i]->paths, i, canvas);
    }
}


static void LabelPatchContourPoint(std::vector< struct patches* > &input_patches, cv::Size &matSize, cv::Mat &canvas){
    
    for(int i =0 ;i < input_patches.size(); i++){
        LabelContour(input_patches[i]->paths, i, canvas);
    }
}

#endif /* functionLabelContour_h */
