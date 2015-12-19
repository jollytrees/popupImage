//
//  functionClickPatch.h
//  popupImage
//
//  Created by jollytrees on 11/15/15.
//
//

#ifndef functionClickPatch_h
#define functionClickPatch_h

static int clickPatch(std::vector< struct patches* > patches, cv::Point p){
    
    for(int i =0 ;i < patches.size(); i++){
        
        if(patches[i]->paths.size() > 1){
            
            bool outerContourInFlag = false;
            bool innerContoursInFlag = false;
            int dist = pointPolygonTest( patches[i]->paths[0], p, true );
            if( dist >=0 )
                outerContourInFlag = true;
            
            for(int j = 1; j < patches[i]->paths.size(); j++){
                int dist = pointPolygonTest( patches[i]->paths[j], p, true );
                if( dist >=0 )
                    innerContoursInFlag = true;
                else
                    innerContoursInFlag = false;
            }
            
            if(innerContoursInFlag == false && outerContourInFlag == true){
                return i;
            }
            
        }else{
            for(int j = 0; j < patches[i]->paths.size(); j++){
                int dist = pointPolygonTest( patches[i]->paths[j], p, true );
                if( dist >=0 ){
                    return i;
                }
            }
        }
    }
    
    return -1;
}

#endif /* functionClickPatch_h */
