//
//  popupObjAlgorithm.h
//  Color_Segmentation
//
//  Created by jollytrees on 11/8/15.
//  Copyright © 2015 jollytrees. All rights reserved.
//

#ifndef popupObjAlgorithm_h
#define popupObjAlgorithm_h

#include "popupObject.h"

#include <list>


//interface of algorithms
class popupObjAlgorithm
{
protected:
    //constructor
    popupObjAlgorithm(){}
    
public:
    //destructor
    virtual ~popupObjAlgorithm(){}
    
    //execute the process
    virtual bool execute(popupObject *obj) = 0;
    
    //the description of the process
    virtual std::string toString();
};

class popupObjAlgorithmList : public std::list<popupObjAlgorithm*>
{
public:
    //execute the process
    bool execute(popupObject *obj, std::string processName, bool useTimer);
    
    //release
    void release();
};


#endif /* popupObjAlgorithm_h */
