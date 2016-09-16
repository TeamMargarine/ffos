#ifndef _TROUT_RW_H
#define _TROUT_RW_H
#include "phy_rw.h"
#ifdef  __cplusplus
extern "C" {
#endif
enum EMODULE{
	REG,
	WIFI_RAM,
	BT_RAM,
	POWER,
	LEGACY,
	MODULE_MAX
};

enum DEBUG_LEVEL{
	DEBUG_INFO,
	DEBUG_ERROR,
	DEBUG_LEVEL_MAX
};

enum CONFIG_MODE{
	BIN_MODE=1,
	ASIC_MODE
};

typedef enum {
	NONE_MODE    = 0,
	WIFI_MODE    = 1,
	FM_MODE      = 2,
	BT_MODE      = 4,	
} MODE;

#define MODULE_MASK 0x01
#define ADDR_MASK 0x02
#define DEBUG_MASK 0x04
#define ALL_MASK (MODULE_MASK|ADDR_MASK|DEBUG_MASK)

struct trout_rw_config{
	enum EMODULE module;
	 unsigned int addr;	
	enum DEBUG_LEVEL debug_level;
	unsigned int flag;
};

#define TROUT_DIR "driver/trout"

#define TROUT_CONFIG_NODE "/proc/"TROUT_DIR"/rw_config"
#define TROUT_DATA_NODE "/proc/"TROUT_DIR"/rw_buffer"
#define TROUT_CONFIG_MODE_STA_NODE "/sys/module/itm_sta/parameters/config_mode"
#define TROUT_CONFIG_MODE_AP_NODE "/sys/module/itm_ap/parameters/config_mode"
#define TROUT_CONFIG_MODE_NPI_NODE "/sys/module/itm_npi/parameters/config_mode"

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

int trw_get_legacy(int * val,unsigned int function);
#ifdef  __cplusplus
}
#endif

#endif
