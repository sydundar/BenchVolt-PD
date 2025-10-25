#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


#define STUSB4500_I2C_ADDR 0x28<<1 // 7-bit I2C adresi
#define REGISTER_COUNT      0x95 // STUSB4500'ün okunabilir maksimum register adresi


#define	REG_BCD_TYPEC_REV_LOW          0x06 // BCD_TYPEC_REV_LOW register
#define	REG_BCD_TYPEC_REV_HIGH         0x07 // BCD_TYPEC_REV_HIGH register
#define	REG_BCD_USBPD_REV_LOW          0x08 // BCD_USBPD_REV_LOW register
#define	REG_BCD_USBPD_REV_HIGH         0x09 // BCD_USBPD_REV_HIGH register
#define	REG_DEVICE_CAPAB_HIGH          0x0A // DEVICE_CAPAB_HIGH register
#define	REG_ALERT_STATUS_1             0x0B // ALERT_STATUS_1 register
#define	REG_ALERT_STATUS_1_MASK        0x0C // ALERT_STATUS_1_MASK register
#define	REG_PORT_STATUS_0              0x0D // PORT_STATUS_0 register
#define	REG_PORT_STATUS_1              0x0E // PORT_STATUS_1 register
#define	REG_TYPEC_MONITORING_STATUS_0  0x0F // TYPEC_MONITORING_STATUS_0 register
#define	REG_TYPEC_MONITORING_STATUS_1  0x10 // TYPEC_MONITORING_STATUS_1 register
#define	REG_CC_STATUS                  0x11 // CC_STATUS register
#define	REG_CC_HW_FAULT_STATUS_0       0x12 // CC_HW_FAULT_STATUS_0 register
#define	REG_CC_HW_FAULT_STATUS_1       0x13 // CC_HW_FAULT_STATUS_1 register
#define	REG_PD_TYPEC_STATUS            0x14 // PD_TYPEC_STATUS register
#define	REG_TYPEC_STATUS               0x15 // TYPEC_STATUS register
#define	REG_PRT_STATUS                 0x16 // PRT_STATUS register
#define	REG_PHY_STATUS                 0x17 // PHY_STATUS register
#define	REG_CC_CAPABILITY_CTRL         0x18 // CC_CAPABILITY_CTRL register
#define	REG_PRT_TX_CTRL                0x19 // PRT_TX_CTRL register
#define	REG_PD_COMMAND_CTRL            0x1A // PD_COMMAND_CTRL register
#define	REG_MONITORING_CTRL_0          0x20 // MONITORING_CTRL_0 register
#define	REG_MONITORING_CTRL_2          0x22 // MONITORING_CTRL_2 register
#define	REG_RESET_CTRL                 0x23 // RESET_CTRL register
#define	REG_VBUS_DISCHARGE_TIME_CTRL   0x25 // VBUS_DISCHARGE_TIME_CTRL register
#define	REG_VBUS_DISCHARGE_CTRL        0x26 // VBUS_DISCHARGE_CTRL register
#define	REG_VBUS_CTRL                  0x27 // VBUS_CTRL register
#define	REG_PE_FSM                     0x29 // PE_FSM register
#define	REG_GPIO_SW_GPIO               0x2D // GPIO_SW_GPIO register
#define	REG_DEVICE_ID                  0x2F // DEVICE_ID register
#define	REG_RX_BYTE_CNT                0x30 // RX_BYTE_CNT register
#define	REG_RX_HEADER_LOW              0x31 // RX_HEADER_LOW register
#define	REG_RX_HEADER_HIGH             0x32 // RX_HEADER_HIGH register
#define	REG_RX_DATA_OBJ1_0             0x33 // RX_DATA_OBJ1_0 register
#define	REG_RX_DATA_OBJ1_1             0x34 // RX_DATA_OBJ1_1 register
#define	REG_RX_DATA_OBJ1_2             0x35 // RX_DATA_OBJ1_2 register
#define	REG_RX_DATA_OBJ1_3             0x36 // RX_DATA_OBJ1_3 register
#define	REG_RX_DATA_OBJ2_0             0x37 // RX_DATA_OBJ2_0 register
#define	REG_RX_DATA_OBJ2_1             0x38 // RX_DATA_OBJ2_1 register
#define	REG_RX_DATA_OBJ2_2             0x39 // RX_DATA_OBJ2_2 register
#define	REG_RX_DATA_OBJ2_3             0x3A // RX_DATA_OBJ2_3 register
#define	REG_RX_DATA_OBJ3_0             0x3B // RX_DATA_OBJ3_0 register
#define	REG_RX_DATA_OBJ3_1             0x3C // RX_DATA_OBJ3_1 register
#define	REG_RX_DATA_OBJ3_2             0x3D // RX_DATA_OBJ3_2 register
#define	REG_RX_DATA_OBJ3_3             0x3E // RX_DATA_OBJ3_3 register
#define	REG_RX_DATA_OBJ4_0             0x3F // RX_DATA_OBJ4_0 register
#define	REG_RX_DATA_OBJ4_1             0x40 // RX_DATA_OBJ4_1 register
#define	REG_RX_DATA_OBJ4_2             0x41 // RX_DATA_OBJ4_2 register
#define	REG_RX_DATA_OBJ4_3             0x42 // RX_DATA_OBJ4_3 register
#define	REG_RX_DATA_OBJ5_0             0x43 // RX_DATA_OBJ5_0 register
#define	REG_RX_DATA_OBJ5_1             0x44 // RX_DATA_OBJ5_1 register
#define	REG_RX_DATA_OBJ5_2             0x45 // RX_DATA_OBJ5_2 register
#define	REG_RX_DATA_OBJ5_3             0x46 // RX_DATA_OBJ5_3 register
#define	REG_RX_DATA_OBJ6_0             0x47 // RX_DATA_OBJ6_0 register
#define	REG_RX_DATA_OBJ6_1             0x48 // RX_DATA_OBJ6_1 register
#define	REG_RX_DATA_OBJ6_2             0x49 // RX_DATA_OBJ6_2 register
#define	REG_RX_DATA_OBJ6_3             0x4A // RX_DATA_OBJ6_3 register
#define	REG_RX_DATA_OBJ7_0             0x4B // RX_DATA_OBJ7_0 register
#define	REG_RX_DATA_OBJ7_1             0x4C // RX_DATA_OBJ7_1 register
#define	REG_RX_DATA_OBJ7_2             0x4D // RX_DATA_OBJ7_2 register
#define	REG_RX_DATA_OBJ7_3             0x4E // RX_DATA_OBJ7_3 register
#define	REG_TX_HEADER_LOW              0x51 // TX_HEADER_LOW register
#define	REG_TX_HEADER_HIGH             0x52 // TX_HEADER_HIGH register
#define	REG_DPM_PDO_NUMB               0x70 // DPM_PDO_NUMB register
#define	REG_DPM_SNK_PDO1_0             0x85 // DPM_SNK_PDO1_0 register
#define	REG_DPM_SNK_PDO1_1             0x86 // DPM_SNK_PDO1_1 register
#define	REG_DPM_SNK_PDO1_2             0x87 // DPM_SNK_PDO1_2 register
#define	REG_DPM_SNK_PDO1_3             0x88 // DPM_SNK_PDO1_3 register
#define	REG_DPM_SNK_PDO2_0             0x89 // DPM_SNK_PDO2_0 register
#define	REG_DPM_SNK_PDO2_1             0x8A // DPM_SNK_PDO2_1 register
#define	REG_DPM_SNK_PDO2_2             0x8B // DPM_SNK_PDO2_2 register
#define	REG_DPM_SNK_PDO2_3             0x8C // DPM_SNK_PDO2_3 register
#define	REG_DPM_SNK_PDO3_0             0x8D // DPM_SNK_PDO3_0 register
#define	REG_DPM_SNK_PDO3_1             0x8E // DPM_SNK_PDO3_1 register
#define	REG_DPM_SNK_PDO3_2             0x8F // DPM_SNK_PDO3_2 register
#define	REG_DPM_SNK_PDO3_3             0x90 // DPM_SNK_PDO3_3 register
#define	REG_RDO_REG_STATUS_0           0x91 // RDO_REG_STATUS_0 register
#define	REG_RDO_REG_STATUS_1           0x92 // RDO_REG_STATUS_1 register
#define	REG_RDO_REG_STATUS_2           0x93 // RDO_REG_STATUS_2 register
#define	REG_RDO_REG_STATUS_3           0x94 // RDO_REG_STATUS_3 register



/////////

#ifndef STUSB4500_LOG
#define STUSB4500_LOG(fmt, ...)
#endif // STUSB4500_LOG

typedef bool (*stusb4500_write_t)(
  uint16_t addr, uint8_t reg, void const* buf, size_t len, void* context);
typedef bool (*stusb4500_read_t)(uint16_t addr, uint8_t reg, void* buf, size_t len, void* context);
typedef uint32_t (*stusb4500_get_ms_func_t)(void);

typedef uint16_t stusb4500_current_t;
typedef uint16_t stusb4500_voltage_t;

enum {
    STUSB4500_GPIO_CFG_SW_CTRL = 0x00UL,
    STUSB4500_GPIO_CFG_ERROR_RECOVERY = 0x01UL,
    STUSB4500_GPIO_CFG_DEBUG = 0x02UL,
    STUSB4500_GPIO_CFG_SINK_POWER = 0x03UL,
};
typedef uint8_t stusb4500_gpio_cfg_t;

enum {
    STUSB4500_GPIO_STATE_HIZ = 0UL,
    STUSB4500_GPIO_STATE_LOW = 1UL,
};
typedef uint8_t stusb4500_gpio_state_t;

typedef struct {
    uint16_t addr;
    stusb4500_write_t write;
    stusb4500_read_t read;
    void* context;
} stusb4500_t;

typedef struct {
    stusb4500_current_t min_current_ma;
    stusb4500_voltage_t min_voltage_mv;
    stusb4500_voltage_t max_voltage_mv;
    stusb4500_get_ms_func_t get_ms;
} stusb4500_config_t;

typedef struct {
    // PDO1 voltage fixed to 5V
    stusb4500_current_t pdo1_current_ma;

    stusb4500_voltage_t pdo2_voltage_mv;
    stusb4500_current_t pdo2_current_ma;

    stusb4500_voltage_t pdo3_voltage_mv;
    stusb4500_current_t pdo3_current_ma;

    // This current is used if PDO current is 0
    stusb4500_current_t pdo_current_fallback;
    // 1, 2, or 3
    uint8_t num_valid_pdos;
    // Choose the source PDO's current if voltage matches
    bool use_src_current;
    // Do not fall back to 5V if no PDO match
    bool only_above_5v;
    // GPIO configuration. See stusb4500_gpio_cfg_t
    stusb4500_gpio_cfg_t gpio_cfg;
} stusb4500_nvm_config_t;

bool stusb4500_negotiate(
  stusb4500_t const* dev, stusb4500_config_t const* config, bool on_interrupt);
bool stusb4500_set_gpio_state(stusb4500_t const* dev, stusb4500_gpio_state_t state);

bool stusb4500_nvm_read(stusb4500_t const* dev, uint8_t* nvm);
bool stusb4500_nvm_flash(stusb4500_t const* dev, stusb4500_nvm_config_t const* config);
void read_active_pdo();
