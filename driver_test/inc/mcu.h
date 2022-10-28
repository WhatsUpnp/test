#ifndef _MCU_H_
#define _MCU_H_

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "i2c_api.h"
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "gpio.h"

#define REG_MAX_NUM     104

#define STA_PIR_MASK			0x01
#define STA_BATLOW_MASK			0x02
#define STA_FKEY_MASK		    0x04//Function Key
#define STA_CHGFULL_MASK		0x08
#define STA_TAMPER_MASK			0x10
#define STA_DCIN_MASK			0x20
#define STA_DKEY_MASK			0x40//Ding Dong Key

#define EVT_KEY_MASK		0x01
#define EVT_PWRDN_MASK		0x02
#define EVT_BATLOW_MASK		0x04
#define EVT_DCPLUG_MASK		0x08
#define EVT_RING_MASK		0x10
#define EVT_QRCODE_MASK		0x20
#define EVT_BATLWARN_MASK	0x40
#define EVT_STC_MASK        0x80

#define CNTL_PWR_MASK			0x03
#define CNTL_WDTE_MASK			0x04
#define CNTL_PIRE_MASK			0x08
#define CNTL_WIFIE_MASK			0x10
#define CNTL_CHGE_MASK			0x20
#define CNTL_LEDE_MASK			0xc0

//register map
#define REG_Status      0x00
#define REG_Command     0x01
#define REG_ADCL        0x02
#define REG_ADCH        0x03
#define REG_Time0       0x04
#define REG_Time1       0x05
#define REG_Time2       0x06
#define REG_Time3       0x07
#define REG_Version     0x08
#define REG_Cntl        0x09
#define REG_POR         0x0A
#define REG_EVT         0x0B
#define REG_CFG         0x0C
#define REG_TWK         0x0D
#define REG_TZone       0x0E
#define REG_Cust0       0x10
#define REG_Cust1       0x14
#define REG_Cust2       0x18
#define REG_Cust3       0x1C
#define REG_Cust4       0x20
#define REG_Cust5       0x24
#define REG_Cust6       0x28
#define REG_Cust7       0x2C
#define REG_PSCH        0x30
#define REG_PWID        0x45
#define REG_PTHRL       0x46
#define REG_PTHRH       0x47
#define REG_PIRC        0x48
#define REG_DAY         0x4A
#define REG_HOUR        0x4B
#define REG_BLVTS       0x4C
#define REG_PID         0x4E
#define REG_Cntl2       0x4F
#define REG_RFSN        0x50
#define REG_RFCmd       0x53
#define REG_BDelta      0x54
#define REG_BLVTW       0x56
#define REG_BHVTP       0x58
#define REG_POR2        0x5a
#define REG_WDT         0x5b
#define REG_EVT2        0x5c
#define REG_BATL        0x5d
#define REG_PCT         0x5e
#define REG_DIV         0x5f
#define REG_BLVTP       0x60
#define REG_ACK         0x62
#define REG_IPACK       0x7f

static int read_mcu_i2c(unsigned char reg, unsigned char* val);
static int write_mcu_i2c(unsigned char reg, unsigned char val);

int mcu_set_command(uint8_t cmd);
int read_mcu_event(unsigned int* val);

int print_mcu_regs(void);
void key_detect(void);
int pir(unsigned char sta);
void wdt(int time);
void feed_wdt(void);


#endif
