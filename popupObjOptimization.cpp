//
//  popupObjOptimization.cpp
//  popupImage
//
//  Created by jollytrees on 11/24/15.
//
//

#include "popupObjOptimization.hpp"

static void printPath(int s, int n, int tar,  popupObject *obj, string str, vector<int> root, vector<vector<vector<int> > >  &tempPath){
    
    if (n!= 1){
        n = n-1;
        for(size_t j=0; j< obj->neighborsOfOriginalPatch[s].size(); j++){
            ostringstream oss;

            int k = obj->neighborsOfOriginalPatch[s][j];
            bool isRoot = false;
            for(int i=0; i<root.size(); i++){
                //oss << root[i]<<" ";
                if (k==root[i]) {
                    isRoot = true;
                }
            }
            if(  isRoot ||  k==tar || k==obj->originalBackPatch || k==obj->originalFloorPatch) continue;
           
            oss   << k <<" " << n<<" " << tar<<"    ";
            vector<int> rootNext = root;
            rootNext.push_back(k);
            string strNext;
            strNext = str + oss.str();
            
            printPath(k,n, tar, obj, strNext, rootNext, tempPath);
        }
        //cout << endl;
    
    }else{
        cout << str << endl;
        //root.clear();
        std::stringstream ss;
        
        ss << str;
        int i,n,s;
        int li, ln, ls;
        int c =0;
        
        std::stringstream ss1;

        while(ss >> i >> n >> s ){
            if (c==0) {
                c++;
                li = i;
                ln = n;
                ls = s;
                ss1 <<" d["<<i <<"][" <<n<<"]["<< s << "]"  << " = ";

                continue;
            }
            ss1 <<" d["<<i <<"][" <<n<<"]["<< s << "]->";
            ss1 <<" ["<<li <<"][" <<ln<<"]"<< i << " " << tempPath[li][ln].size() << endl;
            vector<int> ::iterator it;
            it = find (tempPath[li][ln].begin(), tempPath[li][ln].end(), i);
            if (it == tempPath[li][ln].end())
                tempPath[li][ln].push_back(i);
            li = i;
            ln = n;
            ls = s;
            c++;

        }
        
        string str = ss1.str().substr(0, ss1.str().length()-2);
       // cout << str<< endl;

        return;
    }

}

static void findFoldlinePositionOfPatch(size_t &positionLineSize, popupObject *obj){

    int Idx = 0;
    for(size_t lineIdx=0; lineIdx< obj->foldLine.size(); lineIdx++){

        if(obj->foldLine[lineIdx]->isOriginalType){
            obj->positionLineIdxOfPatch[obj->foldLine[lineIdx]->connPatch[0]].push_back(make_pair(Idx, obj->foldLine[lineIdx]->line.first.x));
            obj->positionLineToFoldLine[Idx] = (int)lineIdx;
            obj->foldLineToPositionLine[lineIdx].push_back(Idx);
            Idx++;
            obj->positionLineIdxOfPatch[obj->foldLine[lineIdx]->connPatch[1]].push_back(make_pair(Idx, obj->foldLine[lineIdx]->line.first.x));
            obj->positionLineToFoldLine[Idx] = (int)lineIdx;
            obj->foldLineToPositionLine[lineIdx].push_back(Idx);

            Idx++;
        }else{
            obj->positionLineIdxOfPatch[obj->foldLine[lineIdx]->connPatch[0]].push_back(make_pair(Idx, obj->foldLine[lineIdx]->line.first.x));
            obj->positionLineIdxOfPatch[obj->foldLine[lineIdx]->connPatch[1]].push_back(make_pair(Idx, obj->foldLine[lineIdx]->line.first.x));
            obj->foldLineToPositionLine[lineIdx].push_back(Idx);
            obj->positionLineToFoldLine[Idx] = (int)lineIdx;
                Idx++;
        }
    }
    positionLineSize = Idx;
    
    for(size_t i=0; i< obj->positionLineIdxOfPatch.size(); i++){
        sort(obj->positionLineIdxOfPatch[i].begin(), obj->positionLineIdxOfPatch[i].end(), compareLongest);
        reverse(obj->positionLineIdxOfPatch[i].begin(), obj->positionLineIdxOfPatch[i].end());
    }
    
}

static bool isExist(vector<int> &neighbors, int p){
    std::vector<int>::iterator it;
    it = find (neighbors.begin(), neighbors.end(), p);
    if (it != neighbors.end()) return true;
    
    return false;
}

static void findNeighborsOfOriginalPatch(popupObject *obj){
    for(size_t lineIdx=0; lineIdx< obj->foldLine.size(); lineIdx++){
        if(obj->foldLine[lineIdx]->isOriginalType){
            int sPch = obj->foldLine[lineIdx]->originalConnPatch[0];
            int tPch = obj->foldLine[lineIdx]->originalConnPatch[1];
            if(!isExist(obj->neighborsOfOriginalPatch[sPch], tPch)) obj->neighborsOfOriginalPatch[sPch].push_back(tPch);
            if(!isExist(obj->neighborsOfOriginalPatch[tPch], sPch)) obj->neighborsOfOriginalPatch[tPch].push_back(sPch);
        }
    }
}

static void findNeighborsOPossiblefPatch(popupObject *obj){
    for(size_t lineIdx=0; lineIdx< obj->foldLine.size(); lineIdx++){
        int sPch = obj->foldLine[lineIdx]->connPatch[0];
        int tPch = obj->foldLine[lineIdx]->connPatch[1];
        obj->neighborsOfPossiblePatch[sPch].push_back(tPch);
        obj->neighborsOfPossiblePatch[tPch].push_back(sPch);
    }
}

void popupObjOptimization::initialize(popupObject *obj, GRBModel &model){
    
    patchSize = obj->possiblePatches.size();
    foldLineSize = obj->foldLine.size();
    originalPatchSize = obj->boundaryFoldLineConnGroupMap.size();
    distSize = originalPatchSize-3;
    
    //sort x postion for each Patch
    obj->positionLineIdxOfPatch.resize(obj->possiblePatches.size());
    obj->foldLineToPositionLine.resize(foldLineSize);

    findFoldlinePositionOfPatch(positionLineSize, obj);
    
    //neighbors of original patch
    obj->neighborsOfOriginalPatch.resize(obj->classifiedPatches.size());
    findNeighborsOfOriginalPatch(obj);
    
    //neighbor of possible patch
    obj->neighborsOfPossiblePatch.resize(obj->possiblePatches.size());
    findNeighborsOPossiblefPatch(obj);
    
    f.resize(foldLineSize);
    o.resize(patchSize);
    X.resize(positionLineSize);
    Y.resize(positionLineSize);
    Z.resize(foldLineSize);
    s.resize(patchSize);
    c.resize(foldLineSize);
    k.resize(foldLineSize);
    
    
//    d = new GRBVar**[originalPatchSize];
//    for (size_t w = 0; w < originalPatchSize; ++w){
//        model.update();
//        d[w] = new GRBVar*[originalPatchSize];
//        for( size_t i =0; i < originalPatchSize; i++){
//            d[w][i] = model.addVars((int)originalPatchSize);
//            model.update();
//            for (size_t s = 0; s < originalPatchSize; ++s){
//                d[w][i][s] = model.addVar(0.0, 1.0, 0.0, GRB_INTEGER);
//                model.update();
                

//            }
//        }
//    }

    d = new GRBVar*[originalPatchSize];
    for (size_t w = 0; w < originalPatchSize; ++w){
        model.update();
        d[w] = model.addVars((int)originalPatchSize);
        model.update();
        for (size_t n = 0; n < originalPatchSize; ++n){
            d[w][n] = model.addVar(0.0, 1.0, 0.0, GRB_INTEGER);
//                    model.update();
        }
    }

    dp = new GRBVar***[originalPatchSize];
    for (size_t w = 0; w < originalPatchSize; ++w){
        model.update();
        dp[w] = new GRBVar**[originalPatchSize];
        for( size_t i =0; i < originalPatchSize; i++){
            dp[w][i] = new GRBVar*[originalPatchSize];
            model.update();
            for (size_t s = 0; s < originalPatchSize; ++s){
                dp[w][i][s] = model.addVars((int)originalPatchSize);
                model.update();
                for (size_t k = 0; k < originalPatchSize; ++k){
                    dp[w][i][s][k] = model.addVar(0.0, 1.0, 0.0, GRB_INTEGER);
                }
                
            }
        }
    }

    
    pathIndicator= new GRBVar*[patchSize];
    for (size_t w = 0; w < obj->foldLinePathsOfPatch.size(); ++w){
        model.update();
        pathIndicator[w] = model.addVars((int)obj->foldLinePathsOfPatch[w].size());
        model.update();
        for (size_t s = 0; s < obj->foldLinePathsOfPatch[w].size(); ++s){
            pathIndicator[w][s] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY);
        }
    }

    
    fMap = new GRBVar*[patchSize];
    for (size_t w = 0; w < patchSize; ++w){
        model.update();
        fMap[w] = model.addVars((int)patchSize);
        model.update();
        for (size_t s = 0; s < patchSize; ++s){
            fMap[w][s] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY);
        }
    }
    
    cMap = new GRBVar*[patchSize];
    for (size_t w = 0; w < patchSize; ++w){
        model.update();
        cMap[w] = model.addVars((int)patchSize);
        model.update();
        for (size_t s = 0; s < patchSize; ++s){
            cMap[w][s] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY);
        }
    }
    
    oriFMap = new GRBVar*[originalPatchSize];
    for (size_t w = 0; w < originalPatchSize; ++w){
        oriFMap[w] = model.addVars((int)originalPatchSize);
        model.update();
        for (size_t s = 0; s < originalPatchSize; ++s){
            oriFMap[w][s] = model.addVar(0.0, 1.0, 0.0, GRB_INTEGER);
        }
    }
    
    b = new GRBVar*[patchSize];
    for (size_t w = 0; w < patchSize; ++w){
        b[w] = model.addVars((int)patchSize);
        model.update();
        for (size_t s = 0; s < patchSize; ++s){
            b[w][s] = model.addVar(0.0, 2.0, 0.0, GRB_INTEGER);
        }
    }
    
    a = new GRBVar*[patchSize];
    for (size_t w = 0; w < patchSize; ++w){
        a[w] = model.addVars((int)patchSize);
        model.update();
        for (size_t s = 0; s < patchSize; ++s){
            a[w][s] = model.addVar(0.0, 1.0, 0.0, GRB_INTEGER);
        }
    }
    
    for (size_t i=0;i<patchSize;i++) {
        o[i] = model.addVar(0.0, 1.0, 0, GRB_BINARY,"");
        s[i] = model.addVar(0.0, 1.0, 0, GRB_BINARY,"");
    }
    for (size_t i=0;i<foldLineSize;i++) {
        k[i] = model.addVar(-10, 10.0, 0, GRB_INTEGER);
        f[i] = model.addVar(0.0, 1.0, 0, GRB_INTEGER,"");
        c[i] = model.addVar(0.0, 1.0, 0, GRB_INTEGER,"");
    }
    
    for(size_t i=0; i< positionLineSize; i++){
        X[i] = model.addVar(0, obj->initMatSize.width, 0, GRB_INTEGER);
        Y[i] = model.addVar(0, obj->initMatSize.width, 0, GRB_INTEGER);
    }
    
    model.update();


    ps = new GRBVar*[patchSize];
    pcl = new GRBVar*[patchSize];
    pccl = new GRBVar*[patchSize];
    pdl = new GRBVar*[patchSize];
    pdcl = new GRBVar*[patchSize];
    pcr = new GRBVar*[patchSize];
    pccr = new GRBVar*[patchSize];
    pdr = new GRBVar*[patchSize];
    pdcr = new GRBVar*[patchSize];
    for (size_t p = 0; p < patchSize; ++p){
        model.update();
        ps[p] = model.addVars(MAX_STABILITY_DEPTH);
        pcl[p] = model.addVars(MAX_STABILITY_DEPTH);
        pccl[p] = model.addVars(MAX_STABILITY_DEPTH);
        pdl[p] = model.addVars(MAX_STABILITY_DEPTH);
        pdcl[p] = model.addVars(MAX_STABILITY_DEPTH);
        pcr[p] = model.addVars(MAX_STABILITY_DEPTH);
        pccr[p] = model.addVars(MAX_STABILITY_DEPTH);
        pdr[p] = model.addVars(MAX_STABILITY_DEPTH);
        pdcr[p] = model.addVars(MAX_STABILITY_DEPTH);
        model.update();
        for (size_t d = 0; d < MAX_STABILITY_DEPTH; ++d){
            ps[p][d] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY);
            pcl[p][d] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY);
            pccl[p][d] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY);
            pdl[p][d] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY);
            pdcl[p][d] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY);
            pcr[p][d] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY);
            pccr[p][d] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY);
            pdr[p][d] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY);
            pdcr[p][d] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY);
        }
    }

 /*   on_same_patch = new GRBVar*[patchSize];
    for (size_t i = 0; i < patchSize; ++i){
        model.update();
        on_same_patch[i] = model.addVars(patchSize);
        for (size_t j = 0; j < foldLineSize; ++j)
            on_same_patch[i][j] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY);
    }

    on_same_patch_left = new GRBVar*[patchSize];
    for (size_t i = 0; i < patchSize; ++i){
        model.update();
        on_same_patch_left[i] = model.addVars(patchSize);
        for (size_t j = 0; j < foldLineSize; ++j)
            on_same_patch_left[i][j] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY);
    }

    on_same_patch_right = new GRBVar*[patchSize];
    for (size_t i = 0; i < patchSize; ++i){
        model.update();
        on_same_patch_right[i] = model.addVars(patchSize);
        for (size_t j = 0; j < foldLineSize; ++j)
            on_same_patch_right[i][j] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY);
    }*/
}

void popupObjOptimization::foldability(popupObject *obj, GRBModel &model){

    //set c : cut indicator
    for(size_t lineIdx =0; lineIdx < obj->foldLine.size(); lineIdx++){
        if(!obj->foldLine[lineIdx]->isOriginalType)
            model.addQConstr( c[lineIdx] == 0 );
        else
            model.addQConstr( c[lineIdx] == 1- f[lineIdx]);
    }
    
//    for(size_t lineIdx =0; lineIdx < obj->foldLine.size(); lineIdx++){
//        if(!obj->foldLine[lineIdx]->isOriginalFoldLine){
//            int idx1 = obj->foldLine[lineIdx]->connOriFoldLine[0];
//            int idx2 = obj->foldLine[lineIdx]->connOriFoldLine[1];
//            model.addQConstr( f[lineIdx] <= f[idx1] + f[idx2] );
//            model.addQConstr( f[lineIdx]*f[idx1] == f[lineIdx]*f[idx2] );
//        }
//    }
    
    //set orientation of floor and black patch
    model.addQConstr( o[obj->floorPatch]==0 );
    model.addQConstr( o[obj->backPatch]==1 );
    
    //set (o(pi) + i(f))(1 − c(f)) = (2k + o(pj))(1 − c(f))
    for(size_t lineIdx =0; lineIdx < obj->foldLine.size(); lineIdx++){
        int pchIdx1 = obj->foldLine[lineIdx]->connPatch[0];
        int pchIdx2 = obj->foldLine[lineIdx]->connPatch[1];
        model.addQConstr( (o[pchIdx1] + f[lineIdx])*(1-c[lineIdx]) == ( 2*k[lineIdx] + o[pchIdx2])*(1-c[lineIdx]) );
    }
    
    for(size_t i=0; i< obj->positionLineIdxOfPatch.size(); i++){
        for(int j=0; j< (int)obj->positionLineIdxOfPatch[i].size()-1; j++){
            int fs = obj->positionLineIdxOfPatch[i][j].first;
            int ft = obj->positionLineIdxOfPatch[i][j+1].first;
            
            model.addQConstr( X[fs]*o[i] ==  X[ft]*o[i]);
            model.addQConstr( Y[fs]*(1-o[i]) ==  Y[ft]*(1-o[i]));
            model.addQConstr( X[fs]*(1-o[i]) <=  X[ft]*(1-o[i]));
            model.addQConstr( Y[fs]*o[i] <=  Y[ft]*o[i]);
            
            if((int)i==obj->backPatch){
                model.addQConstr( X[fs] == obj->centralXPosition );
                model.addQConstr( X[ft] == obj->centralXPosition );
            }
            if((int)i==obj->floorPatch){
                model.addQConstr( Y[fs] == 0);
                model.addQConstr( Y[ft] == 0 );
            }
        }
    }

    for(size_t fIdx=0; fIdx< obj->foldLine.size(); fIdx++){
        if(obj->foldLine[fIdx]->isOriginalType){
            int fs = obj->foldLineToPositionLine[fIdx][0];
            int ft = obj->foldLineToPositionLine[fIdx][1];
            model.addQConstr( X[fs]*(1-c[fIdx]) ==  X[ft]*(1-c[fIdx]));
            model.addQConstr( Y[fs]*(1-c[fIdx]) ==  Y[ft]*(1-c[fIdx]));
            model.addQConstr( X[fs] <= obj->centralXPosition );
            model.addQConstr( X[ft] <= obj->centralXPosition );
            model.addQConstr( Y[fs] >= 0 );
            model.addQConstr( Y[ft] >= 0 );
        } else {
            int fs = obj->foldLineToPositionLine[fIdx][0];
            model.addQConstr( X[fs] <= obj->centralXPosition );
            model.addQConstr( Y[fs] >= 0 );
        }
    }
}

//void popupObjOptimization::stability(popupObject *obj, GRBModel &model){
    
//    //set fMap
//    for (size_t i = 0; i < patchSize; i++){
//        for (size_t j = 0; j < patchSize; j++){
//            if(obj->possibleFoldLineConnMap[i][j]!=NULL){
//                int idx = obj->possibleFoldLineConnMap[i][j]->foldLineIdx;
//                model.addQConstr( fMap[i][j]==f[idx]);
//            }else
//                model.addQConstr( fMap[i][j]==0);
//        }
//    }
    
//    //set cMap
//    for (size_t i = 0; i < patchSize; i++){
//        for (size_t j = 0; j < patchSize; j++){
//            if(obj->possibleFoldLineConnMap[i][j]!=NULL){
//                int idx = obj->possibleFoldLineConnMap[i][j]->foldLineIdx;
//                if(!obj->foldLine[idx]->isOriginalFoldLine)
//                    model.addQConstr( cMap[i][j] == 0 );
//                else
//                    model.addQConstr( cMap[i][j] == 1- f[idx]);
//            }else
//                model.addQConstr( cMap[i][j]==0);
//        }
//    }
    
    
//    //set a
//    for (size_t i = 0; i < patchSize; i++)
//        for (size_t j = 0; j < patchSize; j++)
//            model.addQConstr( a[i][j]==fMap[i][j]);
    
//    //set constraint
//    for (size_t i = 0; i < patchSize; i++){
//        for (size_t k = 0; k < patchSize; k++){
            
//            GRBQuadExpr QRhs1 = *new GRBQuadExpr();
//            GRBQuadExpr QRhs2 = *new GRBQuadExpr();
//            QRhs1= QRhs2 = 0;
//            for(size_t n = 0; n < obj->neighborsOfPossiblePatch[k].size(); n++){
//                int j = obj->neighborsOfPossiblePatch[k][n];
//                QRhs1 += a[i][j]*fMap[j][k];
//               // QRhs2 += a[i][j]*s[j] + b[i][j]*s[j]*0.5;
//            }
//            if(i!=k){
//                model.addQConstr( b[i][k] <= QRhs1);
//            }else{
//                model.addQConstr( b[i][k] == 0);
//            }
//           // model.addQConstr( 1.5*s[i] <= QRhs2);
    
//        }
//    }
    
//   /*
//    for(int i=0; i<patchSize; i++){
//        for(int j=0; j<patchSize; j++){
//            //1,2,3
//            //model.addQConstr( s[i]*(1-fMap[i][j]-cMap[i][j]) == s[j]*(1-fMap[i][j]-cMap[i][j]));
//            for(int k = 0; k<patchSize; k++){
//                //model.addQConstr( a[i][k]*( 1-fMap[i][j] ) == a[j][k]*( 1-fMap[i][j]) );
//                //model.addQConstr( b[i][k]*( 1-fMap[i][j]-cMap[i][j] ) == b[j][k]*( 1-fMap[i][j]-cMap[i][j]) );
//            }
//        }
//    }
//    */
//}

void popupObjOptimization::stability(popupObject *obj, GRBModel &model){
//    for (size_t i = 0; i < patchSize; i++){
//        for (size_t j = 0; j < patchSize; j++){
//            GRBQuadExpr sum_left = *new GRBQuadExpr();
//            GRBQuadExpr sum_right = *new GRBQuadExpr();
//            sum_left = sum_right = 0;
//            for(size_t n = 0; n < obj->neighborsOfPossiblePatch[i].size(); n++){
//                int k = obj->neighborsOfPossiblePatch[i][n];
//                if (obj->isLeftOrRightNeighbor(j, k) == 0) {
//                    sum_left += on_same_patch_left[i][k];
//                } else if (obj->isLeftOrRightNeighbor(j, k) == 1) {
//                    sum_right += on_same_patch_right[i][k];
//                }
//            }
//            model.addQConstr(on_same_patch_left[i][j] <= sum_left);
//            model.addQConstr(on_same_patch_right[i][j] <= sum_right);
//            model.addQConstr(on_same_patch[i][j] <= on_same_patch_left[i][j] + on_same_patch_right[i][j]);
//        }
//    //set constraint
//    for (size_t i = 0; i < patchSize; i++){
//        for (size_t d = 0; d < MAX_STABILITY_DEPTH - 1; d++){

//            GRBQuadExpr QRhs_pcl = *new GRBQuadExpr();
//            GRBQuadExpr QRhs_pccl = *new GRBQuadExpr();
//            GRBQuadExpr QRhs_pdl = *new GRBQuadExpr();
//            GRBQuadExpr QRhs_pdcl = *new GRBQuadExpr();
//            GRBQuadExpr QRhs_pcr = *new GRBQuadExpr();
//            GRBQuadExpr QRhs_pccr = *new GRBQuadExpr();
//            GRBQuadExpr QRhs_fdr = *new GRBQuadExpr();
//            GRBQuadExpr QRhs_fdcr = *new GRBQuadExpr();
////            GRBQuadExpr QRhs_fddl = *new GRBQuadExpr();
//            QRhs_psl = QRhs_pcl = QRhs_pccl = QRhs_pd = QRhs_pdcl = 0;
//            for(size_t n = 0; n < obj->neighborsOfPossiblePatch[i].size(); n++){
//                int j = obj->neighborsOfPossiblePatch[i][n];
//                if (obj->isLeftOrRightNeighbor(i, j) == 0) {
//                    QRhs_pcl += ps[j][d]*fMap[i][j];
//                    QRhs_pccl += pcl[j][d]*fMap[i][j];
//                    QRhs_pdcl += pdl[j][d]*fMap[i][j];
//                } else if (obj->isLeftOrRightNeighbor(i, j) == 1) {
//                    QRhs_pcr += ps[j][d]*fMap[i][j];
//                    QRhs_pccr += pcr[j][d]*fMap[i][j];
//                    QRhs_pdcr += pdr[j][d]*fMap[i][j];
//                }
//            }

//            for(size_t n = 0; n < obj->patchesOnSameOriginalPatch[i].size(); n++){
//                int j = obj->patchesOnSameOriginalPatch[i][n];

//                if (obj->isLeftOrRightNeighbor(i, j) == 0) {
//                    QRhs_pcl += pcl[j][d]*on_same_patch[i][j];
//                    QRhs_pccl += pccl[j][d]*on_same_patch[i][j];
//                    QRhs_pdcl += pdcl[j][d]*on_same_patch[i][j];
//                } else if (obj->isLeftOrRightNeighbor(i, j) == 1) {
//                    QRhs_pcr += pcr[j][d]*on_same_patch[i][j];
//                    QRhs_pccr += pccr[j][d]*on_same_patch[i][j];
//                    QRhs_pdcr += pdcr[j][d]*on_same_patch[i][j];
//                }
//                QRhs_pdl += pccl[j][d]*on_same_patch[i][j];
//                QRhs_pdr += pccr[j][d]*on_same_patch[i][j];
//            }
//            model.addQConstr( pcl[i][d] <= QRhs_pcl);
//            model.addQConstr( pccl[i][d] <= QRhs_pccl);
//            model.addQConstr( 2 * pdl[i][d] <= QRhs_pdl);
//            model.addQConstr( pdcl[i][d] <= QRhs_pdcl);
//            model.addQConstr( pcr[i][d] <= QRhs_pcr);
//            model.addQConstr( pccr[i][d] <= QRhs_pccr);
//            model.addQConstr( 2 * pdr[i][d] <= QRhs_pdr);
//            model.addQConstr( pdcr[i][d] <= QRhs_pdcr);
//        }
//    }
//    for (size_t i = 0; i < patchSize; i++){
//        for (size_t d = 1; d < MAX_STABILITY_DEPTH; d++){
//            GRBQuadExpr QRhs_ps = *new GRBQuadExpr();
//            for (size_t smaller_d = 0; smaller_d < d; smaller_d++){
//                QRhs_ps += pcl[i][d - 1] * pcr[i][smaller_d] + pcl[i][smaller_d] * pcr[i][d - 1];

//                QRhs_ps += pdl[i][d - 1] * pdr[i][smaller_d] + pdl[i][smaller_d] * pdr[i][d - 1];

//                QRhs_ps += pdcl[i][d - 1] * pdcr[i][smaller_d] + pdcl[i][smaller_d] * pdcr[i][d - 1];

//                QRhs_ps += pcl[i][d - 1] * pccr[i][smaller_d] + pcl[i][smaller_d] * pccr[i][d - 1]
//                        + pccl[i][d - 1] * pcr[i][smaller_d] + pccl[i][smaller_d] * pcr[i][d - 1];
//                QRhs_ps += pcl[i][d - 1] * pdr[i][smaller_d] + pcl[i][smaller_d] * pdr[i][d - 1]
//                        + pdl[i][d - 1] * pcr[i][smaller_d] + pdl[i][smaller_d] * pcr[i][d - 1];
//                QRhs_ps += pcl[i][d - 1] * pdcr[i][smaller_d] + pcl[i][smaller_d] * pdcr[i][d - 1]
//                        + pdcl[i][d - 1] * pcr[i][smaller_d] + pdcl[i][smaller_d] * pcr[i][d - 1];

//                QRhs_ps += pccl[i][d - 1] * pdr[i][smaller_d] + pccl[i][smaller_d] * pdr[i][d - 1]
//                        + pdl[i][d - 1] * pccr[i][smaller_d] + pdl[i][smaller_d] * pccr[i][d - 1];

//                QRhs_ps += pdl[i][d - 1] * pdcr[i][smaller_d] + pdl[i][smaller_d] * pdcr[i][d - 1]
//                        + pdcl[i][d - 1] * pdr[i][smaller_d] + pdcl[i][smaller_d] * pdr[i][d - 1];
//            }
//            model.addQConstr( ps[i][d] <= QRhs_ps );
//        }
//    }
//    model.addQConstr( s[obj->floorPatch][0] == 1 );
//    model.addQConstr( s[obj->backPatch][0] == 1 );

//    for (size_t i = 0; i < patchSize; i++){
//        GRBQuadExpr sum = *new GRBQuadExpr(0);
//        sum = 0;
//        for (size_t d = 0; d < MAX_STABILITY_DEPTH; d++){
//            sum += ps[i][d];
//        }
//        model.addQConstr(sum == 1);
//    }

//   /*
//    for(int i=0; i<patchSize; i++){
//        for(int j=0; j<patchSize; j++){
//            //1,2,3
//            //model.addQConstr( s[i]*(1-fMap[i][j]-cMap[i][j]) == s[j]*(1-fMap[i][j]-cMap[i][j]));
//            for(int k = 0; k<patchSize; k++){
//                //model.addQConstr( a[i][k]*( 1-fMap[i][j] ) == a[j][k]*( 1-fMap[i][j]) );
//                //model.addQConstr( b[i][k]*( 1-fMap[i][j]-cMap[i][j] ) == b[j][k]*( 1-fMap[i][j]-cMap[i][j]) );
//            }
//        }
//    }
//    */
}

void popupObjOptimization::connectivity(popupObject *obj, GRBModel &model){
    
    //set oriFMap
    for (size_t i = 0; i < originalPatchSize; ++i){
        for (size_t j = 0; j < originalPatchSize; ++j){
            if(obj->originalFoldLineConnMap[i][j].size()>0){
                GRBQuadExpr QRhs = *new GRBQuadExpr(0);
                for(size_t k=0; k < obj->originalFoldLineConnMap[i][j].size(); k++){
                    int idx = obj->originalFoldLineConnMap[i][j][k]->foldLineIdx;
                    QRhs += f[idx];
                }
                model.addQConstr( oriFMap[i][j]<=QRhs);
            }else
                model.addQConstr( oriFMap[i][j]==0);
        }
    }

    int starting_patch = -1;
    for(size_t s=0; s<originalPatchSize; s++){
        if (!obj->isOriginalBasePatch((int)s)){
            starting_patch = s;
            break;
        }
    }
    //set d
    for(size_t n = distSize; n>=1; n--){
//        for(size_t s=0; s<originalPatchSize; s++){
            for(size_t i=0; i< originalPatchSize; i++){
           
                if(i==starting_patch || obj->isOriginalBasePatch((int)i)) continue;
                if(n==1)
                    model.addQConstr( d[i][n] == oriFMap[i][starting_patch] );
                else{
                    GRBQuadExpr QRhs = *new GRBQuadExpr(0);
                    for(size_t k =0; k< obj->neighborsOfOriginalPatch[i].size(); k++){
                        int j = obj->neighborsOfOriginalPatch[i][k];
                        if(j!=(int)starting_patch && !obj->isOriginalBasePatch((int)j))
                            QRhs += d[j][n-1]*oriFMap[i][j] ;
                    }
                    model.addQConstr( d[i][n] <= QRhs );
                }
            }
    }

    //For global connectivity, every patch should have a path to ps
        for(size_t i=0; i< originalPatchSize; i++){
            if(i==starting_patch || obj->isOriginalBasePatch((int)i)) continue;
            GRBQuadExpr sum;
            for(size_t n=1; n<distSize; n++){
                sum += d[i][n];
            }
            model.addQConstr( sum == 1 );
        }
    
}

bool popupObjOptimization::execute(popupObject *obj)
{
    GRBEnv env = GRBEnv();
    GRBModel model = GRBModel(env);

    initialize(obj, model);

    model.update();
    model.set(GRB_StringAttr_ModelName,"fold line assignment");
    model.set(GRB_IntAttr_ModelSense, GRB_MINIMIZE);
    /*model.getEnv().set(GRB_DoubleParam_MIPGap, 0.005);
    model.getEnv().set(GRB_DoubleParam_NodeLimit, 1000)*/;
    model.getEnv().set(GRB_DoubleParam_TimeLimit, 300);

    //set objective function
    GRBQuadExpr GRBobj = 0;
    for (size_t j = 0; j < foldLineSize; j++){
        //length cost
        if(!obj->foldLine[j]->isOriginalType){
            double dist = sqrt(pow((double)obj->foldLine[j]->line.first.y-obj->foldLine[j]->line.second.y,2.0));
            GRBobj += (obj->initMatSize.height - dist)/(obj->initMatSize.height)*f[j];
        }
    }

    //position cost
    for(size_t i=0; i< obj->positionLineIdxOfPatch.size(); i++){
        for(size_t j=0; j< obj->positionLineIdxOfPatch[i].size(); j++){
            int fIdx = obj->positionLineToFoldLine[obj->positionLineIdxOfPatch[i][j].first];
            int oriX = obj->foldLine[fIdx]->line.first.x;
            int pIdx = obj->positionLineIdxOfPatch[i][j].first;
            GRBobj += (X[pIdx]+Y[pIdx]-oriX)*(X[pIdx]+Y[pIdx]-oriX);
        }
    }
    model.setObjective(GRBobj);

    //set constraint
    foldability(obj, model);
    /*stability(obj, model);*/
    connectivity(obj, model);
    
    for(size_t lineIdx=0; lineIdx< obj->foldLine.size(); lineIdx++){
        if(obj->foldLine[lineIdx]->isCuttedLine){
            cout << lineIdx << " " << endl;
            model.addQConstr( f[lineIdx] == 0 );
        }
    }

    try {
        model.update();
        model.optimize();

        //print & set result
        for (size_t i=0;i<foldLineSize;i++) {
            cout << " f[" << i << "] = " << (double)f[i].get(GRB_DoubleAttr_X) <<endl;
            obj->activeFoldLine.push_back((double)f[i].get(GRB_DoubleAttr_X));
        }
        for (size_t i=0;i<patchSize;i++) {
            cout << " o[" << i << "] = " << (double) o[i].get(GRB_DoubleAttr_X) <<endl;
            obj->orientation.push_back((double)o[i].get(GRB_DoubleAttr_X));
        }
        /*for (size_t i=0;i<foldLineSize;i++)
            cout << " c[" << i << "] = " << c[i].get(GRB_DoubleAttr_X) <<endl;*/
        
        obj->X.resize(positionLineSize);
        obj->Y.resize(positionLineSize);
        for(size_t i=0; i< obj->positionLineIdxOfPatch.size(); i++){
            for(size_t j=0; j< obj->positionLineIdxOfPatch[i].size(); j++){
                int fIdx = obj->positionLineToFoldLine[obj->positionLineIdxOfPatch[i][j].first];
                int oriX = obj->foldLine[fIdx]->line.first.x;
                int pIdx = obj->positionLineIdxOfPatch[i][j].first;
                cout << " X[" << pIdx << "] = " << (double)X[pIdx].get(GRB_DoubleAttr_X)<<",";
                cout << " Y[" << pIdx << "] = " << (double)Y[pIdx].get(GRB_DoubleAttr_X)<<", ";
                cout << "X+Y = "<<(double)X[pIdx].get(GRB_DoubleAttr_X)+ (double)Y[pIdx].get(GRB_DoubleAttr_X)<<", Original X = "<<oriX << endl;
                obj->X[pIdx] = (double)X[pIdx].get(GRB_DoubleAttr_X);
                obj->Y[pIdx] = (double)Y[pIdx].get(GRB_DoubleAttr_X);
                obj->oriX.push_back((double)obj->foldLine[fIdx]->line.first.x);
            }
        }
 
    if (model.get(GRB_IntAttr_IsMIP) == 0)
        throw GRBException("Model is not a MIP");
    
    }catch(GRBException e) {
        cout << "Error code = " << e.getErrorCode() << endl;
        cout << e.getMessage() << endl;
    }catch (...) {printf("Exception...\n");exit(1);}
    return true;
}

