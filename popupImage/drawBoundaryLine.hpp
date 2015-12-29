//
//  drawBoundaryLine.hpp
//  Color_Segmentation
//
//  Created by jollytrees on 11/14/15.
//  Copyright © 2015 jollytrees. All rights reserved.
//

#ifndef drawBoundaryLine_hpp
#define drawBoundaryLine_hpp

#include <stdio.h>
#include "popupObjAlgorithm.h"

class drawBoundaryLine : public popupObjAlgorithm
{
public:
    virtual bool execute(popupObject *obj);
    
};

#endif /* drawBoundaryLine_hpp */
