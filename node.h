#ifndef NODE_H_
#define NODE_H_
#include <iostream>
#include <queue>
#include <cstdlib>
#include <pthread.h>
#include <stdio.h>
#include <map>

using namespace std;

// Global variables
int run;
map<int,int> mst;
pthread_mutex_t mst_mutex;

class Node;

typedef struct edgeInput {
  int left;
  int right;
  int weight;
}edgeInput;

map<int, edgeInput> allEdges;
// BASIC: 0, BRANCH: 1, REJECT: 2
typedef struct edge {
  int state;
  int weight;
  Node* node;
}edge;

typedef struct message {
  int id;
  int args[3];
  int weight;
}message;

// SLEEPING: 0, FIND: 1, FOUND: 2
class Node {
  public:
    Node();
    ~Node();
    int state;
    edge *edges;
    int ind;
    int id;
    int numEdges;
    int level;
    int bestEdge;
    int bestWeight;
    int testEdge;
    int parent;
    int findCount;
    queue<message> messages;
    pthread_mutex_t mutex;
    void print_output();
    void init(int numEdges, int* weights, Node** nodes);
    void addMessage(message);
    void readMessage();
    int findMinEdge();
    void wakeup();
    void connect(int, int);
    void initiate(int, int, int, int);
    void test();
    void testMessage(int, int, int);
    void accept(int);
    void reject(int);
    void report();
    void reportMessage(int, int);
    void changeRootMessage(int);
    void changeRoot();
};
#endif // Node.h
