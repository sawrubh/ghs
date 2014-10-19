#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <pthread.h>
#include "node.cpp"

using namespace std;

void *run_thread(void *arg) {
  Node *node;
  node = (Node *)arg;
  while(run)
    node->readMessage();
  return NULL;
}

int main(void){
  int i,j,k,rc,numNodes,w,*weights;
  FILE *fp;
  fp = fopen("input.txt","r");
  fscanf(fp,"%d\n",&numNodes);
  pthread_t *threads;
  Node *nodes;
  Node *nbrs[numNodes];
  threads = (pthread_t *)malloc(numNodes*sizeof(pthread_t));
  nodes = new Node[numNodes];
  weights = (int *)malloc(numNodes*sizeof(int));
  for(i=0;i<numNodes;i++){
    k=0;
    for(j=0;j<numNodes;j++){
      fscanf(fp,"%d\t",&w);
      if(w!=0){
        weights[k]=w;
        nbrs[k]=&nodes[j];
        k++;

        if(j<i){
          edgeInput e;
          e.left = j;
          e.right= i;
          allEdges.insert(pair<int,edgeInput>(w,e));
        }
      }
    }
    fscanf(fp,"\n");
    nodes[i].init(k,weights,nbrs);
    nodes[i].ind=i;
  }
  fclose(fp);
  run = 1;
  for (i=0;i<numNodes;i++){
    rc = pthread_create(&threads[i],NULL,run_thread,(void *) &nodes[i]);
  }
  nodes[0].wakeup();

  pthread_exit(NULL);
  free(nodes);
  free(threads);
  free(weights);
  free(nbrs);
  return 0;
}
