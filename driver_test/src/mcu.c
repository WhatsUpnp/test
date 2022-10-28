#include "mcu.h"

#define I2C_BUS		0
#define I2C_ADDR	(0X3C >> 1)

#define WDT_INT_PIN 52
#define GPIO_INT_PIN 53

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

int print_mcu_regs(void)
{
    int i;
    struct i2c_msg msgs[2];
    unsigned char tmp[1], tmp2[128] = {0};

    tmp[0]        = 0;
    msgs[0].addr  = I2C_ADDR;
    msgs[0].flags = 0;
    msgs[0].len   = 1;
    msgs[0].buf   = tmp;

    msgs[1].addr  = I2C_ADDR;
    msgs[1].flags = 1;
    msgs[1].len   = REG_MAX_NUM;
    msgs[1].buf   = tmp2;

    if (gm_i2c_transfer(msgs, 2) != 2) {
        printf("Read MCU i2c failed.\n");
        return -1;
    }
    printf("\n==========\nregister:\n==========\n");
	printf("XX  00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n");
	printf("---------------------------------------------------");
    for (i = 0; i < REG_MAX_NUM; i++){
		if((i%16) == 0)printf("\n%02x: ",i);
		printf("%02x ", tmp2[i]);
    }
    printf("\n");
    return 0;
}

static int read_mcu_i2c(unsigned char reg, unsigned char* val) {
#if 1
    struct i2c_msg msgs[2];
    unsigned char tmp[1], tmp2[1];
    unsigned char retries = 4;

    tmp[0]        = reg;
    msgs[0].addr  = I2C_ADDR;
    msgs[0].flags = 0;
    msgs[0].len   = 1;
    msgs[0].buf   = tmp;

    tmp2[0]       = 0;
    msgs[1].addr  = I2C_ADDR;
    msgs[1].flags = 1;
    msgs[1].len   = 1;
    msgs[1].buf   = tmp2;

    while (--retries && gm_i2c_transfer(msgs, 2) != 2) {
        usleep(10 * 1000);
    }

    *val = tmp2[0];
    return 0;
#else
	WLOGE(myLog, "Read MCU i2c failed.\n");
	return -1;
#endif
}

static int write_mcu_i2c(unsigned char reg, unsigned char val) {
#if 1
    struct i2c_msg msgs[1];
    unsigned char tmp[2];
    unsigned char retries = 4;

    tmp[0] = reg;
    tmp[1] = val;

    msgs[0].addr  = I2C_ADDR;
    msgs[0].flags = 0;
    msgs[0].len   = 2;
    msgs[0].buf   = tmp;

    while (--retries && gm_i2c_transfer(msgs, 1) != 1) {
        usleep(10 * 1000);
    }
    return 0;
#else
	WLOGE(myLog, "Write MCU i2c failed.\n");
	return -1;
#endif
}

int read_mcu_event(unsigned int* val) {
    unsigned char data = 0;
    if (read_mcu_i2c(REG_EVT, &data) < 0) {
        printf("Read mcu event failed.\n");	
        return -1;
    }
    *val = data;

    if (read_mcu_i2c(REG_EVT2, &data) < 0) {
        printf("Read mcu event2 failed.\n");
        return -3;
    }
    *val |= data << 8;
    return 0;
}

void key_detect()
{
	unsigned int evt = 0;
	unsigned char val = 0;
	gpio_set_direction(GPIO_INT_PIN, 0);
	while(1)
	{
		if(gpio_get_input_value(GPIO_INT_PIN) == 1){
			read_mcu_event(&evt);
			if(evt & EVT_KEY_MASK){
				printf("KEY_RST press\n");
			}
			else if(evt & EVT_RING_MASK){
				printf("KEY_RING press\n");
			}
		}
		read_mcu_i2c(REG_Status, &val);
		if(!(val & STA_TAMPER_MASK)){
			printf("TAMPER: 0\n");
		}
		usleep(20*1000);
	}
}

void *pir_get_sta_thread(void *args)
{
	unsigned char val = 0;
	while(1)
	{
		read_mcu_i2c(REG_Status, &val);
		if(val & STA_PIR_MASK){
			printf("PIR Status: 1\n");
		}
	}
}
int pir(unsigned char sta)
{
	unsigned int evt = 0, flag = 0, i = 0;
	unsigned char val = 0;

	if(sta){
		pthread_t pir_thread_id;
		pthread_create(&pir_thread_id, NULL, pir_get_sta_thread, NULL);
	}

	read_mcu_i2c(REG_Cntl, &val);
	val |= CNTL_PIRE_MASK;
	write_mcu_i2c(REG_Cntl, val);//Enable PIR

	while(1)
	{
		if(gpio_get_input_value(GPIO_INT_PIN) == 1){
			read_mcu_event(&evt);
			if(evt & EVT_STC_MASK){
				printf("PIR Event enable\n");
				i = 0;
				flag = 1;
			}
		}
		if(flag){
			if(i++ >= 50){
				i = 0;
				flag = 0;
				printf("PIR Event disable\n");
			}
		}
		usleep(20*1000);
	}
	return 0;
}

void wdt(int time)
{
	unsigned char val = 0;
	read_mcu_i2c(REG_Cntl, &val);
	val |= CNTL_WDTE_MASK;
	write_mcu_i2c(REG_Cntl, val);//Enable watch dog
	if(time == 0){
		printf("close watch dog \n");
	}
	else{
		printf("set wdt time %d\n", time);
	}
	write_mcu_i2c(REG_WDT, time);
}

void feed_wdt(void)
{
	gpio_set_direction(WDT_INT_PIN, 1);
	while(1)
	{
		gpio_output(WDT_INT_PIN, 1);
		sleep(1);
		gpio_output(WDT_INT_PIN, 0);
		sleep(1);
	}
}

int mcu_set_command(uint8_t cmd)
{	
	return i2c_writes(I2C_BUS, I2C_ADDR, REG_Command, &cmd, 1);
}
