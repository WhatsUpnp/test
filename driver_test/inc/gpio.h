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
#ifndef GPIO_LIB_H
#define GPIO_LIB_H

#ifdef __cplusplus
extern "C"{
#endif

/****************************************************************
Function   : gpio_set_direction
Description: set gpio direction as output or input.
Parameter  : gpio: the gpio number
             output: 1 - set gpio direction as output.
                     0 - set gpio direction as input.
Return     :  0: Success
             -1: Fail
****************************************************************/
extern int gpio_set_direction(int gpio, int output);

/****************************************************************
Function   : gpio_output
Description: gpio output high or low level
Parameter  : gpio: the gpio number.
             high: 1: output high level. 0: output low level
Return     :  0: Success
             -1: Fail
****************************************************************/
extern int gpio_output(int gpio, int high);

/****************************************************************
Function   : gpio_get_input_value
Description: set gpio direction as input
             and get gpio value (high or low level).
Parameter  : gpio: the gpio number
Return     :  1: gpio is high level
              0: gpio is low level
             -1: Fail
****************************************************************/
extern int gpio_get_input_value(int gpio);

/****************************************************************
Function   : gpio_get_value
Description: get gpio value (high or low level)
Parameter  : gpio: the gpio number.
Return     :  1: gpio is high level
              0: gpio is low level
             -1: Fail
****************************************************************/
extern int gpio_get_value(int gpio);

#ifdef __cplusplus
}
#endif

#endif //GPIO_LIB_H
