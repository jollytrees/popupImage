//
//  popupObjFindClassifiedPatchConnection.hpp
//  Color_Segmentation
//
//  Created by jollytrees on 11/9/15.
//  Copyright © 2015 jollytrees. All rights reserved.
//

#ifndef popupObjFindClassifiedPatchConnection_hpp
#define popupObjFindClassifiedPatchConnection_hpp

#include <stdio.h>

#include "popupObjAlgorithm.h"

class popupObjFindClassifiedPatchConnection : public popupObjAlgorithm
{
public:
    virtual bool execute(popupObject *obj);
    
};

#endif /* popupObjFindClassifiedPatchConnection_hpp */
