#include "stusb4500.h"
#include "stm32_sw_i2c.h"
#include <stdio.h>
#include <main.h>
#include <string.h>

// STUSB4500 registers
#define STUSB_PORT_STATUS 0x0EUL
#define STUSB_PRT_STATUS 0x16UL
#define STUSB_CMD_CTRL 0x1AUL
#define STUSB_RESET_CTRL 0x23UL
#define STUSB_PE_FSM 0x29UL
#define STUSB_GPIO3_SW_GPIO 0x2DUL
#define STUSB_WHO_AM_I 0x2FUL
#define STUSB_RX_BYTE_CNT 0x30UL
#define STUSB_RX_HEADER 0x31UL
#define STUSB_RX_DATA_OBJ 0x33UL
#define STUSB_TX_HEADER 0x51UL
#define STUSB_DPM_SNK_PDO1 0x85UL

// STUSB4500 masks/constants
#define STUSB4500_ID 0x25UL
#define STUSB4500B_ID 0x21UL
#define STUSB_SW_RESET_ON 0x01UL
#define STUSB_SW_RESET_OFF 0x00UL
#define STUSB_ATTACH 0x01UL
#define STUSB_PRT_MESSAGE_RECEIVED 0x04UL
#define STUSB_SRC_CAPABILITIES_MSG 0x01UL
#define STUSB_PE_SNK_READY 0x18UL

// Maximum number of source power profiles
#define MAX_SRC_PDOS 10UL

// PD protocol commands, see USB PD spec Table 6-3
#define PD_CMD 0x26UL
#define PD_SOFT_RESET 0x000DUL
#define PD_GET_SRC_CAP 0x0007UL

// See USB PD spec Table 6-1
#define MESSAGE_HEADER_POS 0UL
#ifdef USBPD_REV30_SUPPORT
#define MESSAGE_HEADER_MSK (0x1FUL << MESSAGE_HEADER_POS)
#else // USBPD_REV30_SUPPORT
#define MESSAGE_HEADER_MSK (0x0FUL << MESSAGE_HEADER_POS)
#endif // USBPD_REV30_SUPPORT
#define HEADER_NUM_DATA_OBJECTS_POS 12UL
#define HEADER_NUM_DATA_OBJECTS_MSK (0x07UL << HEADER_NUM_DATA_OBJECTS_POS)
#define HEADER_MESSAGE_TYPE(header) (((header)&MESSAGE_HEADER_MSK) >> MESSAGE_HEADER_POS)
#define HEADER_NUM_DATA_OBJECTS(header)                                                            \
    (((header)&HEADER_NUM_DATA_OBJECTS_MSK) >> HEADER_NUM_DATA_OBJECTS_POS)

// See USB PD spec Section 7.1.3 and STUSB4500 Section 5.2 Table 16
#define PDO_TYPE_POS 30UL
#define PDO_TYPE_MSK (0x03UL << PDO_TYPE_POS)
#define PDO_TYPE(pdo) (((pdo)&PDO_TYPE_MSK) >> PDO_TYPE_POS)
#define PDO_TYPE_FIXED 0x00UL

#define PDO_CURRENT_POS 0UL
#define PDO_CURRENT_MSK (0x03FFUL << PDO_CURRENT_POS)
#define PDO_CURRENT_RESOLUTION 10UL
#define FROM_PDO_CURRENT(pdo)                                                                      \
    ((((pdo)&PDO_CURRENT_MSK) >> PDO_CURRENT_POS) * PDO_CURRENT_RESOLUTION)
#define TO_PDO_CURRENT(ma) ((((ma) / PDO_CURRENT_RESOLUTION) << PDO_CURRENT_POS) & PDO_CURRENT_MSK)

#define PDO_VOLTAGE_POS 10UL
#define PDO_VOLTAGE_MSK (0x03FFUL << PDO_VOLTAGE_POS)
#define PDO_VOLTAGE_RESOLUTION 50UL
#define FROM_PDO_VOLTAGE(pdo)                                                                      \
    ((((pdo)&PDO_VOLTAGE_MSK) >> PDO_VOLTAGE_POS) * PDO_VOLTAGE_RESOLUTION)
#define TO_PDO_VOLTAGE(mv) ((((mv) / PDO_VOLTAGE_RESOLUTION) << PDO_VOLTAGE_POS) & PDO_VOLTAGE_MSK)

#define TIMEOUT_MS 500UL

typedef uint32_t stusb4500_power_t;
typedef uint32_t stusb4500_pdo_t;
typedef uint8_t stusb4500_pd_state_t;

stusb4500_t dev = {
    .addr = STUSB4500_I2C_ADDR,
    .write = my_stusb4500_write,
    .read = my_stusb4500_read,
    .context = NULL
};

stusb4500_config_t config = {
    .min_current_ma = 500,  // 0.5A minimum
    .min_voltage_mv = 5000,  // 5V minimum
    .max_voltage_mv = 20000, // 20V maksimum
    .get_ms = my_get_ms
};



////////////////////////////////////////////////////



static void local_write_reg(uint8_t reg_addr, uint8_t data) {
    uint8_t reg[2] = {reg_addr, data};
    // main.c'deki I2C yapınıza uygun yazma
    I2C_transmit(STUSB4500_I2C_ADDR, reg, 2);
}


void HardReset()
{
    local_write_reg(REG_RESET_CTRL, 0x01); // SW_RST
    HAL_Delay(20);                         // Bekle
    local_write_reg(REG_RESET_CTRL, 0x00); // Reseti Kaldir
}

////////////////////////////////////////////////////

static bool send_pd_message(stusb4500_t const* dev, uint16_t msg) {
    uint8_t cmd = PD_CMD;
    return dev->write(dev->addr, STUSB_TX_HEADER, &msg, sizeof(uint16_t), dev->context) &&
           dev->write(dev->addr, STUSB_CMD_CTRL, &cmd, 1, dev->context);
}

static bool is_present(stusb4500_t const* dev) {
    uint8_t res;
    if (!dev->read(dev->addr, STUSB_WHO_AM_I, &res, 1, dev->context)) return false;

    return (res == STUSB4500_ID || res == STUSB4500B_ID);
}

static bool
  wait_until_ready_with_timeout(stusb4500_t const* dev, stusb4500_config_t const* config) {
    stusb4500_pd_state_t pd_state;
    uint32_t start = 0;

    if (config->get_ms) {
        start = config->get_ms();
    }

    do {
        if (config->get_ms && (config->get_ms() - start > TIMEOUT_MS)) return false;
        if (!dev->read(dev->addr, STUSB_PE_FSM, &pd_state, 1, dev->context)) return false;
    } while (pd_state != STUSB_PE_SNK_READY);

    return true;
}




static bool write_pdo( stusb4500_t const* dev,  stusb4500_current_t current_ma,  stusb4500_voltage_t voltage_mv,  uint8_t pdo_num)
{
    if (pdo_num < 1 || pdo_num > 3) return false;

    // Format the sink PDO
    stusb4500_pdo_t pdo = TO_PDO_CURRENT(current_ma) | TO_PDO_VOLTAGE(voltage_mv);

    // Write the sink PDO
    return dev->write(dev->addr, STUSB_DPM_SNK_PDO1 + sizeof(stusb4500_pdo_t) * (pdo_num - 1), &pdo, sizeof(stusb4500_pdo_t), dev->context);
}

bool Set_PD_VoltageCurrent(uint8_t pdo_num, uint32_t voltage_mv, uint32_t current_ma) {

	if (write_pdo(&dev, (stusb4500_current_t)current_ma, (stusb4500_voltage_t)voltage_mv, pdo_num))
	{
		send_soft_reset();
		return true;
    }
     return false;
}

static bool load_optimal_pdo( stusb4500_t const* dev,stusb4500_config_t const* config, stusb4500_pdo_t const* src_pdos,  uint8_t num_pdos)
{
    bool found = false;

    stusb4500_current_t opt_pdo_current = config->min_current_ma;
    stusb4500_voltage_t opt_pdo_voltage = config->min_voltage_mv;
    stusb4500_power_t opt_pdo_power =
      (stusb4500_power_t)opt_pdo_voltage * (stusb4500_power_t)opt_pdo_current / 1000UL;

    // Search for the optimal PDO, if any
    for (int i = 0; i < num_pdos; i++) {
        stusb4500_pdo_t const pdo = src_pdos[i];

        // Extract PDO parameters
        stusb4500_current_t pdo_current = FROM_PDO_CURRENT(pdo);
        stusb4500_voltage_t pdo_voltage = FROM_PDO_VOLTAGE(pdo);
        stusb4500_power_t pdo_power =
          (stusb4500_power_t)pdo_current * (stusb4500_power_t)pdo_voltage / 1000UL;

        STUSB4500_LOG(
          "Detected Source PDO: %2d.%03dV, %d.%03dA, %3d.%03dW\r\n",
          (int)(pdo_voltage / 1000UL),
          (int)(pdo_voltage % 1000UL),
          (int)(pdo_current / 1000UL),
          (int)(pdo_current % 1000UL),
          (int)((stusb4500_power_t)pdo_power / 1000UL),
          (int)((stusb4500_power_t)pdo_power % 1000UL));

        if (
          PDO_TYPE(pdo) != PDO_TYPE_FIXED || pdo_current < config->min_current_ma ||
          pdo_voltage < config->min_voltage_mv || pdo_voltage > config->max_voltage_mv)
            continue;
        if (pdo_power > opt_pdo_power) {
            opt_pdo_current = pdo_current;
            opt_pdo_voltage = pdo_voltage;
            opt_pdo_power = pdo_power;
            found = true;
        }
    }

    STUSB4500_LOG(
      "\r\nSelecting optimal PDO based on user parameters: %d.%03dV - %d.%03dV, >= "
      "%d.%03dA\r\n",
      (int)(config->min_voltage_mv / 1000UL),
      (int)(config->min_voltage_mv % 1000UL),
      (int)(config->max_voltage_mv / 1000UL),
      (int)(config->max_voltage_mv % 1000UL),
      (int)(config->min_current_ma / 1000UL),
      (int)(config->min_current_ma % 1000UL));
    if (found) {
        STUSB4500_LOG(
          "Selected PDO: %d.%03dV, %d.%03dA, %d.%03dW\r\n\r\n",
          (int)(opt_pdo_voltage / 1000UL),
          (int)(opt_pdo_voltage % 1000UL),
          (int)(opt_pdo_current / 1000UL),
          (int)(opt_pdo_current % 1000UL),
          (int)((stusb4500_power_t)opt_pdo_power / 1000UL),
          (int)((stusb4500_power_t)opt_pdo_power % 1000UL));
    } else {
        STUSB4500_LOG("No suitable PDO found\r\n\r\n");
        return false;
    }

    // Push the new PDO
    if (!write_pdo(dev, opt_pdo_current, opt_pdo_voltage, 3)) return false;

    return true;
}


static bool load_max_pdo(stusb4500_t const* dev, stusb4500_config_t const* config, stusb4500_pdo_t const* src_pdos, uint8_t num_pdos)
{
    bool found = false;

    // Başlangıç değerlerini 0 yapıyoruz ki her türlü geçerli PDO aday olabilsin
    stusb4500_current_t max_pdo_current = 0;
    stusb4500_voltage_t max_pdo_voltage = 0;
    stusb4500_power_t max_pdo_power = 0;

    STUSB4500_LOG("Searching for MAXIMUM Power PDO (Limit: %d.%03dV)\r\n",
                 (int)(config->max_voltage_mv / 1000UL), (int)(config->max_voltage_mv % 1000UL));

    for (int i = 0; i < num_pdos; i++) {
        stusb4500_pdo_t const pdo = src_pdos[i];

        // PDO parametrelerini ayıkla
        stusb4500_current_t pdo_current = FROM_PDO_CURRENT(pdo);
        stusb4500_voltage_t pdo_voltage = FROM_PDO_VOLTAGE(pdo);
        stusb4500_power_t pdo_power = (stusb4500_power_t)pdo_current * (stusb4500_power_t)pdo_voltage / 1000UL;

        STUSB4500_LOG("Source PDO [%d]: %2d.%03dV, %d.%03dA, %3d.%03dW\r\n", i,
                     (int)(pdo_voltage / 1000UL), (int)(pdo_voltage % 1000UL),
                     (int)(pdo_current / 1000UL), (int)(pdo_current % 1000UL),
                     (int)(pdo_power / 1000UL), (int)(pdo_power % 1000UL));

        // KRİTİK FİLTRE: Sadece Sabit (Fixed) voltaj ve donanımın yanmayacağı (Max Voltaj) PDO'ları kabul et
        if (PDO_TYPE(pdo) != PDO_TYPE_FIXED || pdo_voltage > config->max_voltage_mv) {
            continue;
        }

        // Karşılaştırma: Eğer bu PDO şu ana kadar bulunan en yüksek güçten büyük veya eşitse kaydet
        // (Eşitlik durumu voltajı daha yüksek olan profili tercih etmenizi sağlar)
        if (pdo_power >= max_pdo_power) {
            max_pdo_current = pdo_current;
            max_pdo_voltage = pdo_voltage;
            max_pdo_power = pdo_power;
            found = true;
        }
    }

    if (found)
    {
        STUSB4500_LOG("--> SELECTED MAX PDO: %d.%03dV, %d.%03dA, %d.%03dW\r\n\r\n",
                     (int)(max_pdo_voltage / 1000UL), (int)(max_pdo_voltage % 1000UL),
                     (int)(max_pdo_current / 1000UL), (int)(max_pdo_current % 1000UL),
                     (int)(max_pdo_power / 1000UL), (int)(max_pdo_power % 1000UL));

        // PDO3 slotuna en yüksek gücü yaz
        if (!write_pdo(dev, max_pdo_current, max_pdo_voltage, 3)) return false;
    }
    else
    {
        STUSB4500_LOG("!! No suitable PDO found under voltage limit !!\r\n\r\n");
        return false;
    }

    return true;
}


static void print_all_pdos_for_ui(stusb4500_pdo_t const* src_pdos, uint8_t num_pdos) {
    // Python'un verinin başladığını anlaması için bir header basalım
    STUSB4500_LOG("UI_PDO_LIST_START\r\n");

    for (int i = 0; i < num_pdos; i++) {
        stusb4500_pdo_t const pdo = src_pdos[i];

        stusb4500_current_t pdo_current = FROM_PDO_CURRENT(pdo);
        stusb4500_voltage_t pdo_voltage = FROM_PDO_VOLTAGE(pdo);
        stusb4500_power_t pdo_power = (stusb4500_power_t)pdo_current * (stusb4500_power_t)pdo_voltage / 1000UL;

        // Format: PDO_INDEX,VOLTAGE_MV,CURRENT_MA,POWER_MW
        // Örn: 0,5000,3000,15000
        STUSB4500_LOG("%d,%u,%u,%u\r\n",
            i,
            (unsigned int)pdo_voltage,
            (unsigned int)pdo_current,
            (unsigned int)pdo_power);
    }

    STUSB4500_LOG("UI_PDO_LIST_END\r\n");
}

bool stusb4500_negotiate(stusb4500_t const* dev, stusb4500_config_t const* config, bool on_interrupt , bool OnlyPrint) {
    uint8_t buffer[MAX_SRC_PDOS * sizeof(stusb4500_pdo_t)];
    uint16_t header;
    uint32_t start = 0;

    if (!config) return false;

    // Sanity check to see if STUSB4500 is there
    if (!is_present(dev)) return false;

    // Check that cable is attached
    if (
      !dev->read(dev->addr, STUSB_PORT_STATUS, buffer, 1, dev->context) ||
      !(buffer[0] & STUSB_ATTACH))
        return false;

    // Force transmission of source capabilities if not responding to an STUSB_ATTACH interrupt
    if (!on_interrupt) {
        if (!wait_until_ready_with_timeout(dev, config)) return false;
        if (!send_pd_message(dev, PD_GET_SRC_CAP)) return false; // WARNING: Device behavior is vendor-specific; some trigger a reset on PD_GET_SRC_CAP, while others reset on PD_SOFT_RESET.

    }

    if (config->get_ms) {
        start = config->get_ms();
    }

    while (1) {
        // Check for timeout
        if (config->get_ms && (config->get_ms() - start > TIMEOUT_MS)) return false;

        // Read the port status to look for a source capabilities message
        if (!dev->read(dev->addr, STUSB_PRT_STATUS, buffer, 1, dev->context)) return false;

        // Message has not arrived yet
        if (!(buffer[0] & STUSB_PRT_MESSAGE_RECEIVED)) continue;

        // Read message header
        if (!dev->read(dev->addr, STUSB_RX_HEADER, &header, sizeof(header), dev->context))
            return false;

        // Not a data/source capabilities message, continue waiting
        if (
          !HEADER_NUM_DATA_OBJECTS(header) ||
          HEADER_MESSAGE_TYPE(header) != STUSB_SRC_CAPABILITIES_MSG)
            continue;

        // Read number of received bytes
        if (!dev->read(dev->addr, STUSB_RX_BYTE_CNT, buffer, 1, dev->context))
        	return false;

        // Check for missing data
        if (buffer[0] != HEADER_NUM_DATA_OBJECTS(header) * sizeof(stusb4500_pdo_t))
        	return false;

        break;
    }

    // Read source capabilities
    // WARNING: This must happen very soon after the previous code block is executed. The source
    // will send an accept message which partially overwrites the source capabilities message.
    // Use i2c clock >= 300 kHz

    if (!dev->read(
          dev->addr,
          STUSB_RX_DATA_OBJ,
          buffer,
          HEADER_NUM_DATA_OBJECTS(header) * sizeof(stusb4500_pdo_t),
          dev->context))
        return false;

    // Wait for idle state before loading new PDO
    if (!wait_until_ready_with_timeout(dev, config)) return false;

    if (OnlyPrint)
    {
    	    print_all_pdos_for_ui((stusb4500_pdo_t*)buffer, HEADER_NUM_DATA_OBJECTS(header));
    	    return true;
    }
    else
    {
    	// Find and load the optimal PDO, if any
		if (!load_max_pdo(dev, config, (stusb4500_pdo_t*)buffer, HEADER_NUM_DATA_OBJECTS(header)))
			return false;
		else  return send_pd_message(dev, PD_GET_SRC_CAP); // WARNING: Device behavior is vendor-specific; some trigger a reset on PD_GET_SRC_CAP, while others reset on PD_SOFT_RESET.
    }


}

bool stusb4500_set_gpio_state(stusb4500_t const* dev, stusb4500_gpio_state_t state) {
    // Sanity check to see if STUSB4500 is there
    if (!is_present(dev)) return false;
    // Set GPIO state
    return dev->write(dev->addr, STUSB_GPIO3_SW_GPIO, &state, sizeof(state), dev->context);
}


void NegotiatetoMaxPDO()
{
   stusb4500_negotiate(&dev, &config, false,false);
}



void GetPDOList()
{
	stusb4500_negotiate(&dev, &config, false,true);
}


void dumpRegisters() {
    uint8_t reg[1];
    uint8_t rxBuffer[1];
    char buffer[64];

    USB_SendMessage("STUSB4500 Register Dump:\n");
    for (uint8_t i = 0; i <= REGISTER_COUNT; i++) {
        reg[0] = i;
        I2C_receive(STUSB4500_I2C_ADDR, reg, rxBuffer, 1, 1);

        // Diziye kaydetmeden direkt ekrana basıyoruz
        snprintf(buffer, sizeof(buffer), "Register 0x%02X: 0x%02X\r\n", i, rxBuffer[0]);
        USB_SendMessage(buffer);
    }
}


void read_active_pdo() {
    uint8_t rdo_bytes[4];
    uint32_t rdo;

    for (int i = 0; i < 4; i++) {
        rdo_bytes[i] = read_register(0x91 + i); // DPM_ACTIVE_RDO
    }

    rdo = (rdo_bytes[3] << 24) | (rdo_bytes[2] << 16) | (rdo_bytes[1] << 8) | rdo_bytes[0];

    uint8_t object_position = (rdo >> 28) & 0x07; // Aktif PDO index'i (1-based)
    printf("Active PDO index: %d\n", object_position);
}


bool my_stusb4500_write(uint16_t addr, uint8_t reg, void const* buf, size_t len, void* context) {
    uint8_t data[len + 1];
    data[0] = reg;
    memcpy(&data[1], buf, len);
    return (_Bool)I2C_transmit((uint8_t)addr, data, len + 1);
}


bool my_stusb4500_read(uint16_t addr, uint8_t reg, void* buf, size_t len, void* context) {
    uint8_t r_addr[1] = { reg };
    return (_Bool)I2C_receive((uint8_t)addr, r_addr, (uint8_t*)buf, 1, (uint8_t)len);
}


// Zaman damgası fonksiyonu (Timeout'lar için kritik)
uint32_t my_get_ms(void) {
    return HAL_GetTick(); // Örn: STM32 kullanıyorsanız
}



