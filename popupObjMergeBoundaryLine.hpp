//
//  popupObjMergeBoundaryLine.hpp
//  Color_Segmentation
//
//  Created by jollytrees on 11/10/15.
//  Copyright © 2015 jollytrees. All rights reserved.
//

#ifndef popupObjMergeBoundaryLine_hpp
#define popupObjMergeBoundaryLine_hpp

#include <stdio.h>
#include "popupObjAlgorithm.h"

class popupObjMergeBoundaryLine : public popupObjAlgorithm
{
public:
    virtual bool execute(popupObject *obj);
    
};

#endif /* popupObjMergeBoundaryLine_hpp */

