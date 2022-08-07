#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>

#define min(a,b) (((a)<(b))?(a):(b))
#define NOID -1

// Struct that represents a "process control block"
typedef struct Process{
    int id;
    int arrivalTime;
    int remainingTime;
    int burstTime;
    int completed;

    // Metrics calculations
    int waitingTime;
    int turnaroundTime;
    int timeWaitedSince;  // The time (e.g. T=14) that the process has been waiting for

    // Linked list
    struct Process *next;
    struct Process *prev;
} Process;

// Stores a doubly linked list of process structs
typedef struct Queue{
    Process *front;
    Process *rear;
} Queue;

// global vars
// used to track nodes and completion status
Process **gNodeArray = NULL;
int totalNumberOfProcesses = 0;

Queue *initQueue() {
    Queue *q = (Queue *)malloc(sizeof(Queue));
    q->front = NULL;
    q->rear = NULL;

    return q;
}

Process *initProcess(int arrivalTime, int burstTime) {
    Process *p = (Process *)malloc(sizeof(Process));
    p->id = -1;
    p->arrivalTime = arrivalTime; 
    p->remainingTime = burstTime;
    p->burstTime = burstTime;
    p->completed = 0;

    // Metrics calculations
    p->waitingTime = 0;
    p->turnaroundTime = 0;
    p->timeWaitedSince = arrivalTime;  // The time (e.g. T=14) that the process has been waiting for

    // Linked list
    p->next = NULL;
    p->prev = NULL;

    return p;
}

/*
 * Compares two processes p1 and p2 and returns 1 if
 * p1 is of a higher priority than p2
 */
int isHigherPriority(Process *p1, Process* p2) {
    if(p1->arrivalTime == p2->arrivalTime) {
        return p1->remainingTime < p2->remainingTime;
    }
    return p1->arrivalTime < p2->arrivalTime;
}

/*
Adds process to queue
Queues process by arrival time, then remaining time
*/
void enqueue(Queue *q, Process * newNode) {
    // special case for the first node inserted
    if(q->front == NULL) {
        q->front = newNode;
        q->rear = newNode;
        return;
    }

    // insert at the back if the new process is the lowest priority process so far
    if(!isHigherPriority(newNode, q->rear)) {
        newNode->prev = q->rear;
        q->rear->next = newNode;
        q->rear = newNode;
        return;
    }

    // insert at the front if the new process is the highest priority process so far
    if(isHigherPriority(newNode, q->front)) {
        newNode->next = q->front;
        q->front->prev = newNode;
        q->front = newNode;
        return;
    }

    Process *current = q->front;

    // look for insertion point
    while (current->next != NULL && !isHigherPriority(newNode, current)) {
        current = current->next;
    }

    // insert before current
    newNode->prev = current->prev;
    newNode->next = current;
    current->prev->next = newNode;
    current->prev = newNode;
}

/*
Adds process to queue
Queue by process remaining time, regardless of arrival time
*/
void enqueueSJF(Queue *q, int id){
    Process *newNode = gNodeArray[id];
    newNode->prev = NULL;
    newNode->next = NULL;

    // special case for the first node inserted
    if(q->front == NULL) {
        q->front = newNode;
        q->rear = newNode;
        return;
    }

    Process *current = q->front;

    // look for insertion point
    while (current->next != NULL && current->remainingTime <= newNode->remainingTime) {
        current = current->next;
    }

    // special case for longest job in queue yet
    if (current->next == NULL && newNode->remainingTime >= current->remainingTime){
        current->next = newNode;
        newNode->prev = current;
        q->rear = newNode;
        return;
    }
    // special case for if inserting before first node
    if (current == q->front){
        current->prev = newNode;
        newNode->next = current;
        q->front = newNode;
        return;
    }

    // insert before current
    newNode->prev = current->prev;
    newNode->next = current;
    current->prev->next = newNode;
    current->prev = newNode;
}

/*
Removes node from queue and sets its status to completed
*/
Process *dequeue(Queue *q) {
    if(q->front == NULL) {
        #ifdef DEBUG
        fprintf(stderr, "Attempted to dequeue from empty queue!\n");
        #endif
        return NULL; 
    }
    Process *poppedNode = q->front;
    // if removing only node
    if (q->front == q->rear){
        q->front = NULL;
        q->rear = NULL;
        return poppedNode;
    }
    // else shift front to next node & unset prev ptr
    q->front = q->front->next;
    q->front->prev = NULL;

    return poppedNode;
}

void display(Queue *q) {
    // print queue from in the order "front...back"
    Process *qPtr = q->front;
    printf("Current Queue: ");

    if(qPtr == NULL) {
        printf("Empty!");
    } else {
        while(qPtr != NULL) {
            printf("(%d %d %d)", qPtr->id, qPtr->arrivalTime, qPtr->remainingTime);
            qPtr = qPtr->next;
        }
    }
    printf("\n");
}

/*
Display queue up till specified arrival time (arrivalCap)
*/
void displayTillArrival(Queue *q, int arrivalCap) {
    // print queue from in the order "front...back"
    Process *qPtr = q->front;
    printf("Current Queue: ");

    if(qPtr == NULL) {
        printf("Empty!");
    } else {
        while(qPtr != NULL && qPtr->arrivalTime <= arrivalCap) {
            printf("(%d %d)", qPtr->arrivalTime, qPtr->remainingTime);
            qPtr = qPtr->next;
        }
    }
    printf("\n");
}

void clearQueue(Queue *q){
    q->front = NULL;
    q->rear = NULL;
}

/*
Splits all processes into fast and slow queue at the threshold point, ignoring those processes after arrival (> arrival)
*/
void splitQueue(Process * allProcesses[], int totalNumberOfProcesses, Queue *fastQueue, Queue *slowQueue, int threshold, int arrival){
    for (int i = 0; i < totalNumberOfProcesses; ++i){
        Process *current = allProcesses[i];
        current->next = NULL;
        current->prev = NULL;
        // if job already completed, move on
        if (current->completed)
            continue;

        // queue into respective queue up till specified arrival time
        if (current->arrivalTime > arrival){
            continue;
        }

        if (current->remainingTime <= threshold)
            // queue job into fast queue like SJF
            enqueueSJF(fastQueue, current->id);
        else
            enqueue(slowQueue, current);
    }
}

/*
Get harmonic mean of current queue, up till specified arrival time (arrivalCap)
*/
int getHMean(Queue * queue){
    int count = 0;
    double accumulator = 0.0;

    for(Process * curProcess = queue->front; curProcess != NULL; curProcess = curProcess->next){
        accumulator += pow((double)curProcess->remainingTime, -1);
        count++;
    }
    // add 0.5 to ensure proper rounding up/down when casting to int
    accumulator = pow((accumulator/count), -1) + 0.5;
    return (int)accumulator;
}

/*
Get mean of current queue, up till specified arrival time (0 <= x <= arrivalCap)
*/
float getMean(Process *allProcesses[], int totalNumberOfProcesses, int arrivalCap){
    float accumulator = 0;
    int count = 0;
    for (int i = 0; i < totalNumberOfProcesses; ++i){
        if (allProcesses[i]->arrivalTime > arrivalCap)
            continue;
        if (allProcesses[i]->completed)
            continue;
        accumulator += allProcesses[i]->remainingTime;
        count++;
    }
    // ensure proper rounding
    return accumulator / count;
}

/*
Executes process with quantum

If quantum <= 0, it will be executed with no limit
Also updates the relevant calculation fields such as waitingTime, lastExecutionTime and turnaroundTime (if applicable)

returns the executionTime of the process
*/
int runProcess(Process *q, int quantum, int curTime){
    if(q == NULL){
        return 0;
    }
    int executionTime = q->remainingTime;
    if(quantum > 0){
        executionTime = min(quantum, executionTime);
    }

    q->remainingTime -= executionTime;
    if(q->remainingTime <= 0){
        q->completed = 1;

        q->turnaroundTime = (curTime + executionTime) - q->arrivalTime;
    }

    q->waitingTime += curTime - q->timeWaitedSince;
    q->timeWaitedSince = curTime + executionTime;

    return executionTime;
}

// Returns TRUE if there exist an incomplete process
bool hasIncompleteProcess(Process * processes[], int numProcesses){
    for(int i = 0; i < numProcesses; ++i){
        if(!processes[i]->completed){
            return true;
        }
    }
    return false;
}

// Performs the scheduling algorithm
void startScheduling(){
    int curTime = 0;
    while(hasIncompleteProcess(gNodeArray, totalNumberOfProcesses)){
        Queue *fastQueue = initQueue();
        Queue *slowQueue = initQueue();

        float mean = getMean(gNodeArray, totalNumberOfProcesses, curTime);
        // Split the queue
        splitQueue(gNodeArray, totalNumberOfProcesses, fastQueue, slowQueue, mean, curTime);

        #ifdef DEBUG
            printf("Time: %d\n", curTime);
            printf("Fast queue: ");
            display(fastQueue);

            printf("Slow queue: ");
            display(slowQueue);
        #endif

        // if currently no jobs, fast forward time
        if(fastQueue->front == NULL && slowQueue->front == NULL){
            curTime++;
            free(fastQueue);
            free(slowQueue);
            continue;
        }

        // execute all processes in fast queue, at time t
        while (fastQueue->front != NULL){
            Process *fastProcess = dequeue(fastQueue);
            curTime += runProcess(fastProcess, 0, curTime);
        }

        // Pick one job from the long queue
        Process *slowProcess = dequeue(slowQueue);
        // flag to check if process has been interrupted before
        int hasPreempted = 0;
        for (int i = 0; i < getHMean(slowQueue); ++i){
            // only allow slow process to be interrupted once
            if (!hasPreempted){
                Queue *simuFastQueue = initQueue();
                Queue *simuSlowQueue = initQueue();
                splitQueue(gNodeArray, totalNumberOfProcesses, simuFastQueue, simuSlowQueue, mean, curTime);
                // only interrupt process is new short process is <= 1/3 of current process remaining time
                if (simuFastQueue->front != NULL && simuFastQueue->front->burstTime * 3 <= slowProcess->remainingTime){
                    curTime += runProcess(slowProcess, simuFastQueue->front->burstTime, curTime);
                    Process *specialProcess = dequeue(simuFastQueue);
                    curTime += runProcess(specialProcess, 0, curTime);
                    // prevent process from being interrupted again by setting flag
                    hasPreempted = 1;
                }
                free(simuFastQueue);
                free(simuSlowQueue);
            }
            curTime += runProcess(slowProcess, 1, curTime);
        }
        free(fastQueue);
        free(slowQueue);
    }
}

int main(int argc, char * argv[]) {

    if(argc != 2) {
        printf("Usage: %s <input_file_name>\n", argv[0]);
        return -1;
    }

    char *inputFileName = argv[1];

    if(!strstr(inputFileName, ".txt"))
        strcat(inputFileName, ".txt");

    FILE *inputFile = fopen(inputFileName, "r");

    if(inputFile == NULL) {
        printf("Input file not found! Exiting...");
        return -1;
    }

    char line[64];
    // individual queues
    Queue *longProcessQueue = initQueue();
    Queue *shortProcessQueue = initQueue();

    int highestArrival = 0;
    int arrivalTime, burstTime;
    // preliminary queue to calculate mean/median
    Queue *tempQueue = initQueue();
    while(fscanf(inputFile, "%d %d\n", &arrivalTime, &burstTime) == 2) {

        #ifdef DEBUG
            printf("queuing %d %d\n",arrivalTime,burstTime);
        #endif
        // initial queue, hence id is -1
        Process * p = initProcess(arrivalTime, burstTime);
        enqueue(tempQueue, p);

        if (burstTime > highestArrival)
            highestArrival = burstTime;
        #ifdef DEBUG 
            display(tempQueue); 
        #endif
        totalNumberOfProcesses++;
    }

    gNodeArray = malloc(totalNumberOfProcesses * sizeof(Process*));
    Process *current = tempQueue->front;
    for (int i=0; i < totalNumberOfProcesses; ++i){
        gNodeArray[i] = current;
        current->id = i;
        current = current->next;
    }

    // Start of actual algorithm
    startScheduling();
    // End of actual algorithm

    // Print statistics
    int totalWaitTime = 0, totalTurnaroundTime = 0, maxTurnAroundTime = 0, maxWaitingTime = 0;
    #ifdef DEBUG
    printf("ID\tWait Time\tTurnaround Time\n");
    #endif
    for(int i = 0; i < totalNumberOfProcesses; ++i){
        Process *p = gNodeArray[i];
        #ifdef DEBUG
        printf("%d\t\t%d\t\t%d\n", p->id, p->waitingTime, p->turnaroundTime);
        #endif
        totalWaitTime += p->waitingTime;
        totalTurnaroundTime += p->turnaroundTime;
        // get max timings
        if (p->turnaroundTime > maxTurnAroundTime)
            maxTurnAroundTime = p->turnaroundTime;
        if (p->waitingTime > maxWaitingTime)
            maxWaitingTime = p->waitingTime; 
        free(p);
    }

    printf("average turnaround time: %lf\n", (float)totalTurnaroundTime/totalNumberOfProcesses);
    printf("maximum turnaround time: %d\n", maxTurnAroundTime);
    printf("average waiting time: %lf\n", (float)totalWaitTime/totalNumberOfProcesses);
    printf("maximum waiting time: %d\n", maxWaitingTime);
    return 0;
}
