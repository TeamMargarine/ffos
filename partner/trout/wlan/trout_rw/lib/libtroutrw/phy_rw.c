#include "libtrw.h"
#include "phy_rw.h"
#include <unistd.h>

#define rMAC_PHY_REG_ACCESS_CON 0x8638
#define rMAC_PHY_REG_RW_DATA    0x863c
#define  REGBIT0          0x01
#define BIT0    0X01
#define BIT1    0X02
typedef  unsigned int UWORD32;
typedef  unsigned char UWORD8;


static void wait_ready_timeout(int ms)
{
	UWORD32 val   = 0;
	UWORD32 delay;
	
	trw_read_reg(&val,rMAC_PHY_REG_ACCESS_CON);
	
	while((val & REGBIT0) == REGBIT0){
		usleep(1000);
		delay++;
		if(delay > ms){
			printf("read phy reg is timeout\n");
			break;
		}
		trw_read_reg(&val,rMAC_PHY_REG_ACCESS_CON);
	}
}

static void internal_change_bank(UWORD8 ra, UWORD32 rd)
{
	unsigned int val;
	wait_ready_timeout(10);
	trw_write_reg(rd,rMAC_PHY_REG_RW_DATA);
	val  = ((UWORD8)ra << 2); /* Register address set in bits 2 - 9 */
	val &= ~ BIT1;            /* Read/Write bit set to 0 for write */
	val |= BIT0;              /* Access bit set to 1 */

	trw_write_reg(val,rMAC_PHY_REG_ACCESS_CON);
}

/*****************************************************************************/
/*                                                                           */
/*  Function Name : read_dot11_phy_reg                                       */
/*                                                                           */
/*  Description   : This function reads the Ittiam PHY register.             */
/*                                                                           */
/*  Inputs        : 1) Address of register                                   */
/*                  2) Pointer to value to be updated                        */
/*                                                                           */
/*  Globals       : None                                                     */
/*                                                                           */
/*  Processing    : This function checks the lock bit and then reads the     */
/*                  PHY register by writing the address to the serial        */
/*                  control register, polling the status bit and then        */
/*                  reading the register.                                    */
/*                                                                           */
/*  Outputs       : None                                                     */
/*  Returns       : None                                                     */
/*  Issues        : None                                                     */
/*                                                                           */
/*****************************************************************************/
int  trw_read_phy_reg(unsigned int * rd,unsigned int ra)
{
    UWORD32 val = 0;
    UWORD32 delay = 0;
	
    wait_ready_timeout(10);

    val  = ((UWORD8)ra << 2); /* Register address set in bits 2 - 9 */
    val |= BIT1;              /* Read/Write bit set to 1 for read */
    val |= BIT0;              /* Access bit set to 1 */

    trw_write_reg(val,rMAC_PHY_REG_ACCESS_CON );

    wait_ready_timeout(10);
	
    return trw_read_reg(rd, rMAC_PHY_REG_RW_DATA); /* Read data from register */
}


int trw_write_phy_reg(unsigned int rd,unsigned int ra)
{
	UWORD32 val   = 0;
	UWORD32 v, swich = 0;
	int ret;

	trw_read_phy_reg(&v,0xFF);
	if(v){
		/* it the current bank is not bank0, switch to bank0 by zhao*/
		internal_change_bank(0xff,  0x00);
		swich = 1;
	}
	wait_ready_timeout(10);

	ret = trw_write_reg(rd,rMAC_PHY_REG_RW_DATA);
	val = ((UWORD8)ra << 2); 
	val &= ~ BIT1;            
	val |= BIT0;              

	trw_write_reg(val, rMAC_PHY_REG_ACCESS_CON);

	if(swich)
		internal_change_bank(0xff,  v);
	return ret;
}
