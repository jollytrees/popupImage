//
//  popupObjFindConnection.cpp
//  Color_Segmentation
//
//  Created by jollytrees on 11/8/15.
//  Copyright © 2015 jollytrees. All rights reserved.
//

#include "popupObjFindDividedPatchConnection.hpp"
#include "functionFindConnection.h"

bool popupObjFindDividedPatchConnection::execute(popupObject *obj)
{
    findConnection(obj->dividedPatches, obj->dividedConnMap,
                   obj->dividedConnSize, obj->dividedPatchIntersection);
    return true;
}
