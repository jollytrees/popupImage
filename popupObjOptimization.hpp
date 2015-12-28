//
//  popupObjOptimization.hpp
//  popupImage
//
//  Created by jollytrees on 11/24/15.
//
//

#ifndef popupObjOptimization_hpp
#define popupObjOptimization_hpp

#include <stdio.h>
#include "popupObjAlgorithm.h"
#include "gurobi_c++.h"

class popupObjOptimization : public popupObjAlgorithm
{
public:
    virtual bool execute(popupObject *obj);
    
    void initialize(popupObject *obj, GRBModel &model);
    void foldability(popupObject *obj, GRBModel &model);
    void stability(popupObject *obj, GRBModel &model);
    void connectivity(popupObject *obj, GRBModel &model);

    size_t patchSize;
    size_t foldLineSize;
    size_t originalPatchSize;
    size_t distSize;
    size_t positionLineSize;

    vector<GRBVar> f,o,X,Y,Z,s,c,k;
    
    GRBVar** b = 0;
    GRBVar** a = 0;
    GRBVar** fMap= 0;
    GRBVar** cMap= 0;
    GRBVar** oriFMap= 0;
    GRBVar** d= 0;
    GRBVar**** dp= 0;

    
    GRBVar** pathIndicator = 0;


};
#endif /* popupObjFindFoldibility_hpp */
