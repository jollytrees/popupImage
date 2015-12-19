//
//  Mat2QImage.h
//  popupImage
//
//  Created by jollytrees on 11/15/15.
//
//

#ifndef Mat2QImage_h
#define Mat2QImage_h

static QImage mat2QImage(cv::Mat const& mat, QImage::Format format){
    return QImage(mat.data, mat.cols, mat.rows, mat.step, format).copy();
}

#endif /* Mat2QImage_h */
