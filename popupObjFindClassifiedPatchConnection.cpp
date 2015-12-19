//
//  popupObjFindClassifiedPatchConnection.cpp
//  Color_Segmentation
//
//  Created by jollytrees on 11/9/15.
//  Copyright © 2015 jollytrees. All rights reserved.
//

#include "popupObjFindClassifiedPatchConnection.hpp"
#include "functionFindConnection.h"

bool popupObjFindClassifiedPatchConnection::execute(popupObject *obj)
{
    findConnection(obj->classifiedPatches, obj->classifiedConnMap,
                   obj->classifiedConnSize, obj->classifiedPatchIntersection);
    return true;
}