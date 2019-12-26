#include<cstdio>
#include<cstring>
#include<cstdlib>
#include<ctime>
#include<iostream>
#include<algorithm>
#include "fat.h"

using namespace std;

const int fileNumber = BlockSize / DirSize;

FILE *fileSystem;
struct ReserveBlock sys;
struct FCBBlock *fcblist;   //�Ѵ��ļ�����
int *fat;
int maxFileId = 0;

void openFileSystem(char filename[])
{
    fileSystem = fopen(filename,"rb+");

    fread(&sys,1,BlockSize,fileSystem); //���뱣����

    //Ϊfat������ռ�
    fat = (int *)malloc(BlockSize * sys.fatblocknum);

    for(int i = 0; i < sys.fatblocknum; i++) {
        fread(fat + i * BlockSize / sizeof(int),1,BlockSize,fileSystem);
    }
    fcblist = (struct FCBBlock *)malloc((sizeof(struct FCBBlock)));
    fcblist->next = NULL;
}

void closeFileSystem()
{
    long offset = sys.resblocknum * BlockSize;
    fseek(fileSystem, offset, SEEK_SET);
    for(int i = 0; i < sys.fatblocknum; i++) {
        fwrite(fat + i * BlockSize / sizeof(int),1,BlockSize,fileSystem);
    }
    free(fat);
    fclose(fileSystem);
}

void listDir()
{
    char str[] = "fileName            fileLength            date\n";

    long offset = (sys.fatblocknum + sys.resblocknum) * BlockSize;
    fseek(fileSystem, offset, SEEK_SET);
    struct DirBlock *dir = (struct DirBlock *)malloc(BlockSize);
    printf("%s",str);

    for(int i = 0; i < sys.rootblocknum; i++) {

        fread(dir, 1, BlockSize, fileSystem);
        struct DirBlock *tp = dir;
        for(int j = 0; j < fileNumber; j++) {
            if(tp->filename[0] != '\0') {
                printf("%s      %20ld          %d-%d-%d\n",tp->filename,tp->filelen,tp->year,tp->month,tp->day);
            }
            tp++;
        }

    }
    free(dir);
}

void setFileTime(struct DirBlock *tp)
{
    time_t timep;       //��ȡϵͳ��ǰ����
    time(&timep);
    struct tm *p = gmtime(&timep);
    tp->year = p->tm_year + 1900;
    tp->month = p->tm_mon + 1;
    tp->day = p->tm_mday;
}
int creatFile(char filename[])
{
    long offset = (sys.fatblocknum + sys.resblocknum) * BlockSize;
    long pos = -1;

    struct DirBlock *dir = (struct DirBlock *)malloc(BlockSize);
    fseek(fileSystem,offset,SEEK_SET);

    struct DirBlock *preDir = (struct DirBlock *)malloc(BlockSize);

    int flag = true;

    for(int i = 0; i < sys.rootblocknum; i++) {
        fread(dir,1,BlockSize,fileSystem);

        struct DirBlock *tp = dir;

        for(int j = 0; j < fileNumber; j++) {
            if(flag && tp->filename[0] == '\0') {

                strcpy(tp->filename,filename);
                tp->filelen = 0;
                tp->firstblockaddr = -1;

                setFileTime(tp);

                memcpy(preDir,dir,BlockSize);  //���Ǻ�����Ҫд��root����λ��
                tp->filename[0] = '\0';

                pos = offset;               //��¼�ļ�ָ��ƫ����

                flag = false;
            }

            if(!strcmp(tp->filename,filename)) {   //��ͬ���ļ����˳�
                printf("the file has exit\n");
                free(dir);
                return -1;
            }

            tp++;
        }
        offset = ftell(fileSystem);
    }

    fseek(fileSystem,pos,SEEK_SET);

    //ע��������Ҫ��preDir��һ����д��root��
    fwrite(preDir,1,BlockSize,fileSystem);

    free(dir);
    free(preDir);
    return 1;
}

void resetFat(int first)
{
    while(first != -1) {
        int tmp = fat[first];
        fat[first] = 0;
        first = tmp;
    }
}
int deleteFile(char filename[])
{
    //���Ȳ鿴����ļ��ǲ����Ѵ򿪵��ļ����Ѵ򿪵��ļ��ǲ���ɾ����

    struct FCBBlock *p = fcblist->next;
    while(p && strcmp(p->fileinfo.filename,filename)) {
        p = p->next;
    }
    if(p != NULL) {
        printf("the file is openning\n");
        return -1;
    }


    long offset = (sys.fatblocknum + sys.resblocknum) * BlockSize;
    struct DirBlock *dir = (struct DirBlock *)malloc(BlockSize);
    fseek(fileSystem,offset,SEEK_SET);
    for(int i = 0; i < sys.rootblocknum; i++) {

        fread(dir,1,BlockSize,fileSystem);
        struct DirBlock *tp = dir;

        for(int j = 0; j < fileNumber; j++) {
            if(!strcmp(tp->filename,filename)) {
                tp->filename[0] = '\0';

                resetFat(tp->firstblockaddr); //������ļ�ռ�õ�fat���鸴λ
                fseek(fileSystem,offset,SEEK_SET);
                fwrite(dir,1,BlockSize,fileSystem);
                return 1;
            }
            tp++;
        }
        offset = ftell(fileSystem);
    }

    printf("the file does not exist\n");
    return -1;
}
void getMaxFileId()
{
    struct FCBBlock *p = fcblist->next;
    while(p != NULL) {
        maxFileId = max(maxFileId,p->fileid);
        p = p->next;
    }
}

void convertDirBlock(struct FCBBlock *p,struct DirBlock *tp,int op)
{
    if(0 == op) {
        p->fileinfo.day = tp->day;
        p->fileinfo.dirflag = tp->dirflag;
        p->fileinfo.filelen = tp->filelen;

        strcpy(p->fileinfo.filename,tp->filename);
        strcpy(p->fileinfo.fillchar,tp->fillchar);

        p->fileinfo.firstblockaddr = tp->firstblockaddr;
        p->fileinfo.month = tp->month;
        p->fileinfo.year = tp->year;
    }
    else if(1 == op) {
        tp->day = p->fileinfo.day;
        tp->dirflag = p->fileinfo.dirflag;
        tp->filelen = p->fileinfo.filelen;

        strcpy(tp->filename,p->fileinfo.filename);
        strcpy(tp->fillchar,p->fileinfo.fillchar);

        tp->firstblockaddr = p->fileinfo.firstblockaddr;
        tp->month = p->fileinfo.month;
        tp->year = p->fileinfo.year;
    }
}
int openFile(char filename[])
{
    //����fileId

    long offset = (sys.fatblocknum + sys.resblocknum) * BlockSize;
    struct DirBlock *dir = (struct DirBlock *)malloc(BlockSize);

    fseek(fileSystem,offset,SEEK_SET);

    for(int i = 0; i < sys.rootblocknum; i++) {
        fread(dir,1,BlockSize,fileSystem);
        struct DirBlock *tp = dir;

        for(int j = 0; j < fileNumber; j++) {
            if(!strcmp(tp->filename,filename)) {

                struct FCBBlock *p = (struct FCBBlock *)malloc(sizeof(FCBBlock));
                convertDirBlock(p,tp,0);
//                memcpy(p->fileinfo,tp,sizeof(p->fileinfo));
                p->filepos = 0;
                p->fdtblockaddr = offset / BlockSize;
                p->fdtblockindex = j;
                if(maxFileId == 0) {
                    getMaxFileId();
                }
                p->fileid = ++maxFileId;

                p->next = fcblist->next;

                fcblist->next = p;

                return p->fileid;
            }
            tp++;
        }
        offset = ftell(fileSystem);
    }
    printf("the file does not exist\n");
    return -1;
}
int closeFile(int fileId)
{

    //��fcblist���ҵ���fileidֵ��ͬ��

    struct FCBBlock *p = fcblist->next;
    struct FCBBlock *pre = fcblist;

    while(p && p->fileid != fileId) {
        pre = p;
        p = p->next;
    }

    if(p == NULL) {
        printf("the file does not exist\n");
        return -1;
    }
    pre->next = p->next;


    //�ڴ��ļ��ڼ���ܻ���ļ�����д������Ҫ�Ѹı����Ϣд���ļ�

    long offset = (sys.fatblocknum + sys.resblocknum) * BlockSize;

    fseek(fileSystem,offset,SEEK_SET);
    struct DirBlock *dir = (struct DirBlock *)malloc(BlockSize);

    for(int i = 0; i <  sys.rootblocknum; i++) {
        fread(dir,1,BlockSize,fileSystem);
        struct DirBlock *tp = dir;
        for(int j = 0; j < fileNumber; j++) {
            if(!strcmp(p->fileinfo.filename,tp->filename)) {
                convertDirBlock(p,tp,1);
                fseek(fileSystem,offset,SEEK_SET);
                fwrite(dir,1,BlockSize,fileSystem);
                free(dir);
                return 1;
            }
            tp++;
        }
        offset = ftell(fileSystem);
    }

    free(dir);
    printf("the file does not exist\n");
    return -1;
}
int readFile(char ptr[],long n,int fileId)
{
    //nΪ��ȡ�ֽڳ�

    //ÿ�ζ�ȡһ��BlockSize��С�����ݣ�����fat�еĿ����Ϣ���ļ�������Ѱ��

    struct FCBBlock *p = fcblist->next;

    while(p && p->fileid != fileId) {
        p = p->next;
    }

    if(p == NULL) {
        printf("the file does not exist\n");
        return -1;
    }


    //�ڵ�ǰ�ļ�ָ��ȷ��������£����ļ�ָ��ָ���λ�ÿ�ʼ��ȡ�ļ���
    //������Ҫ�ж�Ҫ��ȡ���ֽ�����С�Ƿ񳬹���δ�������ֽ���������Ǿ���Ҫ����n

    if((p->filepos + n) > p->fileinfo.filelen) {
        n = p->fileinfo.filelen - p->filepos;
    }

    int firstBlock = p->fileinfo.firstblockaddr;

    /**
    * ���filepos����ָ���ļ���ʼλ��ʱ��
    * ��ô����Ҫ����firstBlock��λ��
    **/

    long pos = p->filepos;

    while(pos >= BlockSize) {
        pos -= BlockSize;
        firstBlock = fat[firstBlock];
    }

//cout<<firstBlock<<endl;
    //�ɷ�Ϊ���׶εĶ�
    //��һ�׶Σ�������BlockSize * firstBlock - filePos��С
    //�ڶ��׶Σ���������BlockSize��
    //�����׶Σ�����ʱfirstBlock * BlockSize - n��С

    char str[512];
    while(n > 0 && fat[firstBlock] != -1) {
        //����Ӧ������һ��������ʶÿ��Ҫ��ȡ���ֽ���
        int byteSize = 0;

        if(pos != 0) {
            byteSize = BlockSize - pos; // ��һ�׶ζ�ȡ�ֽ���
        }

        fseek(fileSystem,firstBlock * BlockSize,SEEK_SET);
        fread(str,1,BlockSize,fileSystem);

        printf("%s",str+byteSize);

        firstBlock = fat[firstBlock];

        n -= byteSize;
    }

    fseek(fileSystem,firstBlock * BlockSize,SEEK_SET);

    fread(str,1,BlockSize,fileSystem);  //ֻ��ȡn���ֽ�
//    cout<<p->filepos<<" "<<n<<endl;

    for(long i = p->filepos; i < p->filepos + n; i++) {
        printf("%c",str[i]);
    }
//    str[n] = '\0';  //��Ϊ֮ǰ�Ĵ洢����������str������������������Ҫ�����ַ�����β����������ܻὫԭ�ȵ��������

//    printf("%s\n",str);
//    printf("ppp");
}
int getFatLength()
{
    return sys.fatblocknum * BlockSize / sizeof(int);
}
int appendBlock(struct FCBBlock *lp,int n)
{
    //nΪ��Ҫ���ӵĿ���

    int firstBlock = lp->fileinfo.firstblockaddr;

    while(firstBlock != -1 && fat[firstBlock] != -1) {
        firstBlock = fat[firstBlock];
    }

    int *arr = (int *)malloc((n + 2) * 4);
    int arrLength = 0;

    int fatSize = getFatLength();

    for(int i = 0; i < fatSize; i++) {
        if(fat[i] == 0) {
            arr[arrLength++] = i;
        }

        if(arrLength == n) {
            break;
        }
    }

    if(arrLength < n) {
        printf("the system has not enough block to allcate\n");
        return -1;
    }

//    printf("\n");
//    for(int i = 0; i < arrLength; i++) {
//        printf("%d ",arr[i]);
//    }
//    printf("\n");

    int p = 0;
    if(firstBlock == -1) {

        firstBlock = arr[p++];
        lp->fileinfo.firstblockaddr = firstBlock;
    }

    for(int i = p; i < arrLength ; i++) {

        fat[firstBlock] = arr[i];
        firstBlock = arr[i];
    }
    fat[firstBlock] = -1;


//    printf("\n");
//
//    for(int i = 0; i < 1024; i++) {
//        printf("%d ",fat[i]);
//
//        if(i % 50 == 0) {
//            printf("\n");
//        }
//    }
//    printf("\n");
    return 0;
}

int getBlock(int len)
{
    int res = len / BlockSize;
    res += (len % BlockSize) ? 1 : 0;
    return res;
}

int writeFile(char ptr[],int n,int fileId)
{
    //nΪ��Ҫд����ֽ���

    /*д��n���ֽ�
    * ��Ҫ�жϵ�ǰ�ļ���ռfat���Ƿ��ܴ�����n���ֽ�
    * ������ܴ��£�����ҪΪ����ļ���������fat��
    * ע��fat��洢�Ŀ�ž�ӳ�����ļ�������
    */

    struct FCBBlock *p = fcblist->next;

    while(p && p->fileid != fileId) {
        p = p->next;
    }

    if(p == NULL) {
        printf("the file does not exist\n");
        return -1;
    }
    int allBlock = getBlock(p->fileinfo.filelen + n);   //�õ�����ļ��ܹ�Ҫռ�õĿ���
    int hasBlock = getBlock(p->fileinfo.filelen); //�õ�����ļ��Ѿ�ռ�õĿ���

    int needBlock = 0;
    needBlock = allBlock - hasBlock;

    if(needBlock > 0) {
        if(appendBlock(p,needBlock)) {
            return -1;
        }

    }

    long pos = p->filepos;
//    printf("p->filepos = %d",p->filepos);
    int firstBlock = p->fileinfo.firstblockaddr;
//    printf("\nwrite %d\n",firstBlock);
    while(pos >= BlockSize) {
        firstBlock = fat[firstBlock];
        pos -= BlockSize;
    }


    int len = 0;
    char buf[512];
    while(fat[firstBlock] != -1) {
        int m = BlockSize;
        if(pos == 0) {
            m = BlockSize - pos;
        }

        fseek(fileSystem,firstBlock * BlockSize,SEEK_SET);

        fread(buf,1,BlockSize,fileSystem);
        memcpy(buf + pos, ptr + len,m);

        fseek(fileSystem,firstBlock * BlockSize,SEEK_SET);
        fwrite(buf,1,BlockSize,fileSystem);

        len += m;
        p->filepos += m;
        pos = 0;
        firstBlock = fat[firstBlock];
    }
//    printf("len = %d\n",len);
    if(len != n) {
        fseek(fileSystem,firstBlock * BlockSize,SEEK_SET);
        fread(buf,1,BlockSize,fileSystem);
//        printf("buf=%s\n",buf);
//        printf("%d\n",p->filepos % BlockSize);
        memcpy(buf + p->filepos % BlockSize,ptr + len,n - len);
//        printf("buf=%s\n",buf);
        fseek(fileSystem,firstBlock * BlockSize,SEEK_SET);
        fwrite(buf,1,BlockSize,fileSystem);
    }
    p->fileinfo.filelen += n;
//    printf("\npos=%d\n",p->filepos);
    p->filepos = p->fileinfo.filelen;

    return n;
}
int judgeEof(int fileId)
{
    struct FCBBlock *p;
    p = fcblist->next;

    while(p && p->fileid != fileId) {
        p = p->next;
    }

    if(p == NULL) {
        printf("the file does not exist\n");
        return -1;
    }
    if(p->filepos < p->fileinfo.filelen) {
        return 1;//��δ�����ļ�β
    }
    return 0;//��ʱ�ļ�ָ��ָ���ļ�β
}
long getFilePos(int fileId)
{
    struct FCBBlock *p = fcblist->next;

    while(p && p->fileid != fileId) {
        p = p->next;
    }
    if(p == NULL) {
        printf("the file does not exist\n");
        return -1;
    }
    return p->filepos;
}
int setFilePos(int fileId,long offset)
{

    if(offset < 0) {   //���������ļ�ָ��Ϊ��
        return -1;
    }
    struct FCBBlock *p = fcblist->next;

    while(p && p->fileid != fileId) {
        p = p->next;
    }
    if(p == NULL) {
        printf("the file does not exist\n");
        return -1;
    }

    if(offset > p->fileinfo.filelen) {  //���������ļ�ָ������ļ�����
        return -1;
    }

    p->filepos = offset;
    return 1;
}
long getFileLength(char filename[])
{
    //���ȵ������ļ�������ң�����о����뵽��Ŀ¼������

    struct FCBBlock *p = fcblist->next;

    while(p && strcmp(p->fileinfo.filename,filename)) {
        p = p->next;
    }
    if(p != NULL) {
        return p->fileinfo.filelen;
    }


    //�Ѵ��ļ������޷��ҵ����͵���Ŀ¼������

    long offset = (sys.fatblocknum + sys.resblocknum) * BlockSize;

    fseek(fileSystem,offset,SEEK_SET);

    struct DirBlock *dir = (struct DirBlock *)malloc(BlockSize);

    for(int i = 0; i < sys.rootblocknum; i++) {
        fread(dir,1,BlockSize,fileSystem);

        struct DirBlock *tp = dir;

        for(int j = 0; j < fileNumber; j++) {
            if(!strcmp(tp->filename,filename)) {

                free(dir);
                return tp->filelen;
            }
            tp++;
        }
    }
    free(dir);
    printf("the file couldn't found\n");
    return -1;
}
void debug()
{
    struct FCBBlock *p = fcblist->next;
    while(p) {
        printf("%d %s %d %d\n",p->fileid,p->fileinfo.filename,p->filepos,p->fdtblockindex);
        p = p->next;
    }

}
void listOpenFile()
{
    char str[] = "fileName            fileLength        ��ʶ��\n";
    printf("%s\n",str);
    struct FCBBlock *p = fcblist->next;
    while(p) {
        printf("%s      %20d          %d\n",p->fileinfo.filename,p->fileinfo.filelen,p->fileid);
        p = p->next;
    }

}
void menu()
{
    printf("\n\t---------------------------------------------------------\n");
    printf("\t      1--�����ļ�ϵͳ");
    printf("\t          2--ж���ļ�ϵͳ\n");
    printf("\t      3--��ʾĿ¼");
    printf("\t          4--�½��ļ�\n");
    printf("\t      5--ɾ���ļ�");
    printf("\t          6--��ȡ�ļ�\n");
    printf("\t      7--д���ļ�");
    printf("\t          8--���ļ�\n");
    printf("\t      9--�ر��ļ�");
    printf("\t          10--�����ļ�ָ��\n");
    printf("\t      11--�ж��ļ�����");
    printf("\t          12--ȡ���ļ�����\n");
    printf("\t      13--��ʾ�Ѵ��ļ�����");
    printf("\t  14--�˳�\n");
    printf("\t---------------------------------------------------------\n");
}
int inputFileId(int fileId)
{
    printf("input the file Id:");
    scanf("%d",&fileId);
}
char *inputFileName(char filename[])
{

}

int testOpenFile()
{
    int fileId;
    printf("input the file name:");
    char pname[20];
    scanf("%s",pname);
    fileId = openFile(pname);
    return fileId;
}
int testCloseFile()
{

}
void run()
{
    char filename[] = "fatsys.dat";

    int op = 0;
    while(1) {
        menu();
        printf("root@shan$ ");
        scanf("%d",&op);
        int fileId = 0;
        if(1 == op) {
            openFileSystem(filename);
        }
        else if(2 == op) {
            closeFileSystem();
        }
        else if(3 == op) {
            listDir();
        }
        else if(4 == op) {
            printf("input the file name:");
            char pname[20];
            scanf("%s",pname);
            creatFile(pname);
        }
        else if(5 == op) {
            printf("input the file name:");
            char pname[20];
            scanf("%s",pname);
            deleteFile(pname);
        }
        else if(6 == op) {
//            printf("input the file Id:");
//            scanf("%d",&fileId);
            fileId = testOpenFile();
            printf("input the offset:");
            long offset = 0;
            scanf("%ld",&offset);
            setFilePos(fileId,offset);
//            printf("input the length you want to read:");
            int n = BlockSize;
//            scanf("%d",&n);
            readFile(" ",n,fileId);
            closeFile(fileId);
        }
        else if(7 == op) {
            int fileId = testOpenFile();
            printf("input the offset:");
            long offset = 0;
            scanf("%ld",&offset);
            setFilePos(fileId,offset);
            printf("input what you want to read:");
            char str[BlockSize * 4];
//            gets(str);
            scanf("%s",str);
            writeFile(str,strlen(str),fileId);
            closeFile(fileId);
        }
        else if(8 == op) {
            fileId = testOpenFile();
        }
        else if(9 == op) {
            printf("input the file Id:");
            scanf("%d",&fileId);
            closeFile(fileId);
        }
        else if(10 == op) {
            printf("input the file Id:");
            scanf("%d",&fileId);
            printf("input the offset:");
            long offset;
            scanf("%ld",&offset);
            setFilePos(fileId,offset);
        }
        else if(11 == op) {

        }
        else if(12 == op) {
            printf("input the file name:");
            char pname[20];
            scanf("%s",pname);
            long len = getFileLength(pname);
            printf("the file length: %ld\n",len);
        }
        else if(13 == op) {
            listOpenFile();
        }
        else if(14 == op) {
            return;
        }
        else {
            ;
        }
    }

}
int main()
{
    run();
    return 0;
}
