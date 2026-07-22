#ifndef REMOTE_H
#define REMOTE_H

#include "main.h"



/* Function Prototypes */
void Remote_Parse_Byte(uint8_t byte);
void Remote_Process_Command(char* cmd);
void Remote_Send_Response(const char* msg);
uint32_t micros(void);
void Process_ARB_Binary_Payload(uint8_t *payload, uint16_t total_bytes);
#endif /* REMOTE_H */
