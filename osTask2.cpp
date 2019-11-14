#include<iostream>
#include<cstdio>
#include<cstring>
using namespace std;

const int process_MAXN = 5; // the process number
const int resource_MAXN = 3; // the resource number


struct bankAlgorithm
{
private:
    int Available[resource_MAXN] = {2,3,0};
    int tmpAvailable[process_MAXN][resource_MAXN]; //
    int Max[process_MAXN][resource_MAXN] = { {7, 5, 3},{3, 2, 1},{9, 0, 2},{2, 2, 2},{4, 3, 3} };
    int Allocation[process_MAXN][resource_MAXN] = {{0, 1, 0},{2, 0, 0},{3, 0, 2},{2,1,1},{0, 0, 2}};
    int Need[process_MAXN][resource_MAXN] = {{7, 4, 3},{1, 2, 2},{6, 0, 0},{0, 1, 1},{4, 3, 1}};
    int processId = 1;
    int request[3] = {1,0,2};
    int visit[process_MAXN][3] = {{0}}; // the priority of visit
public:
    void mainCtrl();
    void sayHelloworld();
    void inputData();
    void outputData();
    bool sendRequest();
    void tryToAskResource();
    void showInformation();
    void showError();
};

void bankAlgorithm::mainCtrl()
{
    inputData();
    outputData();
    sendRequest();
}
void bankAlgorithm::sayHelloworld()
{
    printf("\thello the cruel world\n");
}
void bankAlgorithm::inputData()
{
    printf("input the process id and the three request resource\n");
    scanf("%d%d%d%d",&processId,&request[0],&request[1],&request[2]);

}
void bankAlgorithm::outputData()
{
    printf("ProcessId:%d\nrequest:",processId);
    for(int i = 0; i < resource_MAXN; i++) {
        printf("%d ",request[i]);
    }
    printf("\n");
}


bool bankAlgorithm::sendRequest() //no bug
{
    for(int i = 0; i < resource_MAXN; i++) {
        if(request[i] > Need[processId][i]) {
            printf("The requested resource has exceeded the required resource\n");
            return false;
        }
        if(request[i] > Available[i]) {
            printf("The requested resource has exceeded the Available resource\n");
            return false;
        }
    }

    tryToAskResource();
}
void bankAlgorithm::tryToAskResource()
{
    for(int i = 0; i < resource_MAXN; i++) {
        Available[i] -= request[i];
        Need[processId][i] -= request[i];
        Allocation[processId][i] += request[i];
    }

    int cnt = 0; //the visitted priority
    for(int i = 0; i < process_MAXN; i++) {
        visit[i][0] = 0;
    }
    while(true) {
        bool judgeToRun = true;
        for(int i = 0; i < process_MAXN; i++) {
            if(!visit[i][0]) {
                bool flag = true;
                for(int j = 0; j < resource_MAXN; j++) {
                    if(Available[j] < Need[i][j]) {
                        flag = false;
                        break;
                    }
                }
                if(flag) {
                    for(int j = 0; j < resource_MAXN; j++) {
                        tmpAvailable[i][j] = Available[j];
                        Available[j] += Allocation[i][j];
                    }
                    judgeToRun = false;
                    visit[i][0] = 1;
                    visit[i][1] = cnt++;
                    visit[i][2] = i;
//                    cout<<"i = "<<i<<endl;
                }
            }
        }
        if(judgeToRun) {break;}
    }
//    sayHelloworld();
//    cout<<cnt<<' '<<process_MAXN<<endl;
    if(cnt != process_MAXN) {
        showError();
        return;
    }
    showInformation();
}

void bankAlgorithm::showInformation()
{
    printf("Resource\tMax\tAvailable     Allocation    Need      Work\n");
    int tmp[3];
    int len = sizeof(visit[0]);
    for(int i = 0; i < process_MAXN; i++) {
        for(int j = i + 1; j < process_MAXN; j++) {
            if(visit[i][1] > visit[j][1]) {

                memcpy(tmp,visit[i],len);
                memcpy(visit[i],visit[j],len);
                memcpy(visit[j],tmp,len);
            }
        }
    }
    for(int i = 0; i < process_MAXN; i++) {
        int process_id = visit[i][2];
        printf("\t%d ",process_id);
        printf("\t");
        for(int j = 0; j < resource_MAXN; j++) {
            printf("%d ",Max[process_id][j]);
        }
        printf("\t   ");
        for(int j = 0; j < resource_MAXN; j++) {
            printf("%d ",tmpAvailable[process_id][j]);
        }
        printf("\t");
        for(int j = 0; j < resource_MAXN; j++) {
            printf("%d ",Allocation[process_id][j]);
        }
        printf("\t    ");
        for(int j = 0; j < resource_MAXN; j++) {
            printf("%d ",Need[process_id][j]);
        }
        printf("\t");
        for(int j = 0; j < resource_MAXN; j++) {
            printf("%d ",tmpAvailable[process_id][j] + Allocation[process_id][j]);
        }
        printf("\n");
    }
}
void bankAlgorithm::showError()
{
    printf("Sorry,you can't allocate resources to %d.\nBecause the Available resource can't satisfy any of the Process\n",processId);
}
int main()
{
    struct bankAlgorithm shan;
    shan.mainCtrl();
    return 0;
}
