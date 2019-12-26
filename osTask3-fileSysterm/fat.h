#define BlockSize  512
#define DirSize 32
#define RootSize 2
struct ReserveBlock{
   int sysblocknum;/*�ļ�ϵͳ�ܿ���*/
   int resblocknum;/*���������*/
   int fatblocknum;/*FAT�����*/
   int rootblocknum;/*��Ŀ¼������*/
   char fillchar[BlockSize-4*sizeof(int)];/*����ֽ�*/
};
struct DirBlock{/*���*/
       char filename[11];  /*�ļ����޳�11���ַ�*/
       char fillchar[DirSize-2*sizeof(int)-sizeof(long int)-14];/*����ֽ�*/
       char dirflag;       /*Ŀ¼���*/
       char month,day;     /*�¡���*/
       int year;           /*��*/
       int firstblockaddr;  /*�ļ��׿���*/
       long filelen;       /*�ļ�����*/
};
struct FCBBlock{/*�ڴ�*/
   int fileid;         /*�ļ���ʶ*/
   struct DirBlock fileinfo; /*Ŀ¼��Ϣ*/
   long filepos;             /*�ļ���дָ��*/
   int fdtblockaddr;         /*Ŀ¼�����ڿ��*/
   int fdtblockindex;        /*Ŀ¼�����ڿ������*/
   struct FCBBlock *next;/*ָ����һ���ļ����ƿ��ָ��*/
};

