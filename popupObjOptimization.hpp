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
    //push
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

    GRBVar** ps = 0;
    GRBVar** pcl = 0;
    GRBVar** pccl = 0;
    GRBVar** pdl = 0;
    GRBVar** pdcl = 0;
    GRBVar** pddl = 0;
    GRBVar** pcr = 0;
    GRBVar** pccr = 0;
    GRBVar** pdr = 0;
    GRBVar** pdcr = 0;
    GRBVar** pddr = 0;

//    GRBVar** fs = 0;
//    GRBVar** fc = 0;
//    GRBVar** fd = 0;
//    GRBVar** fdd = 0;
//    GRBVar** fe = 0;

    GRBVar** on_same_patch = 0;
    GRBVar** on_same_patch_left = 0;
    GRBVar** on_same_patch_right = 0;

    GRBVar** pathIndicator = 0;

    const int MAX_STABILITY_DEPTH = 5;
};
#endif /* popupObjFindFoldibility_hpp */
