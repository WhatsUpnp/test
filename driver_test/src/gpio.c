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
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <string.h>

#include "gpio.h"

/****************************************************************
Function   : read_string_from_file
Description: read string from the file
Parameter  : str: the string read from the file.
             str_len: reads in at most one less than str_len characters from the file.
             file_name: the file.
Return     :  0: Success
             -1: Fail
****************************************************************/
int read_string_from_file(char *str, int str_len, const char *file_name)
{
	FILE *fp;

	fp = fopen(file_name, "r");
	if(fp == NULL)
	{
		printf("Cannot open file: %s\n", file_name);
		return -1;
	}

	fgets(str, str_len, fp);

	fclose(fp);

	return 0;
}

/****************************************************************
Function   : write_string_to_file
Description: write the string to the file.
Parameter  : str: the string written to the file.
             file_name: the file.
Return     :  0: Success
             -1: Fail
****************************************************************/
int write_string_to_file(const char *str, const char *file_name)
{
	FILE *fp;

	fp = fopen(file_name, "w");
	if(fp == NULL)
	{
		printf("Cannot open file: %s\n", file_name);
		return -1;
	}

	fputs(str, fp);
	fclose(fp);

	return 0;
}

/****************************************************************
Function   : gpio_set_direction
Description: set gpio direction as output or input.
Parameter  : gpio: the gpio number
             output: 1 - set gpio direction as output.
                     0 - set gpio direction as input.
Return     :  0: Success
             -1: Fail
****************************************************************/
int gpio_set_direction(int gpio, int output)
{
	char str[10], file_name[50];
	int len, ret;

	len = sprintf(file_name, "/sys/class/gpio/gpio%d", gpio);
	file_name[len] = 0;

	if( (access(file_name, F_OK )) == -1 ){
		len = sprintf(str, "%d", gpio);
		str[len] = 0;

		ret = write_string_to_file(str, "/sys/class/gpio/export");
		if(ret)
			return ret;
	}

	len = sprintf(file_name, "/sys/class/gpio/gpio%d/direction", gpio);
	file_name[len] = 0;

	memset(str, 0, sizeof(str));
	ret = read_string_from_file(str, sizeof(str), file_name);
	if(ret)
		return ret;

	if(output){
		if(!strncmp(str, "in", 2)){
			ret = write_string_to_file("out", file_name);
			if(ret)
				return ret;
		}
	} else {
		if(!strncmp(str, "out", 3)){
			ret = write_string_to_file("in", file_name);
			if(ret)
				return ret;
		}
	}

	return 0;
}

/****************************************************************
Function   : gpio_output
Description: gpio output high or low level
Parameter  : gpio: the gpio number.
             high: 1: output high level. 0: output low level
Return     :  0: Success
             -1: Fail
****************************************************************/
int gpio_output(int gpio, int high)
{
	char str[10], file_name[50];
	int len, ret;

	len = sprintf(file_name, "/sys/class/gpio/gpio%d", gpio);
	file_name[len] = 0;

	if( (access(file_name, F_OK )) == -1 ){
		len = sprintf(str, "%d", gpio);
		str[len] = 0;

		ret = write_string_to_file(str, "/sys/class/gpio/export");
		if(ret)
			return ret;
	}
	
	len = sprintf(file_name, "/sys/class/gpio/gpio%d/direction", gpio);
	file_name[len] = 0;

	memset(str, 0, sizeof(str));
	ret = read_string_from_file(str, sizeof(str), file_name);
	if(ret){
		printf("set gpio%d output %s level failed\n", gpio, high?"high":"low");
		return ret;
	}
	
	if(!strncmp(str, "in", 2)){
		ret = write_string_to_file("out", file_name);
		if(ret)
			return ret;
	}

	len = sprintf(file_name, "/sys/class/gpio/gpio%d/value", gpio);
	file_name[len] = 0;

	memset(str, 0, sizeof(str));
	ret = read_string_from_file(str, sizeof(str), file_name);
	if(ret)
		return ret;

	if(high){
		if(str[0] == '0'){
			ret = write_string_to_file("1", file_name);
		}
	}
	else{
		if(str[0] == '1'){
			ret = write_string_to_file("0", file_name);
		}
	}

	if(ret)
		return ret;

	return 0;
}

/****************************************************************
Function   : gpio_get_input_value
Description: set gpio direction as input
             and get gpio value (high or low level).
Parameter  : gpio: the gpio number
Return     :  1: gpio is high level
              0: gpio is low level
             -1: Fail
****************************************************************/
int gpio_get_input_value(int gpio)
{
	char str[10], file_name[50];
	int len, ret, status;

	len = sprintf(file_name, "/sys/class/gpio/gpio%d", gpio);
	file_name[len] = 0;

	if( (access(file_name, F_OK )) == -1 ){
		len = sprintf(str, "%d", gpio);
		str[len] = 0;

		ret = write_string_to_file(str, "/sys/class/gpio/export");
		if(ret)
			return ret;
	}

	len = sprintf(file_name, "/sys/class/gpio/gpio%d/direction", gpio);
	file_name[len] = 0;

	memset(str, 0, sizeof(str));
	ret = read_string_from_file(str, sizeof(str), file_name);
	if(ret)
		return ret;

	if(!strncmp(str, "out", 3)){
		ret = write_string_to_file("in", file_name);
		if(ret)
			return ret;
	}

	len = sprintf(file_name, "/sys/class/gpio/gpio%d/value", gpio);
	file_name[len] = 0;

	memset(str, 0, sizeof(str));
	ret = read_string_from_file(str, sizeof(str), file_name);
	if(ret)
		return ret;

	if(str[0] == '0')
		status = 0;
	else if(str[0] == '1')
		status = 1;
	else
		status = -1;
	
	return status;
}

/****************************************************************
Function   : gpio_get_value
Description: get gpio value (high or low level)
Parameter  : gpio: the gpio number.
Return     :  1: gpio is high level
              0: gpio is low level
             -1: Fail
****************************************************************/
int gpio_get_value(int gpio)
{
	char str[10], file_name[50];
	int len, ret, status;

	len = sprintf(file_name, "/sys/class/gpio/gpio%d", gpio);
	file_name[len] = 0;

	if( (access(file_name, F_OK )) == -1 ){
		len = sprintf(str, "%d", gpio);
		str[len] = 0;

		ret = write_string_to_file(str, "/sys/class/gpio/export");
		if(ret)
			return ret;
	}

	len = sprintf(file_name, "/sys/class/gpio/gpio%d/value", gpio);
	file_name[len] = 0;

	memset(str, 0, sizeof(str));
	ret = read_string_from_file(str, sizeof(str), file_name);
	if(ret)
		return ret;

	if(str[0] == '0')
		status = 0;
	else if(str[0] == '1')
		status = 1;
	else
		status = -1;

	return status;
}
