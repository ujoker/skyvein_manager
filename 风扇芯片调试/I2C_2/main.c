/*
*max31790风扇芯片API
*author：HL
*输入芯片ID从1开始计，项目中用到3个，ID：1--3
*输入风扇ID从1开始计，项目中每个芯片用到5个风扇，ID：1--5
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "smbus.h"

unsigned short SLAVE_ADDR[1] = {0x40}; //芯片i2c地址，现在只有一个芯片。{0x40,0x46,0x58}
unsigned short OFFSET_ADDR[5] = {0x40,0x42,0x44,0x46,0x48}; //PWM目标寄存器地址
unsigned short TACH_REG[5] = {0x18,0x1a,0x1c,0x1e,0x20}; //TACH计数寄存器
unsigned short CONF_REG[5] = {0x02,0x03,0x04,0x05,0x06}; //风扇配置寄存器
unsigned short DYN_REG[5] = {0x08,0x09,0x0a,0x0b,0x0c}; //风扇动态寄存器

typedef struct MAX_TEST
{
	int STATE;
	int* DIS_POS;
}MT;

typedef struct ALL_TEST
{
	int STATE;
	int** DIS_POS;
}AT;

/*float转int,四舍五入*/
int FloatToInt(float in_value)
{
	int out_value;
	out_value = (int)(in_value+0.5);  //四舍五入
	return out_value;
}

/*打印帮助信息，调试用*/
int help_info(void){
	printf("\nUsage: i2ctest SLAVE_ID OFFSET\n");
	printf("\nOr:    i2ctest -t SLAVE_ID \n");
	printf("\nOr:    i2ctest -r SLAVE_ID \n");
	printf("\nOr:    i2ctest SLAVE_ID OFFSET DATA \n");
	printf("\nOr:    i2ctest -c SLAVE_ID FAN_ID DATA \n");
    printf("\nRead/Write or control the i2c slave\nSLAVE_ID = [0x40,0x46,0x58]\nFAN_ID = [0x40,0x42,0x44,0x46]\n");
	printf("\nFor example\n");
	printf("\ti2ctest 0 0x05\t\t\t\tRead the register: 0x05 of the address: 0x40\n");
	printf("\ti2ctest -t 0\t\t\t\tTest the address: 0x40,Fault status test\n");
	printf("\ti2ctest -r 0\t\t\t\tTest the address: 0x40,Fan Dropped test\n");
	printf("\ti2ctest 0 0x05 18\t\t\tWrite 18 to the register: 0x05 of the address: 0x40\n");
	printf("\ti2ctest -c 0 0 18\t\t\tWrite pwm duty cycle 18 to the fan1 of the address: 0x40\n\n");
	
	return 0;
}

/*将命令行输入转换为程序可理解的实际十六进制数，仅限于0~9，不支持a~f，只做测试用，不予改进*/
int CmdToInt(char *argv){
	int cmd_value;
	char *s = argv;
	cmd_value = ((atoi(s+2) / 10) << 4) + atoi(s+2) % 10;//输入格式是0x**，所以是s+2,忽略前面的0x
	return cmd_value;
}

/*
*io端口初始化
*需要root下执行
*修改当前进程操作端口的权限，3表示可读写
*/
int sumbus_io_init(){
	//
	if (iopl(3) == -1) {
        perror("iopl()");
        return -1;
    }
	return 0;
}

/*
*max31790芯片初始化
*输入：芯片ID（SLAVE_ID）
*输出：无
*/
void max31790_init(unsigned char SLAVE_ID){

	cpld_i2c_write(SLAVE_ADDR[SLAVE_ID-1],0x00,GLOBAL_Config);usleep(20000);  //全局配置初始化，各项初始化间隔20ms
	cpld_i2c_write(SLAVE_ADDR[SLAVE_ID-1],0x01,PWM_Freq);usleep(20000); 
	for(int i=0;i<(sizeof(OFFSET_ADDR)/sizeof(OFFSET_ADDR[0]));i++)
		cpld_i2c_write(SLAVE_ADDR[SLAVE_ID-1],CONF_REG[i],FAN_Config);usleep(20000);  //五个风扇配置寄存器
	for(int i=0;i<(sizeof(OFFSET_ADDR)/sizeof(OFFSET_ADDR[0]));i++)
		cpld_i2c_write(SLAVE_ADDR[SLAVE_ID-1],DYN_REG[i],FAN_Dynamic);usleep(20000); 
	cpld_i2c_write(SLAVE_ADDR[SLAVE_ID-1],0x13,FAN_FAULT_Mask);usleep(20000);
	cpld_i2c_write(SLAVE_ADDR[SLAVE_ID-1],0x14,FFO_SSR);usleep(20000);
	
}

/*
*动态屏蔽风扇失效检测/风扇停转功能
*输入：芯片ID（SLAVE_ID），风扇ID（FAN_ID）
*输出：无
*/
void max31790_MASK_STOP(unsigned char SLAVE_ID,unsigned char FAN_ID){
	cpld_i2c_write(SLAVE_ADDR[SLAVE_ID-1],OFFSET_ADDR[FAN_ID-1],0);
	cpld_i2c_write(SLAVE_ADDR[SLAVE_ID-1],OFFSET_ADDR[FAN_ID-1]+1,0);
	usleep(50000);
}

/*
*风扇失效检测。
*频率问题、风扇掉线都会检测到失效
*输入：芯片ID（SLAVE_ID）
*输出：FAN_TEST，FAN_TEST.STATE表示失效风扇个数，FAN_TEST.DIS_POS表示失效风扇编号（1-5）
*/
MT max31790_TEST(unsigned char SLAVE_ID){
	struct MAX_TEST FAN_TEST;
	int test_val,temp_val;
	int pos=1;
	FAN_TEST.STATE = 0;
	test_val = cpld_i2c_read(SLAVE_ADDR[SLAVE_ID-1],0x11);
	//test_val = 0x1b;
	FAN_TEST.DIS_POS=(int *)calloc((sizeof(OFFSET_ADDR)/sizeof(OFFSET_ADDR[0])),sizeof(int));
	while (test_val != 0){
		if((test_val & 1) == 1){
			FAN_TEST.DIS_POS[FAN_TEST.STATE] = pos;
			FAN_TEST.STATE++;
		}
		pos++;
		test_val >>= 1;
	}
	return FAN_TEST;
}

/*
*失效自检：对所有芯片
*输入：无
*输出：失效风扇个数，失效风扇编号（[芯片ID,风扇ID]）
*/
AT max31790_TEST_ALL(){
	struct ALL_TEST fan_test_all;
	struct MAX_TEST fan_test;
	fan_test_all.STATE = 0;
	fan_test_all.DIS_POS = (int **)calloc((sizeof(SLAVE_ADDR)/sizeof(SLAVE_ADDR[0]))*(sizeof(OFFSET_ADDR)/sizeof(OFFSET_ADDR[0])),sizeof(int *));
	for(int i=0;i<((sizeof(SLAVE_ADDR)/sizeof(SLAVE_ADDR[0]))*(sizeof(OFFSET_ADDR)/sizeof(OFFSET_ADDR[0])));i++)
		fan_test_all.DIS_POS[i] = (int *)calloc(2,sizeof(int));
	for(int i=0;i<(sizeof(SLAVE_ADDR)/sizeof(SLAVE_ADDR[0]));i++){
		fan_test = max31790_TEST(i+1);
		if(fan_test.STATE != 0){			
			for(int j=0;j<fan_test.STATE;j++){
				fan_test_all.DIS_POS[fan_test_all.STATE][0] = i+1; //芯片ID：1-3
				fan_test_all.DIS_POS[fan_test_all.STATE][1] = fan_test.DIS_POS[j]; //写入失效风扇ID
				fan_test_all.STATE++;
			}
		}
	}
	return fan_test_all;
}

/*
*风扇满占空比检测
*输入：芯片ID（SLAVE_ID）;风扇ID（FAN_ID）
*输出：满占空比状态，占空比为100时返回1，其他返回0
*/
int max31790_FULL(unsigned char SLAVE_ID,unsigned char FAN_ID ){
	int fan_full_test,msb_val,lsb_val;
	fan_full_test = 0;
	msb_val = cpld_i2c_read(SLAVE_ADDR[SLAVE_ID-1], OFFSET_ADDR[FAN_ID-1]-10);
	lsb_val = cpld_i2c_read(SLAVE_ADDR[SLAVE_ID-1], OFFSET_ADDR[FAN_ID-1]-10+1);
	if(msb_val == 0xff && lsb_val == 0x80)
		fan_full_test = 1;
	return fan_full_test;
}

/*
*风扇掉线检测
*仅有PWM引脚掉线时，占空比为100，此时计数寄存器的高位恒为0x0a，但当占空比接近100的情况下计数寄存器也会误判为0x0a，因此加一层判断
*输入：芯片ID（SLAVE_ID）
*输出：检测结果（FAN_DROP），FAN_DROP.STATE表示掉线风扇的数量，FAN_DROP.DIS_POS记录掉线风扇的编号（1-5）
*/
MT max31790_DROP(unsigned char SLAVE_ID){
	struct MAX_TEST FAN_DROP;
	int drop_num,TACH_MSB,TACH_LSB,OLD_VAL_MSB,OLD_VAL_LSB;
	drop_num = 1;
	FAN_DROP.STATE = 0;
	FAN_DROP.DIS_POS=(int *)calloc((sizeof(TACH_REG)/sizeof(TACH_REG[0])),sizeof(int));
	for(int i=0;i<(sizeof(TACH_REG)/sizeof(TACH_REG[0]));i++){
		TACH_MSB = cpld_i2c_read(SLAVE_ADDR[SLAVE_ID-1],TACH_REG[i]);
		TACH_LSB = cpld_i2c_read(SLAVE_ADDR[SLAVE_ID-1],TACH_REG[i]+1);
		//printf("TACH_MSB: 0x%02x\n", TACH_MSB);
		//printf("TACH_LSB: 0x%02x\n", TACH_LSB);
		if(TACH_MSB == 0xff && TACH_LSB == 0xe0){
			FAN_DROP.DIS_POS[FAN_DROP.STATE] = drop_num;
			FAN_DROP.STATE++;
		}
		if(TACH_MSB == 0x0a){
			OLD_VAL_MSB = cpld_i2c_read(SLAVE_ADDR[SLAVE_ID-1],OFFSET_ADDR[i]-10);
			OLD_VAL_LSB = cpld_i2c_read(SLAVE_ADDR[SLAVE_ID-1],OFFSET_ADDR[i]-10+1);
			max31790_MASK_STOP(SLAVE_ID, i+1); //风扇ID：1-5
			TACH_MSB = cpld_i2c_read(SLAVE_ADDR[SLAVE_ID-1],TACH_REG[i]);
			if(TACH_MSB == 0x0a){
				FAN_DROP.DIS_POS[FAN_DROP.STATE] = drop_num;
				FAN_DROP.STATE++;
			}else{
				cpld_i2c_write(SLAVE_ADDR[SLAVE_ID-1],OFFSET_ADDR[i],OLD_VAL_MSB);
				cpld_i2c_write(SLAVE_ADDR[SLAVE_ID-1],OFFSET_ADDR[i]+1,OLD_VAL_LSB);				
			}
		}
		drop_num++;
	}
	return FAN_DROP;
}

/*
*掉线自检：对所有芯片
*输入：无
*输出：掉线风扇个数，掉线风扇编号（[芯片ID,风扇ID]）
*/
AT max31790_DROP_ALL(){
	struct ALL_TEST fan_drop_all;
	struct MAX_TEST fan_drop;
	fan_drop_all.STATE = 0;
	fan_drop_all.DIS_POS = (int **)calloc((sizeof(SLAVE_ADDR)/sizeof(SLAVE_ADDR[0]))*(sizeof(OFFSET_ADDR)/sizeof(OFFSET_ADDR[0])),sizeof(int *));
	for(int i=0;i<((sizeof(SLAVE_ADDR)/sizeof(SLAVE_ADDR[0]))*(sizeof(OFFSET_ADDR)/sizeof(OFFSET_ADDR[0])));i++)
		fan_drop_all.DIS_POS[i] = (int *)calloc(2,sizeof(int));
	for(int i=0;i<(sizeof(SLAVE_ADDR)/sizeof(SLAVE_ADDR[0]));i++){
		fan_drop = max31790_DROP(i+1); //芯片ID：1-3
		if(fan_drop.STATE != 0){			
			for(int j=0;j<fan_drop.STATE;j++){
				fan_drop_all.DIS_POS[fan_drop_all.STATE][0] = i+1; //芯片ID：1-3
				fan_drop_all.DIS_POS[fan_drop_all.STATE][1] = fan_drop.DIS_POS[j]; //写入失效风扇ID
				fan_drop_all.STATE++;
			}
		}
	}
	return fan_drop_all;
}

/*
*芯片控制函数
*占空比允许可控最大值为99，保留100作为失控检测
*输入：SLAVE_ID|FAN_ID|cpu_value：芯片ID(1--3)|风扇ID(1--5)|CPU温度
*/
void max31790_control(unsigned char SLAVE_ID,unsigned char FAN_ID,unsigned char cpu_value)
{
	float tmp_value;	
	int speed_value,PWM_MSB,PWM_LSB;
	if(cpu_value > 100 || cpu_value < 0){
		printf("cpu temperature is invalid!value:%d\n", cpu_value);
		exit(-1);
	}
	printf("cpu temperature is %d\n", cpu_value);
	//这里应该有一个cpu温度到pwm占空比的换算，不知道正常的工作环境，先忽略
	tmp_value = cpu_value*5.11; //0-100:0-511
	speed_value = FloatToInt(tmp_value);
	PWM_MSB = speed_value >> 1;
	PWM_LSB = speed_value & 1;
	PWM_LSB = PWM_LSB << 7;
	printf("SLAVE_ADDR: 0x%02x\n", SLAVE_ADDR[SLAVE_ID-1]);
	printf("PWM_MSB: 0x%02x\n", PWM_MSB);
	printf("PWM_LSB: 0x%02x\n", PWM_LSB);
	cpld_i2c_write(SLAVE_ADDR[SLAVE_ID-1],OFFSET_ADDR[FAN_ID-1],PWM_MSB);
	cpld_i2c_write(SLAVE_ADDR[SLAVE_ID-1],OFFSET_ADDR[FAN_ID-1]+1,PWM_LSB);
	usleep(50000);
}

int main(int argc, char* argv[]){
	int val,offset,IO_STATE;
	unsigned char SLAVE_ID, FAN_ID;
	struct MAX_TEST test_value,drop_test;
	struct ALL_TEST all_test_value,all_drop_value;
	IO_STATE = sumbus_io_init();	
	if(IO_STATE < 0)
		return -1;
	switch(argc){
	case 2:{
		if(!strcmp(argv[1],"-h") || !strcmp(argv[1],"--help")){
			help_info();
		}else if(!strcmp(argv[1],"-a")){
			all_test_value = max31790_TEST_ALL();
			printf("ALL_TEST_STATE:%d\n", all_test_value.STATE);
			for(int i=0;i < all_test_value.STATE;i++)
				printf("default fan id:%d %d\n", all_test_value.DIS_POS[i][0],all_test_value.DIS_POS[i][1]);
			for(int i=0;i<((sizeof(SLAVE_ADDR)/sizeof(SLAVE_ADDR[0]))*(sizeof(OFFSET_ADDR)/sizeof(OFFSET_ADDR[0])));i++)
				free(all_test_value.DIS_POS[i]);
			free(all_test_value.DIS_POS);
		}else if(!strcmp(argv[1],"-d")){
			all_drop_value = max31790_DROP_ALL();
			printf("ALL_DROP_STATE:%d\n", all_drop_value.STATE);
			for(int i=0;i < all_drop_value.STATE;i++)
				printf("dropped fan id:%d %d\n", all_drop_value.DIS_POS[i][0],all_drop_value.DIS_POS[i][1]);
			for(int i=0;i<((sizeof(SLAVE_ADDR)/sizeof(SLAVE_ADDR[0]))*(sizeof(OFFSET_ADDR)/sizeof(OFFSET_ADDR[0])));i++)
				free(all_drop_value.DIS_POS[i]);
			free(all_drop_value.DIS_POS);
		}else{
			printf("cmd error!\n");
			exit(-1);
		}
	}break;
	case 3 :{
		if(!strcmp(argv[1],"-t")){
			SLAVE_ID = atoi(argv[2]);
			max31790_init(SLAVE_ID);
			test_value = max31790_TEST(SLAVE_ID);
			printf("TEST_STATE:%d\n", test_value.STATE);			
			for(int i=0;i < test_value.STATE;i++)
				printf("%d\n", test_value.DIS_POS[i]);
			free((void*)test_value.DIS_POS);
		}else if(!strcmp(argv[1],"-r")){
			SLAVE_ID = atoi(argv[2]);
			max31790_init(SLAVE_ID);
			drop_test = max31790_DROP(SLAVE_ID);
			printf("drop_test: %d\n", drop_test.STATE);
			for(int i=0;i < drop_test.STATE;i++)
				printf("%d\n", drop_test.DIS_POS[i]);
			free((void*)drop_test.DIS_POS);
		}else{		
			SLAVE_ID = atoi(argv[1]);
			offset = CmdToInt(argv[2]);
			max31790_init(SLAVE_ID);
			val=cpld_i2c_read(SLAVE_ADDR[SLAVE_ID-1],offset); //返回读取到的寄存器的值
			printf("val: 0x%02x\n", val);
		}
	}break;
	case 4:{
		SLAVE_ID = atoi(argv[1]);
		offset = CmdToInt(argv[2]);
		val = atoi(argv[3]);  //写入寄存器的值
		max31790_init(SLAVE_ID);
		cpld_i2c_write(SLAVE_ADDR[SLAVE_ID-1],offset,val);  //函数响应						
	}break;
	case 5:{
		if(!strcmp(argv[1],"-c")){
			SLAVE_ID = atoi(argv[2]);
			FAN_ID = atoi(argv[3]);
			val = atoi(argv[4]); //写入的PWM占空比
			printf("val: 0x%02x\n", val);
			max31790_init(SLAVE_ID);
			max31790_control(SLAVE_ID, FAN_ID, val);
		}else{
			printf("cmd error!\n");
			exit(-1);
		}
	}break;
	default:
		printf("Please input --help or -h  for help information\n");
	}
	
	return 0;
}
