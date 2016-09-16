#ifndef __CALIBRATION_H__

#define	BATTERY_VOL_PATH	"/sys/class/power_supply/battery/real_time_voltage"
#define	BATTERY_ADC_PATH	"/sys/class/power_supply/battery/real_time_vbat_adc"
#define	CALI_CTRL_FILE_PATH	"/dev/block/mmcblk0p24"
#define	CHARGER_STOP_PATH	"/dev/class/power_supply/battery/stop_charge"
#define	BATTER_CALI_CONFIG_FILE	CALI_CTRL_FILE_PATH
typedef enum
{
    DIAG_AP_CMD_ADC  = 0x0001,
    DIAG_AP_CMD_LOOP,
    MAX_DIAG_AP_CMD
} DIAG_AP_CMD_E;


#define AP_ADC_CALIB    1
#define AP_ADC_LOAD     2
#define AP_ADC_SAVE     3
#define	AP_GET_VOLT	4
#define AP_DIAG_LOOP	5


#define	CALI_MAGIC	(0x49424143) //CALI
#define	CALI_COMP	(0x504D4F43) //COMP

typedef struct
{
    unsigned int     	adc[2];           // calibration of ADC, two test point
    unsigned int 	battery[2];       // calibraton of battery(include resistance), two test point
    unsigned int    	reserved[8];      // reserved for feature use.
} AP_ADC_T;

typedef struct
{
	unsigned int	magic;		  // when create ,magic = "CALI" 
	unsigned int	cali_flag;        // cali_flag   default 0xFFFFFFFF, when calibration finished,it is set "COMP"
	AP_ADC_T 	adc_para;         // ADC calibration data.
}CALI_INFO_DATA_T;

typedef struct {
    unsigned short  cmd;        // DIAG_AP_CMD_E
    unsigned short  length;   // Length of structure 
} TOOLS_DIAG_AP_CMD_T;

typedef struct {
    unsigned int operate; // 0: Get ADC   1: Load ADC    2: Save ADC 
    unsigned int parameters[12];
}TOOLS_AP_ADC_REQ_T;

typedef struct {
    unsigned short status;   // ==0: success, != 0: fail
    unsigned short length;   // length of  result
} TOOLS_DIAG_AP_CNF_T;

typedef struct
{
    MSG_HEAD_T  msg_head;
    TOOLS_DIAG_AP_CNF_T diag_ap_cnf;
    TOOLS_AP_ADC_REQ_T ap_adc_req;
}MSG_AP_ADC_CNF;


#define __CALIBRATION_H__

#endif

