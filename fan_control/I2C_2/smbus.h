#ifndef __SMBUS_H_
#define __SMBUS_H_

#define SMB_BASE_ADDR       0xF000

#define HST_SYS_REG         SMB_BASE_ADDR+0x00
#define HST_CNT_REG         SMB_BASE_ADDR+0x02
#define HST_CMD_REG         SMB_BASE_ADDR+0x03
#define XMIT_SLVA_REG       SMB_BASE_ADDR+0x04
#define HST_D0_REG          SMB_BASE_ADDR+0x05

#define SMB_CMD         0x48
//#define SLAVE_ADDR            0x40 //i2c设备的地址



#define MAX_TIMEOUT         100

#define SMBHSTCNT_KILL      0x02

#define SMBHSTSTS_BYTE_DONE 0x80
#define SMBHSTSTS_INUSE_STS 0x40
#define SMBHSTSTS_SMBALERT_STS  0x20
#define SMBHSTSTS_FAILED    0x10
#define SMBHSTSTS_BUS_ERR   0x08
#define SMBHSTSTS_DEV_ERR   0x04
#define SMBHSTSTS_INTR      0x02
#define SMBHSTSTS_HOST_BUSY 0x01

#define STATUS_FLAGS        (SMBHSTSTS_BYTE_DONE | SMBHSTSTS_FAILED | \
        SMBHSTSTS_BUS_ERR | SMBHSTSTS_DEV_ERR | \
        SMBHSTSTS_INTR)

#define GLOBAL_Config  0x20  //0010 0000  2:1位为设置看门狗，这里先禁止此功能
#define PWM_Freq  0x66  //0110 0110  PWM频率设置,设置为25kHz的时候无法控制风扇转速，且会检测到风扇失效
//#define FAN_Config  0x20  //0010 0000  0.5s起转，其他为默认值，可设置风扇失效检测，暂时未使能
#define FAN_Config  0x28  //0010 1000 使能风扇失效检测
#define FAN_Dynamic  0x2C //0010 1100     4C 0100 1100默认值，速度范围没看明白
#define FAN_FAULT_Mask  0x00 //0000 0000不屏蔽风扇失效，触发FAN_FAIL输出（默认值为0x3f，屏蔽）
#define FFO_SSR  0x4A//0100 1010失效风扇占空比为100（默认值0100 0101，继续当前工作模式）


int cpld_i2c_read(int slva,int offset);
void cpld_i2c_write(int slva,int offset,int data);
int iopl (int level);
#endif
