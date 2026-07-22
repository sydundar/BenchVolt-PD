/**
******************************************************************************
* File Name          : USB_PD_core.h
* Date               : 03/09/2015 10:51:46
* Description        : This file contains all the functions prototypes for 
*                      the USB_PD_core  
******************************************************************************
*
* COPYRIGHT(c) 2015 STMicroelectronics
*
* Redistribution and use in source and binary forms, with or without modification,
* are permitted provided that the following conditions are met:
*   1. Redistributions of source code must retain the above copyright notice,
*      this list of conditions and the following disclaimer.
*   2. Redistributions in binary form must reproduce the above copyright notice,
*      this list of conditions and the following disclaimer in the documentation
*      and/or other materials provided with the distribution.
*   3. Neither the name of STMicroelectronics nor the names of its contributors
*      may be used to endorse or promote products derived from this software
*      without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
******************************************************************************
*/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USB_PD_CORE_H
#define __USB_PD_CORE_H
#ifdef __cplusplus
extern "C" {
#endif
  
  /* Includes ------------------------------------------------------------------*/
#if defined (STM32L476xx)
#include "stm32l4xx_hal.h"
#elif defined(STM32F072xB)
#include "stm32f0xx_hal.h"
#elif defined(STM32F401xE)
#include "stm32f4xx_hal.h"
  
#endif


#include "USB_PD_defines_STUSB-GEN1S.h"
  
#define USBPORT_MAX 2
#define I2CBUS_MAX 1
  
#define true 1
#define false 0    
  
#define LE16(addr) (((uint16_t)(*((uint8_t *)(addr))))\
  + (((uint16_t)(*(((uint8_t *)(addr)) + 1))) << 8))

#define LE32(addr) ((((uint32_t)(*(((uint8_t *)(addr)) + 0))) + \
(((uint32_t)(*(((uint8_t *)(addr)) + 1))) << 8) + \
  (((uint32_t)(*(((uint8_t *)(addr)) + 2))) << 16) + \
    (((uint32_t)(*(((uint8_t *)(addr)) + 3))) << 24)))



typedef struct
  {
    uint8_t                                         HW_Reset ;
    STUSB_GEN1S_CC_DETECTION_STATUS_RegTypeDef      Port_Status;/*!< Specifies the Port status register */
    uint8_t TypeC;
    STUSB_GEN1S_CC_STATUS_RegTypeDef                CC_status;
    STUSB_GEN1S_MONITORING_STATUS_RegTypeDef        Monitoring_status;         /*!< Specifies the  */
    STUSB_GEN1S_HW_FAULT_STATUS_RegTypeDef          HWFault_status;
    STUSB_GEN1S_PRT_STATUS_RegTypeDef               PRT_status;    /*!< Specifies t */
    STUSB_GEN1S_PHY_STATUS_RegTypeDef               Phy_status;     /*!<  */

  }USB_PD_StatusTypeDef;


  typedef union
    {
      uint32_t d32;
      struct
        {
          uint32_t Max_Operating_Current :10;
          uint32_t Voltage :10;
          uint8_t PeakCurrent:2;
          uint8_t Reserved :2;
          uint8_t Unchuncked_Extended:1;
          uint8_t Dual_RoleData:1;
          uint8_t Communication:1;
          uint8_t UnconstraintPower:1;
          uint8_t SuspendSupported:1;
          uint8_t DualRolePower :1;
          uint8_t FixedSupply:2;
        } fix;
      struct
        {
          uint32_t Operating_Current :10;
          uint32_t Min_Voltage:10;
          uint32_t Max_Voltage:10;
          uint8_t VariableSupply:2;
        }var;
      struct
        {
          uint32_t Operating_Power :10;
          uint32_t Min_Voltage:10;
          uint32_t Max_Voltage:10;
          uint8_t Battery:2;
        }bat;
      struct
        {
          /*
          uint8_t Max_Current :7;
          uint8_t  Reserved0 :1;
          uint8_t Min_Voltage:8;
          uint8_t  Reserved1 :1;
          uint8_t Max_Voltage:9;
          uint8_t  Reserved2 :2;
          uint8_t ProgDev : 2 ;
          uint8_t Battery:2;
          */
         uint8_t Max_Current :7;
         uint8_t  Reserved0 :1;
         uint16_t Min_Voltage:8; /* to prevent packing issue ?? */
         uint8_t  Reserved1 :1;
         uint16_t Max_Voltage:8;
         uint8_t  Reserved2 :3;
         uint8_t ProgDev : 2 ;
         uint8_t Battery:2;

        }apdo;
    }USB_PD_SRC_PDOTypeDef;




uint8_t Policy_Engine_State[USBPORT_MAX];
uint8_t Policy_Engine_Previous_State[USBPORT_MAX];
uint8_t Go_disable_once[USBPORT_MAX];
uint8_t TypeC_state[USBPORT_MAX];
uint8_t TypeC_Previous_state[USBPORT_MAX];
uint8_t Final_Nego_done[USBPORT_MAX] ;
uint8_t Core_Process_suspended = 0 ;

uint8_t Cut[USBPORT_MAX];
STUSB_GEN1S_ALERT_STATUS_MASK_RegTypeDef Alert_Mask;
USB_PD_StatusTypeDef PD_status[USBPORT_MAX] ;
USB_PD_SNK_PDO_TypeDef PDO_SNK[USBPORT_MAX][3];

USB_PD_SRC_PDOTypeDef PDO_FROM_SRC[USBPORT_MAX][7];
uint8_t PDO_FROM_SRC_Num[USBPORT_MAX]={0};
uint8_t PDO_FROM_SRC_Num_Sel[USBPORT_MAX]={0};
uint8_t PDO_FROM_SRC_Valid[USBPORT_MAX]={0};
STUSB_GEN1S_RDO_REG_STATUS_RegTypeDef Nego_RDO[USBPORT_MAX];
uint8_t TypeC_Only_status[USBPORT_MAX] = {0} ;
uint8_t PDO_SNK_NUMB[USBPORT_MAX];

typedef uint8_t bool;


  typedef struct  
    {
      uint8_t Usb_Port;
      uint8_t I2cBus ;
      uint8_t I2cDeviceID_7bit;
      uint8_t Dev_Cut;
      uint8_t Alert_GPIO_Pin;
      uint8_t Alert_GPIO_Bank;
    }USB_PD_I2C_PORT;




/** @defgroup USBPD_MsgHeaderStructure_definition USB PD Message header Structure definition
* @brief USB PD Message header Structure definition
* @{
*/
typedef union
  {
    uint16_t d16;
    struct
      {
#if defined(USBPD_REV30_SUPPORT)
        uint16_t MessageType :                  /*!< Message Header's message Type                      */
          5;
#else /* USBPD_REV30_SUPPORT */
          uint16_t MessageType :                  /*!< Message Header's message Type                      */
            4;
            uint16_t Reserved4 :                    /*!< Reserved                                           */
              1;
#endif /* USBPD_REV30_SUPPORT */
              uint16_t PortDataRole :                 /*!< Message Header's Port Data Role                    */
                1;                                    
                uint16_t SpecificationRevision :        /*!< Message Header's Spec Revision                     */
                  2;       
                  uint16_t PortPowerRole_CablePlug :      /*!< Message Header's Port Power Role/Cable Plug field  */
                    1;       
                    uint16_t MessageID :                    /*!< Message Header's message ID                        */
                      3;    
                      uint16_t NumberOfDataObjects :          /*!< Message Header's Number of data object             */
                        3;                
                        uint16_t Extended :                     /*!< Reserved                                           */
                          1; 
      }
    b;
  } USBPD_MsgHeader_TypeDef;





typedef struct 
  {
    uint8_t PHY;
    uint8_t PRL;
    uint8_t BIST;
    uint8_t PE;
    uint8_t TypeC;      
  }USB_PD_Debug_FSM_TypeDef;

// Policy interface #define
#define Not_Connected               0   // Device is not connected 
#define TypeC_Only                  1   // Device Connected in type C only 
#define Connected_Unknown_SRC_PDOs  2   // Device connected but PDO from source are not available soft reset requested 
#define Connected_5V_PD             3   // Device connected 5V PD 
#define Connected_no_Match_found    4   // Device connected but no ideal matching found 
#define Connected_Matching_ongoing  5   // Device connected matching found request ongoing 
#define Connected_Mached            6   // Device connected matching found power is OK  
#define Not_Connected_attached_wait 7   // Device is not connected but in attached wait (monitoring issue ) reset by sw register is on going  
#define Policy_Defauld              8   // Default no action
#define Hard_Reset_ongoing          9   // Hard reset ongoing



void HW_Reset_state(uint8_t Usb_Port);
void SW_reset_by_Reg(uint8_t Usb_Port);
void Send_Soft_reset_Message(uint8_t Usb_Port);
void Send_Get_Src_Cap_Message(uint8_t Usb_Port);
void usb_pd_init(uint8_t Usb_Port);
void ALARM_MANAGEMENT(uint8_t Usb_Port);
void Read_SNK_PDO(uint8_t Usb_Port);
void Print_SNK_PDO(uint8_t Usb_Port);
void Print_PDO_FROM_SRC(uint8_t Usb_Port);
void Read_RDO(uint8_t Usb_Port);
void Print_RDO(uint8_t Usb_Port);
void Update_PDO(uint8_t Usb_Port,uint8_t PDO_Number,int Voltage,int Current);
void Update_PDO1(uint8_t Usb_Port,int Current);
void Update_Valid_PDO_Number(uint8_t Usb_Port,uint8_t Number_PDO);
int  Find_Matching_SRC_PDO(uint8_t Usb_Port,int Min_Power,int Min_V , int Max_V);
int  Request_SRC_PDO_NUMBER(uint8_t Usb_Port, uint8_t SRC_PDO_position);
int update_SNK_PDO_W_SRC_PDO_NUMBER(uint8_t Usb_Port, uint8_t SRC_PDO_position);
void Negotiate_5V(uint8_t Usb_Port);
void Print_PDO_FROM_SRC(uint8_t Usb_Port);
int Get_Device_STATUS(uint8_t Usb_Port);
void Print_Type_C_Only_Status(uint8_t Usb_Port);



#ifdef __cplusplus
}
#endif

#endif /*usbpd core header */

/**
* @}
*/

/**
* @}
*/

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
