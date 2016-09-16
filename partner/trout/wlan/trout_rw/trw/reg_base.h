#ifndef _REG_BASE_H
#define _REG_BASE_H

#define TROUT_COMMON_BASE   (0x4000U << 2)
#define TCBA	TROUT_COMMON_BASE
#define TCBE	(TCBA + (0xFF << 2))

#define TROUT_SYS_BASE		(0x0)
#define PA_BASE                       (0x00008000)
#define RF_BASE				(0x1000)

#endif
