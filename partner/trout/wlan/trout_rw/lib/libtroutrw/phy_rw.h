#ifndef _PHY_RW_H
#define _PHY_RW_H

#ifdef  __cplusplus
extern "C" {
#endif

int trw_write_phy_reg(unsigned int val,unsigned int addr);
int  trw_read_phy_reg(unsigned int * val,unsigned int addr);

#ifdef  __cplusplus
}
#endif

#endif
