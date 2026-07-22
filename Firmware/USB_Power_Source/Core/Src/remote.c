#include "remote.h"
#include "usbd_cdc_if.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "stm32_sw_I2C.h"
#include <soft_i2c_2.h>
#include <soft_i2c_3.h>
#include <stusb4500.h>

/* Use extern to access variables already updated in main.c loop */
extern float VoltVLow, VoltVHigh;
extern float Currlimit1V8;
extern float Currlimit2V5;
extern float Currlimit3V3;
extern float CurrlimitVLow;
extern float CurrlimitVHigh;

extern ADC_ChannelData channels[];
extern float PCBTemperature;

#define RX_BUFFER_SIZE 128
static char rx_buffer[RX_BUFFER_SIZE];
static uint8_t rx_index = 0;

extern bool channelEnabled[5];
extern void UpdateChannelState(uint8_t row, bool enabled);

#define ARB_MAX_POINTS 1024

extern uint16_t arb_dac_buffer[ARB_MAX_POINTS];
extern uint16_t arb_dwell_buffer[ARB_MAX_POINTS];
extern bool CH4_ARB_Enabled;
extern bool CH5_ARB_Enabled;
extern uint16_t g_repetition;
extern uint16_t g_multiplier;
extern uint16_t g_sample_count;
extern uint32_t ARB_CH4_LastMicros;
extern uint32_t ARB_CH5_LastMicros;
extern uint16_t ARB_CH4_Index;
extern uint16_t ARB_CH5_Index;

void Remote_Parse_Byte(uint8_t byte) {
    if (byte == '\n' || byte == '\r') {
        if (rx_index > 0) {
            rx_buffer[rx_index] = '\0';
            Remote_Process_Command(rx_buffer);
            rx_index = 0;
        }
    } else if (rx_index < RX_BUFFER_SIZE - 1) {
        rx_buffer[rx_index++] = byte;
    }
}

void Remote_Send_Response(const char* msg) {
    if (msg != NULL) {
        CDC_Transmit_FS((uint8_t*)msg, strlen(msg));
    }
}

void Process_ARB_Binary_Payload(uint8_t *payload, uint16_t total_bytes) {
    // Toplam noktayı gelen byte sayısından hesapla: (total_bytes - 6 parametre) / 4 byte per point
    uint16_t pts = (total_bytes - 6) / 4;
    uint32_t offset = 0;

    for (int i = 0; i < pts; i++) {
        arb_dac_buffer[i] = (uint16_t)(payload[offset] | (payload[offset+1] << 8));
        offset += 2;
        arb_dwell_buffer[i] = (uint16_t)(payload[offset] | (payload[offset+1] << 8));
        offset += 2;
    }

    // Parametreler tam burada (Verinin bittiği yer)
    g_repetition = (uint16_t)(payload[offset] | (payload[offset+1] << 8));
    g_multiplier = (uint16_t)(payload[offset+2] | (payload[offset+3] << 8));
    g_sample_count = (uint16_t)(payload[offset+4] | (payload[offset+5] << 8));
}

char response[256];
void Remote_Process_Command(char* cmd) {

    for(int i = 0; cmd[i]; i++) cmd[i] = toupper(cmd[i]);

    /* 1. Identification */
    if (strcmp(cmd, "*IDN?") == 0) {
        Remote_Send_Response("BenchVolt-PD,V1.0,S/N:2026-01\r\n");
    }
    else if (strcmp(cmd, "SYST:BUILD?") == 0) {
            char build_resp[64];
            snprintf(build_resp, sizeof(build_resp), "%s %s\r\n", __DATE__, __TIME__);
            Remote_Send_Response(build_resp);
        }
    /* 2. Output Control - GPIO Mapping based on your main.h */
    else if (strncmp(cmd, "OUTP:CH", 7) == 0) {
            int ch = cmd[7] - '0'; /* Channel index from PC (1-5) */
            char* stat_ptr = strstr(cmd, "STAT ");

            if (stat_ptr) {
                int state = atoi(stat_ptr + 4);
                GPIO_PinState pinState = (state > 0) ? GPIO_PIN_SET : GPIO_PIN_RESET;

                if (ch >= 1 && ch <= 5) {


                    /* Control physical GPIO Pins */
                    if(ch == 1)      HAL_GPIO_WritePin(EN3_GPIO_Port, EN3_Pin, pinState);// 3.3V
                    else if(ch == 2) HAL_GPIO_WritePin(EN2_GPIO_Port, EN2_Pin, pinState);// 2.5V
                    else if(ch == 3) HAL_GPIO_WritePin(EN1_GPIO_Port, EN1_Pin, pinState);// 1.8V
                    else if(ch == 4) HAL_GPIO_WritePin(EN4_GPIO_Port, EN4_Pin, pinState);//VAdjLow
                    else if(ch == 5) EnVAdjHighOutput(pinState);//VAdjHigh

                    if(ch == 4 && pinState == false)  CH4_ARB_Enabled = false; // When channel is closed stop waveform
                    if(ch == 5 && pinState == false)  CH5_ARB_Enabled = false; // When channel is closed stop waveform


                    /* Trigger immediate LCD update for the status circle */
                    UpdateChannelState(ch-1, (state > 0));

                    Remote_Send_Response("OK\r\n");
                } else {
                    Remote_Send_Response("ERR:INVALID_CH\r\n");
                }
            }
        }

    /* --- DC-DC OCP Current Setting (SOUR:CURR:DC <index> <value>) --- */
    else if (strncmp(cmd, "SOUR:CURR:DC", 12) == 0) {
        int dc_idx = cmd[12] - '0';
        char* val_ptr = strrchr(cmd, ' ');
        if (val_ptr && dc_idx >= 1 && dc_idx <= 3) {
            float limit = atof(val_ptr + 1);
            if (dc_idx == 1)      SetDC1CurrentLimit(limit);
            else if (dc_idx == 2) SetDC2CurrentLimit(limit);
            else if (dc_idx == 3) SetVAdjHighCurrentLimit(limit);
            Remote_Send_Response("OK\r\n");


        }
    }
    /* --- Channel Current Limit Setting (SOUR:CURR:CH <index> <value>) --- */
    else if (strncmp(cmd, "SOUR:CURR:CH", 12) == 0) {
        int ch = cmd[12] - '0';
        char* val_ptr = strrchr(cmd, ' ');

        if (val_ptr && ch >= 1 && ch <= 5) {
            float limit = atof(val_ptr + 1);

            if (limit < 0.0f) limit = 0.0f;
            if (limit > 3.0f) limit = 3.0f;

            if (ch == 1)      Currlimit1V8 = limit;
            else if (ch == 2) Currlimit2V5 = limit;
            else if (ch == 3) Currlimit3V3 = limit;
            else if (ch == 4) CurrlimitVLow = limit;
            else if (ch == 5) CurrlimitVHigh = limit;

            Remote_Send_Response("OK\r\n");
        } else {
            Remote_Send_Response("ERR:INVALID_CURR_CH\r\n");
        }
    }

        else if (strncmp(cmd, "OUTP:DC", 7) == 0) {
            int dc_idx = cmd[7] - '0'; // DC indeksi (1 veya 2)
            char* stat_ptr = strstr(cmd, " "); // Boşluktan sonrasını ara

            if (stat_ptr && (dc_idx >= 1 && dc_idx <= 3)) {
                int state = atoi(stat_ptr + 1);
                GPIO_PinState pinState = (state > 0) ? GPIO_PIN_SET : GPIO_PIN_RESET;

                if (dc_idx == 1)
                {
                	EnDC1Output(pinState);
                	UpdateChannelState(0,pinState);
                	UpdateChannelState(1,pinState);
                }
                else if (dc_idx == 2)
                {
                	EnDC2Output(pinState);
                	UpdateChannelState(2,pinState);
                	UpdateChannelState(3,pinState);
                }
                else
                {
                	EnVAdjHighOutput(pinState);
                	UpdateChannelState(4,pinState);
                }
                Remote_Send_Response("OK\r\n");

            } else {
                Remote_Send_Response("ERR:INVALID_DC_CH\r\n");
            }
        }
    /* 3. Voltage Settings for Adjustable Channels */
        else if (strncmp(cmd, "SOUR:VOLT:CH", 12) == 0) {
            int ch = cmd[12] - '0';  // 12. karakter kanal numarası

            char* val_ptr = strstr(cmd, " ");

            if (val_ptr) {
                float val = atof(val_ptr + 1); // Boşluktan bir sonraki karakterden başla

                if (ch == 4) {
                    VoltVLow = val;
                    SetVadjLVoltage(VoltVLow);
                    Remote_Send_Response("OK\r\n");
                } else if (ch == 5) {
                    VoltVHigh = val;
                    VAdjHigh_SetVoltage(VoltVHigh);
                    Remote_Send_Response("OK\r\n");
                }
            }
        }

    /* 4. Measurement Queries - Mapping PC channels to your main.c indices */
    else if (strncmp(cmd, "MEAS:VOLT:CH", 12) == 0) {
        int ch = cmd[12] - '0';
        float val = 0;

        // Mapping based on your ADC_ChannelData channels[] in main.c
        if(ch == 1) val = channels[15].Value;      // 1.8V (Ch_1_8V)
        else if(ch == 2) val = channels[14].Value; // 2.5V (Ch_2_5V)
        else if(ch == 3) val = channels[7].Value;  // 3.3V (Ch_3_3V)
        else if(ch == 4) val = channels[8].Value;  // Vlow (Ch_Vlow)
        else if(ch == 5) val = channels[9].Value;  // Vhigh (Ch_Vhigh)

        snprintf(response, sizeof(response), "%.2f\r\n", val);
        Remote_Send_Response(response);
    }

    /* --- Bulk Measurement Query (MEAS:ALL?) --- */
    else if (strcmp(cmd, "MEAS:ALL?") == 0) {

            uint8_t out1 = (HAL_GPIO_ReadPin(EN3_GPIO_Port, EN3_Pin) == GPIO_PIN_SET) ? 1 : 0; // CH1: 1.8V
            uint8_t out2 = (HAL_GPIO_ReadPin(EN2_GPIO_Port, EN2_Pin) == GPIO_PIN_SET) ? 1 : 0; // CH2: 2.5V
            uint8_t out3 = (HAL_GPIO_ReadPin(EN1_GPIO_Port, EN1_Pin) == GPIO_PIN_SET) ? 1 : 0; // CH3: 3.3V
            uint8_t out4 = (HAL_GPIO_ReadPin(EN4_GPIO_Port, EN4_Pin) == GPIO_PIN_SET) ? 1 : 0; // CH4: VAdjLow
            uint8_t out5 = (HAL_GPIO_ReadPin(EN5_GPIO_Port, EN5_Pin) == GPIO_PIN_SET) ? 1 : 0; // CH5: VAdjHigh

            uint8_t arb4 = CH4_ARB_Enabled ? 1 : 0;
            uint8_t arb5 = CH5_ARB_Enabled ? 1 : 0;

            snprintf(response, sizeof(response),
                "%.2f,%.3f,%.2f,%.3f,%.2f,%.3f,%.2f,%.3f,%.2f,%.3f,%.2f,%.3f,%.1f,%u,%u,%u,%u,%u,%u,%u,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\r\n",
                channels[15].Value, channels[3].Value,  // CH1
                channels[14].Value, channels[2].Value,  // CH2
                channels[7].Value,  channels[1].Value,  // CH3
                channels[8].Value,  channels[4].Value,  // CH4
                channels[9].Value,  channels[5].Value,  // CH5
                channels[10].Value, channels[0].Value,  // CH6 / Input
                PCBTemperature,                         // System Temp

                out1, out2, out3, out4, out5,
                arb4, arb5,

                Currlimit1V8,
                Currlimit2V5,
                Currlimit3V3,
                CurrlimitVLow,
                CurrlimitVHigh,

                VoltVLow,
                VoltVHigh
            );

            Remote_Send_Response(response);
            return;
        }
    /* --- DC-DC Output Control (OUTP:DC<index> <1/0>) --- */
        else if (strncmp(cmd, "OUTP:DC", 7) == 0) {
            int dc_idx = cmd[7] - '0';
            char* stat_ptr = strstr(cmd, " ");

            if (stat_ptr && (dc_idx == 1 || dc_idx == 3)) {
                int state = atoi(stat_ptr + 1);
                bool enable = (state > 0);

                if (dc_idx == 1)      EnDC1Output(enable);
                else if (dc_idx == 2) EnDC2Output(enable);
                else if (dc_idx == 3) EnVAdjHighOutput(enable);
                Remote_Send_Response("OK\r\n");
            } else {
                Remote_Send_Response("ERR:INVALID_DC_CH\r\n");
            }
        }
    /* --- DC-DC Voltage Setting (SOUR:VOLT:DC<index> <value>) --- */
        else if (strncmp(cmd, "SOUR:VOLT:DC", 12) == 0) {
            int dc_idx = cmd[12] - '0';
            char* val_ptr = strrchr(cmd, ' ');

            if (val_ptr != NULL && dc_idx >= 1 && dc_idx <= 3) {
                float val = atof(val_ptr + 1);

                if (dc_idx == 1)      SetVoltageDC1(val);
                else if (dc_idx == 2)
                {
                	if (val > 5.5f )
                		SetVoltageDC2(5.5);
                	else
                		SetVoltageDC2(val);
                }
                else if (dc_idx == 3) VAdjHigh_SetVoltage(val); // Python'daki 3. kanal (TPS55289)

                Remote_Send_Response("OK\r\n");
            } else {
                Remote_Send_Response("ERR:INVALID_DC_PARAMS\r\n");
            }
        }

    	/* --- STUSB4500 Manuel PDO Ayarı --- */
        else if (strncmp(cmd, "SOUR:PDO:SET", 8) == 0) {
            int idx, mv, ma;
            char* param_ptr = strstr(cmd, " ");

            if (param_ptr != NULL) {
                if (sscanf(param_ptr, "%d %d %d", &idx, &mv, &ma) == 3) {

                    if (Set_PD_VoltageCurrent((uint8_t)idx, (uint32_t)mv, (uint32_t)ma)) {
                        Remote_Send_Response("OK:PD_PROFILED\r\n");
                    } else {
                        Remote_Send_Response("ERR:PD_WRITE_FAILED\r\n");
                    }
                } else {
                    Remote_Send_Response("ERR:PARAM_FORMAT\r\n");
                }
            } else {
                Remote_Send_Response("ERR:NO_PARAMS\r\n");
            }
        }


        /* 6. Manual Soft Reset Trigger */
        else if (strcmp(cmd, "SOFT:RESET") == 0) {
            send_soft_reset();
        }
        else if (strcmp(cmd, "HARD:RESET") == 0) {
        	HardReset();
        }
        else if (strcmp(cmd, "SOUR:PD:LIST?") == 0) {
        	GetPDOList();
            }
        else if (strcmp(cmd, "DUMP:ALL") == 0) {
        	//dumpRegisters();
            }
        else if (strcmp(cmd, "SOUR:PD:CONF:MAX") == 0) {
        	NegotiatetoMaxPDO();

            }
        else if (strncmp(cmd, "GET:DCDC:STAT", 13) == 0) {

            char *param_ptr = strchr(cmd, ' ');
            int ch = 0;

            if (param_ptr != NULL) {
                ch = atoi(param_ptr + 1);
            }

            uint8_t status_val = 0;
            if (ch == 1)      status_val = ReadStatusDC1();
            else if (ch == 2) status_val = ReadStatusDC2();
            else if (ch == 3) status_val = VAdjHigh_GetStatus();
            else {
                Remote_Send_Response("ERR:INVALID_CH\n");
                return;
            }

            char stat_resp[10];
            snprintf(stat_resp, sizeof(stat_resp), "%02X\n", status_val);
            Remote_Send_Response(stat_resp);
        }
    		/* --- ARB Binary Data Transfer --- */


    /* --- 1. ARB VERİLERİNİ PARÇA PARÇA ALMA KOMUTU --- */
            // Örnek Komut: SOUR:WAVE:CH4:ARB:DATA 0,80,1,90,2,100,3
            else if (strncmp(cmd, "SOUR:WAVE:CH", 12) == 0 && strstr(cmd, ":ARB:DATA ")) {
                int ch = cmd[12] - '0';
                char *data_ptr = strstr(cmd, "DATA ") + 5;

                // strtok ile virgülleri ayıklayarak okuyoruz
                char *token = strtok(data_ptr, ",");
                if (token != NULL) {
                    int start_idx = atoi(token); // İlk token başlangıç indeksidir
                    int current_idx = start_idx;

                    while (current_idx < ARB_MAX_POINTS) {
                        char *v_tok = strtok(NULL, ","); // Voltaj
                        char *d_tok = strtok(NULL, ","); // Dwell (Adım)

                        if (v_tok == NULL || d_tok == NULL) break; // Satır sonu geldi, döngüden çık

                        // Verileri RAM'deki asıl array'lere kaydet
                        arb_dac_buffer[current_idx] = (uint16_t)atoi(v_tok);
                        arb_dwell_buffer[current_idx] = (uint16_t)atoi(d_tok);
                        current_idx++;
                    }

                    // Python'a sıradaki paketi gönderebilmesi için ONAY (ACK) ver
                    char resp[32];
                    snprintf(resp, sizeof(resp), "OK:ACK:CH%d\r\n", ch);
                    Remote_Send_Response(resp);
                }
            }

            /* --- 2. YÜKLEME BİTİNCE ARB'Yİ BAŞLATMA KOMUTU --- */
            // Örnek Komut: SOUR:WAVE:CH4:ARB:START 1024,1,0
            else if (strncmp(cmd, "SOUR:WAVE:CH", 12) == 0 && strstr(cmd, ":ARB:START ")) {
                int ch = cmd[12] - '0';
                char *data_ptr = strstr(cmd, "START ") + 6;

                char *pts_tok = strtok(data_ptr, ",");
                char *m_tok   = strtok(NULL, ",");
                char *r_tok   = strtok(NULL, ",");

                if (pts_tok && m_tok && r_tok) {
                    g_sample_count = atoi(pts_tok);
                    g_multiplier   = atoi(m_tok);
                    g_repetition   = atoi(r_tok);

                    // Kanalları Aktif Et
                    if (ch == 4) {
                        CH4_ARB_Enabled = true;
                        ARB_CH4_Index = 0;
                        ARB_CH4_LastMicros = micros();
                    } else if (ch == 5) {
                        CH5_ARB_Enabled = true;
                        ARB_CH5_Index = 0;
                        ARB_CH5_LastMicros = micros();
                    }

                    char resp[64];
                    snprintf(resp, sizeof(resp), "OK:CH%d_ARB_STARTED_PTS:%d\r\n", ch, g_sample_count);
                    Remote_Send_Response(resp);
                }
            }
        else if (strcmp(cmd, "JUMP:BOOTLOADER") == 0) {

                Remote_Send_Response("OK:JUMPING_TO_BOOTLOADER\r\n");
                HAL_Delay(100); // USB mesajının bilgisayara ulaşması için kısa bir bekleme

                // 1. Flash kilidini aç
                HAL_FLASH_Unlock();

                // 2. Bootloader'ın okuduğu mühür sayfasını SİL (CRC'yi 0xFFFFFFFF yap)
                FLASH_EraseInitTypeDef EraseInitStruct;
                uint32_t PageError = 0;
                EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
                EraseInitStruct.PageAddress = 0x0801F800; // PARAM_PAGE_ADDR (Bootloader'daki ile aynı olmalı)
                EraseInitStruct.NbPages = 1;
                HAL_FLASHEx_Erase(&EraseInitStruct, &PageError);

                HAL_FLASH_Lock();

                // 3. İşlemciye donanımsal reset at!
                // Cihaz açılacak, CRC mühürü silindiği için Bootloader Uygulamaya geçemeyecek ve yükleme modunda bekleyecek.
                NVIC_SystemReset();
            }


}


/*
System & Identification
*IDN? – Query device identity (Manufacturer, Model, Serial Number).
SYST:BUILD? – Queries the firmware build date and time.
SOFT:RESET – Trigger a software reset on the STUSB4500 to restart PD negotiation.
HARD:RESET – Perform a full hardware reboot of the device.
DUMP:ALL – Dump all STUSB4500 register values to the terminal for debugging.

Output & Channel Control
OUTP:CH1 STAT 1 – Enable Channel 1 (1.8V). (Set to 0 to disable).
OUTP:CH2 STAT 1 – Enable Channel 2 (2.5V). (Set to 0 to disable).
OUTP:CH3 STAT 1 – Enable Channel 3 (3.3V). (Set to 0 to disable).
OUTP:CH4 STAT 1 – Enable Channel 4 (VAdjLow). (Set to 0 to disable).
OUTP:CH5 STAT 1 – Enable Channel 5 (VAdjHigh). (Set to 0 to disable).
OUTP:DC1 1 – Enable DC-DC Converter 1. (Set to 0 to disable).
OUTP:DC2 1 – Enable DC-DC Converter 2. (Set to 0 to disable).

Voltage Configuration
SOUR:VOLT:CH4 1.25 – Set VAdjLow output to 1.25V.
SOUR:VOLT:CH5 12.0 – Set VAdjHigh output to 12.0V.
SOUR:VOLT:DC1 5.0 – Set DC-DC 1 output to 5.0V.
SOUR:VOLT:DC2 12.0 – Set DC-DC 2 output to 12.0V.

Measurement & Monitoring
MEAS:VOLT:CH1 – Read instantaneous voltage for Channel 1.
MEAS:VOLT:CH2 – Read instantaneous voltage for Channel 2.
MEAS:VOLT:CH3 – Read instantaneous voltage for Channel 3.
MEAS:VOLT:CH4 – Read instantaneous voltage for Channel 4.
MEAS:VOLT:CH5 – Read instantaneous voltage for Channel 5.
MEAS:ALL? – Bulk query: returns Voltage, Current (for all channels), and PCB Temperature.

USB-PD & Negotiation
SOUR:PDO:SET 3 15000 3000 – Configure PDO 3 to 15V / 3A (via high-level driver).
SOUR:PD:LIST? – List all Power Data Objects (PDOs) supported by the connected power adapter.
SOUR:PD:CONF:MAX – Manually trigger a new negotiation to Maximum available PDO.
 */
