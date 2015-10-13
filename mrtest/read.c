#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include "pdma-ioctl.h"

unsigned long reverse(unsigned long x){
    unsigned long y;
    y = (x & 0x01) << 7;
    y |= (x & 0x02) << 5;
    y |= (x & 0x04) << 3;
    y |= (x & 0x08) << 1;
    y |= (x & 0x10) >> 1;
    y |= (x & 0x20) >> 3;
    y |= (x & 0x40) >> 5;
    y |= (x & 0x80) >> 7;
    
    return y;
}

int main(int argc, char *argv[])
{
    struct pdma_info info;
    struct pdma_rw_reg ctrl; 
    char *pval, *buf;
    int fd, len, ret, i, count;
    char *fl = "/dev/pdma";
    char *head = "readout";
    int ct = 0;
    
    /*combine filename*/
    if(argc != 2){
        printf("please input logc:\n");
        return 0;
    }
    char *tail = argv[1];
    char *filename = (char *)malloc(strlen(head) + strlen(tail) + 4);
    strcpy(filename, head);
    strcat(filename, tail);
    strcat(filename, ".txt");
    printf("filename: %s\n", filename);

    /* open */
    fd = open(fl, O_RDWR);
    if (fd == -1) {
        printf("open failed for pdma device %s \n", fl);
        return -1;
    }

    /*get len*/
    ctrl.type = 0;
    ctrl.addr = 0;
    ret = ioctl(fd, PDMA_IOC_RW_REG, (unsigned long)&ctrl);
    if (ret == -1) {
        printf("get info failed\n");
        close(fd);
        return -1;
    }
    

    len = ctrl.val;
    if(len%8) len += 4;
    printf("read len from reg: %d\n", ctrl.val);


    /* get info */
    ret = ioctl(fd, PDMA_IOC_INFO, (unsigned long)&info);
    if (ret == -1) {
        printf("get info failed\n");
        close(fd);
        return -1;
    }

	if(len%info.rd_block_sz)
		count = len/info.rd_block_sz + 1;
	else count = len/info.rd_block_sz;
	printf("need read %d times\n", count);
	
    /* malloc buf */
    buf = (char *)malloc(count * info.rd_block_sz);
    memset(buf, 0, count*info.rd_block_sz);
    if (!buf) {
        printf("alloc buffer failed\n");
        close(fd);
        return -1;
    }

    /* start read */
	i = 0;
	//while(i < count){
		ret = read(fd, buf + i*info.rd_block_sz, info.rd_block_sz);
		if (ret <= 0) {
			printf("read failed\n");
			ret = -1;
		}
		i++;
		printf("%d read finished\n", i);
	//}
	
    /*reset */
    ctrl.type = 1;
    ctrl.addr = 0;
    ctrl.val = 1;
    ret = ioctl(fd, PDMA_IOC_RW_REG, (unsigned long)&ctrl);
    if (ret == -1) {
        printf("get info failed\n");
        close(fd);
        return -1;
    }
    
    
    if(access(filename, 0) == 0) remove(filename);
    FILE *fn = fopen(filename, "w");
	unsigned char *val32 = (unsigned char *)buf;
	for (i = 0; i < info.rd_block_sz; i++) {  //len*2
		if(i%8 == 0) fprintf(fn, "n= %d, ", i);
		fprintf(fn, "%x(%d) ", val32[i], reverse(val32[i]));
		if((i+1)%8 == 0) fprintf(fn, "\n");
	}	
    close(fn);

    //free(buf);
    close(fd);
    return ret;
}
