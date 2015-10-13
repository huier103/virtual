#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <fcntl.h>

#include "pdma-ioctl.h"

//#define  NODEV
#define DEV
#define DEBUG

int main(int argc, char **argv){	
    int fd;
    int i, ret;
    int encodelen, code_bit;
    unsigned char *buf, *outbuf;
    struct pdma_info info;
    struct pdma_rw_reg ctrl;
    int j, k;

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
    info.wt_block_sz = 8300;
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
    code_bit = encodelen/4;
    printf("code_len: %d\n", code_bit);
    printf("code_16x: %08x\n", code_bit<<19);

#ifdef NODEV
    code_bit = code_bit << 19;
    printf("code_bit : %08x\n", code_bit);
#endif

#ifdef DEBUG
    printf("outbuf data:\n");
//    FILE *tt = fopen("data1024.txt", "w");
    for(i = 0; i < encodelen; i++){
        if(outbuf[i] == 0) outbuf[i] = 0x80;
        else outbuf[i] = 0x7f;
        printf("%x ", outbuf[i]);
//        fprintf(tt, "%x", outbuf[i]);
//        if((i+1)%8 == 0) fprintf(tt, "\n");
	}
//    close(tt);
    printf("\n encodelen : %d\n", encodelen);
#endif

    for(i = 0; i < encodelen; i+=8){
        j = 7; 
        for(k = i; k < i+8; k++){
            buf[k] = outbuf[i+j];
            j--;
        }
    }

#ifdef DEBUG    
    printf("buf data: \n");
    for(i = 0; i < encodelen; i++){
        printf("%x ", buf[i]);
    }
    printf("\n");
#endif

#ifdef DEV       
    ret = write(fd, buf, encodelen);
    if(ret < 0){
        printf("write failed\n");
        return -1;
    }
    printf("write finish\n");

    code_bit = code_bit << 19;
    
    ctrl.type = 1; //write
    ctrl.addr = 0;
    ctrl.val = code_bit; 
    printf("code_bit : %08x\n", code_bit);
    ret = ioctl(fd, PDMA_IOC_RW_REG, (unsigned long)&ctrl);
    if(ret == -1){
        printf("pdma_ioc_rw_reg error\n");
        return -1;
    }
#endif
    
    close(fd);
    return 0;
}
   
