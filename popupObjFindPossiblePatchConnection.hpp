//
//  popupObjFindPossiblePatchConnection.hpp
//  popupImage
//
//  Created by jollytrees on 11/17/15.
//
//

#ifndef popupObjFindPossiblePatchConnection_hpp
#define popupObjFindPossiblePatchConnection_hpp

#include <stdio.h>

#include "popupObjAlgorithm.h"
#include "functionFindMatIntersection.h"
class popupObjFindPossiblePatchConnection : public popupObjAlgorithm
{
public:
    virtual bool execute(popupObject *obj);
    
};

#endif /* popupObjFindPossiblePatchConnection_hpp */
