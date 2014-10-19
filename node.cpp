#include <pthread.h>
#include "node.h"

#define INF 1000000

void Node::init(int numEdges, int weights[], Node* nodes[]) {
  this->numEdges = numEdges;
  this->edges = (edge *)malloc(sizeof(edge) * numEdges);
  for (int i = 0; i < numEdges; ++i) {
    this->edges[i].state = 0;
    this->edges[i].weight = weights[i];
    this->edges[i].node = nodes[i];
  }
  this->state = 0;
  this->bestEdge = -1;
  this->bestWeight = INF;
  this->testEdge = -1;
  this->parent = -1;
  this->level = -1;
  this->findCount = -1;
  this->id = INF;
  this->mutex = PTHREAD_MUTEX_INITIALIZER;
}

Node::Node() {}

Node::~Node() {
  delete(this->edges);
}

void Node::addMessage(message msg) {
  pthread_mutex_lock(&(this->mutex));
  this->messages.push(msg);
  pthread_mutex_unlock(&(this->mutex));
}

void Node::readMessage() {
  if (!this->messages.empty()) {
    pthread_mutex_lock(&(this->mutex));
    message msg = this->messages.front();
    this->messages.pop();
    int i;
    for (i = 0; i < this->numEdges; ++i) {
      if (this->edges[i].weight == msg.weight)
        break;
    }
    pthread_mutex_unlock(&(this->mutex));
    if(this->state == 0)
      this->wakeup();

    switch(msg.id) {
      case 0:
        connect(msg.args[0], i);
        break;
      case 1:
        initiate(msg.args[0], msg.args[1], msg.args[2], i);
        break;
      case 2:
        testMessage(msg.args[0], msg.args[1], i);
        break;
      case 3:
        accept(i);
        break;
      case 4:
        reject(i);
        break;
      case 5:
        reportMessage(msg.args[0], i);
        break;
      case 6:
        changeRootMessage(i);
        break;
      case 7:
        this->wakeup();
    }
  }
}

int Node::findMinEdge() {
  int min = INF, index = 0;
  for (int i = 0; i < numEdges; i++) {
    if (this->edges[i].weight < min) {
      min = this->edges[i].weight;
      index = i;
    }
  }
  return index;
}

void Node::wakeup() {
  int minEdge = findMinEdge();
  // Branch state
  this->edges[minEdge].state = 1;
  pthread_mutex_lock(&(mst_mutex));
  mst.insert(pair<int,int>(this->edges[minEdge].weight,1));
  pthread_mutex_unlock(&(mst_mutex));
  this->level = 0;
  // Found State
  this->state = 2;
  this->findCount = 0;
  message msg;
  msg.id = 0;
  msg.args[0] = 0;
  msg.weight = this->edges[minEdge].weight;
  this->edges[minEdge].node->addMessage(msg);
}

void Node::connect(int L, int j) {
  if (L < this->level) {
    // Branch state
    this->edges[j].state = 1;
    message msg;
    msg.id = 1;
    msg.args[0] = this->level;
    msg.args[1] = this->id;
    msg.args[2] = this->state;
    msg.weight = this->edges[j].weight;
    this->edges[j].node->addMessage(msg);
  }
  else if (this->edges[j].state == 0) {
    message msg;
    msg.id = 0; // Connect Message
    msg.args[0] = L;
    msg.weight = this->edges[j].weight;
    this->addMessage(msg);
  }
  else {
    message msg;
    msg.id = 1; // Initiate Message
    msg.args[0] = this->level + 1;
    msg.args[1] = this->edges[j].weight;
    msg.args[2] = 1; // Find State
    msg.weight = this->edges[j].weight;
    this->edges[j].node->addMessage(msg);
  }
}

void Node::initiate(int L, int id, int stateOfNode, int j) {
  this->level = L; // LN <- L
  this->id = id; // FN <- F
  this->state = stateOfNode; // SN <- S
  this->parent = j; // in-branch <- j
  this->bestEdge = -1; // best-edge <- nil
  this->bestWeight = INF; // best-wt <- infinity
  for (int i = 0; i < numEdges; ++i) {
    if (i != j && this->edges[i].state == 1) {
      message msg;
      msg.id = 1; // Initiate Message
      msg.args[0] = L;
      msg.args[1] = id;
      msg.args[2] = stateOfNode; // Find State
      msg.weight = this->edges[i].weight;
      this->edges[i].node->addMessage(msg);
    }
  }
  if (stateOfNode == 1) {
    this->findCount = 0;
    test();
  }
}

void Node::test() {
  int i, min, min_ind;
  min = INF;
  min_ind = -1;
  for (i = 0; i < numEdges; i++) {
    if (this->edges[i].state == 0) {
      if(min>this->edges[i].weight){
        min = this->edges[i].weight;
        min_ind = i;
      }
    }
  }
  if(min_ind>=0){
    this->testEdge = min_ind;
    message msg;
    msg.id = 2; // Test Message
    msg.args[0] = this->level;
    msg.args[1] = this->id;
    msg.weight = this->edges[min_ind].weight;
    this->edges[min_ind].node->addMessage(msg);
  }
  else{
    this->testEdge = -1;
    report();
  }
}

void Node::testMessage(int L, int id, int j) {
  if (L > this->level) {
    message msg;
    msg.id = 2; // Test Message
    msg.args[0] = L;
    msg.args[1] = id;
    msg.weight = this->edges[j].weight;
    this->addMessage(msg);
  } 
  else if (id == this->id) {
    if (this->edges[j].state == 0)
      this->edges[j].state = 2;
    if (j != this->testEdge) {
      message msg;
      msg.id = 4; // Reject Message
      msg.weight = this->edges[j].weight;
      this->edges[j].node->addMessage(msg);
    }
    else
      test();
  }
  else{
    message msg;
    msg.id = 3; // Accept Message
    msg.weight = this->edges[j].weight;
    this->edges[j].node->addMessage(msg);
  }
}

void Node::accept(int j) {
  this->testEdge = -1;
  if(this->edges[j].weight<this->bestWeight){
    this->bestEdge = j;
    this->bestWeight = this->edges[j].weight;
  }
  this->report();
}

void Node::reject(int j) {
  if(this->edges[j].state == 0)
    this->edges[j].state = 2;
  this->test();
}

void Node::report() {
  int i,k;
  k=0;
  for(i=0;i<this->numEdges;i++){
    if(this->edges[i].state==1 && i!=this->parent)
      k++;
  }
  if(findCount == k && this->testEdge == -1){
    this->state = 2;
    message msg;
    msg.id=5;
    msg.args[0]=this->bestWeight;
    msg.weight = this->edges[this->parent].weight;
    this->edges[this->parent].node->addMessage(msg);
  }
}

void quicksort(vector<int> &dist, vector<int> &ind, int low, int high) {
  int i = low;
  int j = high;
  int z = dist[(low + high) / 2];
  do {
    while(dist[i] < z) i++;

    while(dist[j] > z) j--;

    if(i <= j) {
      /* swap two elements */
      int t = dist[i];
      int ti = ind[i];
      dist[i]=dist[j];
      dist[j]=t;

      ind[i] = ind[j];
      ind[j]=ti;
      i++; 
      j--;
    }
  } while(i <= j);

  if(low < j) 
    quicksort(dist,ind, low, j);

  if(i < high) 
    quicksort(dist,ind, i, high); 
}
void Node::print_output(){
  FILE *fp;
  vector<int> n1;
  vector<int> n2;
  vector<int> ind;
  int i;
  i = 0;
  for(map<int,int>::iterator it = mst.begin(); it != mst.end(); ++it){
    n1.push_back(allEdges[it->first].left);
    n2.push_back(allEdges[it->first].right);
    ind.push_back(i++);
  }
  quicksort(n1,ind,0,n1.size()-1);
  fp = fopen("output.txt","w");
  for(i=0;i<n1.size();i++){
    fprintf(fp,"%d->%d\n",n1[i],n2[ind[i]]);
  }
  fclose(fp);
}


void Node::reportMessage(int weight, int j) {
  if(j != this->parent){
    if(weight < this->bestWeight){
      this->bestEdge = j;
      this->bestWeight = weight;
    }
    this->findCount += 1;
    report();
  }
  else{ 
    if(this->state == 1){
      message msg;
      msg.id = 5;
      msg.args[0]=weight;
      msg.weight = this->edges[j].weight;
      //place recieved message at queue end
      this->addMessage(msg);
    }
    else if(weight > this->bestWeight){
      changeRoot();
    }
    else if (weight == INF && this->bestWeight == INF){
      print_output();
      run = 0;
      //pthread_exit(NULL);
    }
  }
}	
void Node::changeRoot() {
  message msg;
  if(this->edges[this->bestEdge].state == 1){
    msg.id=6;
    msg.weight = this->edges[this->bestEdge].weight;
    this->edges[this->bestEdge].node->addMessage(msg);
  }
  else{
    msg.id=0;
    msg.args[0]=this->level;
    msg.weight = this->edges[this->bestEdge].weight;
    this->edges[this->bestEdge].node->addMessage(msg);
    this->edges[this->bestEdge].state = 1;
    pthread_mutex_lock(&(mst_mutex));
    mst.insert(pair<int,int>(this->edges[this->bestEdge].weight,1));
    pthread_mutex_unlock(&(mst_mutex));
  }
}

void Node::changeRootMessage(int j) {
  changeRoot();
}
