/* Standard includes. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <time.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include "aw9523.h"

#define DEBUG_LOG 
#define I2C_BUS		1
#define LED_BOARD_ADDR  (0xB6>>1)

#define LED_BOARD_LIGHT_MAX 0
#define LED_BOARD_LIGHT_MAX3_4 1
#define LED_BOARD_LIGHT_MAX2_4 2
#define LED_BOARD_LIGHT_MAX1_4 3

static int button_device = 0;

#define DEV_NAME		"/dev/i2c-"
static int g_fd[3] = {-1,-1,-1};
static pthread_mutex_t mutex_lock = PTHREAD_MUTEX_INITIALIZER;

static int get_bus(int num)
{
	int fd,len;
	char buf[32];

	pthread_mutex_lock(&mutex_lock);
	if(num > 2){
		perror("I2c bus number illegal");
		pthread_mutex_unlock(&mutex_lock);
		return -1;
	}

	if(g_fd[num] < 0){
		len = snprintf(buf, sizeof(buf), DEV_NAME"%d", num);
		if(len < 0){
			perror("gpio err");
			pthread_mutex_unlock(&mutex_lock);
			return -1;
		}
		fd = open(buf, O_RDWR);
		if(fd < 0){
			perror("Open dev failed.\n");
			pthread_mutex_unlock(&mutex_lock);
			return -1;
		}
		
		g_fd[num] = fd;
		pthread_mutex_unlock(&mutex_lock);
		return fd;
	}
	
	pthread_mutex_unlock(&mutex_lock);
	return g_fd[num];
}

static int gm_i2c_transfer(struct i2c_msg *msgs, int len)
{
	uint8_t reg;
	struct i2c_rdwr_ioctl_data packets;
	int fd = get_bus(I2C_BUS);

	packets.msgs = msgs;
	packets.nmsgs = len;
	if(ioctl(fd, I2C_RDWR, &packets) < 0){
		perror("Unable to send i2c write buf data.\n");
		return -1;
	}
}

int gm_i2c_write(unsigned char slave_addr, unsigned char reg_addr, unsigned char val)
{
	struct i2c_msg msgs[1];
    unsigned char tmp[2];
    unsigned char retries = 4;

    tmp[0] = reg_addr;
    tmp[1] = val;

    msgs[0].addr  = slave_addr;
    msgs[0].flags = 0;
    msgs[0].len   = 2;
    msgs[0].buf   = tmp;

    while (--retries && gm_i2c_transfer(msgs, 1) != 1) {
        usleep(10 * 1000);
    }
    return 0;
}

unsigned char gm_i2c_read(unsigned char slave_addr, unsigned char reg_addr)
{
	struct i2c_msg msgs[2];
    unsigned char tmp[1], tmp2[1];
    unsigned char retries = 4;

    tmp[0]        = reg_addr;
    msgs[0].addr  = slave_addr;
    msgs[0].flags = 0;
    msgs[0].len   = 1;
    msgs[0].buf   = tmp;

    tmp2[0]       = 0;
    msgs[1].addr  = slave_addr;
    msgs[1].flags = 1;
    msgs[1].len   = 1;
    msgs[1].buf   = tmp2;

    while (--retries && gm_i2c_transfer(msgs, 2) != 2) {
        usleep(10 * 1000);
    }
    return tmp2[0];
}

int ledBoard_reg_write(u8 reg,u8 val)
{
#if 1
	if(!gm_i2c_write(LED_BOARD_ADDR,reg,val))
		return 0;
	else{
		DEBUG_LOG("reg write err\n");
		return -1;
	}
#else
	return -1;
#endif
}

static u8 ledBoard_reg_read(u8 reg)
{
#if 1
	return gm_i2c_read(LED_BOARD_ADDR,reg);
#else
	return 0;
#endif
}

int ledBoard_ledLight(BUTTON_BOARD_LED led,u8 light)
{
	if(!button_device)
		return -1;
	
	DEBUG_LOG("ledBoard_ledLight: led 0x%x , light %d\n",led,light);
	
	return ledBoard_reg_write(led&0xff,light);
}

int ledBoard_ledallLight(u8 light)
{
	int i;
	
	if(!button_device)
		return -1;
	
	for(i=0;i<16;i++){
		ledBoard_reg_write(LED_BOARD_REG_DIM_10+i,light);
	}
	
	return 0;
}

int ledBoard_blueLight(u8 light)
{
	int i;
	
	if(!button_device)
		return -1;
	
	for(i=0;i<8;i++){
		ledBoard_reg_write(LED_BOARD_REG_DIM_10+i*2,light);
	}
	
	return 0;
}

int ledBoard_whiteLight(u8 light)
{
	int i;
	
	if(!button_device)
		return -1;
	
	for(i=0;i<8;i++){
		ledBoard_reg_write(LED_BOARD_REG_DIM_11+i*2,light);
	}
	
	return 0;
}


int ledBoard_4whiteLight(u8 light)
{
	int i;
	if(!button_device)
		return -1;
	
	for(i=0;i<8;i++){
		if((i&0x01) == 0)
			ledBoard_reg_write(LED_BOARD_REG_DIM_11+i*2,light);
	}
	
	return 0;
}

int ledBoard_4blueLight(u8 light)
{
	int i;
	if(!button_device)
		return -1;	
	
	for(i=0;i<8;i++){
		if((i&0x01) == 0)
			ledBoard_reg_write(LED_BOARD_REG_DIM_10+i*2,light);
	}
	
	return 0;
}

int ledBoard_TopblueLight(u8 light)
{
	int i;
	if(!button_device)
		return -1;
	
	ledBoard_reg_write(LED_BOARD_REG_DIM_10,light);
	ledBoard_reg_write(LED_BOARD_REG_DIM_12,light);
	ledBoard_reg_write(LED_BOARD_REG_DIM_16,light);
	
	return 0;
}
int ledBoard_TopwhiteLight(u8 light)
{
	int i;
	if(!button_device)
		return -1;
	

	ledBoard_reg_write(LED_BOARD_REG_DIM_11,light);
	ledBoard_reg_write(LED_BOARD_REG_DIM_13,light);
	ledBoard_reg_write(LED_BOARD_REG_DIM_17,light);	
	return 0;
}
int ledBoard_BottomblueLight(u8 light)
{
	int i;
	if(!button_device)
		return -1;
	
	ledBoard_reg_write(LED_BOARD_REG_DIM_02,light);
	ledBoard_reg_write(LED_BOARD_REG_DIM_06,light);
	ledBoard_reg_write(LED_BOARD_REG_DIM_04,light);
	
	return 0;
}
int ledBoard_BottomwhiteLight(u8 light)
{
	int i;
	if(!button_device)
		return -1;
	

	ledBoard_reg_write(LED_BOARD_REG_DIM_03,light);
	ledBoard_reg_write(LED_BOARD_REG_DIM_05,light);
	ledBoard_reg_write(LED_BOARD_REG_DIM_07,light);	
	return 0;
}


int ledBoard_init(void)
{
	u8 val = 0;

	DEBUG_LOG("ledBoard_init ... \n");
	val = ledBoard_reg_read(LED_BOARD_REG_ID);
	DEBUG_LOG("device id is 0x%x \n",val);

	if(val != 0x23){
		printf("\n****************************\n");
		printf("found no led board aw9523 device\n");
		printf("\n****************************\n");
		return -1;
	}
	ledBoard_reg_write(LED_BOARD_REG_RST,0); //reset device

	ledBoard_reg_write(LED_BOARD_REG_MODE0,0); //set P0x to led mode
	ledBoard_reg_write(LED_BOARD_REG_MODE1,0); //set P1x to led mode
	ledBoard_reg_write(LED_BOARD_REG_CTL,0x03&LED_BOARD_LIGHT_MAX1_4); //Imax/4	
	
	button_device = 1;
	return 0;
	
}


int led(void)
{
	ledBoard_init();
	ledBoard_ledallLight(0);

	ledBoard_ledLight(BUTTON_BOARD_LED00, 255);
	usleep(500 * 1000);
	ledBoard_ledallLight(0);

	ledBoard_ledallLight(255);
	usleep(500 * 1000);
	ledBoard_ledallLight(0);

	ledBoard_whiteLight(255);
	usleep(500 * 1000);
	ledBoard_ledallLight(0);

	ledBoard_blueLight(255);
	usleep(500 * 1000);
	ledBoard_ledallLight(0);

	ledBoard_4whiteLight(255);
	usleep(500 * 1000);
	ledBoard_ledallLight(0);

	ledBoard_4blueLight(255);
	usleep(500 * 1000);
	ledBoard_ledallLight(0);

	ledBoard_TopblueLight(255);
	usleep(500 * 1000);
	ledBoard_ledallLight(0);

	ledBoard_TopwhiteLight(255);
	usleep(500 * 1000);
	ledBoard_ledallLight(0);

	ledBoard_BottomblueLight(255);
	usleep(500 * 1000);
	ledBoard_ledallLight(0);

	ledBoard_BottomwhiteLight(255);
	usleep(500 * 1000);
	ledBoard_ledallLight(0);

	return 0;
}
