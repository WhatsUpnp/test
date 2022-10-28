#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#include "upgrade.h"
#include "gpio.h"
#include "i2c_api.h"
#include "mcu.h"

typedef uint8_t	u8;
typedef uint16_t u16;
typedef uint32_t u32;

static u8 UpgradingStatus = FALSE;

static int mcu_i2c_transfer(u8 addr, u8* txbuf, u16 txlen, u8* rxbuf, u16 rxlen)
{
	int i;
    u8 msg_num = 0;
    struct i2c_msg msgs[2];
	struct i2c_rdwr_ioctl_data packets;
	int fd = get_bus(0);
    unsigned char retries = 4;

    if(txlen == 0 && rxlen == 0){
        printf("MCU i2c transfer buf length error.\n");
        return -1;
    }

    msgs[0].addr = msgs[1].addr = addr;

    if(txlen && rxlen){
        msgs[0].flags = 0;
        msgs[0].len   = txlen;
        msgs[0].buf   = txbuf;

        msgs[1].flags = 1;
        msgs[1].len   = rxlen;
        msgs[1].buf   = rxbuf;
        msg_num = 2;
    }else{
        if(txlen){
            msgs[0].flags = 0;
            msgs[0].len   = txlen;
            msgs[0].buf   = txbuf;
        }else{
            msgs[0].flags = 1;
            msgs[0].len   = rxlen;
            msgs[0].buf   = rxbuf;
        }

        msg_num = 1;
    }
	packets.msgs = msgs;
	packets.nmsgs = msg_num;

	if(ioctl(fd, I2C_RDWR, &packets) < 0){
		perror("Unable to send i2c read buf data.\n");
		return -1;
	}
	
	return 0;
}

static int unlock_isc_mcu(void)
{
    int ret;
    u8 txbuf[10],rxbuf[10];

    printf("ISC HOSTCMD Command\n");
    //tx HOSTCMD
    txbuf[0] = 0x71;
    rxbuf[0] = 0x00;
    ret = mcu_i2c_transfer(ISP_ADDR, txbuf, 1, rxbuf, 1);
    if(ret < 0 || rxbuf[0] != 0x71){
        printf("ISC Hostcmd Command Failed, ret %d Response %02x\n", ret, rxbuf[0]);
        return -1;
    }

    printf("ISC Unlock Command\n");
    //tx Unlock
    txbuf[0] = 0x74;
    txbuf[1] = 0xFF;
    txbuf[2] = 0xFF;
    txbuf[3] = 0xFF;
    txbuf[4] = 0xFF;
    txbuf[5] = 0xFF;
    txbuf[6] = 0xFF;
    txbuf[7] = 0xFF;
    txbuf[8] = 0xFF;
    rxbuf[0] = 0x00;
    ret = mcu_i2c_transfer(ISP_ADDR, txbuf, 9, rxbuf, 1);
    if(ret < 0 || rxbuf[0] != 0x01){
        printf( "ISC Unlock Command Failed, ret %d, Response %02x \n", ret, rxbuf[0]);
        return -1;
    }

    printf("ISC Unprotect Command \n");
    //tx Unprotect
    txbuf[0] = 0x8D;
    rxbuf[0] = 0x00;
    ret = mcu_i2c_transfer(ISP_ADDR, txbuf, 1, rxbuf, 0);
    if(ret < 0){
        printf("ISC Unprotect Command Failed\n");
        return -1;
    }
    usleep(350000);
    rxbuf[0] = 0x00;
    ret = mcu_i2c_transfer(ISP_ADDR, txbuf, 0, rxbuf, 1);
    if(ret < 0 || rxbuf[0] != 0x01){
        printf("RX ISC Unprotect Command Response Failed, ret %d, Response %02x \n", ret, rxbuf[0]);
        return -1;
    }

    return 0;
}

static int erase_isc_mcu(void)
{
    int ret;
    u8 txbuf[2],rxbuf[2];

    printf("ISC Erase Command\n");
    //tx Main Memory Erase
    txbuf[0] = 0x75;
    rxbuf[0] = 0x00;
    ret = mcu_i2c_transfer(ISP_ADDR, txbuf, 1, rxbuf, 0);
    if(ret < 0){
        printf("TX ISC Main Memory Erase Command Failed, ret %d \n",ret);
        return -1;
    }

    usleep(100000);//MCU internal erase need some time
    rxbuf[0] = 0x00;
    ret = mcu_i2c_transfer(ISP_ADDR,txbuf,0,rxbuf,1);
    if(ret < 0 || rxbuf[0] != 0x01){
        printf("RX ISC Main Memory Erase Command Response Failed, ret %d, Response %02x \n",ret, rxbuf[0]);
        return -1;
    }

    return 0;
}

int upgrade_isc_mcu(const char *name, const char *filename, unsigned int flags)
{
    int fp;
	struct stat fno;
   	u8 pbuf[260],rbuf[260];
    u32 len;
	u32 size;
    u32 addr;
    u8 checksum;
    int retries;
    int ret;
    int i;
    
    if ((fp = open(filename, O_RDONLY)) == -1){
        printf("%s read fail fp %d!\n", filename, fp);
        return -1;
    }

	if(UpgradingStatus == TRUE){
		printf("System is upgrading!");
		close(fp);
		return -1;
	} 
	UpgradingStatus = TRUE;

	stat(filename,&fno);
	printf("\nUpdate %s %s start, file size=%ld \n", name, filename, fno.st_size);
    //Host hold self power
	gpio_set_direction(POWER_HOLD_PIN, 1);
    if(gpio_output(POWER_HOLD_PIN, 1)){
		printf("gpio_output 1 err\n");
	}
    //wait HW level to be stable
    usleep(30000);
    
    //send upgrade command, MCU will reset
    printf("ISC Upgrade Command\n");
	if(mcu_set_command(0x05)){
		printf("Error: Send Reset cmd failed.\n");
		retries = 5;
		printf("Press the reset button of the mcu when counting down to 0 !\n");
		printf("Press the reset button of the mcu when counting down to 0 !\n");
		printf("Press the reset button of the mcu when counting down to 0 !\n");
		while(retries--){
			printf("%d\n", retries);
			sleep(1);
		}
	}

    //MCU will reset to ISP loader, wait 50ms 
    usleep(50000);

    if(unlock_isc_mcu() < 0){
        printf("Send MCU Unlock Command Failed\n");
        ret = -4;
        goto err;
    }
    
    retries = 5;
    while(retries--){

        len = fno.st_size;
        
        //erase MCU main block
        if(erase_isc_mcu() < 0){
            printf("ISC Erase Main Block Failed\n");
            continue;
        }

        if(lseek(fp, 0, SEEK_SET) == -1){
            printf("Seek %s Failed \n", filename);
            continue;
        }
        
        for(addr = 0; len > 0; len -= size)
        {
            memset(pbuf, 0xff, sizeof(pbuf));
            size = len > 256 ? 256 : len;
            if(read(fp, (u8*)&pbuf[3], size) != (long)size) {
                printf("Read %s Failed,Sector %06x \n", filename, addr);
                break;
            }
            pbuf[0] = 0x83;
            pbuf[1] = addr>>16;
            pbuf[2] = addr>>8;
            //calculate checksum of data send to MCU, checksum = cmd + sector addr +  data
            for(checksum=0, i=0; i < (256+3); i++)
                checksum += pbuf[i];

            //program sector
            rbuf[0] = rbuf[1] = 0x00;
            ret = mcu_i2c_transfer(ISP_ADDR, pbuf, (256+3), rbuf, 2);
            if((ret < 0) || (rbuf[1] != 0x01) || (rbuf[0] != checksum)){
                printf("ISC Program Failed, Sector %06x, ret %d, Response %02x, Host Checksum %02x, MCU Checksum %02x\n", addr, ret, rbuf[1], checksum, rbuf[0]);
                break;
            }

            //read sector
            pbuf[0] = 0x81;
            if(mcu_i2c_transfer(ISP_ADDR, pbuf, 3, rbuf, 257) < 0){
                printf( "TX ISC Read Sector Failed, Sector %06x.\n",addr);
                break;            
            }

            //calculate checksum of data read from MCU flash, checksum = cmd + sector addr +  data 
            for(checksum=0, i=0; i < 256; i++)
                checksum += rbuf[i]; 

            checksum += pbuf[0];    
            checksum += pbuf[1];
            checksum += pbuf[2];    

            //compare sector
            if((rbuf[256] != checksum) || memcmp(&pbuf[3], &rbuf[0], 256)){
                printf("ISC Program Verify Failed, Sector %06x, Host Checksum %02x, MCU Checksum %02x\n", addr, checksum, rbuf[256]); 
                break;
            }

            addr += 256;
            printf("ISC Upgrading %s, %04ld/%ld...\n", filename, (fno.st_size - len), fno.st_size);
        }

        if(len == 0){
			printf("ISC Upgrading %s, %04ld/%ld...\n", filename, (fno.st_size - len), fno.st_size);
			break;//program ok
		}
    }    

    close(fp);

    if(len == 0) {
         printf("ISC Exit Host Command \n");
        // tx Exit Host command
        pbuf[0] = 0x76;
        rbuf[0] = 0x00;
        ret = mcu_i2c_transfer(ISP_ADDR, pbuf, 1, rbuf, 1);
        if(ret < 0 || rbuf[0] != 0x01){
            printf("ISC Exit HostCommand Failed.\n");       
            sleep(2); 
            goto err;
        }
		printf("wait MCU to hold power... \n");
        sleep(7);//delay 7s to wait MCU to hold power
        gpio_output(POWER_HOLD_PIN, 0);
        
        if(flags & FMUPGRADE_DELETE) {
            remove(filename);
			printf("remove %s succeed\n", filename);
        }
        else if(flags & FMUPGRADE_RENAME) {
            //upgrade OK, rename file to ".bak"
            char * newname = (char*)malloc(strlen(filename) + 8);
            if(newname) {
                strcpy(newname, filename);
                strcat(newname, ".bak");
                remove(newname);
                rename(filename, newname);
                printf( "Rename %s to %s\n", filename, newname);
                free(newname);
            }else{
                printf( "Can't rename %s\n", filename);
            }
        }
		printf("\nMCU firmware upgrade finished %s\n", filename);
    }else{
		printf("\nMCU firmware upgrade fail %s\n", filename);
		goto err;
	}
	UpgradingStatus = FALSE;
    printf("Update %s Finish, need write %d bytes, left %d bytes\n\n", name, (u32)fno.st_size, len);
    return len == 0;
    
err:
    close(fp);
    gpio_output(POWER_HOLD_PIN,0);
	UpgradingStatus = FALSE;
    return ret;      
}
/*-------------------------------update mcu.img--------------------------------*/
#define IH_MAGIC		0x27051956	/* Image Magic Number */
#define IH_NMLEN		32			/* Image Name Length  */

#define Big2Little32(A) ((((uint32_t)(A)&0xff000000) >> 24)|\
                         (((uint32_t)(A)&0x00ff0000) >> 8) |\
                         (((uint32_t)(A)&0x0000ff00) << 8) |\
                         (((uint32_t)(A)&0x000000ff) << 24))

typedef struct image_header {
	uint32_t		ih_magic;	/* Image Header Magic Number	*/
	uint32_t		ih_hcrc;	/* Image Header CRC Checksum	*/
	uint32_t		ih_time;	/* Image Creation Timestamp	*/
	uint32_t		ih_size;	/* Image Data Size		*/
	uint32_t		ih_load;	/* Data	 Load  Address		*/
	uint32_t		ih_ep;		/* Entry Point Address		*/
	uint32_t		ih_dcrc;	/* Image Data CRC Checksum	*/
	uint8_t			ih_os;		/* Operating System		*/
	uint8_t			ih_arch;	/* CPU architecture		*/
	uint8_t			ih_type;	/* Image Type			*/
	uint8_t			ih_comp;	/* Compression Type		*/
	uint8_t			ih_name[IH_NMLEN];	/* Image Name		*/
} image_header_t;

static const uint32_t crc_table[256] = {
	0x00000000,	0x77073096,	0xee0e612c,	0x990951ba,	0x076dc419,0x706af48f,	0xe963a535,	0x9e6495a3,
	0x0edb8832, 0x79dcb8a4, 0xe0d5e91e,	0x97d2d988,	0x09b64c2b,	0x7eb17cbd,	0xe7b82d07, 0x90bf1d91,
	0x1db71064,	0x6ab020f2,	0xf3b97148,	0x84be41de, 0x1adad47d,	0x6ddde4eb,	0xf4d4b551,	0x83d385c7,
	0x136c9856, 0x646ba8c0,	0xfd62f97a,	0x8a65c9ec,	0x14015c4f,	0x63066cd9, 0xfa0f3d63,	0x8d080df5,
	0x3b6e20c8,	0x4c69105e,	0xd56041e4, 0xa2677172,	0x3c03e4d1,	0x4b04d447,	0xd20d85fd,	0xa50ab56b,
	0x35b5a8fa,	0x42b2986c,	0xdbbbc9d6,	0xacbcf940,	0x32d86ce3, 0x45df5c75,	0xdcd60dcf,	0xabd13d59,
	0x26d930ac,	0x51de003a, 0xc8d75180,	0xbfd06116,	0x21b4f4b5,	0x56b3c423,	0xcfba9599,	0xb8bda50f,
	0x2802b89e,	0x5f058808,	0xc60cd9b2,	0xb10be924, 0x2f6f7c87,	0x58684c11,	0xc1611dab,	0xb6662d3d,
	0x76dc4190, 0x01db7106,	0x98d220bc,	0xefd5102a,	0x71b18589,	0x06b6b51f, 0x9fbfe4a5,	0xe8b8d433,
	0x7807c9a2,	0x0f00f934,	0x9609a88e, 0xe10e9818,	0x7f6a0dbb,	0x086d3d2d,	0x91646c97,	0xe6635c01,
	0x6b6b51f4,	0x1c6c6162,	0x856530d8,	0xf262004e,	0x6c0695ed, 0x1b01a57b,	0x8208f4c1,	0xf50fc457,
	0x65b0d9c6,	0x12b7e950, 0x8bbeb8ea,	0xfcb9887c,	0x62dd1ddf,	0x15da2d49,	0x8cd37cf3, 0xfbd44c65,	
	0x4db26158,	0x3ab551ce,	0xa3bc0074,	0xd4bb30e2, 0x4adfa541,	0x3dd895d7,	0xa4d1c46d,	0xd3d6f4fb,	
	0x4369e96a, 0x346ed9fc,	0xad678846,	0xda60b8d0,	0x44042d73,	0x33031de5, 0xaa0a4c5f,	0xdd0d7cc9,	
	0x5005713c,	0x270241aa,	0xbe0b1010, 0xc90c2086,	0x5768b525,	0x206f85b3,	0xb966d409,	0xce61e49f, 
	0x5edef90e,	0x29d9c998,	0xb0d09822,	0xc7d7a8b4,	0x59b33d17, 0x2eb40d81,	0xb7bd5c3b,	0xc0ba6cad,	
	0xedb88320,	0x9abfb3b6, 0x03b6e20c,	0x74b1d29a,	0xead54739,	0x9dd277af,	0x04db2615, 0x73dc1683,	
	0xe3630b12,	0x94643b84,	0x0d6d6a3e,	0x7a6a5aa8, 0xe40ecf0b,	0x9309ff9d,	0x0a00ae27,	0x7d079eb1,	
	0xf00f9344, 0x8708a3d2,	0x1e01f268,	0x6906c2fe,	0xf762575d,	0x806567cb, 0x196c3671,	0x6e6b06e7,	
	0xfed41b76,	0x89d32be0,	0x10da7a5a, 0x67dd4acc,	0xf9b9df6f,	0x8ebeeff9,	0x17b7be43,	0x60b08ed5,
	0xd6d6a3e8,	0xa1d1937e,	0x38d8c2c4,	0x4fdff252,	0xd1bb67f1, 0xa6bc5767,	0x3fb506dd,	0x48b2364b,
	0xd80d2bda,	0xaf0a1b4c, 0x36034af6,	0x41047a60,	0xdf60efc3,	0xa867df55,	0x316e8eef, 0x4669be79,	
	0xcb61b38c,	0xbc66831a,	0x256fd2a0,	0x5268e236, 0xcc0c7795,	0xbb0b4703,	0x220216b9,	0x5505262f,	
	0xc5ba3bbe, 0xb2bd0b28,	0x2bb45a92,	0x5cb36a04,	0xc2d7ffa7,	0xb5d0cf31, 0x2cd99e8b,	0x5bdeae1d,
	0x9b64c2b0,	0xec63f226,	0x756aa39c, 0x026d930a,	0x9c0906a9,	0xeb0e363f,	0x72076785,	0x05005713, 
	0x95bf4a82,	0xe2b87a14,	0x7bb12bae,	0x0cb61b38,	0x92d28e9b, 0xe5d5be0d,	0x7cdcefb7,	0x0bdbdf21,	
	0x86d3d2d4,	0xf1d4e242, 0x68ddb3f8,	0x1fda836e,	0x81be16cd,	0xf6b9265b,	0x6fb077e1, 0x18b74777,	
	0x88085ae6,	0xff0f6a70,	0x66063bca,	0x11010b5c, 0x8f659eff,	0xf862ae69,	0x616bffd3,	0x166ccf45,	
	0xa00ae278, 0xd70dd2ee,	0x4e048354,	0x3903b3c2,	0xa7672661,	0xd06016f7, 0x4969474d,	0x3e6e77db,	
	0xaed16a4a,	0xd9d65adc,	0x40df0b66, 0x37d83bf0,	0xa9bcae53,	0xdebb9ec5,	0x47b2cf7f,	0x30b5ffe9,
	0xbdbdf21c,	0xcabac28a,	0x53b39330,	0x24b4a3a6,	0xbad03605, 0xcdd70693,	0x54de5729,	0x23d967bf,	
	0xb3667a2e,	0xc4614ab8, 0x5d681b02,	0x2a6f2b94,	0xb40bbe37,	0xc30c8ea1,	0x5a05df1b, 0x2d02ef8d
};

#define	DO1(buf) crc = crc_table[((int)crc ^ (*buf++)) & 0xff] ^ (crc >> 8)
#define	DO2(buf) do {DO1(buf);	DO1(buf); } while (0)
#define	DO4(buf) do {DO2(buf);	DO2(buf); } while (0)
#define	DO8(buf) do {DO4(buf);	DO4(buf); } while (0)

uint32_t crc32(uint32_t crc, const void * b, uint32_t len)
{
	const unsigned char  * buf = (const unsigned char  *)b;
	crc = crc ^ 0xffffffff;

	while (len >= 8) {
		DO8(buf);
		len	-= 8;
	}
	if (len) {
		do {
			DO1(buf);
		} while (--len);
	}
	return crc ^ 0xffffffff;
}

int upgrade_mcu(unsigned char *data, unsigned int leng)
{
   	u8 pbuf[260],rbuf[260];
    u32 len;
	u32 size;
    u32 addr;
    u8 checksum;
    int retries;
    int ret;
    int i;
	unsigned char *pdata = data;

	if(UpgradingStatus == TRUE){
		printf("System is upgrading!");
		return -1;
	} 
	UpgradingStatus = TRUE;

	printf("\nUpdate start, data size=%ld \n", leng);
    //Host hold self power
	gpio_set_direction(POWER_HOLD_PIN, 1);
    if(gpio_output(POWER_HOLD_PIN, 1)){
		printf("gpio_output 1 err\n");
	}
    //wait HW level to be stable
    usleep(30000);
    
	for(i=0; i<3; i++){
		//send upgrade command, MCU will reset
		printf("ISC Upgrade Command\n");
		if(mcu_set_command(0x05)){
			printf("Error: Send Reset cmd failed.\n");
			retries = 5;
			printf("Press the reset button of the mcu when counting down to 0 !\n");
			printf("Press the reset button of the mcu when counting down to 0 !\n");
			printf("Press the reset button of the mcu when counting down to 0 !\n");
			while(retries--){
				printf("%d\n", retries);
				sleep(1);
			}
		}

		//MCU will reset to ISP loader, wait 50ms 
		usleep(50000);

		if(unlock_isc_mcu() < 0){
			printf("Send MCU Unlock Command Failed\n");
		}
		else{
			break;
		}
	}
	if(i >= 3){
		ret = -4;
		goto err;
	}
    
    retries = 5;
    while(retries--){

        len = leng;
        pdata = data;
        //erase MCU main block
        if(erase_isc_mcu() < 0){
            printf("ISC Erase Main Block Failed\n");
            continue;
        }

        for(addr = 0; len > 0; len -= size)
        {
            memset(pbuf, 0xff, sizeof(pbuf));
            size = len > 256 ? 256 : len;
			memcpy((u8*)&pbuf[3], pdata, size);
			pdata += size;
            pbuf[0] = 0x83;
            pbuf[1] = addr>>16;
            pbuf[2] = addr>>8;
            //calculate checksum of data send to MCU, checksum = cmd + sector addr +  data
            for(checksum=0, i=0; i < (256+3); i++)
                checksum += pbuf[i];

            //program sector
            rbuf[0] = rbuf[1] = 0x00;
            ret = mcu_i2c_transfer(ISP_ADDR, pbuf, (256+3), rbuf, 2);
            if((ret < 0) || (rbuf[1] != 0x01) || (rbuf[0] != checksum)){
                printf("ISC Program Failed, Sector %06x, ret %d, Response %02x, Host Checksum %02x, MCU Checksum %02x\n", addr, ret, rbuf[1], checksum, rbuf[0]);
                break;
            }

            //read sector
            pbuf[0] = 0x81;
            if(mcu_i2c_transfer(ISP_ADDR, pbuf, 3, rbuf, 257) < 0){
                printf( "TX ISC Read Sector Failed, Sector %06x.\n",addr);
                break;            
            }

            //calculate checksum of data read from MCU flash, checksum = cmd + sector addr +  data 
            for(checksum=0, i=0; i < 256; i++)
                checksum += rbuf[i]; 

            checksum += pbuf[0];    
            checksum += pbuf[1];
            checksum += pbuf[2];    

            //compare sector
            if((rbuf[256] != checksum) || memcmp(&pbuf[3], &rbuf[0], 256)){
                printf("ISC Program Verify Failed, Sector %06x, Host Checksum %02x, MCU Checksum %02x\n", addr, checksum, rbuf[256]); 
                break;
            }

            addr += 256;
            printf("ISC Upgrading %04ld/%ld...\n", (leng - len), leng);
        }

        if(len == 0){
			printf("ISC Upgrading %04ld/%ld...\n", (leng - len), leng);
			break;//program ok
		}
    }    

    if(len == 0) {
         printf("ISC Exit Host Command \n");
        // tx Exit Host command
        pbuf[0] = 0x76;
        rbuf[0] = 0x00;
        ret = mcu_i2c_transfer(ISP_ADDR, pbuf, 1, rbuf, 1);
        if(ret < 0 || rbuf[0] != 0x01){
            printf("ISC Exit HostCommand Failed.\n");       
            sleep(2); 
			ret = mcu_i2c_transfer(ISP_ADDR, pbuf, 1, rbuf, 1);
			if(ret < 0 || rbuf[0] != 0x01){
				printf("ISC Exit HostCommand Failed.\n");       
				sleep(2); 
				goto err;
			}
        }
		printf("wait MCU to hold power... \n");
        sleep(7);//delay 7s to wait MCU to hold power
        gpio_output(POWER_HOLD_PIN, 0);
		printf("\nMCU firmware upgrade finished\n");
    }else{
		printf("\nMCU firmware upgrade fail\n");
		goto err;
	}

	UpgradingStatus = FALSE;
    printf("Update Finish, need write %d bytes, left %d bytes\n\n", (u32)leng, len);
    return len == 0;
    
err:
    gpio_output(POWER_HOLD_PIN,0);
	UpgradingStatus = FALSE;
    return ret;      
}



int image_check(char *image)
{
	int fd;
	int version;
	uint8_t *pbuf=NULL;
	struct stat st;
	uint32_t len,crc,hcrc;
	image_header_t *hdr=NULL;

	fd = open(image, O_RDONLY);
	if(fd < 0){
		printf("MCU firmware image %s open failed.\n",image);
		//put_bus(0);
		return -1;
	}

	/* read mcu binary file size */
	if(fstat(fd, &st) < 0){
		printf("File stat failed.\n");
		//put_bus(0);
		close(fd);
		return -1;
	}
	
	/* check mcu binary size */
	if(st.st_size > IMG_FILE_MAX){
		printf("File size failed.\n");
		//put_bus(0);
		close(fd);
		return -1;
	}
	
	printf("MCU firmware image name %s\n", image);
	printf("MCU firmware image size %ld\n", st.st_size);

	pbuf = (uint8_t*)malloc(st.st_size);
	if(read(fd, pbuf,st.st_size) != st.st_size){
		printf("Firmware image read failed!\n");
		//put_bus(0);
		close(fd);
		free(pbuf);
		return -1;
	}
	close(fd);

	hdr = (image_header_t*)pbuf;
	if(Big2Little32(hdr->ih_magic) != IH_MAGIC){
		printf("Firmware image bad magic!\n");
		//put_bus(0);
		free(pbuf);
		return  -1;
	}

	len = sizeof(image_header_t);
	hcrc = hdr->ih_hcrc;
	hdr->ih_hcrc = 0;
	crc = crc32(0,pbuf,len);
	if(Big2Little32(hcrc) != crc){
		printf("Firmware image header crc failed! len %d, %08x, %08x!\n",len, Big2Little32(hdr->ih_hcrc), crc);
		//put_bus(0);
		free(pbuf);
		return -1;
	}
	
	len = Big2Little32(hdr->ih_size);
	pbuf = pbuf + sizeof(image_header_t);		
	crc = crc32(0,pbuf,len);
	if(Big2Little32(hdr->ih_dcrc) != crc){
		printf("Firmware image data crc failed! len %d, %08x, %08x!\n",len, Big2Little32(hdr->ih_dcrc), crc);
		//put_bus(0);
		free(hdr);
		return -1;
	}
	
	printf("Firmware image verified ok!\n");		
	free(hdr);
	return 0;
}

int image_update(char *image)
{	
	int fd;
	uint8_t *pbuf = NULL,*data = NULL;
	struct stat st;
	uint32_t len;

	if(image_check(image) == 0){
		fd = open(image, O_RDONLY);
		if(fd < 0){
			printf("MCU firmware image %s open failed.\n",image);
			return -1;
		}

		/* read mcu binary file size */
		if(fstat(fd, &st) < 0){
			printf("File stat failed.\n");
			close(fd);
			return -1;
		}
		
		pbuf = (uint8_t*)malloc(st.st_size);
		if(read(fd, pbuf,st.st_size) != st.st_size){
			printf("Firmware image read failed!\n");
			close(fd);
			goto faill;
		}
		
		close(fd);

		data = pbuf + sizeof(image_header_t);		
		len = st.st_size - sizeof(image_header_t);

		upgrade_mcu(data, len);
		free(pbuf);

		return 0;
	}

	return -1;

faill:
	free(pbuf);
	return -1;
}



