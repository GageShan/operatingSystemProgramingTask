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
struct FCBBlock *fcblist;   //已打开文件表项
int *fat;
int maxFileId = 0;

void openFileSystem(char filename[])
{
    fileSystem = fopen(filename,"rb+");

    fread(&sys,1,BlockSize,fileSystem); //读入保留区

    //为fat区分配空间
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
    time_t timep;       //获取系统当前日期
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

                memcpy(preDir,dir,BlockSize);  //这是后续需要写入root区的位置
                tp->filename[0] = '\0';

                pos = offset;               //记录文件指针偏移量

                flag = false;
            }

            if(!strcmp(tp->filename,filename)) {   //有同名文件，退出
                printf("the file has exit\n");
                free(dir);
                return -1;
            }

            tp++;
        }
        offset = ftell(fileSystem);
    }

    fseek(fileSystem,pos,SEEK_SET);

    //注意这里是要将preDir这一整块写入root区
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
    //首先查看这个文件是不是已打开的文件，已打开的文件是不能删除的

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

                resetFat(tp->firstblockaddr); //将这个文件占用的fat数组复位
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
    //返回fileId

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

    //在fcblist中找到跟fileid值相同的

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


    //在打开文件期间可能会对文件进行写操作，要把改变的信息写入文件

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
    //n为读取字节长

    //每次读取一个BlockSize大小的内容，根据fat中的块号信息到文件分区中寻找

    struct FCBBlock *p = fcblist->next;

    while(p && p->fileid != fileId) {
        p = p->next;
    }

    if(p == NULL) {
        printf("the file does not exist\n");
        return -1;
    }


    //在当前文件指针确定的情况下，从文件指针指向的位置开始读取文件，
    //所以需要判断要读取的字节数大小是否超过尚未被读的字节数，如果是就需要调整n

    if((p->filepos + n) > p->fileinfo.filelen) {
        n = p->fileinfo.filelen - p->filepos;
    }

    int firstBlock = p->fileinfo.firstblockaddr;

    /**
    * 如果filepos不是指向文件开始位置时，
    * 那么就需要调整firstBlock的位置
    **/

    long pos = p->filepos;

    while(pos >= BlockSize) {
        pos -= BlockSize;
        firstBlock = fat[firstBlock];
    }

//cout<<firstBlock<<endl;
    //可分为三阶段的读
    //第一阶段，读完整BlockSize * firstBlock - filePos大小
    //第二阶段，读完整的BlockSize块
    //第三阶段，读此时firstBlock * BlockSize - n大小

    char str[512];
    while(n > 0 && fat[firstBlock] != -1) {
        //这里应该设置一个变量标识每次要读取的字节数
        int byteSize = 0;

        if(pos != 0) {
            byteSize = BlockSize - pos; // 第一阶段读取字节数
        }

        fseek(fileSystem,firstBlock * BlockSize,SEEK_SET);
        fread(str,1,BlockSize,fileSystem);

        printf("%s",str+byteSize);

        firstBlock = fat[firstBlock];

        n -= byteSize;
    }

    fseek(fileSystem,firstBlock * BlockSize,SEEK_SET);

    fread(str,1,BlockSize,fileSystem);  //只读取n个字节
//    cout<<p->filepos<<" "<<n<<endl;

    for(long i = p->filepos; i < p->filepos + n; i++) {
        printf("%c",str[i]);
    }
//    str[n] = '\0';  //因为之前的存储内容曾经将str填满过，所以这里需要主动字符串结尾符，否则可能会将原先的内容输出

//    printf("%s\n",str);
//    printf("ppp");
}
int getFatLength()
{
    return sys.fatblocknum * BlockSize / sizeof(int);
}
int appendBlock(struct FCBBlock *lp,int n)
{
    //n为需要增加的块数

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
    //n为需要写入的字节数

    /*写入n个字节
    * 需要判断当前文件所占fat块是否能存下这n个字节
    * 如果不能存下，就需要为这个文件申请更多的fat块
    * 注：fat块存储的块号就映射着文件分区块
    */

    struct FCBBlock *p = fcblist->next;

    while(p && p->fileid != fileId) {
        p = p->next;
    }

    if(p == NULL) {
        printf("the file does not exist\n");
        return -1;
    }
    int allBlock = getBlock(p->fileinfo.filelen + n);   //得到这个文件总共要占用的块数
    int hasBlock = getBlock(p->fileinfo.filelen); //得到这个文件已经占用的块数

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
        return 1;//尚未到达文件尾
    }
    return 0;//此时文件指针指向文件尾
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

    if(offset < 0) {   //不能设置文件指针为负
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

    if(offset > p->fileinfo.filelen) {  //不能设置文件指针大于文件长度
        return -1;
    }

    p->filepos = offset;
    return 1;
}
long getFileLength(char filename[])
{
    //首先到打开已文件表项查找，如果有就无须到根目录区查找

    struct FCBBlock *p = fcblist->next;

    while(p && strcmp(p->fileinfo.filename,filename)) {
        p = p->next;
    }
    if(p != NULL) {
        return p->fileinfo.filelen;
    }


    //已打开文件表项无法找到，就到根目录区查找

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
    char str[] = "fileName            fileLength        标识符\n";
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
    printf("\t      1--挂载文件系统");
    printf("\t          2--卸载文件系统\n");
    printf("\t      3--显示目录");
    printf("\t          4--新建文件\n");
    printf("\t      5--删除文件");
    printf("\t          6--读取文件\n");
    printf("\t      7--写入文件");
    printf("\t          8--打开文件\n");
    printf("\t      9--关闭文件");
    printf("\t          10--设置文件指针\n");
    printf("\t      11--判断文件结束");
    printf("\t          12--取得文件长度\n");
    printf("\t      13--显示已打开文件表项");
    printf("\t  14--退出\n");
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
