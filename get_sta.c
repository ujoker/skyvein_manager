#include <stdio.h>
#include <stdlib.h>
#include <sys/statfs.h>
#include <sys/vfs.h>
#include <string.h>
#include <errno.h>

#define DEFAULT_DISK_PATH "/home"

typedef  struct statfs DISK,*pDISK;

//获取包含磁盘空间信息的结构体
//参数二：要获取磁盘信息的位置
//返回值：成功返回0，失败返回-1
int getDiskInfo(pDISK diskInfo,const char *path)
{
    char dpath[100]=DEFAULT_DISK_PATH;//设置默认位置
    int flag=0;

    if(NULL!=path)
    {
        strcpy(dpath,path);
    }

    if(-1==(flag=statfs(dpath,diskInfo)))//获取包含磁盘空间信息的结构体
    {
        perror("getDiskInfo statfs fail");
        return -1;
    }

    return 0;
}

//计算磁盘总空间，非超级用户可用空间，磁盘所有剩余空间，计算结果以字符串的形式存储到三个字符串里面，单位为b
int calDiskInfo(char *diskTotal,char *diskAvail,char *diskFree,pDISK diskInfo)
{
    unsigned long long total=0,avail=0,free=0,blockSize=0;
    int flag=0;

    blockSize=diskInfo->f_bsize;//每块包含字节大小
    total=diskInfo->f_blocks*blockSize;//磁盘总空间
    avail=diskInfo->f_bavail*blockSize;//非超级用户可用空间
    free=diskInfo->f_bfree*blockSize;//磁盘所有剩余空间

    //字符串转换
    flag=sprintf(diskTotal,"%llu",total); //单位b,>>20转换单位为Mb
    flag=sprintf(diskAvail,"%llu",avail);
    flag=sprintf(diskFree,"%llu",free);

    if(-1==flag)
    {
        return 0;
    }
    return 1;

}


int main()
{
    DISK diskInfo;
    char str1[30],str2[30],str3[30];

    memset(&diskInfo,0,sizeof(DISK));

    getDiskInfo(&diskInfo,DEFAULT_DISK_PATH);//获取磁盘信息结构体

    calDiskInfo(str1,str2,str3,&diskInfo);//计算磁盘信息结构体

    printf("\ntotal:%s avail:%s free%s\n",str1,str2,str3);
    return 0;
}