
#ifndef __SYSTEM_SECTION_H_
#define __SYSTEM_SECTION_H_

#ifdef __cplusplus
extern "C" {
#endif

#define ISP_ADDR		(0X4E >> 1)
#define POWER_HOLD_PIN	(32*1 + 2)
#define IMG_FILE_MAX	(10 * 1024)	

#define FMUPGRADE_RENAME	0x01  //if upgrade ok, rename the file to .bak
#define FMUPGRADE_DELETE	0x02  //if upgrade ok, delete the file
#define FMUPGRADE_REBOOT	0x04  //if upgrade ok, reboot the system
#define FMUPGRADE_SHUTDOWN	0x08  //if upgrade ok, shutdown the system
#define FMUPGRADE_NONE		0x10  //if upgrade ok, no action

//upgrade status
#define TRUE	 1  
#define FALSE	 0  

int upgrade_isc_mcu(const char *name, const char *filename, unsigned int flags);
int upgrade_mcu(unsigned char *data, unsigned int leng);
int image_update(char *image);
#ifdef __cplusplus
}
#endif

#endif

