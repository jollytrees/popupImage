//
//  popupObjClassifyPatches.hpp
//  Color_Segmentation
//
//  Created by jollytrees on 11/9/15.
//  Copyright © 2015 jollytrees. All rights reserved.
//

#ifndef popupObjClassifyPatches_hpp
#define popupObjClassifyPatches_hpp

#include <stdio.h>
#include "popupObjAlgorithm.h"

class popupObjClassifyPatches : public popupObjAlgorithm
{
public:
    virtual bool execute(popupObject *obj);
    
};

#endif /* popupObjClassifyPatches_hpp */
