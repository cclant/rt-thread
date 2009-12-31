/******************** (C) COPYRIGHT 2009 STMicroelectronics ********************
* File Name          : mass_mal.c
* Author             : MCD Application Team
* Version            : V3.0.1
* Date               : 04/27/2009
* Description        : Medium Access Layer interface
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "platform_config.h"
#include "sdcard.h"
#include "mass_mal.h"
#include "rtthread.h"

uint32_t Mass_Memory_Size[2];
uint32_t Mass_Block_Size[2];
uint32_t Mass_Block_Count[2];

rt_device_t dev_sdio = RT_NULL;

uint16_t MAL_Init(uint8_t lun)
{
    uint16_t status = MAL_OK;

    switch (lun)
    {
    case 0:
        status = MAL_OK;
        break;
    default:
        return MAL_FAIL;
    }
    return status;
}

uint16_t MAL_Write(uint8_t lun, uint32_t Memory_Offset, uint32_t *Writebuff, uint16_t Transfer_Length)
{

    switch (lun)
    {
    case 0:
    {
        dev_sdio->write(dev_sdio,Memory_Offset,Writebuff,Transfer_Length);
    }
    break;
    default:
        return MAL_FAIL;
    }
    return MAL_OK;
}

uint16_t MAL_Read(uint8_t lun, uint32_t Memory_Offset, uint32_t *Readbuff, uint16_t Transfer_Length)
{

    switch (lun)
    {
    case 0:
    {
        dev_sdio->read(dev_sdio,Memory_Offset,Readbuff,Transfer_Length);
    }
    break;
    default:
        return MAL_FAIL;
    }
    return MAL_OK;
}

uint16_t MAL_GetStatus (uint8_t lun)
{
    if(lun == 0)
    {
        return MAL_OK;
    }
    return MAL_FAIL;
}

/******************* (C) COPYRIGHT 2009 STMicroelectronics *****END OF FILE****/
