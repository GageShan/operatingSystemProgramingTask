#include<iostream>
#include<cstring>
#include<cstdlib>
#include<cstdio>
#include<typeinfo>

using namespace std;

struct node
{
    int process_id; // the process ID
    int startTime;
    int requireTime;
    int arriveTime;
    int finishTime;
    int haveRunTime;
    double response;
    struct node *next;
};

class dispatch
{
private:
    int cnt = 0; //the  number of all process
    int readyQueueNumber = 0; // the readyQueue can be fill with five process
    struct node *backQueue;
    struct node *readyQueue;
    struct node *runQueue;
    struct node *finishQueue;
    int sysTime = -1;
public:
    void mainCtrl();
    void init();
    void output(struct node *head);
    double getRespose(struct node *head);
    struct node* readData(struct node *head);
    struct node* sortQueue(struct node *head,int op);
    struct node* swapNode(struct node *a,struct node *b);
    void processDispatch();
    struct node* jobDispatch();
    void work();
    bool judge();
    void print(struct node *head);
};

void dispatch::mainCtrl()
{
    init();
    backQueue = readData(backQueue); // read data from file
    backQueue = sortQueue(backQueue,1);// when op=1 sort the backQueue and op=2 sort the readyQueue
    work();
}
void dispatch::work()
{
    while(true) {
        sysTime++;
        backQueue = sortQueue(backQueue,1);
        processDispatch();
        if(judge()) {break;}
    }
    sortQueue(finishQueue,2);
    printf("\n");
    print(finishQueue);
}
void dispatch::init()
{
    backQueue = (struct node *)malloc(sizeof(struct node));
    backQueue->next = NULL;

    runQueue = (struct node *)malloc(sizeof(struct node));
    runQueue->next = NULL;

    readyQueue = (struct node *)malloc(sizeof(struct node));
    readyQueue->next = NULL;

    finishQueue = (struct node *)malloc(sizeof(struct node));
    finishQueue->next = NULL;
}
bool dispatch::judge()
{
    return backQueue->next == NULL && readyQueue->next == NULL;
}
void dispatch::output(struct node *head)
{
    struct node *p = head->next;
    while(p != NULL) {
        printf("%-2d %-2d %-2d %-2.3f\n",p->process_id,p->requireTime,p->arriveTime,p->response);
        p = p->next;
    }
}
void dispatch::print(struct node *head)
{
    struct node *p = head->next;
    while(p != NULL) {
        printf("id%-3d    n%-3d    a%-3d    s%-3d    f%-3d\n",p->process_id,p->requireTime,p->arriveTime,p->startTime,p->finishTime);
        p = p->next;
    }
}
struct node *dispatch::swapNode(struct node *a,struct node *b)
{
    a->arriveTime = b->arriveTime;
    a->finishTime = b->finishTime;
    a->haveRunTime = b->haveRunTime;
    a->process_id = b->process_id;
    a->requireTime = b->requireTime;
    a->response = b->response;
    a->startTime = b->startTime;
    return a;
}
struct node *dispatch::readData(struct node *head)
{
    FILE *fp = fopen("input.txt","r+");
    if(fp == NULL) {
        printf("the input file can't open\n");
        return head;
    }

    fscanf(fp,"%d",&cnt);
    struct node *p = head;

    for(int i = 0; i < cnt; i++) {
        struct node *tmp = (struct node *)malloc(sizeof(struct node));
        fscanf(fp,"%d%d%d",&tmp->process_id,&tmp->requireTime,&tmp->arriveTime);

        tmp->haveRunTime = tmp->startTime = 0;
        tmp->response = 1.0;

        p->next = tmp;
        p = p->next;

    }
    p->next = NULL;
//    output(head);
    return head;
}

double dispatch::getRespose(struct node *p1)
{
    double waitTime = 1.0 * (p1->arriveTime < sysTime ? (sysTime - p1->arriveTime):0);
//    cout<<"wt:"<<waitTime<<endl;
    return (waitTime + p1->requireTime * 1.0) / (1.0 * p1->requireTime);
}
struct node *dispatch::sortQueue(struct node *head,int op)
{
    struct node *p1 = head->next;

    while(p1 != NULL) {
         {p1->response = getRespose(p1);}

        struct node *p2 = p1->next;
        while(p2 != NULL) {
            {p2->response = getRespose(p2);}
            if(op == 1) {
                if(p1->response < p2->response) {
                    struct node *tmp = (struct node *)malloc(sizeof(struct node));
                    swapNode(tmp,p1);
                    swapNode(p1,p2);
                    swapNode(p2,tmp);
                }
            }
            else if(op == 2) {
                if(p1->startTime > p2->startTime) {
                    struct node *tmp = (struct node *)malloc(sizeof(struct node));
                    swapNode(tmp,p1);
                    swapNode(p1,p2);
                    swapNode(p2,tmp);
                }
            }

            p2 = p2->next;
        }
        p1 = p1->next;
    }
    return head;
}
struct node *dispatch::jobDispatch()
{
    struct node *t = readyQueue;
    while(t->next != NULL) {t = t->next;}

    struct node *p = backQueue->next, *pre = backQueue;

    while(p != NULL) {

        if(readyQueueNumber < 5 && p->arriveTime <= sysTime) {
            readyQueueNumber++;

            if(p->startTime == 0) {
                p->startTime = sysTime;
            }
            t->next = p;
            t = t->next;

            pre->next = p->next;
            p = pre->next;

        }
        else {
            pre = p;
            p = p->next;
        }
    }
    t->next = NULL;
    return readyQueue;
}
void dispatch::processDispatch()
{
     readyQueue = jobDispatch();

     /*base on round robin algorithm,the time slice is one*/

     if(readyQueueNumber > 0) {
        struct node *p = readyQueue->next;
        readyQueue->next = p->next;

        readyQueueNumber--;


        printf("%-4d:%-4d ",sysTime,p->process_id);
        p->haveRunTime++;
        if(p->haveRunTime == p->requireTime) {

            p->finishTime = sysTime;
            p->next = finishQueue->next;
            finishQueue->next = p;
        }
        else {
            struct node *t = readyQueue;
            while(t->next != NULL) { t = t->next;}
            t->next = p;
            p->next= NULL;
            readyQueueNumber++;
        }
     }
}
int main()
{
    class dispatch shan;
    shan.mainCtrl();
    return 0;
}


/*
test data
21
1 	71 	2
2 	62 	13
3 	54 	24
4 	43 	37
5 	58 	38
6 	53 	48
7 	44 	58
8 	67 	68
9 	69 	78
10 	61 	88
11 	42 	98
12 	21 	108
13 	52 	118
14 	33 	128
15 	50 	138
16 	23 	785
17 	60 	793
18 	66 	804
19 	25 	815
20 	48 	826
21 	20 	837

*/