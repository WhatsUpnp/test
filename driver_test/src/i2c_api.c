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

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include "i2c_api.h"

#define MAX_BUF_LENGTH	128
#define DEV_NAME		"/dev/i2c-"

static int g_fd[3] = {-1,-1,-1};
static pthread_mutex_t mutex_lock = PTHREAD_MUTEX_INITIALIZER;

int get_bus(int num)
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

int i2c_writes(int num,uint8_t slave_addr, uint8_t reg_addr, uint8_t *buf, uint32_t length)
{
	uint8_t outbuf[MAX_BUF_LENGTH+1];
	struct i2c_rdwr_ioctl_data packets;
	struct i2c_msg messages[1];
	int fd = get_bus(num);

	pthread_mutex_lock(&mutex_lock);
	if(length > MAX_BUF_LENGTH){
		perror("I2c write buf length illegal.\n");
		pthread_mutex_unlock(&mutex_lock);
		return -1;
	}

	messages[0].addr = slave_addr;
	messages[0].flags = 0;
	messages[0].len = length+1;
	messages[0].buf = outbuf;
	/* The first byte indicates which register we will write */
	outbuf[0] = reg_addr;
	/*
	* The second byte indicates the value to write. Note that for many
	* devices, we can write multiple, sequential registers at once by
	* simply making outbuf bigger.
	*/
	//outbuf[1] = value;
	memcpy(&outbuf[1], buf, length);
	/* Transfer the i2c packets to the kernel and verify it worked */
	packets.msgs = messages;
	packets.nmsgs = 1;
	if(ioctl(fd, I2C_RDWR, &packets) < 0){
		perror("Unable to send i2c write buf data.\n");
		pthread_mutex_unlock(&mutex_lock);
		return -1;
	}

	pthread_mutex_unlock(&mutex_lock);
	return 0;
}

int i2c_reads(int num, uint8_t slave_addr, uint8_t reg_addr, uint8_t *buf, uint32_t length)
{
	uint8_t reg;
	struct i2c_rdwr_ioctl_data packets;
	struct i2c_msg messages[2];
	int fd = get_bus(num);
	/*
	* In order to read a register, we first do a "dummy write" by writing
	* 0 bytes to the register we want to read from. This is similar to
	* the packet in set_i2c_register, except it's 1 byte rather than 2.
	*/
	pthread_mutex_lock(&mutex_lock);
	reg = reg_addr;
	messages[0].addr = slave_addr;
	messages[0].flags = 0;
	messages[0].len = sizeof(reg);
	messages[0].buf = &reg;
	/* The data will get returned in this structure */
	messages[1].addr = slave_addr;
	messages[1].flags = I2C_M_RD/* | I2C_M_NOSTART*/;
	messages[1].len = length;
	messages[1].buf = buf;
	/* Send the request to the kernel and get the result back */
	packets.msgs = messages;
	packets.nmsgs = 2;
	if(ioctl(fd, I2C_RDWR, &packets) < 0){
		perror("Unable to send i2c read buf data.\n");
		pthread_mutex_unlock(&mutex_lock);
		return -1;
	}

	pthread_mutex_unlock(&mutex_lock);
	return 0;
}

int i2c_write_byte(int num,uint8_t slave_addr, uint8_t *buf)
{
	struct i2c_rdwr_ioctl_data packets;
	struct i2c_msg messages[1];
	int fd = get_bus(num);

	pthread_mutex_lock(&mutex_lock);
	if(!buf){
		perror("I2c write byte buf illegal.\n");
		pthread_mutex_unlock(&mutex_lock);
		return -1;
	}

	messages[0].addr = slave_addr;
	messages[0].flags = 0;
	messages[0].len = 1;
	messages[0].buf = buf;

	/* Transfer the i2c packets to the kernel and verify it worked */
	packets.msgs = messages;
	packets.nmsgs = 1;
	if(ioctl(fd, I2C_RDWR, &packets) < 0){
		perror("Unable to send i2c write byte data.\n");
		pthread_mutex_unlock(&mutex_lock);
		return -1;
	}

	pthread_mutex_unlock(&mutex_lock);
	return 0;
}

int i2c_read_byte(int num, uint8_t slave_addr, uint8_t *buf)
{
	struct i2c_rdwr_ioctl_data packets;
	struct i2c_msg messages[1];
	int fd = get_bus(num);

	pthread_mutex_lock(&mutex_lock);
	if(!buf){
		perror("I2c write byte buf illegal.\n");
		pthread_mutex_unlock(&mutex_lock);
		return -1;
	}

	/* The data will get returned in this structure */
	messages[0].addr = slave_addr;
	messages[0].flags = I2C_M_RD/* | I2C_M_NOSTART*/;
	messages[0].len = 1;
	messages[0].buf = buf;
	/* Send the request to the kernel and get the result back */
	packets.msgs = messages;
	packets.nmsgs = 1;
	if(ioctl(fd, I2C_RDWR, &packets) < 0){
		perror("Unable to send i2c read byte data.\n");
		pthread_mutex_unlock(&mutex_lock);
		return -1;
	}

	pthread_mutex_unlock(&mutex_lock);
	return 0;
}

