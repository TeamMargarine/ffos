#ifndef ENG_SETBTADDR_H__
#define ENG_SETBTADDR_H__

#define MAC_ERROR		"FF:FF:FF:FF:FF:FF"
#define BT_PSKEY_FILE	"/data/pskey.txt"
#define BT_MAC_RAND_FILE		"/data/btmac.txt"
#define GET_BTMAC_ATCMD	"AT+SNVM=0,401"
#define GET_BTPSKEY_ATCMD	"AT+SNVM=0,415"
#define SET_BTMAC_ATCMD	"AT+SNVM=1,401"

#define PSKEY_FROM_ANDROID

// used to store BT pskey structure and default values
#define BT_PSKEY_STRUCT_FILE "/system/lib/modules/pskey_bt.txt"

typedef unsigned char uint8;
typedef unsigned int uint32;
typedef unsigned short uint16;

#define BT_ADDRESS_SIZE    6

typedef struct _BT_NV_PARAM{
    uint8 bd_addr[BT_ADDRESS_SIZE];
    uint16 xtal_dac;
}BT_NV_PARAM;

// add by longting.zhao for pskey NV
// pskey file structure
typedef struct _BT_PSKEY_INFO_T{
    uint8   g_dbg_source_sink_syn_test_data;
    uint8   g_sys_sleep_in_standby_supported;
    uint8   g_sys_sleep_master_supported;
    uint8   g_sys_sleep_slave_supported;
    uint32  default_ahb_clk;
    uint32  device_class;
    uint32  win_ext;
    uint32  g_aGainValue[6];
    uint32  g_aPowerValue[5];
    uint8   feature_set[16];
    uint8   device_addr[6];
    uint8  g_sys_sco_transmit_mode; //0: DMA 1: UART 2:SHAREMEM
    uint8  g_sys_uart0_communication_supported; //true use uart0, otherwise use uart1 for debug
    uint8 edr_tx_edr_delay;
    uint8 edr_rx_edr_delay;
    uint8 abcsp_rxcrc_supported;//true:supported; otherwise not supported
    uint8 abcsp_txcrc_supported;//true:supported; otherwise not supported
    uint32 share_memo_rx_base_addr;
    uint32 share_memo_tx_base_addr;
    uint32 share_memo_tx_packet_num_addr;
    uint32 share_memo_tx_data_base_addr;
    uint32 g_PrintLevel;
    uint16 share_memo_tx_block_length;
    uint16 share_memo_rx_block_length;
    uint16 share_memo_tx_water_mark;
    uint16 share_memo_tx_timeout_value;
    uint16 uart_rx_watermark;
    uint16 uart_flow_control_thld;
    uint32 comp_id;
    uint16  uart_clk_divd;
    uint16  pcm_clk_divd;
    uint16 debug_bb_tx_gain;
    uint16 debug_tx_power;
    uint32 tx_channel_compensate;
    uint16 ctrl_reserved;
    uint16 reserved16;
    uint32  reserved[10];
}BT_PSKEY_CONFIG_T;

// pskey NV structure
typedef struct _BT_SPRD_NV_PARAM {
    uint32 host_reserved[2];  //0x0
    uint32 config0;  //0xe0
    uint32  device_class;  //0x001f00
    uint32  win_ext;//0x1E
    uint32  g_aGainValue[6];
    uint32  g_aPowerValue[5];
    uint32   feature_set[4]; //0xFE8DFFFF 0x8379FF9B 0x7FFFA7FF 0x3EF7E000
    uint32 config1;//0x3F300E05
    uint32  g_PrintLevel ; //0xFFFFFFFF
    uint32 config2 ; //0xD7048CF7
    uint32 tx_channel_compensate;   // 0x88888888
    uint32 config3;    // 0x013C0008
    uint32 reserved[10];
}BT_SPRD_NV_PARAM;
// end add by longting.zhao for pskey NV


uint8 ConvertHexToBin(
    uint8        *hex_ptr,     // in: the hexadecimal format string
    uint16       length,       // in: the length of hexadecimal string
    uint8        *bin_ptr      // out: pointer to the binary format string
);

#endif /* BT_RAM_CODE_H__ */

