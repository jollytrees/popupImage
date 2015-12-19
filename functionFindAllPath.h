//
//  functionFindAllPath.h
//  popupImage
//
//  Created by jollytrees on 12/14/15.
//
//

#ifndef functionFindAllPath_h
#define functionFindAllPath_h
#include <stdio.h>
#include <vector>
#include <algorithm>
#include <vector>
#include <queue>
#include <iostream>

using namespace std;

static vector<vector<int> >GRAPH(100);
static vector<vector<int> >PATHS;
static int num = 0;


static inline void print_path(vector<int>path)
{
    vector<int> tempPath;
    // cout<<"[ ";
    for(int i=0;i<path.size();++i)
    {
        //cout<<path[i]<<" ";
        tempPath.push_back(path[i]);
    }
    
    //cout<<"]"<<endl;
    //cout << endl;
    PATHS.push_back(tempPath);
}
static bool isadjacency_node_not_present_in_current_path(int node,vector<int>path)
{
    for(int i=0;i<path.size();++i)
    {
        if(path[i]==node)
            return false;
    }
    return true;
}
static int findpaths(int source ,int target ,int totalnode,int totaledge )
{
    
    vector<int>path;
    path.push_back(source);
    queue<vector<int> >q;
    q.push(path);
    
    while(!q.empty())
    {
        path=q.front();
        q.pop();
        
        int last_nodeof_path=path[path.size()-1];
        if(last_nodeof_path==target)
        {
            
            //cout << num << " ";
            num++;
            print_path(path);
        }
        else
        {
            //print_path(path);
        }
        
        for(int i=0;i<GRAPH[last_nodeof_path].size();++i)
        {
            if(isadjacency_node_not_present_in_current_path(GRAPH[last_nodeof_path][i],path))
            {
                
                vector<int>new_path(path.begin(),path.end());
                new_path.push_back(GRAPH[last_nodeof_path][i]);
                q.push(new_path);
            }
        }
        
        
        
        
    }
    return 1;
}

static bool isExist(int u, int v){
    vector<int>::iterator it;
    it = find(GRAPH[u].begin(), GRAPH[u].end(), v);
    if(it!=GRAPH[u].end()) return true;
    else return false;
}

static void clearGraph(){
    for(int i=0; i< GRAPH.size(); i++)
        GRAPH[i].clear();
    
}

static void createSingleEdgeM(int u, int v){
    if(!isExist(u,v) ){
        GRAPH[u].push_back(v);
    }
}
static void createEdgeM(int u, int v){
    if(!isExist(u,v) ){
        GRAPH[u].push_back(v);
    }
    if(!isExist(v,u) ) {
        GRAPH[v].push_back(u);
    }
}

static void findAllPaths(vector<vector<int> > &paths, int source, int target, int N, int M){
    
    findpaths(source, target, N, M);
    paths = PATHS;
    
}


#endif /* functionFindAllPath_h */
