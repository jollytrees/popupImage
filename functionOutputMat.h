//
//  functionOutputMat.h
//  Color_Segmentation
//
//  Created by jollytrees on 11/14/15.
//  Copyright © 2015 jollytrees. All rights reserved.
//

#ifndef functionOutputMat_h
#define functionOutputMat_h

static void outputMat(int i, std::string folder, std::string title, cv::Mat &output){
    
    std::ostringstream oss;
    oss.str("");
    oss<< i << std::endl;
    
    std::string str = "./" + folder + "/" + title + oss.str()+".png";
    imwrite(str, output);
    
}

#endif /* functionOutputMat_h */
