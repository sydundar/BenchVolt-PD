#include "stusb4500.h"

#include <assert.h>
#include <string.h>

/* NVM Registers
FTP_CUST_PASSWORD REG: address 0x95
    [7:0] : Password required to flash NVM, password = FTP_CUST_PASSWORD = 0x47

FTP_CTRL_0: address 0x96
    [7]   : FTP_CUST_PWR    : Power
    [6]   : FTP_CUST_RST_N  : Reset
    [5]   : --------------  : --------------
    [4]   : FTP_CUST_REQ    : Request operation
    [3]   : --------------  : --------------
    [2:0] : FTP_CUST_SECT   : Sector 0 - 4 selection

FTP_CTRL_1: address 0x97
    [7:3] : FTP_CUST_SER    : Sectors to erase (MSB = sector 4, LSB = sector 0)
    [2:0] : FTP_CUST_OPCODE : Opcode
            000 : Read sector
            001 : Write Program Load register (PL) with data to be written to
                  sector 0 or 1
            010 : Write FTP_CTRL_1[7:3] to Sector Erase register (SER)
            011 : Read PL
            100 : Read SER
            101 : Erase sectors masked by SER
            110 : Program sector selected by FTP_CTRL_0[2:0]
            111 : Soft program sectors masked by SER

RW_BUFFER: address 0x53
    [7:0] : Buffer used for reading and writing data */

#define FTP_CUST_PASSWORD_REG 0x95UL
#define FTP_CUST_PASSWORD 0x47UL
#define FTP_CTRL_0 0x96UL
#define FTP_CUST_PWR 0x80UL
#define FTP_CUST_RST_N 0x40UL
#define FTP_CUST_REQ 0x10UL
#define FTP_CUST_SECT 0x07UL
#define FTP_CTRL_1 0x97UL
#define FTP_CUST_SER 0xF8UL
#define FTP_CUST_OPCODE 0x07UL
#define RW_BUFFER 0x53UL

// Opcodes
#define READ 0x00UL             // Read memory array
#define WRITE_PL 0x01UL         // Shift in data on Program Load (PL) Register
#define WRITE_SER 0x02UL        // Shift in data on Sector Erase (SER) Register
#define READ_PL 0x03UL          // Shift out data on Program Load (PL) Register
#define READ_SER 0x04UL         // Shift out data on Sector Erase (SER) Register
#define ERASE_SECTOR 0x05UL     // Erase memory array
#define PROG_SECTOR 0x06UL      // Program 256b word into EEPROM
#define SOFT_PROG_SECTOR 0x07UL // Soft Program array

// Sector masks
#define SECTOR0 0x01UL
#define SECTOR1 0x02UL
#define SECTOR2 0x04UL
#define SECTOR3 0x08UL
#define SECTOR4 0x10UL

// Register masks
#define I_SNK_PDO1_POS 4UL
#define I_SNK_PDO1_MSK (0x0FUL << I_SNK_PDO1_POS)
#define I_SNK_PDO1_SECTOR 3UL
#define I_SNK_PDO1_OFFSET 2UL

#define I_SNK_PDO2_POS 0UL
#define I_SNK_PDO2_MSK (0x0FUL << I_SNK_PDO2_POS)
#define I_SNK_PDO2_SECTOR 3UL
#define I_SNK_PDO2_OFFSET 4UL

#define I_SNK_PDO3_POS 4UL
#define I_SNK_PDO3_MSK (0x0FUL << I_SNK_PDO3_POS)
#define I_SNK_PDO3_SECTOR 3UL
#define I_SNK_PDO3_OFFSET 5UL

#define I_SNK_PDO_FLEX_POS 2UL
#define I_SNK_PDO_FLEX_MSK (0x03FFUL << I_SNK_PDO_FLEX_POS)
#define I_SNK_PDO_FLEX_SECTOR 4UL
#define I_SNK_PDO_FLEX_OFFSET 3UL

#define V_SNK_PDO2_POS 6UL
#define V_SNK_PDO2_MSK (0x01FFUL << V_SNK_PDO2_POS)
#define V_SNK_PDO2_SECTOR 4UL
#define V_SNK_PDO2_OFFSET 0UL

#define V_SNK_PDO3_POS 0UL
#define V_SNK_PDO3_MSK (0x01FFUL << V_SNK_PDO3_POS)
#define V_SNK_PDO3_SECTOR 4UL
#define V_SNK_PDO3_OFFSET 2UL

#define SNK_PDO_NUMB_POS 1UL
#define SNK_PDO_NUMB_MSK (0x03UL << SNK_PDO_NUMB_POS)
#define SNK_PDO_NUMB_SECTOR 3UL
#define SNK_PDO_NUMB_OFFSET 2UL

#define REQ_SRC_CURRENT_POS 4UL
#define REQ_SRC_CURRENT_MSK (1UL << REQ_SRC_CURRENT_POS)
#define REQ_SRC_CURRENT_SECTOR 4UL
#define REQ_SRC_CURRENT_OFFSET 6UL

#define POWER_ONLY_ABOVE_5V_POS 3UL
#define POWER_ONLY_ABOVE_5V_MSK (1UL << POWER_ONLY_ABOVE_5V_POS)
#define POWER_ONLY_ABOVE_5V_SECTOR 4UL
#define POWER_ONLY_ABOVE_5V_OFFSET 6UL

#define GPIO_CFG_POS 4UL
#define GPIO_CFG_MSK (0x03UL << GPIO_CFG_POS)
#define GPIO_CFG_SECTOR 1UL
#define GPIO_CFG_OFFSET 0UL

// 5 sectors, 8 bytes each
#define NUM_SECTORS 5UL
#define SECTOR_SIZE 8UL
#define NVM_SIZE (NUM_SECTORS * SECTOR_SIZE)

#define PDO_VOLTAGE(mv) ((mv) / 50UL)
#define PDO_CURRENT(ma) (((ma)-250UL) / 250UL)
#define PDO_CURRENT_FLEX(ma) ((ma) / 10UL)

#define MODIFY_REG(reg, data, mask) reg = (((reg) & ~(mask)) | ((data) & (mask)))

static bool enter_write_mode(stusb4500_t const* dev) {
    uint8_t buffer;

    // Write FTP_CUST_PASSWORD to FTP_CUST_PASSWORD_REG
    buffer = FTP_CUST_PASSWORD;
    if (!dev->write(dev->addr, FTP_CUST_PASSWORD_REG, &buffer, 1, dev->context)) return false;

    // RW_BUFFER register must be NULL for Partial Erase feature
    buffer = 0x00;
    if (!dev->write(dev->addr, RW_BUFFER, &buffer, 1, dev->context)) return false;

    /* Begin NVM power on sequence */
    // Reset internal controller
    buffer = 0x00;
    if (!dev->write(dev->addr, FTP_CTRL_0, &buffer, 1, dev->context)) return false;

    // Set PWR and RST_N bits in FTP_CTRL_0
    buffer = FTP_CUST_PWR | FTP_CUST_RST_N;
    if (!dev->write(dev->addr, FTP_CTRL_0, &buffer, 1, dev->context)) return false;
    /* End NVM power on sequence */

    /* Begin sectors erase */
    // Format and mask sectors to erase and write SER write opcode
    buffer = (((SECTOR0 | SECTOR1 | SECTOR2 | SECTOR3 | SECTOR4) << 3) & FTP_CUST_SER) |
             (WRITE_SER & FTP_CUST_OPCODE);
    if (!dev->write(dev->addr, FTP_CTRL_1, &buffer, 1, dev->context)) return false;

    // Load SER write command
    buffer = FTP_CUST_PWR | FTP_CUST_RST_N | FTP_CUST_REQ;
    if (!dev->write(dev->addr, FTP_CTRL_0, &buffer, 1, dev->context)) return false;

    // Wait for execution
    do {
        if (!dev->read(dev->addr, FTP_CTRL_0, &buffer, 1, dev->context)) return false;
    } while (buffer & FTP_CUST_REQ);

    // Write soft program opcode
    buffer = SOFT_PROG_SECTOR & FTP_CUST_OPCODE;
    if (!dev->write(dev->addr, FTP_CTRL_1, &buffer, 1, dev->context)) return false;

    // Load soft program command
    buffer = FTP_CUST_PWR | FTP_CUST_RST_N | FTP_CUST_REQ;
    if (!dev->write(dev->addr, FTP_CTRL_0, &buffer, 1, dev->context)) return false;

    // Wait for execution
    do {
        if (!dev->read(dev->addr, FTP_CTRL_0, &buffer, 1, dev->context)) return false;
    } while (buffer & FTP_CUST_REQ);

    // Write erase sectors opcode
    buffer = ERASE_SECTOR & FTP_CUST_OPCODE;
    if (!dev->write(dev->addr, FTP_CTRL_1, &buffer, 1, dev->context)) return false;

    // Load erase sectors command
    buffer = FTP_CUST_PWR | FTP_CUST_RST_N | FTP_CUST_REQ;
    if (!dev->write(dev->addr, FTP_CTRL_0, &buffer, 1, dev->context)) return false;

    // Wait for execution
    do {
        if (!dev->read(dev->addr, FTP_CTRL_0, &buffer, 1, dev->context)) return false;
    } while (buffer & FTP_CUST_REQ);
    /* End sectors erase */

    return true;
}

static bool enter_read_mode(stusb4500_t const* dev) {
    uint8_t buffer;

    // Write FTP_CUST_PASSWORD to FTP_CUST_PASSWORD_REG
    buffer = FTP_CUST_PASSWORD;
    if (!dev->write(dev->addr, FTP_CUST_PASSWORD_REG, &buffer, 1, dev->context)) return false;

    /* Begin NVM power on sequence */
    // Reset internal controller
    buffer = 0x00;
    if (!dev->write(dev->addr, FTP_CTRL_0, &buffer, 1, dev->context)) return false;

    // Set PWR and RST_N bits in FTP_CTRL_0
    buffer = FTP_CUST_PWR | FTP_CUST_RST_N;
    if (!dev->write(dev->addr, FTP_CTRL_0, &buffer, 1, dev->context)) return false;
    /* End NVM power on sequence */

    return true;
}

static bool read_sector(stusb4500_t const* dev, uint8_t sector, uint8_t* sector_data) {
    if (!sector_data) return false;

    uint8_t buffer;

    // Set PWR and RST_N bits in FTP_CTRL_0
    buffer = FTP_CUST_PWR | FTP_CUST_RST_N;
    if (!dev->write(dev->addr, FTP_CTRL_0, &buffer, 1, dev->context)) return false;

    // Write sector read opcode
    buffer = (READ & FTP_CUST_OPCODE);
    if (!dev->write(dev->addr, FTP_CTRL_1, &buffer, 1, dev->context)) return false;

    // Select sector to read and load sector read command
    buffer = (sector & FTP_CUST_SECT) | FTP_CUST_PWR | FTP_CUST_RST_N | FTP_CUST_REQ;
    if (!dev->write(dev->addr, FTP_CTRL_0, &buffer, 1, dev->context)) return false;

    // Wait for execution
    do {
        if (!dev->read(dev->addr, FTP_CTRL_0, &buffer, 1, dev->context)) return false;
    } while (buffer & FTP_CUST_REQ);

    // Read sector data bytes from RW_BUFFER register
    if (!dev->read(dev->addr, RW_BUFFER, sector_data, 8, dev->context)) return false;

    // Reset internal controller
    buffer = 0x00;
    if (!dev->write(dev->addr, FTP_CTRL_0, &buffer, 1, dev->context)) return false;

    return true;
}

static bool write_sector(stusb4500_t const* dev, uint8_t sector_num, uint8_t const* sector_data) {
    if (!sector_data) return false;

    uint8_t buffer;

    // Write the 8 byte programming data to the RW_BUFFER register
    if (!dev->write(dev->addr, RW_BUFFER, sector_data, 8, dev->context)) return false;

    // Set PWR and RST_N bits in FTP_CTRL_0
    buffer = FTP_CUST_PWR | FTP_CUST_RST_N;
    if (!dev->write(dev->addr, FTP_CTRL_0, &buffer, 1, dev->context)) return false;

    // Write PL write opcode
    buffer = (WRITE_PL & FTP_CUST_OPCODE);
    if (!dev->write(dev->addr, FTP_CTRL_1, &buffer, 1, dev->context)) return false;

    // Load PL write command
    buffer = FTP_CUST_PWR | FTP_CUST_RST_N | FTP_CUST_REQ;
    if (!dev->write(dev->addr, FTP_CTRL_0, &buffer, 1, dev->context)) return false;

    // Wait for execution
    do {
        if (!dev->read(dev->addr, FTP_CTRL_0, &buffer, 1, dev->context)) return false;
    } while (buffer & FTP_CUST_REQ);

    // Write program sector opcode
    buffer = (PROG_SECTOR & FTP_CUST_OPCODE);
    if (!dev->write(dev->addr, FTP_CTRL_1, &buffer, 1, dev->context)) return false;

    // Load program sector command
    buffer = (sector_num & FTP_CUST_SECT) | FTP_CUST_PWR | FTP_CUST_RST_N | FTP_CUST_REQ;
    if (!dev->write(dev->addr, FTP_CTRL_0, &buffer, 1, dev->context)) return false;

    // Wait for execution
    do {
        if (!dev->read(dev->addr, FTP_CTRL_0, &buffer, 1, dev->context)) return false;
    } while (buffer & FTP_CUST_REQ);

    return true;
}

static bool exit_rw_mode(stusb4500_t const* dev) {
    uint8_t buffer;

    // Clear FTP_CTRL registers
    buffer = FTP_CUST_RST_N;
    if (!dev->write(dev->addr, FTP_CTRL_0, &buffer, 1, dev->context)) return false;
    buffer = 0x00;
    if (!dev->write(dev->addr, FTP_CTRL_1, &buffer, 1, dev->context)) return false;

    // Clear password
    buffer = 0x00;
    if (!dev->write(dev->addr, FTP_CUST_PASSWORD_REG, &buffer, 1, dev->context)) return false;

    return true;
}

static void apply_config(uint8_t* nvm, stusb4500_nvm_config_t const* config) {
    uint8_t(*p_nvm)[SECTOR_SIZE] = (uint8_t(*)[SECTOR_SIZE])nvm;

    MODIFY_REG(
      p_nvm[I_SNK_PDO1_SECTOR][I_SNK_PDO1_OFFSET],
      (PDO_CURRENT(config->pdo1_current_ma) << I_SNK_PDO1_POS) & I_SNK_PDO1_MSK,
      I_SNK_PDO1_MSK);

    MODIFY_REG(
      p_nvm[I_SNK_PDO2_SECTOR][I_SNK_PDO2_OFFSET],
      (PDO_CURRENT(config->pdo2_current_ma) << I_SNK_PDO2_POS) & I_SNK_PDO2_MSK,
      I_SNK_PDO2_MSK);

    MODIFY_REG(
      p_nvm[I_SNK_PDO3_SECTOR][I_SNK_PDO3_OFFSET],
      (PDO_CURRENT(config->pdo3_current_ma) << I_SNK_PDO3_POS) & I_SNK_PDO3_MSK,
      I_SNK_PDO3_MSK);

    MODIFY_REG(
      *((uint16_t*)&p_nvm[I_SNK_PDO_FLEX_SECTOR][I_SNK_PDO_FLEX_OFFSET]),
      (PDO_CURRENT_FLEX(config->pdo_current_fallback) << I_SNK_PDO_FLEX_POS) & I_SNK_PDO_FLEX_MSK,
      I_SNK_PDO_FLEX_MSK);

    MODIFY_REG(
      *((uint16_t*)&p_nvm[V_SNK_PDO2_SECTOR][V_SNK_PDO2_OFFSET]),
      (PDO_VOLTAGE(config->pdo2_voltage_mv) << V_SNK_PDO2_POS) & V_SNK_PDO2_MSK,
      V_SNK_PDO2_MSK);

    MODIFY_REG(
      *((uint16_t*)&p_nvm[V_SNK_PDO3_SECTOR][V_SNK_PDO3_OFFSET]),
      (PDO_VOLTAGE(config->pdo3_voltage_mv) << V_SNK_PDO3_POS) & V_SNK_PDO3_MSK,
      V_SNK_PDO3_MSK);

    MODIFY_REG(
      p_nvm[SNK_PDO_NUMB_SECTOR][SNK_PDO_NUMB_OFFSET],
      (config->num_valid_pdos << SNK_PDO_NUMB_POS) & SNK_PDO_NUMB_MSK,
      SNK_PDO_NUMB_MSK);

    MODIFY_REG(
      p_nvm[REQ_SRC_CURRENT_SECTOR][REQ_SRC_CURRENT_OFFSET],
      (config->use_src_current << REQ_SRC_CURRENT_POS) & REQ_SRC_CURRENT_MSK,
      REQ_SRC_CURRENT_MSK);

    MODIFY_REG(
      p_nvm[POWER_ONLY_ABOVE_5V_SECTOR][POWER_ONLY_ABOVE_5V_OFFSET],
      (config->only_above_5v << POWER_ONLY_ABOVE_5V_POS) & POWER_ONLY_ABOVE_5V_MSK,
      POWER_ONLY_ABOVE_5V_MSK);

    MODIFY_REG(
      p_nvm[GPIO_CFG_SECTOR][GPIO_CFG_OFFSET],
      (config->gpio_cfg << GPIO_CFG_POS) & GPIO_CFG_MSK,
      GPIO_CFG_MSK);
}

bool stusb4500_nvm_read(stusb4500_t const* dev, uint8_t* nvm) {
    if (!nvm) return false;

    if (!enter_read_mode(dev)) return false;

    uint8_t* p_nvm = nvm;
    for (uint8_t sector = 0; sector < NUM_SECTORS; sector++) {
        if (!read_sector(dev, sector, p_nvm)) return false;
        p_nvm += SECTOR_SIZE;
    }

    if (!exit_rw_mode(dev)) return false;

    return true;
}

bool stusb4500_nvm_flash(stusb4500_t const* dev, stusb4500_nvm_config_t const* config) {
    uint8_t nvm[NUM_SECTORS][SECTOR_SIZE];
    uint8_t nvm_modified[NUM_SECTORS][SECTOR_SIZE];

    if (!config) return false;

    if (!stusb4500_nvm_read(dev, (uint8_t*)nvm)) return false;

    memcpy(nvm_modified, nvm, NVM_SIZE);
    apply_config((uint8_t*)nvm_modified, config);

    if (memcmp(nvm_modified, nvm, NVM_SIZE) == 0) return exit_rw_mode(dev);

    if (!enter_write_mode(dev)) return false;

    uint8_t* p_nvm = (uint8_t*)nvm_modified;
    for (uint8_t sector = 0; sector < NUM_SECTORS; sector++) {
        if (!write_sector(dev, sector, p_nvm)) return false;
        p_nvm += SECTOR_SIZE;
    }

    if (!exit_rw_mode(dev)) return false;

    if (!stusb4500_nvm_read(dev, (uint8_t*)nvm)) return false;

    return (memcmp(nvm, nvm_modified, NVM_SIZE) == 0);
}
