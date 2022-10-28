/******************************************************************************

 Copyright (c) 2020 VIA Technologies, Inc. All Rights Reserved.

 This PROPRIETARY SOFTWARE is the property of VIA Technologies, Inc.
 and may contain trade secrets and/or other confidential information of
 VIA Technologies, Inc. This file shall not be disclosed to any third
 party, in whole or in part, without prior written consent of VIA.

 THIS PROPRIETARY SOFTWARE AND ANY RELATED DOCUMENTATION ARE PROVIDED AS IS,
 WITH ALL FAULTS, AND WITHOUT WARRANTY OF ANY KIND EITHER EXPRESS OR IMPLIED,
 AND VIA TECHNOLOGIES, INC. DISCLAIMS ALL EXPRESS OR IMPLIED
 WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET
 ENJOYMENT OR NON-INFRINGEMENT.

******************************************************************************/
#ifndef __I2C_API_H__
#define __I2C_API_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

int get_bus(int num);

int i2c_write_byte(int num,uint8_t slave_addr, uint8_t *buf);
int i2c_read_byte(int num, uint8_t slave_addr, uint8_t *buf);

int i2c_writes(int num,uint8_t slave_addr, uint8_t reg_addr, uint8_t *buf, uint32_t length);
int i2c_reads(int num, uint8_t slave_addr, uint8_t reg_addr, uint8_t *buf, uint32_t length);

#ifdef __cplusplus
}
#endif

#endif
