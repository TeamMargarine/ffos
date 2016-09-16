#include <fcntl.h>
#include "engopt.h"
#include <pthread.h>
#include "engphasecheck.h"


#define PHASE_CHECKE_FILE "/productinfo/productinfo.bin"

int eng_phasechecktest(void)
{
	SP09_PHASE_CHECK_T phase;
	
	return eng_getphasecheck(&phase);
}

int eng_getphasecheck(SP09_PHASE_CHECK_T* phase_check)
{
	int ret = 0;
	int len;
	int fd = open(PHASE_CHECKE_FILE,O_RDONLY);
	if (fd <0){
		ENG_LOG("%s open fail",__FUNCTION__);
	}
	len = read(fd,phase_check,sizeof(SP09_PHASE_CHECK_T));

	ENG_LOG("Magic=%d",phase_check->Magic);
	
	ENG_LOG("SN1=%s",phase_check->SN1);
	ENG_LOG("SN2=%s",phase_check->SN2);
	
	if (len <= 0){
		ret = 1;
	}
	close(fd);
	return ret;
}


