#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <fcntl.h>

#include "pdma-ioctl.h"

//#define  NODEV
#define DEV
//#define DEBUG
#define WFILE

int main(int argc, char **argv){	
    int fd;
    int i, ret;
    int encodelen, K_byte, code_bit;
    unsigned char *buf, *outbuf;
    struct pdma_info info;
    struct pdma_rw_reg ctrl;
    int j, k, count;
    char *filename = "dataByte.txt";
     
#ifdef DEV
	fd = open("/dev/pdma", O_RDWR);
	if (fd == -1){
		printf("open failed for pdma device\n");
        return -1;
    }
    
    /*reset*/
    ctrl.type = 1; //write
    ctrl.addr = 0;
    ctrl.val = 1; 
    ret = ioctl(fd, PDMA_IOC_RW_REG, (unsigned long)&ctrl);
    if(ret == -1){
        printf("pdma_ioc_rw_reg error\n");
        return -1;
    }
    
    ret = ioctl(fd, PDMA_IOC_INFO, (unsigned long)&info);
    printf("wt_block_sz--%d\n", info.wt_block_sz);
    if(ret == -1){
        printf("get info failed\n");
        close(fd);
        return -1;
    }
    
#endif

#ifdef NODEV
    info.wt_block_sz = 16384;
#endif

	//get file size
	K_byte = getfilesize(filename);
	//the code bit len that write into FPGA 
	code_bit = (K_byte << 3) + 4; 
	//after encode, the len is encodelen byte 
	encodelen = code_bit * 4 * 2;  
//#ifdef DEBUG
	printf("\n encodelen : %d\n", encodelen);
    printf("code_len: %d\n", code_bit);
    printf("code_reg: %08x\n", code_bit<<19);
//#endif
    
	//malloc buf	
	if(encodelen%info.wt_block_sz == 0)
		count = encodelen/info.wt_block_sz;
	else count = encodelen/info.wt_block_sz + 1;
    printf("count: %d\n", count);
	
    outbuf = (unsigned char *)malloc(encodelen);
	buf = (unsigned char *)malloc(count * info.wt_block_sz);
    if(!buf){
        printf("alloc buf failed\n");
        close(fd);
        return -1;
    }

#ifdef DEV
    code_bit = code_bit << 19;   
    ctrl.val = code_bit; 
    printf("code_bit : %08x\n", code_bit);
    ret = ioctl(fd, PDMA_IOC_RW_REG, (unsigned long)&ctrl);
    if(ret == -1){
        printf("pdma_ioc_rw_reg error\n");
        return -1;
    }
#endif	

    memset(outbuf, 0, encodelen);
    memset(buf, 0, count * info.wt_block_sz);

    getoutput(filename, K_byte, outbuf);
	for(i = 0; i < encodelen; i++){
        if(outbuf[i] == 1) outbuf[i] = 0x7f;
        else outbuf[i] = 0x80;
	}
    outbuf[encodelen-5] = 0x81;


	//swap every 8byte
    for(i = 0; i < encodelen; i+=8){
        j = 7; 
        for(k = i; k < i+8; k++){
            buf[k] = outbuf[i+j];
            j--;
        }
    }

#ifdef DEBUG    
	printf("outbuf data:\n");
	for(i = 0; i < encodelen; i++){
		printf("%x ", outbuf[i]);
	}
	printf("\n \n");

    printf("buf data: \n");
    for(i = 0; i < encodelen; i++){
        printf("%x ", buf[i]);
    }
    printf("\n");
#endif

#ifdef WFILE
    if(access("writeout.txt", 0) == 0) remove("writeout.txt");
    FILE *tt = fopen("writeout.txt", "w");
    for(i = 0; i < count*info.wt_block_sz; i++){  
        if(i%8 == 0) fprintf(tt, "n=%d, ", i);
        fprintf(tt, "%x ", buf[i]);
        if((i+1)%8 == 0) fprintf(tt, "\n");
	}
    fclose(tt);
#endif

#ifdef DEV      
    /*wiret data*/
    i = 0;
    while(i < count){
		ret = write(fd, buf+i*info.wt_block_sz, info.wt_block_sz); 
		if(ret != info.wt_block_sz){
			printf("write failed\n");
			return -1;
		}
		printf("%d write finish\n", i+1);
		i++;
	}

#endif
    free(buf);
    close(fd);
    return 0;
}
   
