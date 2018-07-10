
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
//#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/io.h> /* linux-specific */
#include <sys/perm.h>
#include "smbus.h"



int chk_smbus_busy(void){
    int status;
    status = inb(HST_SYS_REG);
	//printf("status flags (%02x)\n",status);
    if (status & SMBHSTSTS_HOST_BUSY) {
        printf("SMBus is busy, can't use it!\n");
        return -1;
    }
    status &= STATUS_FLAGS;
    if (status) {
        printf("Clearing status flags (%02x)\n",status);
        outb(status, HST_SYS_REG);
        status = inb(HST_SYS_REG) & STATUS_FLAGS;
        if (status) {
            printf("Failed clearing status flags (%02x)\n",status);
            return -1;
        }
    }
    return 0;
}

int chk_smbus_error(int status,int timeout){
    int result = 0;

    /* If the SMBus is still busy, we give up */
    if (timeout) {
        printf("Transaction timeout\n");
        /* try to stop the current command */
        //printf("Terminating the current operation\n");
        outb(inb(HST_CNT_REG) | SMBHSTCNT_KILL, HST_CNT_REG);
        usleep(1);
        outb(inb(HST_CNT_REG) & (~SMBHSTCNT_KILL), HST_CNT_REG);

        /* Check if it worked */
        status = inb(HST_SYS_REG);
        if ((status & SMBHSTSTS_HOST_BUSY) ||
                !(status & SMBHSTSTS_FAILED))
            printf("Failed terminating the transaction\n");
        outb(STATUS_FLAGS, HST_SYS_REG);
        return -2;
    }

    if (status & SMBHSTSTS_FAILED) {
        result = -3;
        //printf("Transaction failed\n");
    }
    if (status & SMBHSTSTS_DEV_ERR) {
        result = -4;
        //printf("No response\n");
    }
    if (status & SMBHSTSTS_BUS_ERR) {
        result = -5;
        //printf("Lost arbitration\n");
    }

    if (result) {
        /* Clear error flags */
        outb(status & STATUS_FLAGS, HST_SYS_REG);
        status = inb(HST_SYS_REG) & STATUS_FLAGS;
        if (status) {
            printf("Failed clearing status flags at end of transaction (%02x)\n",status);
        }
    }
    return result;
}

int cpld_i2c_read(int slva,int offset){
    int value,ret,status,timeout=0;
    outb(slva+0x01,XMIT_SLVA_REG);
    outb(offset,HST_CMD_REG);
    ret = chk_smbus_busy();
    if (ret < 0)
        return ret;
    outb(SMB_CMD,HST_CNT_REG);
    do {
        usleep(1);
        status = inb(HST_SYS_REG);
    } while ((status & SMBHSTSTS_HOST_BUSY) && (timeout++ < MAX_TIMEOUT));
    ret = chk_smbus_error(status,timeout > MAX_TIMEOUT);
    if (ret < 0 )
        return ret;
    outb(SMBHSTSTS_INTR,HST_SYS_REG);
    value = inb(HST_D0_REG);
    return value;
}

void cpld_i2c_write(int slva,int offset,int data){
    int ret,status,timeout=0;
    outb(slva,XMIT_SLVA_REG);
    outb(offset,HST_CMD_REG);
    outb(data,HST_D0_REG);
    ret = chk_smbus_busy();
    if (ret < 0){
		printf("Function:[%s], ret = %d\n",__FUNCTION__, ret);
        exit(-1);}
    outb(SMB_CMD,HST_CNT_REG);
    do {
        usleep(1);
        status = inb(HST_SYS_REG);
    } while ((status & SMBHSTSTS_HOST_BUSY) && (timeout++ < MAX_TIMEOUT));
    ret = chk_smbus_error(status,timeout > MAX_TIMEOUT);
    if (ret<0){
		printf("Function:[%s], ret = %d\n",__FUNCTION__, ret);
        exit(-1);}
    outb(SMBHSTSTS_INTR,HST_SYS_REG);
}
