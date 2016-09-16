#include "libtrw.h"
#include "lua_api.h"
#include "reg_base.h"

int trw_set_mode(enum CONFIG_MODE);
int trw_get_mode();

int trw_set_power(unsigned int val,unsigned int addr);
int trw_get_power(unsigned int addr);

int trw_write_reg(unsigned int val,unsigned int addr);
int  trw_read_reg(unsigned int * val,unsigned int addr);

int trw_write_wifi_ram(unsigned char * buf,int len ,unsigned int offset);
int trw_read_wifi_ram(unsigned char * buf ,int len,unsigned int offset);

int trw_write_bt_ram(unsigned char * buf,int len ,unsigned int offset);
int trw_read_bt_ram(unsigned char * buf ,int len,unsigned int offset);

static int lua_trw_set_reg (lua_State *L)
{
	int ret = 0;
	unsigned int addr = luaL_checknumber(L,2);
	unsigned int val = luaL_checknumber(L,1);
	ret = trw_write_reg(val,addr);
	lua_pushinteger(L,ret);
  	return 1;
}

static int lua_trw_get_reg (lua_State *L) 
{
	int ret = 0;
	unsigned int addr = luaL_checknumber(L,1);
	unsigned int val = 0;
	ret = trw_read_reg(&val,addr);
	lua_pushinteger(L,val);
  	return 1;
}

static int lua_trw_set_comm (lua_State *L)
{
	int ret = 0;
	unsigned int addr = luaL_checknumber(L,2);
	unsigned int val = luaL_checknumber(L,1);
	addr = TCBA + (addr << 2);
	ret = trw_write_reg(val,addr);
	lua_pushinteger(L,ret);
  	return 1;
}

static int lua_trw_get_comm (lua_State *L) 
{
	int ret = 0;
	unsigned int addr = luaL_checknumber(L,1);
	unsigned int val = 0;
	addr = TCBA + (addr << 2);
	ret = trw_read_reg(&val,addr);
	lua_pushinteger(L,val);
  	return 1;
}

static int lua_trw_set_sys (lua_State *L)
{
	int ret = 0;
	unsigned int addr = luaL_checknumber(L,2);
	unsigned int val = luaL_checknumber(L,1);
	addr = ((unsigned int)(TROUT_SYS_BASE+addr)) << 2 ;
	ret = trw_write_reg(val,addr);
	lua_pushinteger(L,ret);
  	return 1;
}

static int lua_trw_get_sys (lua_State *L) 
{
	int ret = 0;
	unsigned int addr = luaL_checknumber(L,1);
	unsigned int val = 0;
	addr = ((unsigned int)(TROUT_SYS_BASE+addr)) << 2 ;
	ret = trw_read_reg(&val,addr);
	lua_pushinteger(L,val);
  	return 1;
}

static int lua_trw_set_mac(lua_State *L)
{
	int ret = 0;
	unsigned int addr = luaL_checknumber(L,2);
	unsigned int val = luaL_checknumber(L,1);
	addr = addr + PA_BASE;
	ret = trw_write_reg(val,addr);
	lua_pushinteger(L,ret);
  	return 1;
}

static int lua_trw_get_mac (lua_State *L) 
{
	int ret = 0;
	unsigned int addr = luaL_checknumber(L,1);
	unsigned int val = 0;
	addr = addr + PA_BASE;
	ret = trw_read_reg(&val,addr);
	lua_pushinteger(L,val);
  	return 1;
}

static int lua_trw_set_rf(lua_State *L)
{
	int ret = 0;
	unsigned int addr = luaL_checknumber(L,2);
	unsigned int val = luaL_checknumber(L,1);
	addr = ((unsigned int)(addr + RF_BASE)) << 2;
	ret = trw_write_reg(val,addr);
	lua_pushinteger(L,ret);
  	return 1;
}

static int lua_trw_get_rf (lua_State *L) 
{
	int ret = 0;
	unsigned int addr = luaL_checknumber(L,1);
	unsigned int val = 0;
	addr = ((unsigned int)(addr + RF_BASE)) << 2;
	ret = trw_read_reg(&val,addr);
	lua_pushinteger(L,val);
  	return 1;
}

static int lua_trw_set_mode (lua_State *L) 
{
	int ret = 0;
	unsigned int mode = luaL_checknumber(L,1);
	ret = trw_set_mode(mode);
	lua_pushinteger(L,ret);
  	return 1;
}

static int lua_trw_get_mode (lua_State *L) 
{
 	int ret = 0;
	ret = trw_get_mode();
	lua_pushinteger(L,ret);
  	return 1;
}

static int lua_trw_set_power (lua_State *L) 
{
 	int ret = 0;
	unsigned int addr = luaL_checknumber(L,2);
	unsigned int power = luaL_checknumber(L,1);
	ret = trw_set_power(power,addr);
	lua_pushinteger(L,ret);
  	return 1;
}
static int lua_trw_get_power (lua_State *L)
{
   	int ret = 0;
	unsigned int addr = luaL_checknumber(L,1);
	ret = trw_get_power(addr);
	lua_pushinteger(L,ret);
  	return 1;
}
static int lua_trw_set_phy (lua_State *L)
{
 	int ret = 0;
	unsigned int addr = luaL_checknumber(L,2);
	unsigned int val = luaL_checknumber(L,1);
	ret = trw_write_phy_reg(val,addr);
	lua_pushinteger(L,ret);
  	return 1;
}
static int lua_trw_get_phy (lua_State *L)
{
 	int ret = 0;
	unsigned int addr = luaL_checknumber(L,1);
	unsigned int val = 0;
	ret = trw_read_phy_reg(&val,addr);
	lua_pushinteger(L,val);
  	return 1;
}
static int lua_trw_set_wifi_ram (lua_State *L) 
{
  return 0;
}
static int lua_trw_get_wifi_ram (lua_State *L) 
{
  return 0;
}
static int lua_trw_set_bt_ram (lua_State *L) 
{
  return 0;
}
static int lua_trw_get_bt_ram (lua_State *L) 
{
  return 0;
}

static const luaL_Reg trw_funcs[] = {
  {"set_reg", lua_trw_set_reg},
  {"get_reg", lua_trw_get_reg},

  {"set_comm", lua_trw_set_comm},
  {"get_comm", lua_trw_get_comm},

  {"set_sys", lua_trw_set_sys},
  {"get_sys", lua_trw_get_sys},

  {"set_mac", lua_trw_set_mac},
  {"get_mac", lua_trw_get_mac},

  {"set_rf", lua_trw_set_rf},
  {"get_rf", lua_trw_get_rf},
  
  {"set_phy", lua_trw_set_phy},
  {"get_phy", lua_trw_get_phy},
  
  {"set_mode", lua_trw_set_mode},
  {"get_mode", lua_trw_get_mode},
  
  {"set_power", lua_trw_set_power},
  {"get_power", lua_trw_get_power},
  
  {"set_wifi_ram", lua_trw_set_wifi_ram},
  {"get_wifi_ram", lua_trw_get_wifi_ram},
  
  {"set_bt_ram", lua_trw_set_bt_ram},
  {"get_bt_ram", lua_trw_get_bt_ram},
  
  {NULL, NULL}
};


LUAMOD_API int luaopen_trw (lua_State *L) {
  /* set global _G */
  lua_newtable(L);
  luaL_setfuncs(L, trw_funcs, 0);
  lua_setglobal(L,"trw");
  return 0;
}

