#include<stdio.h>
#include<string.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<unistd.h>

#include "libtrw.h"

/*-------------------private function --------------------*/
static int find_file(const char * name)
{
	int fd = open(name,O_RDONLY);
	if(fd < 0 ){
		return 0;
	}
	close(fd);
	return 1;
}

static int write_file(const char * name,char * buf,int len)
{
	int fd,ret = -1,left = len  ;
	char * p = buf;

	//printf("write_file :%s,len=%d",name,len);
	fd = open(name,O_WRONLY);
	if(fd < 0 ){
		printf("open file %s failed\n",name);
		return -1;
	}
	while(left>0 && (ret = write(fd,p,left))>0){
		p+=ret;
		left-=ret;
	}
	close(fd);
	return len - left;
}

static int read_file(const char * name,char * buf,int len)
{
	int fd,ret = -1,left = len  ;
	char * p = buf;
	fd = open(name,O_RDONLY);
	if(fd < 0 ){
		printf("read_file :%s failed\n",name);
		return -1;
	}
	while(left>0 && (ret = read(fd,p,left))>0){
		p+=ret;
		left-=ret;
	}
	
	close(fd);
	return len - left;
}

static int trw_read(unsigned char * buf,int len ,unsigned int addr ,enum EMODULE m)
{
	int mode = trw_get_mode();
	int ret = -1;
	if(mode == BIN_MODE){
		struct trout_rw_config config;
		config.addr = addr;
		config.module = m;
		config.flag = ADDR_MASK|MODULE_MASK;
		if(write_file(TROUT_CONFIG_NODE,(char*)&config,sizeof(config)) == sizeof(config)){
			ret = read_file(TROUT_DATA_NODE,(char*)buf,len);
		}else{
			printf("[%s]: write file %s failed\n",__FUNCTION__ , TROUT_CONFIG_NODE);
		}
	}else if(mode == ASIC_MODE){
		ret = -2;
	}else{
		printf("get wrong mode type (%d)\n",mode);
	}
	return ret;
}

static int trw_write(unsigned char * buf,int len ,unsigned int addr ,enum EMODULE m)
{
	int mode = trw_get_mode();
	int ret = -1;
	if(mode == BIN_MODE){
		struct trout_rw_config config;
		config.addr = addr;
		config.module = m;
		config.flag = ADDR_MASK|MODULE_MASK;
		if(write_file(TROUT_CONFIG_NODE,(char*)&config,sizeof(config)) == sizeof(config)){
			ret = write_file(TROUT_DATA_NODE,(char*)buf,len);
		}
	}else if(mode == ASIC_MODE){
		ret = -2;
	}
	return ret;
}

/*-------------------public function --------------------*/

int trw_set_mode(enum CONFIG_MODE mode)
{
	char *file=TROUT_CONFIG_MODE_STA_NODE;
	if(find_file(TROUT_CONFIG_MODE_STA_NODE)){
		file = TROUT_CONFIG_MODE_STA_NODE;
	}else if(find_file(TROUT_CONFIG_MODE_AP_NODE)){
		file = TROUT_CONFIG_MODE_AP_NODE;
	}else if(find_file(TROUT_CONFIG_MODE_NPI_NODE)){
		file = TROUT_CONFIG_MODE_NPI_NODE;
	}
	return write_file(file,mode==BIN_MODE ? "1":"2",2);
}

int trw_get_mode()
{
	char buf[2]={0,};
	char *file=TROUT_CONFIG_MODE_STA_NODE;
	if(find_file(TROUT_CONFIG_MODE_STA_NODE)){
		file = TROUT_CONFIG_MODE_STA_NODE;
	}else if(find_file(TROUT_CONFIG_MODE_AP_NODE)){
		file = TROUT_CONFIG_MODE_AP_NODE;
	}else if(find_file(TROUT_CONFIG_MODE_NPI_NODE)){
		file = TROUT_CONFIG_MODE_NPI_NODE;
	}
	read_file(file,buf,sizeof(buf));
	return atoi(buf);
}

int trw_set_power(unsigned int val,unsigned int addr)
{
	return trw_write((unsigned char *)&val,sizeof(val),addr,POWER);
}

int trw_get_power(unsigned int addr)
{
	int val=-1;
	trw_read((unsigned char *)&val,sizeof(val),addr,POWER);
	return val;
}

int trw_get_legacy(int * val,unsigned int function)
{
	return trw_read((unsigned char *)val,sizeof(*val),function,LEGACY);
}

int trw_write_reg(unsigned int val,unsigned int addr)
{
	return trw_write((unsigned char *)&val,sizeof(val),addr,REG);
}

int  trw_read_reg(unsigned int * val,unsigned int addr)
{
	return trw_read((unsigned char *)val,sizeof(*val),addr,REG);
}


int trw_write_wifi_ram(unsigned char * buf,int len ,unsigned int offset)
{
	return trw_write(buf,len,offset,WIFI_RAM);
}

int trw_read_wifi_ram(unsigned char * buf ,int len,unsigned int offset)
{
	return trw_read(buf,len,offset,WIFI_RAM);
}

int trw_write_bt_ram(unsigned char * buf,int len ,unsigned int offset)
{
	return trw_write(buf,len,offset,BT_RAM);
}

int trw_read_bt_ram(unsigned char * buf ,int len,unsigned int offset)
{
	return trw_read(buf,len,offset,BT_RAM);
}
