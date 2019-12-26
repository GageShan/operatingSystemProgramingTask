#include<cstdio>
#include<cstring>
#include<iostream>
#include<cstdlib>
#include "fat.h"

using namespace std;


void run()
{
    struct ReserveBlock sys;
    char filename[]="fatsys.dat";
    char fillblock[BlockSize];               //BlockSize大小为512
    int *fat;
    int i,j;
    FILE *fp;

    memset(&sys,255,BlockSize);


    printf("Please Input FileSystem's Block Num(5~32767):");
//    scanf("%d",&(sys.sysblocknum));                                  //文件系统总块数
    sys.sysblocknum = 1000;

    sys.resblocknum=1;                // 保留块数


    sys.fatblocknum=sys.sysblocknum/(BlockSize/sizeof(int))+
                 ((sys.sysblocknum%(BlockSize/sizeof(int)))?1:0);   //FAT表块数

    fat=(int*)malloc(BlockSize*sys.fatblocknum);              //为fat分配内存大小


    memset(fat,255,BlockSize*sys.fatblocknum);

    memset(fat,0,sizeof(int)*sys.sysblocknum);


    sys.rootblocknum=RootSize;                //根目录块数，固定为2


    j=sys.resblocknum+sys.fatblocknum+sys.rootblocknum;    //j为总文件系统空间大小

    for (i=0;i<j;i++)
     fat[i]=-1;                             //初始化为-1

    memset(&fillblock,0,BlockSize);             //fillblock表示其他块大小，初始化为0

    fp=fopen(filename,"w+b");


    fwrite(&sys,1,BlockSize,fp); /*写保留块*/


    for (i=0;i<sys.fatblocknum;i++)/*写FAT表所占块*/ {

       fwrite(fat+i*BlockSize/sizeof(int),1,BlockSize,fp);

    }

    j=sys.resblocknum+sys.fatblocknum;       //

    for(i=0;i<(sys.sysblocknum-j);i++)/*写其它块*/ {
        fwrite(&fillblock,1,BlockSize,fp);

    }

    fclose(fp);
    free(fat);
}
int main()
{
    run();

    return 0;
}

