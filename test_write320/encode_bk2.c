#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <fcntl.h>

#include "pdma-ioctl.h"

//#define  NODEV
#define DEV

int main(int argc, char **argv){	
    int fd;
    int i, ret;
    int encodelen;
    unsigned char *buf, *outbuf;
    struct pdma_info info;
    struct pdma_rw_reg ctrl;
    int j, k;
    int count = 35;

/*    if(argc != 2){
        printf("input count!\n");
        return 0;
    }
    int count = atoi(argv[1]);
*/

#ifdef DEV
	fd = open("/dev/pdma", O_RDWR);
	if (fd == -1){
		printf("open failed for pdma device\n");
        return -1;
    }
    
    ret = ioctl(fd, PDMA_IOC_INFO, (unsigned long)&info);
    if(ret == -1){
        printf("get info failed\n");
        close(fd);
        return -1;
    }
#endif

#ifdef NODEV
    info.wt_block_sz = 4096;
    count = 1;
#endif

    buf = (unsigned char *)malloc(info.wt_block_sz);
    //outbuf = (unsigned char *)malloc(272);
    outbuf = (unsigned char *)malloc(info.wt_block_sz);
    if(!buf){
        printf("alloc buf failed\n");
        close(fd);
        return -1;
    }
    
    encodelen = getoutput(outbuf);
    printf("outbuf data:\n");
    for(i = 0; i < encodelen; i++){
        if(outbuf[i] == 0) outbuf[i] = 0x80;
        else outbuf[i] = 0x7f;
        printf("%x ", outbuf[i]);
	}
    printf("\n encodelen : %d\n", encodelen);
    
    for(i = 0; i < encodelen; i+=8){
        j = 7; 
        for(k = i; k < i+8; k++){
            buf[k] = outbuf[i+j];
            j--;
        }
    }
    
    printf("buf data: \n");
    for(i = 0; i < encodelen; i++)
        printf("%x ", buf[i]);
    printf("\n");
    
    ctrl.type = 1; //write
    ctrl.addr = 0;
    ctrl.val = 0x02200000; 
    
    //fill buf
    j = 0; k = 0;
    while(k < count){
        for(i = 0; i < info.wt_block_sz; i++){
            buf[i] = buf[j%encodelen];
            j++;
            //if(i < 1024) printf("buf[%d]: %x\t\t", i, buf[i]);
        }
        j = j%encodelen;
#ifdef NODEV
        k++;
#endif

#ifdef DEV       
        ret = write(fd, buf, info.wt_block_sz);
        if(ret != info.wt_block_sz){
            printf("write failed\n");
            return -1;
        }
        printf("%d write finish\n", k);

        for(i = 0; i < info.wt_block_sz/encodelen; i++){
            ret = ioctl(fd, PDMA_IOC_RW_REG, (unsigned long)&ctrl);
            if(ret == -1){
                printf("pdma_ioc_rw_reg error\n");
                return -1;
            }
        }
        k++;
        //up to encodelen, more ino_rw_reg
        if(k%(info.wt_block_sz%encodelen) == 0)
            ret = ioctl(fd, PDMA_IOC_RW_REG, (unsigned long)&ctrl);
#endif
    }
    close(fd);
    return 0;
}
