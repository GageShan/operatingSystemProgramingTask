#define BlockSize  512
#define DirSize 32
#define RootSize 2
struct ReserveBlock{
   int sysblocknum;/*文件系统总块数*/
   int resblocknum;/*保留块块数*/
   int fatblocknum;/*FAT表块数*/
   int rootblocknum;/*根目录区块数*/
   char fillchar[BlockSize-4*sizeof(int)];/*填充字节*/
};
struct DirBlock{/*外存*/
       char filename[11];  /*文件名限长11个字符*/
       char fillchar[DirSize-2*sizeof(int)-sizeof(long int)-14];/*填充字节*/
       char dirflag;       /*目录标记*/
       char month,day;     /*月、日*/
       int year;           /*年*/
       int firstblockaddr;  /*文件首块块号*/
       long filelen;       /*文件长度*/
};
struct FCBBlock{/*内存*/
   int fileid;         /*文件标识*/
   struct DirBlock fileinfo; /*目录信息*/
   long filepos;             /*文件读写指针*/
   int fdtblockaddr;         /*目录项所在块号*/
   int fdtblockindex;        /*目录项所在块内序号*/
   struct FCBBlock *next;/*指向下一个文件控制块的指针*/
};

