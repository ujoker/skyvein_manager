#include <stdio.h>
#include <unistd.h>

typedef struct CPU_PACKED 
{
	char name[20];
	unsigned int user;
	unsigned int nice;
	unsigned int system;
	unsigned int idle;
	unsigned int iowait;
	unsigned int irq;
	unsigned int softirq;
	
}CPU_OCCUPY;

int cal_cpuoccupy(CPU_OCCUPY *t1, CPU_OCCUPY *t2)
{
	unsigned long dI;
}

void get_cpumessage(CPU_OCCUPY *cpustat)
{
	FILE *fd;
	char buff[256];
	CPU_OCCUPY *cpu_occupy;
	cpu_occupy = cpustat;
	fd = fopen("/proc/stat", "r");
	fgets(buff, sizeof(buff), fd);
	sscanf(buff, "%s %u %u %u %u", cpu_occupy->name, &cpu_occupy->user, &cpu_occupy->nice, &cpu_occupy->system, &cpu_occupy->idle);
	printf("%u %u %u %u\n",cpu_occupy->user, cpu_occupy->nice, cpu_occupy->system, cpu_occupy->idle);
	fclose(fd);
}

int main()
{
	CPU_OCCUPY cpu_stat1;
	CPU_OCCUPY cpu_stat2;
	int cpu;
	get_cpumessage((CPU_OCCUPY *)&cpu_stat1);
	//get_cpumessage((CPU_OCCUPY *)&cpu_stat2);
	//cpu = cal_cpuoccupy((CPU_OCCUPY *)&cpu_stat1, (CPU_OCCUPY *)&cpu_stat2);
	//printf("cpu usage:%d.\n", cpu);
	return 0;
}