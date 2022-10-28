#ifndef __AW9523_H_
#define __AW9523_H_

typedef unsigned char u8;
typedef unsigned short u16;

#define LED_BOARD_REG_ID		0x10
#define LED_BOARD_REG_CTL		0x11
#define LED_BOARD_REG_MODE0		0x12
#define LED_BOARD_REG_MODE1		0x13

#define LED_BOARD_REG_DIM_10	0x20
#define LED_BOARD_REG_DIM_11	0x21
#define LED_BOARD_REG_DIM_12	0x22
#define LED_BOARD_REG_DIM_13	0x23
#define LED_BOARD_REG_DIM_00	0x24
#define LED_BOARD_REG_DIM_01	0x25
#define LED_BOARD_REG_DIM_02	0x26
#define LED_BOARD_REG_DIM_03	0x27
#define LED_BOARD_REG_DIM_04	0x28
#define LED_BOARD_REG_DIM_05	0x29
#define LED_BOARD_REG_DIM_06	0x2a
#define LED_BOARD_REG_DIM_07	0x2b
#define LED_BOARD_REG_DIM_14	0x2c
#define LED_BOARD_REG_DIM_15	0x2d
#define LED_BOARD_REG_DIM_16	0x2e
#define LED_BOARD_REG_DIM_17	0x2f

#define LED_BOARD_REG_RST		0x7f

#define AW9523_LED_LIGHT_OFF 	0
#define AW9523_LED_LIGHT_MAX 	8

typedef enum {
	BUTTON_BOARD_LED00 = LED_BOARD_REG_DIM_00,
	BUTTON_BOARD_LED01 = LED_BOARD_REG_DIM_01,
	BUTTON_BOARD_LED02 = LED_BOARD_REG_DIM_02,
	BUTTON_BOARD_LED03 = LED_BOARD_REG_DIM_03,
	BUTTON_BOARD_LED04 = LED_BOARD_REG_DIM_04,
	BUTTON_BOARD_LED05 = LED_BOARD_REG_DIM_05,
	BUTTON_BOARD_LED06 = LED_BOARD_REG_DIM_06,
	BUTTON_BOARD_LED07 = LED_BOARD_REG_DIM_07,
	BUTTON_BOARD_LED10 = LED_BOARD_REG_DIM_10,
	BUTTON_BOARD_LED11 = LED_BOARD_REG_DIM_11,
	BUTTON_BOARD_LED12 = LED_BOARD_REG_DIM_12,
	BUTTON_BOARD_LED13 = LED_BOARD_REG_DIM_13,
	BUTTON_BOARD_LED14 = LED_BOARD_REG_DIM_14,
	BUTTON_BOARD_LED15 = LED_BOARD_REG_DIM_15,
	BUTTON_BOARD_LED16 = LED_BOARD_REG_DIM_16,
	BUTTON_BOARD_LED17 = LED_BOARD_REG_DIM_17,

}BUTTON_BOARD_LED;

int ledBoard_reg_write(u8 reg,u8 val);
int ledBoard_ledLight(BUTTON_BOARD_LED led,u8 light);
int ledBoard_ledallLight(u8 light);
int ledBoard_whiteLight(u8 light);
int ledBoard_blueLight(u8 light);
int ledBoard_4whiteLight(u8 light);
int ledBoard_4blueLight(u8 light);
int ledBoard_TopblueLight(u8 light);
int ledBoard_TopwhiteLight(u8 light);
int ledBoard_BottomblueLight(u8 light);
int ledBoard_BottomwhiteLight(u8 light);
int ledBoard_init(void);
int led(void);

#endif

