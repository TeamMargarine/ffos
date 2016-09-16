#ifndef _ENG_CLIENT_H
#define _ENG_CLIENT_H

#ifdef __cplusplus
extern "C" {
#endif

#define ENG_BUF_LEN 		2048

#define ENG_SIMTYPE			"persist.msms.phone_count"
#define ENG_PC_DEV			"/dev/input/event0"
#define ENG_ATAUTO_DEV		"/dev/CHNPTY12"
#define ENG_MODEM_DEV		"/dev/CHNPTY13"
#define ENG_MODEM_DEV2		"/dev/CHNPTY14"
#define ENG_ERR_TIMEOUT     "timeout"
#define ENG_ERR_DATERR     	"daterr"


struct eng_buf_struct {
	int buf_len;
	char buf[ENG_BUF_LEN];
};

int eng_client(int port, int type);

#ifdef __cplusplus
}
#endif

#endif
