#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "obj.h"
#include "libtrw.h"
#include "string.h"
#include "lua.h"
#include "reg_base.h"

extern lua_State *trw_L;

static unsigned int get_lua_v(const char * name,int * ok)
{
	if(name == NULL){
		if(ok) *ok = 0;
		return 0;
	}
	lua_getglobal(trw_L,name);
	return lua_tonumberx(trw_L, 1,ok);
}

static unsigned int get_addr(const char * argv,int *ok)
{
	unsigned int addr;
	if(argv == NULL ){
		if(ok) *ok = 0;
		return 0;
	}
	if(isdigit(argv[0])){
		addr = simple_strtou(argv,NULL,0);
		*ok = 1;
	}else{
		addr = get_lua_v(argv,ok);
	}
	return addr;
}
static int mode_set(struct _obj * obj,int argc,const char * argv[])
{
	int ret = 0;
	if(argc != 1){
		ret = -1;
		goto err;
	}
	if(strcmp("bin",argv[0]) == 0){
		trw_set_mode(BIN_MODE);
	}else if(strcmp("asic",argv[0]) == 0){
		trw_set_mode(ASIC_MODE);
	}else{
		ret = -2;
		goto err;
	}
	
	return 0;
err:
	printf("wrong param: you can use:  trw set mode {bin|asic}\n");
	return ret;
}

static int mode_get(struct _obj * obj,int argc,const char * argv[])
{
	int ret = 0;
	if(argc != 0){
		ret = -1;
		goto err;
	}
	printf("%s\n",trw_get_mode()==BIN_MODE?"bin":"asic");
	return 0;
err:
	printf("wrong param: you can use:  trw get mode\n");
	return ret;
}

static int reg_set(struct _obj * obj ,int argc,const char * argv[])
{
	int ret = 0;
	unsigned int val=0,addr=0;
	if(argc != 2){
		ret = -1;
		goto err;
	}
	addr = get_addr(argv[0],&ret);
	if(!ret){
		goto err;
	}
	val = simple_strtou(argv[1],NULL,0);
	if( (ret = trw_write_reg(val,addr)) < 0){
		printf("write failed ret = %d\n",ret);
	}
	return ret;
err:
	printf("wrong param: you can use:  trw set reg 0x22 21\n");
	return ret;
}

static int reg_get(struct _obj *obj ,int argc,const char * argv[])
{
	int ret = 0;
	unsigned int val=0,addr=0;
	if(argc != 1){
		ret = -1;
		goto err;
	}
	addr = get_addr(argv[0],&ret);
	if(!ret){
		goto err;
	}
	if( (ret = trw_read_reg(&val,addr)) > 0){
		printf("0x%08X\n",val);
		
	}else{
		printf("read failed ret = %d\n",ret);
	}
	return ret;
err:
	printf("wrong param: you can use:  trw get reg 0x22\n");
	return ret;
}

static int phy_set(struct _obj *obj ,int argc,const char * argv[])
{
	int ret = 0;
	unsigned int val=0,addr=0;
	if(argc != 2){
		ret = -1;
		goto err;
	}
	addr = get_addr(argv[0],&ret);
	if(!ret){
		goto err;
	}
	val = simple_strtou(argv[1],NULL,0);
	if( (ret = trw_write_phy_reg(val,addr)) < 0){
		printf("write failed ret = %d\n",ret);
	}
	return ret;
err:
	printf("wrong param: you can use:  trw set phy 0x22 25\n");
	return ret;
}

static int phy_get(struct _obj *obj ,int argc,const char * argv[])
{
	int ret = 0;
	unsigned int val=0,addr=0;
	if(argc != 1){
		ret = -1;
		goto err;
	}
	addr = get_addr(argv[0],&ret);
	if(!ret){
		goto err;
	}
	if( (ret = trw_read_phy_reg(&val,addr)) > 0){
		printf("0x%08X\n",val);
		
	}else{
		printf("read failed ret = %d\n",ret);
	}
	return ret;
err:
	printf("wrong param: you can use:  trw get phy 0x22\n");
	return ret;
	
}

static int comm_set(struct _obj *obj ,int argc,const char * argv[])
{
	int ret = 0;
	unsigned int val=0,addr=0;
	if(argc != 2){
		ret = -1;
		goto err;
	}
	addr = get_addr(argv[0],&ret);
	if(!ret){
		goto err;
	}
	addr = TCBA + (addr << 2);
	val = simple_strtou(argv[1],NULL,0);
	if( (ret = trw_write_reg(val,addr)) < 0){
		printf("write failed ret = %d\n",ret);
	}
	return ret;
err:
	printf("wrong param: you can use:  trw set comm 0x22 0x25\n");
	return ret;
}

static int comm_get(struct _obj *obj ,int argc,const char * argv[])
{
	int ret = 0;
	unsigned int val=0,addr=0;
	if(argc != 1){
		ret = -1;
		goto err;
	}
	addr = get_addr(argv[0],&ret);
	if(!ret){
		goto err;
	}
	addr = TCBA + (addr << 2);
	if( (ret = trw_read_reg(&val,addr)) > 0){
		printf("0x%08X\n",val);
		
	}else{
		printf("read failed ret = %d\n",ret);
	}
	return ret;
err:
	printf("wrong param: you can use:  trw get comm 0x22\n");
	return ret;
	
}

static int sys_set(struct _obj *obj ,int argc,const char * argv[])
{
	int ret = 0;
	unsigned int val=0,addr=0;
	if(argc != 2){
		ret = -1;
		goto err;
	}
	addr = get_addr(argv[0],&ret);
	if(!ret){
		goto err;
	}
	addr = ((unsigned int)(TROUT_SYS_BASE+addr)) << 2 ;
	val = simple_strtou(argv[1],NULL,0);
	if( (ret = trw_write_reg(val,addr)) < 0){
		printf("write failed ret = %d\n",ret);
	}
	return ret;
err:
	printf("wrong param: you can use:  trw set sys 0x22 0x25\n");
	return ret;
}

static int sys_get(struct _obj *obj ,int argc,const char * argv[])
{
	int ret = 0;
	unsigned int val=0,addr=0;
	if(argc != 1){
		ret = -1;
		goto err;
	}
	addr = get_addr(argv[0],&ret);
	if(!ret){
		goto err;
	}
	addr = ((unsigned int)(TROUT_SYS_BASE+addr)) << 2 ;
	if( (ret = trw_read_reg(&val,addr)) > 0){
		printf("0x%08X\n",val);
		
	}else{
		printf("read failed ret = %d\n",ret);
	}
	return ret;
err:
	printf("wrong param: you can use:  trw get sys 0x22\n");
	return ret;
	
}

static int mac_set(struct _obj *obj ,int argc,const char * argv[])
{
	int ret = 0;
	unsigned int val=0,addr=0;
	if(argc != 2){
		ret = -1;
		goto err;
	}
	addr = get_addr(argv[0],&ret);
	if(!ret){
		goto err;
	}
	addr = addr + PA_BASE;
	val = simple_strtou(argv[1],NULL,0);
	if( (ret = trw_write_reg(val,addr)) < 0){
		printf("write failed ret = %d\n",ret);
	}
	return ret;
err:
	printf("wrong param: you can use:  trw set mac 0x22 0x25\n");
	return ret;
}

static int mac_get(struct _obj *obj ,int argc,const char * argv[])
{
	int ret = 0;
	unsigned int val=0,addr=0;
	if(argc != 1){
		ret = -1;
		goto err;
	}
	addr = get_addr(argv[0],&ret);
	if(!ret){
		goto err;
	}
	addr = addr + PA_BASE;
	if( (ret = trw_read_reg(&val,addr)) > 0){
		printf("0x%08X\n",val);
		
	}else{
		printf("read failed ret = %d\n",ret);
	}
	return ret;
err:
	printf("wrong param: you can use:  trw get mac 0x22\n");
	return ret;
	
}

static int rf_set(struct _obj *obj ,int argc,const char * argv[])
{
	int ret = 0;
	unsigned int val=0,addr=0;
	if(argc != 2){
		ret = -1;
		goto err;
	}
	addr = get_addr(argv[0],&ret);
	if(!ret){
		goto err;
	}
	addr = ((unsigned int)(addr + RF_BASE)) << 2;
	val = simple_strtou(argv[1],NULL,0);
	if( (ret = trw_write_reg(val,addr)) < 0){
		printf("write failed ret = %d\n",ret);
	}
	return ret;
err:
	printf("wrong param: you can use:  trw set rf 0x22 0x25\n");
	return ret;
}

static int rf_get(struct _obj *obj ,int argc,const char * argv[])
{
	int ret = 0;
	unsigned int val=0,addr=0;
	if(argc != 1){
		ret = -1;
		goto err;
	}
	addr = get_addr(argv[0],&ret);
	if(!ret){
		goto err;
	}
	addr = ((unsigned int)(addr + RF_BASE)) << 2;
	if( (ret = trw_read_reg(&val,addr)) > 0){
		printf("0x%08X\n",val);
		
	}else{
		printf("read failed ret = %d\n",ret);
	}
	return ret;
err:
	printf("wrong param: you can use:  trw get rf 0x22\n");
	return ret;
	
}

static int power_set(struct _obj *obj ,int argc,const char * argv[])
{
	int ret = -1;
	MODE mode =WIFI_MODE;
	unsigned int val;
	
	if(argc <1 && argc>2){
		goto err;
	}

	if(argc == 2){
		if(strcmp("wifi",argv[0]) ==0 ){
			mode = WIFI_MODE;
		}else if(strcmp("bt",argv[0]) ==0){
			mode = BT_MODE;
		}else if(strcmp("fm",argv[0]) ==0){
			mode = FM_MODE;
		}else{
			goto err;
		}
	}

	if(strcmp("on",argv[argc-1]) ==0 ){
		val = 1;
	}else if(strcmp("off",argv[argc-1]) ==0){
		val = 0;
	}else{
		goto err;
	}
	return trw_set_power(val,mode);
err:
	printf("wrong param: you can use:  trw set power {wifi|fm|bt} {on|off}\n");
	return ret;
}

static int power_get(struct _obj *obj ,int argc,const char * argv[])
{
	int ret = -1;
	MODE mode =NONE_MODE;
	
	if(argc !=1){
		goto err;
	}

	if(strcmp("wifi",argv[0]) ==0 ){
		mode = WIFI_MODE;
	}else if(strcmp("bt",argv[0]) ==0){
		mode = BT_MODE;
	}else if(strcmp("fm",argv[0]) ==0){
		mode = FM_MODE;
	}else{
		goto err;
	}
	printf("%s\n", (trw_get_power(mode)==1)?"on":"off");
	return 0;
err:
	printf("wrong param: you can use:  trw get power {wifi|fm|bt}\n");
	return ret;
}

static int wifi_ram_set(struct _obj * obj ,int argc,const char * argv[])
{
	return 0;
}

static int wifi_ram_get(struct _obj * obj ,int argc,const char * argv[])
{
	return 0;
}

static int bt_ram_set(struct _obj * obj ,int argc,const char * argv[])
{
	return 0;
}

static int bt_ram_get(struct _obj * obj,int argc,const char * argv[])
{
	return 0;
}

static int legacy_set(struct _obj * obj ,int argc,const char * argv[])
{
	return 0;
}

static int legacy_get(struct _obj * obj,int argc,const char * argv[])
{
	int ret = -1;
	int val=0;
	if(argc !=1){
		goto err;
	}
	if(strcmp("2",argv[0]) ==0 ){
		trw_get_legacy(&val,2);
	}else if(strcmp("8",argv[0]) ==0){
		trw_get_legacy(&val,8);
	}else{
		goto err;
	}
	return 0;
err:
	printf("wrong param: you can use:  trw get  legacy  {2|8}\n");
	return ret;
}

obj_t obj_list[] = {
	{"reg",{reg_set,reg_get}},
	{"phy",{phy_set,phy_get}},
	{"comm",{comm_set,comm_get}},
	{"sys",{sys_set,sys_get}},
	{"mac",{mac_set,mac_get}},
	{"rf",{rf_set,rf_get}},
	{"mode",{mode_set,mode_get}},
	{"power",{power_set,power_get}},
	{"wifiram",{wifi_ram_set,wifi_ram_get}},
	{"btram",{bt_ram_set,bt_ram_get}},
	{"legacy",{legacy_set,legacy_get}},
	{NULL,{NULL,NULL}},
};

