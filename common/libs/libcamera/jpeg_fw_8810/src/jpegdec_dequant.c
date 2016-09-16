/******************************************************************************
 ** File Name:      JpegDec_dequant.c                                            *
 ** Author:         yi.wang													  *
 ** DATE:           07/12/2007                                                *
 ** Copyright:      2007 Spreadtrum, Incoporated. All Rights Reserved.        *
 ** Description:    Initialize the encoder									  *
 ** Note:           None                                                      *
******************************************************************************/
/******************************************************************************
 **                        Edit History                                       *
 ** ------------------------------------------------------------------------- *
 ** DATE           NAME             DESCRIPTION                               *
 ** 07/12/2007     yi.wang	         Create.                                  *
******************************************************************************/
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "sc8810_video_header.h"

#if !defined(_SIMULATION_)
//#include "os_api.h"
#endif

/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

//#if defined(JPEG_DEC)
//////////////////////////////////////////////////////////////////////////

uint8  *jpeg_fw_quant_tbl[2];
uint16 *jpeg_fw_quant_tbl_new[2];

PUBLIC void JPEGFW_InitQuantTbl(JPEG_QUALITY_E level)
{
	JPEG_CODEC_T *jpeg_fw_codec = Get_JPEGDecCodec();

	SCI_ASSERT(jpeg_fw_codec != PNULL);

	jpeg_fw_codec->quant_tbl[JPEG_FW_LUM_ID] = jpeg_fw_lum_quant_tbl_default[level];
	jpeg_fw_codec->quant_tbl[JPEG_FW_CHR_ID] = jpeg_fw_chr_quant_tbl_default[level];

	jpeg_fw_codec->tbl_map[JPEG_FW_Y_ID].quant_tbl_id = JPEG_FW_LUM_ID;
	jpeg_fw_codec->tbl_map[JPEG_FW_U_ID].quant_tbl_id = JPEG_FW_CHR_ID;
	jpeg_fw_codec->tbl_map[JPEG_FW_V_ID].quant_tbl_id = JPEG_FW_CHR_ID;
}

PUBLIC JPEG_RET_E JPEGFW_AdjustQuantTbl_Dec()
{
	int32 time_out_flag = 0;
	uint8 tbl_id = 0;
	uint8 tbl_num = 0;
	uint32 qtable_addr = (uint32)JDEC_IQUANT_TBL_ADDR;
	uint32 cmd = 0;
	int32 tmp = 0;
	int32 index1 = 0, index2 = 0;
	uint16 yuv_id = 0;
	JPEG_CODEC_T *jpeg_fw_codec = Get_JPEGDecCodec();

	SCI_ASSERT(jpeg_fw_codec != PNULL);

	if(jpeg_fw_codec->num_components == 1) 
	{
		tbl_num = 1;
	}else
	{
		tbl_num = 2;
	}
	
	if(!jpeg_fw_codec->progressive_mode)
	{
		cmd = (1<<4) | (1<<3);	
		VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_CFG_OFF, cmd, "DCAM_CFG: allow software to access the vsp buffer");
		
		time_out_flag = VSP_READ_REG_POLL(VSP_DCAM_REG_BASE+DCAM_CFG_OFF, (1<<7), (1<<7), TIME_OUT_CLK, "DCAM_CFG: polling dcam clock status");	

		if(time_out_flag != 0)
		{
			return JPEG_FAILED;
		}

		for(tbl_id = 0; tbl_id < tbl_num; tbl_id++)
		{
			const uint8  *quant_tbl_current = PNULL;
			uint32 i = 0;

			yuv_id = jpeg_fw_codec->tbl_map[tbl_id].quant_tbl_id;
			quant_tbl_current = jpeg_fw_codec->quant_tbl[yuv_id];

			for(i=0; i < JPEG_FW_DCTSIZE2; i+=2)
			{				
				index1 = jpeg_fw_ASIC_DCT_Matrix[i];
				index2 = jpeg_fw_ASIC_DCT_Matrix[i+1];	/*lint !e661 */
				
				tmp = ((quant_tbl_current[index1] & 0xFFFF) | (quant_tbl_current[index2]<<16));
				VSP_WRITE_REG(qtable_addr, tmp, "INV_QUANT_TAB_ADDR: Write Qtalbe into Qbuffer");
				qtable_addr += 4;		
			#if _CMODEL_
				{
				//	FILE *qfile = fopen("D:/SC6800H/code/Firmware/jpeg_codec/simulation/VC/Dec_WinPrj/trace/dct/qtable.txt", "ab+");
				//	fprintf_oneWord_hex(qfile, tmp);
				//	fclose(qfile);
				}
			#endif//_CMODEL_	
			}
		}
		
		cmd = (0<<4) | (1<<3);		//allow hardware to access the vsp buffer
		VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_CFG_OFF, cmd, "DCAM_CFG: configure DCAM register");
		
		time_out_flag = VSP_READ_REG_POLL(VSP_DCAM_REG_BASE+DCAM_CFG_OFF, 0, 0, TIME_OUT_CLK, "DCAM_CFG: polling dcam clock status");

		if(time_out_flag != 0)
		{
			return JPEG_FAILED;
		}
	}
	
	return JPEG_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////
//#endif //JPEG_DEC
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
