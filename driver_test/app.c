#include <stdio.h>
#include <string.h>
#include <stdint.h> 
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>

#include "gpio.h"
#include "i2c_api.h"
#include "mcu.h"
#include "aw9523.h"
#include "sample-adc.h"
#include "DIO.h"
#include "upgrade.h"

static void usage(void)
{
	const char *str="\nExample\n\t"
					"./app [option]\n"
					"option:\n\t"
					"mcu		print mcu regs\n\t"
					"isp [file]	mcu isp upgrade\n\t"
					"key		key detect\n\t"
					"pir		pir test\n\t"
					"wdt [time]	watch dog\n\t"
					"feed		feed dog\n\t"
					"led		aw9523 test\n\t"
					"adc		Light Sensor test\n\t"
					"nfc		NFC test\n\t"
					"DIDO		DIDO test\n\t"
					"irled		IR_LED test\n\t"
					"irc		IRCTUN test\n\t"
					"grled		green led and red led test\n";

	printf("%s", str);
}

int read_terminal_info(char *cmd, char *output)
{
    FILE *fp = NULL;

    fp = popen(cmd, "r");
    if (fp == NULL){
        printf("popen faild. (%d, %s)\n", errno, strerror(errno));
        return -1;
    }

    fread(output, 1, sizeof(output), fp);
    pclose(fp);
	
    return 0;
}

int kill_process(char *name)
{
	char buf[10] = {0};
	char cmd[256] = {0};

	if(name == NULL){
		printf("process: NULL\n");
		return -1;
	}
	printf("kill process: %s\n", name);

	memset(buf, 0, sizeof(buf));
	memset(cmd, 0, sizeof(cmd));
	sprintf(cmd, "ps | grep -v grep | grep %s | awk '{print $1}'", name);
	//sprintf(cmd, "pidof %s", name);
	read_terminal_info(cmd, buf);

	memset(cmd, 0, sizeof(cmd));
	sprintf(cmd, "kill -9 %s", buf);
	printf("%s\n", cmd);
	system(cmd);

	return 0;
}

int main(int argc,char * argv[])
{
	int i, ret;
	int time = 0;
	unsigned char val, tmp;
	
	if(argc < 2){
		usage();
		return 0;
	}

	if(!strcmp(argv[1], "mcu")){
		print_mcu_regs();
	}

	else if(!strcmp(argv[1], "isp")){
		if(argv[2] != NULL){
			kill_process("daemonservice");
			kill_process("avencoder");
			kill_process("veyes");
			kill_process("rtsp_server");

			printf("update fw: %s \n", argv[2]);
			sleep(1);
			if(strstr(argv[2], ".bin"))
				upgrade_isc_mcu("mcu FW: ", argv[2], FMUPGRADE_NONE);
			else if (strstr(argv[2], ".img"))
				image_update(argv[2]);
			
		}
		else{
			printf("param error!\n");
		}
	}
		
	else if(!strcmp(argv[1], "key")){
		key_detect();
	}

	else if(!strcmp(argv[1], "pir")){
		if(argv[2] != NULL){
			pir(1);
		}
		pir(0);
	}

	else if(!strcmp(argv[1], "wdt")){
		if(argv[2] != NULL){
			time = atoi(argv[2]);
		}
		wdt(time);
	}

	else if(!strcmp(argv[1], "feed")){
		feed_wdt();
	}

	else if(!strcmp(argv[1], "led")){
		led();
	}

	else if(!strcmp(argv[1], "adc")){
		adc();
	}

	if(!strcmp(argv[1], "nfc")){
		NFC();
	}

	else if(!strcmp(argv[1], "DIDO")){
		DI_DO();
	}

	else if(!strcmp(argv[1], "irled")){
		IR_LED_TEST();
	}

	else if(!strcmp(argv[1], "irc")){
		IRCTUN_TEST();
	}

	else if(!strcmp(argv[1], "grled")){
		GLED_RLED();
	}

	return 0;
}
