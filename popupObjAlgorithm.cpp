//
//  popupObjAlgorithm.cpp
//  Color_Segmentation
//
//  Created by jollytrees on 11/8/15.
//  Copyright © 2015 jollytrees. All rights reserved.
//

#include <stdio.h>
#include "popupObjAlgorithm.h"


std::string popupObjAlgorithm::toString()
{
    return typeid(*this).name();
}

//execute all the process in the list
bool popupObjAlgorithmList::execute(popupObject *obj, std::string processName, bool useTimer)
{
    
    std::list< popupObjAlgorithm* >::iterator ait = this->begin();
    for( ; ait!= this->end(); ait++ )
    {
        std::cout << (*ait)->toString() << std::endl;
        
        if( !(*ait)->execute(obj) )
        {
            std::cout << (*ait)->toString() << " fails." << std::endl;
            break;
        }
    }

    
    
    /*if( !obj->isReady )
        return false;
    
    Utils_Timer timer;
    
    if(useTimer)
        timer.start( processName );
    
    //start the algorithm
    std::list< popupObjAlgorithm* >::iterator ait = this->begin();
    for( ; ait!= this->end(); ait++ )
    {
        if( useTimer )
            timer.start( (*ait)->toString() );
        
        if( !(*ait)->execute(obj) )
        {
            std::cout << (*ait)->toString() << " fails." << std::endl;
            break;
        }
        
        if( useTimer )
            timer.stop( (*ait)->toString() );
    }
    
    if(useTimer)
    {
        timer.stop( processName );
        std::cout << std::endl;
    }*/
    
    return true;
}

void popupObjAlgorithmList::release()
{
    std::list< popupObjAlgorithm* >::iterator ait = this->begin();
    for( ; ait!= this->end(); ait++ )
        delete *ait;
    this->clear();
}
