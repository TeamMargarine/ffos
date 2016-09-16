#define LOG_TAG "nvitemd"

#include <cutils/log.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <string.h>
#include <mtd/mtd-user.h>
#include <time.h>
#include <math.h>
#include <hardware_legacy/power.h>
#include "crc32b.h"

/* #define TEST_MODEM    1 */
#define DATA_BUF_LEN    (2048 * 10)
#define min(A,B)	(((A) < (B)) ? (A) : (B))

typedef uint16_t file_number_t;
typedef struct {
        file_number_t type;	/* 0 : fixnv ; 1 : runtimenv ; 2 : productinfo */
        uint16_t req_type;	/* 0 : data ; 1 : sync command */
        uint32_t offset;
        uint32_t size;
} request_header_t;

#define FIXNV_SIZE              (64 * 1024)
#define RUNTIMENV_SIZE		    (256 * 1024)
#define PRODUCTINFO_SIZE	    (3 * 1024)

#ifdef CONFIG_EMMC
/* see g_sprd_emmc_partition_cfg[] in u-boot/nand_fdl/fdl-2/src/fdl_partition.c */ 
#define PARTITION_MODEM		    "/dev/block/mmcblk0p2"
#define PARTITION_DSP		    "/dev/block/mmcblk0p3"
#define PARTITION_FIX_NV1	    "/dev/block/mmcblk0p4"
#define PARTITION_FIX_NV2	    "/dev/block/mmcblk0p5"
#define PARTITION_RUNTIME_NV1	"/dev/block/mmcblk0p6"
#define PARTITION_RUNTIME_NV2	"/dev/block/mmcblk0p7"
#define PARTITION_PROD_INFO1	"/dev/block/mmcblk0p8"
#define PARTITION_PROD_INFO2	"/dev/block/mmcblk0p9"
#define EMMC_MODEM_SIZE		    (8 * 1024 * 1024)
#else
#define PARTITION_MODEM		"modem"
#define PARTITION_DSP		"dsp"
#define F2R1_MODEM_SIZE		(3500 * 1024)
#define F4R2_MODEM_SIZE		(8 * 1024 * 1024)
#endif

#define MODEM_IMAGE_OFFSET  0
#define MODEM_BANK          "guestOS_2_bank"
#define DSP_IMAGE_OFFSET    0
#define DSP_BANK            "dsp_bank"
#define BUFFER_LEN	        (512)
#define REQHEADINFOCOUNT	(3)

unsigned char data[DATA_BUF_LEN];
static int req_head_info_count = 0;
int mcp_size = 512, modem_image_len = 0;
char gch, modem_mtd[256], dsp_mtd[256];
FILE * gfp;
char gbuffer[BUFFER_LEN];

/* nand version */
unsigned short indexvalue = 0;
unsigned char fixnv_buffer[FIXNV_SIZE + 4];
unsigned long fixnv_buffer_offset;
unsigned char phasecheck_buffer[PRODUCTINFO_SIZE + 8];
unsigned long phasecheck_buffer_offset;
static int update_finxn_flag = 0;
/* nand version */

int check_mocor_reboot(void);
char *loadFile(char *pathName, int *fileSize);

void log2file(void)
{
	if (gfp == NULL)
		return;

	fputs(gbuffer, gfp);
	fflush(gfp);
}

void req_head_info(request_header_t head) 
{
	memset(gbuffer, 0, BUFFER_LEN);
	if (head.req_type == 0)
		sprintf(gbuffer, "\ntype = %d  req_type = %d  offset = %d  size = %d\n", 
            head.type, head.req_type, head.offset, head.size);
	else
		sprintf(gbuffer, "\nreq_type = %d\n", head.req_type);
	log2file();
}

int copyfile(const char *src, const char *dst)
{
    int ret, length, srcfd, dstfd;
	struct stat statbuf;

	if ((src == NULL) || (dst == NULL))
		return -1;

	if (strcmp(src, dst) == 0)
		return -1;

	if (strcmp(src, "/proc/mtd") == 0) {
		ret = 800;
		length = ret;
	} else {
		ret = stat(src, &statbuf);
		if (ret == ENOENT) {
			ALOGE("src file is not exist\n");
			return -1;
		}
		length = statbuf.st_size;
	}

	if (length == 0) {
		ALOGE("src file length is 0\n");
		return -1;
	}

	memset(data, 0, DATA_BUF_LEN);
	srcfd = open(src, O_RDONLY);
	if (srcfd < 0) {
		ALOGE("open %s error\n", src);
		return -1;
	}

	ret = read(srcfd, data, length);
	close(srcfd);
	if (ret != length) {
		ALOGE("read error length = %d  ret = %d\n", length, ret);
		return -1;
	}

	if (access(dst, 0) == 0)
		remove(dst);

	dstfd = open(dst, O_RDWR | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
	if (dstfd < 0) {
		ALOGE("create %s error\n", dst);
		return -1;
	}

	ret = write(dstfd, data, length);
	close(dstfd);
	if (ret != length) {
		ALOGE("write error length = %d  ret = %d\n", length, ret);
		return -1;
	}

	return length;
}

unsigned short calc_checksum(unsigned char *dat, unsigned long len)
{
	unsigned long checksum = 0;
	unsigned short *pstart, *pend;

	if (0 == (unsigned long)dat % 2)  {
		pstart = (unsigned short *)dat;
		pend = pstart + len / 2;
		while (pstart < pend) {
			checksum += *pstart;
			pstart ++;
		}
		if (len % 2)
			checksum += *(unsigned char *)pstart;
	} else {
		pstart = (unsigned char *)dat;
		while (len > 1) {
			checksum += ((*pstart) | ((*(pstart + 1)) << 8));
			len -= 2;
			pstart += 2;
		}
		if (len)
			checksum += *pstart;
	}
	checksum = (checksum >> 16) + (checksum & 0xffff);
	checksum += (checksum >> 16);

	return (~checksum);
}


int update_fixnvfile(const char *src, unsigned long size)
{
    int srcfd;
	unsigned long ret, length;
	unsigned long index, crc;
	unsigned short sum = 0, *dataaddr;
	unsigned char *flag;

	sum = calc_checksum(fixnv_buffer, size - 4);
	dataaddr = (unsigned short *)(fixnv_buffer + size - 4);
	*dataaddr = sum;
	dataaddr = (unsigned short *)(fixnv_buffer + size - 2);
	*dataaddr = (indexvalue + 1) % 100; /* 0 -- 99 */

	flag = (unsigned char *)(fixnv_buffer + size); *flag = 0x5a;
	flag ++; *flag = 0x5a;
	flag ++; *flag = 0x5a;
	flag ++; *flag = 0x5a;

	if (req_head_info_count < REQHEADINFOCOUNT) {
		req_head_info_count++;
		memset(gbuffer, 0, BUFFER_LEN);
		sprintf(gbuffer, "sum = 0x%04x  indexvalue = %d\n", sum, indexvalue);
		log2file();
	}

#ifdef CONFIG_EMMC
	srcfd = open(src, O_RDWR, 0);
#else
	srcfd = open(src, O_RDWR | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
#endif
	if (srcfd < 0) {
		memset(gbuffer, 0, BUFFER_LEN);
		sprintf(gbuffer, "create %s error\n", src);
		log2file();
		return -1;
	}

	ret = write(srcfd, fixnv_buffer, size + 4);
	if (ret != (size + 4)) {
		memset(gbuffer, 0, BUFFER_LEN);
		sprintf(gbuffer, "write %s error size = %d  ret = %d\n", src, (size + 4), ret);
		log2file();
	} else {
		if (req_head_info_count < REQHEADINFOCOUNT) {
			req_head_info_count++;
			memset(gbuffer, 0, BUFFER_LEN);
			sprintf(gbuffer, "write %s right size = %d  ret = %d\n", src, (size + 4), ret);
			log2file();
		}
	}
	fsync(srcfd);
	close(srcfd);

	return 1;
}

int update_backupfile(const char *dst, unsigned long size)
{
	int dstfd;
	unsigned long ret, length;

	if (req_head_info_count < REQHEADINFOCOUNT) {
		req_head_info_count++;
		memset(gbuffer, 0, BUFFER_LEN);
		sprintf(gbuffer, "update bkupfixnv = %s  size = %d\n", dst, size);
		log2file();
	}

#ifdef CONFIG_EMMC
	dstfd = open(dst, O_RDWR, 0);
#else
	dstfd = open(dst, O_RDWR | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
#endif
	if (dstfd < 0) {
		memset(gbuffer, 0, BUFFER_LEN);
		sprintf(gbuffer, "create %s error\n", dst);
		log2file();
		return -1;
	}

	ret = write(dstfd, fixnv_buffer, size + 4);
	if (ret != (size + 4)) {
		memset(gbuffer, 0, BUFFER_LEN);
		sprintf(gbuffer, "write %s error size = %d  ret = %d\n", dst, (size + 4), ret);
		log2file();
	} else {
		if (req_head_info_count < REQHEADINFOCOUNT) {
			req_head_info_count++;
			memset(gbuffer, 0, BUFFER_LEN);
			sprintf(gbuffer, "write %s right size = %d  ret = %d\n", dst, (size + 4), ret);
			log2file();
		}
	}
	fsync(dstfd);
	close(dstfd);

	return 1;
}

#ifdef CONFIG_EMMC
int update_productinfofile_emmc(const char *src, const char *dst, unsigned long size)
{
    int srcfd, dstfd;
	unsigned long ret, length;
	unsigned long index, crc;
	unsigned short sum = 0, *dataaddr;

	if (strcmp(src, dst) == 0)
		return -1;

	/* 5a is defined by raw data */
	phasecheck_buffer[size] = 0x5a;
	phasecheck_buffer[size + 1] = 0x5a;
	phasecheck_buffer[size + 2] = 0x5a;
	phasecheck_buffer[size + 3] = 0x5a;

	/* crc32 */
	crc = crc32b(0xffffffff, phasecheck_buffer, size + 4);
	phasecheck_buffer[size + 4] = (crc & (0xff << 24)) >> 24;
	phasecheck_buffer[size + 5] = (crc & (0xff << 16)) >> 16;
	phasecheck_buffer[size + 6] = (crc & (0xff << 8)) >> 8;
	phasecheck_buffer[size + 7] = crc & 0xff;

	srcfd = open(src, O_RDWR, 0);
	if (srcfd < 0) {
		memset(gbuffer, 0, BUFFER_LEN);
		sprintf(gbuffer, "create %s error\n", src);
		log2file();
		return -1;
	}

	ret = write(srcfd, phasecheck_buffer, size + 8);
	fsync(srcfd);
	if (ret != (size + 8)) {
		memset(gbuffer, 0, BUFFER_LEN);
		sprintf(gbuffer, "write %s error size = %d  ret = %d\n", src, (size + 8), ret);
		log2file();
	} else {
		if (req_head_info_count < REQHEADINFOCOUNT) {
			req_head_info_count++;
			memset(gbuffer, 0, BUFFER_LEN);
			sprintf(gbuffer, "write %s right size = %d  ret = %d\n", src, (size + 8), ret);
			log2file();
		}
	}
	close(srcfd);

	dstfd = open(dst, O_RDWR, 0);
	if (dstfd < 0) {
		memset(gbuffer, 0, BUFFER_LEN);
		sprintf(gbuffer, "create %s error\n", dst);
		log2file();
		return -1;
	}

	ret = write(dstfd, phasecheck_buffer, size + 8);
	fsync(dstfd);
	if (ret != (size + 4)) {
		memset(gbuffer, 0, BUFFER_LEN);
		sprintf(gbuffer, "write %s error size = %d  ret = %d\n", dst, (size + 4), ret);
		log2file();
	} else {
		if (req_head_info_count < REQHEADINFOCOUNT) {
			req_head_info_count++;
			memset(gbuffer, 0, BUFFER_LEN);
			sprintf(gbuffer, "write %s right size = %d  ret = %d\n", dst, (size + 4), ret);
			log2file();
		}
	}
	close(dstfd);
	return 1;
}
#else
int update_productinfofile(const char *src, const char *dst, unsigned long size)
{
    int srcfd, dstfd;
	unsigned long ret, length;
	unsigned long index, crc;
	unsigned short sum = 0, *dataaddr;

	if (strcmp(src, dst) == 0)
		return -1;

	sum = calc_checksum(phasecheck_buffer, size);
	dataaddr = (unsigned short *)(phasecheck_buffer + size);
	*dataaddr = sum;
	dataaddr = (unsigned short *)(phasecheck_buffer + size + 2);
	*dataaddr = (indexvalue + 1) % 100; /* 0 -- 99 */

	if (req_head_info_count < REQHEADINFOCOUNT) {
		req_head_info_count++;
		memset(gbuffer, 0, BUFFER_LEN);
		sprintf(gbuffer, "sum = 0x%04x  indexvalue = %d  new = %d\n", sum, indexvalue, *dataaddr);
		log2file();
	}

	srcfd = open(src, O_RDWR | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);;
	if (srcfd < 0) {
		memset(gbuffer, 0, BUFFER_LEN);
		sprintf(gbuffer, "create %s error\n", src);
		log2file();
		return -1;
	}

	ret = write(srcfd, phasecheck_buffer, size + 4);
	fsync(srcfd);
	if (ret != (size + 4)) {
		memset(gbuffer, 0, BUFFER_LEN);
		sprintf(gbuffer, "write %s error size = %d  ret = %d\n", src, (size + 4), ret);
		log2file();
	} else {
		if (req_head_info_count < REQHEADINFOCOUNT) {
			req_head_info_count++;
			memset(gbuffer, 0, BUFFER_LEN);
			sprintf(gbuffer, "write %s right size = %d  ret = %d\n", src, (size + 4), ret);
			log2file();
		}
	}
	close(srcfd);

	dstfd = open(dst, O_RDWR | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
	if (dstfd < 0) {
		memset(gbuffer, 0, BUFFER_LEN);
		sprintf(gbuffer, "create %s error\n", dst);
		log2file();
		return -1;
	}

	ret = write(dstfd, phasecheck_buffer, size + 4);
	fsync(dstfd);
	if (ret != (size + 4)) {
		memset(gbuffer, 0, BUFFER_LEN);
		sprintf(gbuffer, "write %s error size = %d  ret = %d\n", dst, (size + 4), ret);
		log2file();
	} else {
		if (req_head_info_count < REQHEADINFOCOUNT) {
			req_head_info_count++;
			memset(gbuffer, 0, BUFFER_LEN);
			sprintf(gbuffer, "write %s right size = %d  ret = %d\n", dst, (size + 4), ret);
			log2file();
		}
	}

	close(dstfd);
	return 1;
}
#endif

char select_log_file(void) 
{
	int ret;
	FILE *fp;
	char ch;

	ret = opendir("/data/local");
	if (ret <= 0)
		ret = mkdir("/data/local", 771);

	ret = opendir("/data/local/tmp");
	if (ret <= 0)
		ret = mkdir("/data/local/tmp", 771);

	ret = access("/data/local/tmp/index.txt", 0);
	if (ret != 0) {
		ALOGE("/data/local/tmp/index.txt is not exist, create it\n");
		fp = fopen("/data/local/tmp/index.txt", "w+");
		if (fp == NULL) {
			ALOGE("can not creat /data/local/tmp/index.txt\n");
			return 0;
		} else {
			fputc('1', fp);
			fclose(fp);
			return '1';
		}
	}
	ALOGE("/data/local/tmp/index.txt is exist\n");
	fp = fopen("/data/local/tmp/index.txt", "r+");
	fseek(fp, 0, SEEK_SET);
	ch = fgetc(fp);
	ch++;
	if (ch == '6')
		ch = '1';
	fseek(fp, 0, SEEK_SET);
	fputc(ch, fp);
	fclose(fp);
	
    return ch;
}

/* cat /proc/mtd */ 
char *get_proc_mtd(void) 
{
	int fd, fileSize;
	char *buf;

    copyfile("/proc/mtd", "/data/local/tmp/mtd.txt");
	fd = open("/data/local/tmp/mtd.txt", O_RDONLY);
    if (fd < 0) {
		ALOGE("can not open /data/local/tmp/mtd.txt\n");
		return 0;
	}
	
    fileSize = lseek(fd, 0, SEEK_END);
	if (fileSize < 0) {
		ALOGE("fileSize is error\n");
		return 0;
	}
	buf = (char *)malloc((fileSize) + 1);
	if (buf == 0) {
		ALOGE("Malloc buffer failed\n");
		return 0;
	}
	if (lseek(fd, 0, SEEK_SET) < 0) {
		ALOGE("lseek header error\n");
		return 0;
	}
	if (read(fd, buf, fileSize) != fileSize) {
		free(buf);
		buf = 0;
	} else
		buf[fileSize] = 0;
	
    return buf;
}

int get_mcp_size(char *buf) 
{
	char *pos;
	int pos_len;
	char number[8];
	int nandsize;

	if (buf == 0)
		return 0;

    pos = strstr(buf, "MiB");
	if (pos == 0) {
		ALOGE("failed to find MiB\n");
		return 0;
	}
	pos_len = 0;
	while (*pos != ' ') {
		pos--;
		pos_len++;
	}
	pos++;
	pos_len--;
	memset(number, 0, 8);
	strncpy(number, pos, pos_len);
	nandsize = atoi(number);
	
    return nandsize;
}

void allocat_mtd_partition(char *name, char *mtdpath) 
{
	strcpy(mtdpath, "/dev/mtd/");
	strncat(mtdpath, name, strlen(name));
} 

void get_mtd_partition(char *buf, char *name, char *mtdpath)
{
	char *pos;
	int pos_len;
	
    pos = strstr(buf, name);
	if (pos == 0)
		ALOGE("failed to find %s\n", name);
	while (*pos != ':') {
		pos--;
	}
	pos_len = 0;
	while (*pos != 'm') {
		pos--;
		pos_len++;
	}
	strcpy(mtdpath, "/dev/mtd/");
	strncat(mtdpath, pos, pos_len);
}

int main(int argc, char **argv) 
{
	int pipe_fd = 0, fix_nvitem_fd = 0, runtime_nvitem_fd = 0, runtime_nvitem_bkfd = 0, product_info_fd = 0, fd = 0, ret = 0;
	int r_cnt, w_cnt, res, total, length, rectotal;
	request_header_t req_header;
	char *mtdbuf = NULL;
	unsigned long crc;
	unsigned short *indexaddress;

	ret = nice(-16);
	if ((-1 == ret) && (errno == EACCES))
		ALOGE("set priority fail\n");

    gch = select_log_file();
	gfp = NULL;
	if (gch == '1')
		gfp = fopen("/data/local/tmp/1.txt", "w+");
	else if (gch == '2')
		gfp = fopen("/data/local/tmp/2.txt", "w+");
	else if (gch == '3')
		gfp = fopen("/data/local/tmp/3.txt", "w+");
	else if (gch == '4')
		gfp = fopen("/data/local/tmp/4.txt", "w+");
	else if (gch == '5')
		gfp = fopen("/data/local/tmp/5.txt", "w+");
	else
		ALOGE("nothing is selected\n");
	if (gfp == NULL)
		ALOGE("\nno log file\n");
	
#ifdef CONFIG_EMMC
	modem_image_len = EMMC_MODEM_SIZE;
	memset(gbuffer, 0, BUFFER_LEN);
	sprintf(gbuffer, "modem length : %d\n", modem_image_len);
	log2file();

	memset(modem_mtd, 0, 256);
	strcpy(modem_mtd, PARTITION_MODEM);
	memset(gbuffer, 0, BUFFER_LEN);
	sprintf(gbuffer, "modem emmc dev : %s\n", modem_mtd);
	log2file();
	
    memset(dsp_mtd, 0, 256);
	strcpy(dsp_mtd, PARTITION_DSP);
	memset(gbuffer, 0, BUFFER_LEN);
	sprintf(gbuffer, "dsp emmc dev : %s\n", dsp_mtd);
	log2file();
#else //#ifdef CONFIG_EMMC
	mtdbuf = get_proc_mtd();
	mcp_size = get_mcp_size(mtdbuf);
	if (mcp_size >= 512)
		modem_image_len = F4R2_MODEM_SIZE;
	else
		modem_image_len = F2R1_MODEM_SIZE;
	
    memset(gbuffer, 0, BUFFER_LEN);
	sprintf(gbuffer, "mcp size : %d  modem length : %d\n", mcp_size, modem_image_len);
	log2file();
	
    memset(modem_mtd, 0, 256);
	get_mtd_partition(mtdbuf, PARTITION_MODEM, modem_mtd);
	memset(gbuffer, 0, BUFFER_LEN);
	sprintf(gbuffer, "modem mtd dev : %s\n", modem_mtd);
	log2file();
	
    memset(dsp_mtd, 0, 256);
	get_mtd_partition(mtdbuf, PARTITION_DSP, dsp_mtd);
	memset(gbuffer, 0, BUFFER_LEN);
	sprintf(gbuffer, "dsp mtd dev : %s\n", dsp_mtd);
	log2file();

	if (mtdbuf != 0)
		free(mtdbuf);
#endif //#ifdef CONFIG_EMMC

reopen_vbpipe:
    req_head_info_count = 0;
	memset(gbuffer, 0, BUFFER_LEN);
	sprintf(gbuffer, "reopen vbpipe\n");
	log2file();
	
    pipe_fd = open("/dev/vbpipe1", O_RDWR);
	if (pipe_fd < 0) {
		memset(gbuffer, 0, BUFFER_LEN);
		sprintf(gbuffer, "cannot open vbpipe1\n");
		log2file();
		exit(1);
	}

    while (1) {
		r_cnt = read(pipe_fd, &req_header, sizeof(request_header_t));
		acquire_wake_lock(PARTIAL_WAKE_LOCK, "NvItemdLock");
		if (req_head_info_count < REQHEADINFOCOUNT) {
			req_head_info_count++;
			memset(gbuffer, 0, BUFFER_LEN);
			sprintf(gbuffer, "1 r_cnt = %d\n", r_cnt);
			log2file();
		}
		/* write_proc_file("/proc/nk/stop", 0,  "2");     --->   echo 2 > /proc/nk/stop */ 
		/* write_proc_file("/proc/nk/restart", 0,  "2");  --->   echo 2 > /proc/nk/restart */
        	if (r_cnt == 0) {
            		/* check if the system is shutdown , if yes just reload the binary image */
            		check_mocor_reboot();
			/* when read returns 0 : it means vbpipe has received
			 * sysconf so we should  close the file descriptor
			 * and re-open the pipe.
			 */
            		close(pipe_fd);
            		memset(gbuffer, 0, BUFFER_LEN);
			sprintf(gbuffer, "goto reopen vbpipe\n");
			log2file();
			goto reopen_vbpipe;
		}

		if (r_cnt < 0) {
			memset(gbuffer, 0, BUFFER_LEN);
			sprintf(gbuffer, "1 no nv data : %d\n", r_cnt);
			log2file();
			release_wake_lock("NvItemdLock");
			continue;
		}

		if (req_head_info_count < REQHEADINFOCOUNT) {
			req_head_info_count++;
			req_head_info(req_header);
		}

		if (req_header.req_type == 1) {
			/* 1 : sync command */
			/* mocor update file : fixnv --> runtimenv --> backupfixnv */
			if (update_finxn_flag == 1)
#ifdef CONFIG_EMMC
				update_backupfile(PARTITION_FIX_NV2, FIXNV_SIZE);
#else
				update_backupfile("/backupfixnv/fixnv.bin", FIXNV_SIZE);
#endif

			update_finxn_flag = 0;
			
	        /* 1 : sync command */
            memset(gbuffer, 0, BUFFER_LEN);
		    sprintf(gbuffer, "sync start\n");
		    log2file();

		    	sync();

            memset(gbuffer, 0, BUFFER_LEN);
		    sprintf(gbuffer, "sync end\n");
		    log2file();

            req_header.type = 0;
		    memset(gbuffer, 0, BUFFER_LEN);
		    sprintf(gbuffer, "write back sync start\n");
		    log2file();
		    
            w_cnt = write(pipe_fd, &req_header, sizeof(request_header_t));
		    memset(gbuffer, 0, BUFFER_LEN);
		    sprintf(gbuffer, "write back sync end w_cnt = %d\n", w_cnt);
		    log2file();
            if (w_cnt < 0) {
			    memset(gbuffer, 0, BUFFER_LEN);
			    sprintf(gbuffer, "failed to write sync command\n");
			    log2file();
		    }
            release_wake_lock("NvItemdLock");
		    continue;
		}

		if (req_header.type == 0) {
			if ((req_header.offset + req_header.size) > FIXNV_SIZE) {
				memset(gbuffer, 0, BUFFER_LEN);
				sprintf(gbuffer, "Fixnv is too big\n");
				log2file();
			}

			memset(fixnv_buffer, 0xff, FIXNV_SIZE + 4);
#ifdef CONFIG_EMMC
			fix_nvitem_fd = open(PARTITION_FIX_NV1, O_RDWR, 0);
#else
			fix_nvitem_fd = open("/fixnv/fixnv.bin", O_RDWR | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
#endif
            		if (fix_nvitem_fd < 0) {
				memset(gbuffer, 0, BUFFER_LEN);
				sprintf(gbuffer, "cannot open fixnv mtd device\n");
				log2file();
				close(pipe_fd);
				release_wake_lock("NvItemdLock");
				exit(1);
			}

                	fd = fix_nvitem_fd;
			length = FIXNV_SIZE;
			read(fd, fixnv_buffer, length + 4);
			close(fd);

			indexaddress = (unsigned short *)(fixnv_buffer + length - 2);
			indexvalue = *indexaddress;
			fixnv_buffer_offset = req_header.offset;
		} else if (req_header.type == 1) {
			if ((req_header.offset + req_header.size) > RUNTIMENV_SIZE) {
			    memset(gbuffer, 0, BUFFER_LEN);
			    sprintf(gbuffer, "Runtimenv is too big\n");
			    log2file();
			}

#ifdef CONFIG_EMMC
			runtime_nvitem_fd = open(PARTITION_RUNTIME_NV1, O_RDWR, 0);
#else
			runtime_nvitem_fd = open("/runtimenv/runtimenv.bin", O_RDWR | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
#endif
            		if (runtime_nvitem_fd < 0) {
				memset(gbuffer, 0, BUFFER_LEN);
				sprintf(gbuffer, "cannot open runtimenv mtd device\n");
				log2file();
				close(pipe_fd);
				release_wake_lock("NvItemdLock");
				exit(1);
			}

            		fd = runtime_nvitem_fd;
			length = RUNTIMENV_SIZE;

			res = lseek(fd, req_header.offset, SEEK_SET);
			if (res < 0) {
				memset(gbuffer, 0, BUFFER_LEN);
				sprintf(gbuffer, "lseek runtime_nvitem_fd errno\n");
				log2file();
			}

#ifdef CONFIG_EMMC
			runtime_nvitem_bkfd = open(PARTITION_RUNTIME_NV2, O_RDWR, 0);
#else
			runtime_nvitem_bkfd = open("/runtimenv/runtimenvbkup.bin", O_RDWR | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
#endif
			if (runtime_nvitem_bkfd < 0) {
				memset(gbuffer, 0, BUFFER_LEN);
				sprintf(gbuffer, "cannot open runtimenv mtd backip device\n");
				log2file();
				close(pipe_fd);
				close(fd);
				release_wake_lock ("NvItemdLock");
				exit(1);
			}
			res = lseek(runtime_nvitem_bkfd, req_header.offset, SEEK_SET);
			if (res < 0) {
				memset(gbuffer, 0, BUFFER_LEN);
				sprintf(gbuffer, "lseek runtime_nvitem_bkfd errno\n");
				log2file();
			}
		} else if (req_header.type == 2) {
			/* phase check */
			if ((req_header.offset + req_header.size) > PRODUCTINFO_SIZE) {
				memset(gbuffer, 0, BUFFER_LEN);
				sprintf(gbuffer, "Productinfo is too big\n");
				log2file();
			}

			memset(phasecheck_buffer, 0xff, PRODUCTINFO_SIZE + 8);
#ifdef CONFIG_EMMC
			product_info_fd = open(PARTITION_PROD_INFO1, O_RDWR, 0);
#else
			product_info_fd = open("/productinfo/productinfo.bin", O_RDWR | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
#endif
			if (product_info_fd < 0) {
				memset(gbuffer, 0, BUFFER_LEN);
				sprintf(gbuffer, "cannot open productinfo mtd device\n");
				log2file();
				close(pipe_fd);
				release_wake_lock("NvItemdLock");
				exit(1);
			}

			fd = product_info_fd;
			length = PRODUCTINFO_SIZE;
#ifdef CONFIG_EMMC
			read(fd, phasecheck_buffer, length + 8);
#else
			read(fd, phasecheck_buffer, length + 4);
			indexaddress = (unsigned short *)(phasecheck_buffer + length + 2);
			indexvalue = *indexaddress;
#endif
			close(fd);
			phasecheck_buffer_offset = req_header.offset;
		} else {
			memset(gbuffer, 0, BUFFER_LEN);
			sprintf(gbuffer, "file type error\n");
			log2file();
			release_wake_lock("NvItemdLock");
			continue;
		}

		total = req_header.size;
		while (total > 0) {
			memset(data, 0xff, DATA_BUF_LEN);
			if (total > DATA_BUF_LEN)
				r_cnt = read(pipe_fd, data, DATA_BUF_LEN);
			else
				r_cnt = read(pipe_fd, data, total);
			
            		if (req_head_info_count < REQHEADINFOCOUNT) {
				req_head_info_count++;
				memset(gbuffer, 0, BUFFER_LEN);
				sprintf(gbuffer, "2 r_cnt = %d  size = %d\n", r_cnt, total);
				log2file();
			}

			if (r_cnt == 0) {
				if (length == RUNTIMENV_SIZE) {
					close(fd);
					close(runtime_nvitem_bkfd);
				}
				check_mocor_reboot();
				close(pipe_fd);
				memset(gbuffer, 0, BUFFER_LEN);
				sprintf(gbuffer, "goto reopen vbpipe\n");
				log2file();
				release_wake_lock("NvItemdLock");
				goto reopen_vbpipe;
			}

			if (r_cnt < 0) {
				memset(gbuffer, 0, BUFFER_LEN);
				sprintf(gbuffer, "2 no nv data : %d\n", r_cnt);
				log2file();
				continue;
			}

			/*if (r_cnt > 1024)
				array_value(length, data, 1024);
			else
				array_value(length, data, r_cnt);*/

			if (length == RUNTIMENV_SIZE) {
				w_cnt = write(fd, data, r_cnt);
				if (w_cnt < 0) {
					memset(gbuffer, 0, BUFFER_LEN);
					sprintf(gbuffer, "\nfailed to write runrimenv write\n");
					log2file();
					continue;
				}
				//////////////
				w_cnt = write(runtime_nvitem_bkfd, data, r_cnt);
				if (w_cnt < 0) {
					memset(gbuffer, 0, BUFFER_LEN);
					sprintf(gbuffer, "\nfailed to write runrimebkupnv write\n");
					log2file();
					continue;
				}
				//////////////
			} else if (length == FIXNV_SIZE) {
				memcpy(fixnv_buffer + fixnv_buffer_offset, data, r_cnt);
				fixnv_buffer_offset += r_cnt;
			} else if (length == PRODUCTINFO_SIZE) {
				memcpy(phasecheck_buffer + phasecheck_buffer_offset, data, r_cnt);
				phasecheck_buffer_offset += r_cnt;
			}

			total -= r_cnt;
		} /* while (total > 0) */
		if (length == RUNTIMENV_SIZE) {
			fsync(fd);
			close(fd);
			fsync(runtime_nvitem_bkfd);
			close(runtime_nvitem_bkfd);

			if (req_head_info_count < REQHEADINFOCOUNT) {
				req_head_info_count++;
				memset(gbuffer, 0, BUFFER_LEN);
				sprintf(gbuffer, "\nwrite runrimenv and runrimebkupnv finished\n");
				log2file();
			}
		} else if (length == PRODUCTINFO_SIZE) {
#ifdef CONFIG_EMMC
			update_productinfofile_emmc(PARTITION_PROD_INFO1, PARTITION_PROD_INFO2, length);
#else
			update_productinfofile("/productinfo/productinfo.bin", "/productinfo/productinfobkup.bin", length);
#endif
		} else if (length == FIXNV_SIZE) {
			update_finxn_flag = 1;
#ifdef CONFIG_EMMC
			update_fixnvfile(PARTITION_FIX_NV1, length);
#else
			update_fixnvfile("/fixnv/fixnv.bin", length);
#endif
		}

		release_wake_lock("NvItemdLock");
	} /* while(1) */

	memset(gbuffer, 0, BUFFER_LEN);
	sprintf(gbuffer, "close pipe_fd fixnv runtimenv\n");
	log2file();
	
    fclose(gfp);
	close(pipe_fd);
	
    return 0;
}

char *loadOpenFile(int fd, int *fileSize) 
{
	char *buf;
	
    *fileSize = lseek(fd, 0, SEEK_END);
	if (*fileSize < 0) {
		memset(gbuffer, 0, BUFFER_LEN);
		sprintf(gbuffer, "loadOpenFile error\n");
		log2file();
		return 0;
	}
	
    buf = (char *)malloc((*fileSize) + 1);
	if (buf == 0) {
		memset(gbuffer, 0, BUFFER_LEN);
		sprintf(gbuffer, "Malloc failed\n");
		log2file();
		return 0;
	}
	
    if (lseek(fd, 0, SEEK_SET) < 0) {
		memset(gbuffer, 0, BUFFER_LEN);
		sprintf(gbuffer, "lseek error\n");
		log2file();
		return 0;
	}

	memset(gbuffer, 0, BUFFER_LEN);
	sprintf(gbuffer, "file size %d \n", *fileSize);
	log2file();
	
    if (read(fd, buf, *fileSize) != *fileSize) {
		free(buf);
		buf = 0;
	} else
		buf[*fileSize] = 0;
	
    return buf;
}

char *loadFile(char *pathName, int *fileSize) 
{
	int fd;
	char *buf;
	
    memset(gbuffer, 0, BUFFER_LEN);
	sprintf(gbuffer, "pathName = %s\n", pathName);
	log2file();
	fd = open(pathName, O_RDONLY);
	if (fd < 0) {
		memset(gbuffer, 0, BUFFER_LEN);
		sprintf(gbuffer, "Unknown file %s\n", pathName);
		log2file();
		return 0;
	}
	
    buf = loadOpenFile(fd, fileSize);
	close(fd);
	
    return buf;
}

int check_proc_file(char *file, char *string) 
{
	int filesize;
	char *buf;
	
    buf = loadFile(file, &filesize);
	if (!buf) {
		memset(gbuffer, 0, BUFFER_LEN);
		sprintf(gbuffer, "failed to load %s \n", file);
		log2file();
		return -1;
	}
	
    memset(gbuffer, 0, BUFFER_LEN);
	sprintf(gbuffer, "buf = %s\n", buf);
	log2file();
	if (strstr(buf, string)) {
		memset(gbuffer, 0, BUFFER_LEN);
		sprintf(gbuffer, "String found <%s> in <%s>\n", string, file);
		log2file();
		return 0;
	}
	
    memset(gbuffer, 0, BUFFER_LEN);
	sprintf(gbuffer, "failed to find %s in %s\n", string, file);
	log2file();
	
    return 1;
}

int write_proc_file(char *file, int offset, char *string) 
{
	int fd, stringsize, res = -1;
	
    fd = open(file, O_RDWR);
	if (fd < 0) {
		memset(gbuffer, 0, BUFFER_LEN);
		sprintf(gbuffer, "Unknown file %s \n", file);
		log2file();
		return 0;
	}

	if (lseek(fd, offset, SEEK_SET) != offset) {
		memset(gbuffer, 0, BUFFER_LEN);
		sprintf(gbuffer, "Cant lseek file %s \n", file);
		log2file();
		goto leave;
	}

	stringsize = strlen(string);
	if (write(fd, string, stringsize) != stringsize) {
		memset(gbuffer, 0, BUFFER_LEN);
		sprintf(gbuffer, "Could not write %s in %s \n", string, file);
		log2file();
		goto leave;
	}

	res = 0;
	memset(gbuffer, 0, BUFFER_LEN);
	sprintf(gbuffer, "Wrote %s in file %s \n", string, file);
	log2file();

leave:
    close(fd);
	
    return res;
}

int loadimage(char *fin, int offsetin, char *fout, int offsetout, int size)  
{
	int res = -1, fdin, fdout, bufsize, i, rsize, rrsize, wsize;
	char buf[8192];
#ifdef TEST_MODEM
	int fd_modem_dsp;
#endif

	memset(gbuffer, 0, BUFFER_LEN);
	sprintf(gbuffer, "Loading %s in bank %s:%d %d\n", fin, fout, offsetout, size);
	log2file();

	fdin = open(fin, O_RDONLY, 0);
	fdout = open(fout, O_RDWR, 0);
	if (fdin < 0) {
		memset(gbuffer, 0, BUFFER_LEN);
		sprintf(gbuffer, "failed to open %s \n", fin);
		log2file();
		return -1;
	}
	if (fdout < 0) {
		memset(gbuffer, 0, BUFFER_LEN);
		sprintf(gbuffer, "failed to open %s \n", fout);
		log2file();
		return -1;
	}
	
#ifdef TEST_MODEM
	if (size == modem_image_len)
	    fd_modem_dsp = open("/data/local/tmp/modem.bin", O_RDWR | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
	else
	    fd_modem_dsp = open("/data/local/tmp/dsp.bin", O_RDWR | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
#endif
	if (lseek(fdin, offsetin, SEEK_SET) != offsetin) {
	    memset(gbuffer, 0, BUFFER_LEN);
		sprintf(gbuffer, "failed to lseek %d in %s \n", offsetin, fin);
		log2file();
		goto leave;
	}

	if (lseek(fdout, offsetout, SEEK_SET) != offsetout) {
	    memset(gbuffer, 0, BUFFER_LEN);
	    sprintf(gbuffer, "failed to lseek %d in %s \n", offsetout, fout);
	    log2file();
	    goto leave;
	}

	for (i = 0; size > 0; i += min(size, sizeof(buf))) {
		rsize = min(size, sizeof(buf));
		rrsize = read(fdin, buf, rsize);
		if (rrsize != rsize) {
			memset(gbuffer, 0, BUFFER_LEN);
			sprintf(gbuffer, "failed to read %s \n", fin);
			log2file();
			goto leave;
		}
		wsize = write(fdout, buf, rsize);
#ifdef TEST_MODEM
		wsize = write(fd_modem_dsp, buf, rsize);
#endif
		if (wsize != rsize) {
			memset(gbuffer, 0, BUFFER_LEN);
			sprintf(gbuffer, "failed to write %s [wsize = %d  rsize = %d  remain = %d]\n",
                fout, wsize, rsize, size);
			log2file();
			goto leave;
		}
		size -= rsize;
	}

	res = 0;
leave:
    close(fdin);
	close(fdout);
#ifdef TEST_MODEM
	fsync(fd_modem_dsp);
	close(fd_modem_dsp);
#endif
	
    return res;
}

#define MONITOR_APP   "/system/bin/sprd_monitor"

/* helper function to get pid from process name */
static int get_task_pid(char *name)
{
    DIR *d;
    struct dirent *de;
    char cmdline[1024];

    d = opendir("/proc");
    if (d == 0) return -1;

    while ((de = readdir(d)) != 0) {
        if(isdigit(de->d_name[0])) {
            int pid = atoi(de->d_name);
            int fd, ret;
            sprintf(cmdline, "/proc/%d/cmdline", pid);
            fd = open(cmdline, O_RDONLY);
            if (fd <= 0) continue;
            ret = read(fd, cmdline, 1023);
            close(fd);
            if (ret < 0) ret = 0;
            cmdline[ret] = 0;
            if (strcmp(name, cmdline) == 0)
            return pid;
        }
    }
    return -1;
}

int check_reboot(int guestID)
{
	char guest_dir[256];
	char buf[256];
	pid_t pid;

	pid = get_task_pid(MONITOR_APP);

    memset(gbuffer, 0, BUFFER_LEN);
	sprintf(gbuffer, "Check morco reboot \n");
	log2file();
	
    memset(guest_dir, 0, 256);
	memset(buf, 0, 256);
	sprintf(guest_dir, "/proc/nk/guest-%02d", guestID);
	sprintf(buf, "%s/status", guest_dir);
	memset(gbuffer, 0, BUFFER_LEN);
	sprintf(gbuffer, "buf = %s\n", buf);
	log2file();
	
    if (check_proc_file(buf, "not started") == 0) {
		memset(buf, 0, 256);
		sprintf(buf, "%s/%s", guest_dir, MODEM_BANK);
		memset(gbuffer, 0, BUFFER_LEN);
		sprintf(gbuffer, "buf = %s\n", buf);
		log2file();
		/* gillies set modem_image_len 0x600000 */
        loadimage(modem_mtd, MODEM_IMAGE_OFFSET, buf, 0x1000, modem_image_len);
		memset(buf, 0, 256);
		sprintf(buf, "%s/%s", guest_dir, DSP_BANK);
		memset(gbuffer, 0, BUFFER_LEN);
		sprintf(gbuffer, "buf = %s\n", buf);
		log2file();

		/* dsp is only 3968KB, 0x420000 is too big */
        loadimage(dsp_mtd, DSP_IMAGE_OFFSET, buf, 0x20000, (3968 * 1024));
	if (pid > 0)
		kill(pid, SIGUSR1);

        memset(gbuffer, 0, BUFFER_LEN);
		sprintf(gbuffer, "kill 1 sprd_monitor\n");
		log2file();
		
        write_proc_file("/proc/nk/restart", 0, "2");
		write_proc_file("/proc/nk/resume", 0, "2");
		memset(gbuffer, 0, BUFFER_LEN);
		sprintf(gbuffer, "wait for 20s\n");
		log2file();
		
        sleep(20);

	if (pid > 0)
		kill(pid, SIGUSR2);
	memset(gbuffer, 0, BUFFER_LEN);
	sprintf(gbuffer, "kill 2 sprd_monitor\n");
	log2file();

        return 1;
	} else {
		memset(gbuffer, 0, BUFFER_LEN);
		sprintf(gbuffer, "guest seems started \n");
		log2file();
	}

	return 0;
}

int check_mocor_reboot(void) 
{
	return check_reboot(2);
}

