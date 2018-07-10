#include <stdio.h>  
#include <sys/sysinfo.h>
//#include <linux/unistd.h>     /* 包含调用 _syscallX 宏等相关信息*/  
//#include <linux/kernel.h>     /* 包含sysinfo结构体信息*/  

//_syscall1(int, sysinfo, struct sysinfo*, info);  

int main(int argc, char *agrv[])  
 
{       
	struct sysinfo s_info;  
	int error;  
	error = sysinfo(&s_info);  
	printf("\ncode error=%d\n",error);  
	printf("Uptime = %lus\nLoad: 1 min%lu / 5 min %lu / 15 min %lu\n"  
		"RAM: total %lu / free %lu /shared%lu\n"  
		"Memory in buffers = %lu\nSwap:total%lu/free%lu\n"  
		"Number of processes = %d\n",  
		s_info.uptime, s_info.loads[0],  
		s_info.loads[1], s_info.loads[2],  
		s_info.totalram, s_info.freeram,
		s_info.sharedram, s_info.bufferram,
		s_info.totalswap, s_info.freeswap,  
		s_info.procs);  

	return 0;    
}  
