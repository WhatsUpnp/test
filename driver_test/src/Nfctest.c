#include    <stdio.h>      /*???????????????*/
#include    <termios.h>    /*PPSIX?????????*/
#include    <fcntl.h>      /*??????????*/
#include 	<stdlib.h>
#include 	<unistd.h>
#include 	<errno.h>
#include 	<sys/wait.h>
#include 	<string.h>
//#include	"VIA_20180313.h"

#define ERROR	1
#define	OK		0

#define CommandReg	0x01
#define ComIEnReg	0x02
#define DivIEnReg	0x03
#define ComIrqReg	0x04
#define DivIrqReg	0x05
#define ErrorReg	0x06
#define Status1Reg	0x07
#define Status2Reg	0x08
#define FIFODataReg 0x09
#define FIFOLevelReg 0x0A
#define WaterLevelReg 0x0B
#define ControlReg	0x0C
#define BitFramingReg 0x0D
#define CollReg	0x0E
#define ModeReg 0x11
#define TxModeReg 0x12
#define RxModeReg 0x13
#define TxControlReg 0x14
#define TxAutoReg 0x15
#define TxSelReg 0x16
#define RxSelReg 0x17
#define RxThresholdReg 0x18
#define DemodReg 0x19
#define MfTxReg 0x1C
#define MfRxReg 0x1D
#define SerialSpeedReg 0x1F
#define CRCMSBReg 0x21
#define CRCLSBReg 0x22
#define ModWidthReg 0x24
#define GsNOffReg 0x23
#define TxBitPhaseReg 0x25
#define RFCfgReg 0x26
#define GsNReg 0x27
#define CWGsPReg 0x28
#define ModGsPReg 0x29
#define TModeReg 0x2A
#define TPrescalerReg 0x2B
#define TReloadMSBReg 0x2C
#define TReloadLSBReg 0x2D
#define TCounterValMSBReg 0x2E
#define TCounterValLSBReg 0x2F
#define TestSel1Reg 0x31
#define TestSel2Reg 0x32
#define TestPinEnReg 0x33
#define TestPinValueReg 0x34
#define TestBusReg 0x35
#define AutoTestReg 0x36
#define VersionReg 0x37
#define AnalogTestReg 0x38
#define TestDAC1Reg 0x39
#define TestDAC2Reg 0x3A
#define TestADCReg 0x3B

#define Idle	0x00
#define Mem		0x01
#define RandomID	0x02
#define CalcCRC	0x03
#define Transmit	0x04
#define NoCmdChange	0x07
#define Receive	0x08
#define Transceive	0x0C
#define MFAuthent	0x0E
#define SoftReset	0x0F
#define MAXRLEN 18

struct PICC_A_STR
{
unsigned char ATQA[2];
unsigned char UID_len;
unsigned char UID[12];
unsigned char SAK;
};

int uart_fd = -1;
#define MAXLINE 1024
int uart_init(int baud);

int my_read(unsigned char *value, size_t count)
{
	unsigned int maxfdp;
	fd_set fds;
	struct timeval tv;
	int r;
	
	FD_ZERO( &fds );
	FD_SET( uart_fd, &fds );
	maxfdp = uart_fd;

	tv.tv_sec = 0;  /* Timeout. */
	tv.tv_usec = 10000;
	r = select( maxfdp + 1, &fds, NULL, NULL, &tv );
	if( r<=0 )
		return r;

	return read(uart_fd, value, count);
}

unsigned char Read_Reg(unsigned char reg_add) {
    unsigned char ucAddr;
    unsigned char reg_value;
    unsigned char isTimeout;
	unsigned int m_count;
	
	//tcflush(uart_fd, TCIOFLUSH);  
	
    ucAddr = (reg_add & 0x3F) | 0x80;
    m_count = write(uart_fd, &ucAddr,sizeof(ucAddr));
	
	m_count = my_read(&reg_value, 1);
	if( m_count<=0 ){
		printf("Read_Reg 0x%02x error\n", reg_add);
	}

    return reg_value;
}

unsigned char Write_Reg(unsigned char reg_add, unsigned char reg_value) {
    unsigned char ucAddr, tmpAddr=0;
    unsigned char isTimeout;
	 int m_count;

	//tcflush(uart_fd, TCIOFLUSH);  
	
    ucAddr = (reg_add & 0x3F);
    m_count = write(uart_fd, &ucAddr,sizeof(ucAddr));

	m_count = my_read(&tmpAddr, 1);
	if( m_count<=0 ){
		printf("Write_Reg 0x%02x = 0x%02x error\n", reg_add,reg_value);
	}else if (tmpAddr != ucAddr) {
       printf("Write_Reg failed, Addr[0x%02x!=0x%02x] Val:0x%02x\n", ucAddr, tmpAddr, reg_value);
        return 0;
    }

    m_count = write(uart_fd, &reg_value,sizeof(reg_value));
    return 1;
}

unsigned char Set_BitMask(unsigned char reg_add, unsigned char mask) {
    unsigned char result;
    result = Write_Reg(reg_add, Read_Reg(reg_add) | mask); // set bit mask
    return result;
}

unsigned char Clear_BitMask(unsigned char reg_add, unsigned char mask) {
    unsigned char result;
    result = Write_Reg(reg_add, Read_Reg(reg_add) & ~mask); // clear bit mask
    return result;
}

void Read_FIFO(unsigned char length, unsigned char* fifo_data) {
    int i;
    for (i = 0; i < length; i++) {
        fifo_data[i] = Read_Reg(FIFODataReg);
    }
    return;
}

void Write_FIFO(unsigned char length, unsigned char* fifo_data) {
    int i;
    for (i = 0; i < length; i++) {
        Write_Reg(FIFODataReg, fifo_data[i]);
    }
    return;
}

unsigned char Clear_FIFO(void) {
    Set_BitMask(FIFOLevelReg, 0x80); //???FIFO????
    if (Read_Reg(FIFOLevelReg) == 0)
        return OK;
    else {
        printf("Clear_FIFO error");
        return ERROR;
    }
}

unsigned char Pcd_Comm(unsigned char Command,
                       unsigned char* pInData,
                       unsigned char InLenByte,
                       unsigned char* pOutData,
                       unsigned int* pOutLenBit) {
    char status           = ERROR;
    unsigned char irqEn   = 0x00;
    unsigned char waitFor = 0x00;
    unsigned char lastBits;
    unsigned char n;
    unsigned int i;

    switch (Command) {
    case MFAuthent:
        irqEn   = 0x12;
        waitFor = 0x10;
        break;
    case Transceive:
        irqEn   = 0x77;
        waitFor = 0x30;
        break;
    default:
        break;
    }

    Write_Reg(ComIEnReg, irqEn | 0x80);
    Clear_BitMask(ComIrqReg, 0x80);
    Write_Reg(CommandReg, Idle);
    Set_BitMask(FIFOLevelReg, 0x80);

    for (i = 0; i < InLenByte; i++) {
        Write_Reg(FIFODataReg, pInData[i]);
    }
    Write_Reg(CommandReg, Command);

    if (Command == Transceive) {
        Set_BitMask(BitFramingReg, 0x80);
    }

    i = 30; //???????????????????M1??????????25ms
    do {
        n = Read_Reg(ComIrqReg);
        i--;
    } while ((i != 0) && !(n & 0x01) && !(n & waitFor));

    Clear_BitMask(BitFramingReg, 0x80);

    if (i != 0) {
        if (!(Read_Reg(ErrorReg) & 0x1B)) {
            status = OK;
            if (n & irqEn & 0x01) {
                status = ERROR;
            }
            if (Command == Transceive) {
                n        = Read_Reg(FIFOLevelReg);
                lastBits = Read_Reg(ControlReg) & 0x07;
                if (lastBits) {
                    *pOutLenBit = (n - 1) * 8 + lastBits;
                } else {
                    *pOutLenBit = n * 8;
                }
                if (n == 0) {
                    n = 1;
                }
                if (n > MAXRLEN) {
                    n = MAXRLEN;
                }
                for (i = 0; i < n; i++) {
                    pOutData[i] = Read_Reg(FIFODataReg);
                }
            }
        } else {
            status = ERROR;
        }
    }

    Set_BitMask(ControlReg, 0x80); // stop timer now
    Write_Reg(CommandReg, Idle);
    return status;
}

unsigned char Pcd_Comm1(unsigned char Command,
                        unsigned char* pInData,
                        unsigned char InLenByte,
                        unsigned char* pOutData,
                        unsigned int* pOutLenBit) {
    unsigned char result;
    unsigned char rx_temp  = 0; //?????????????
    unsigned char rx_len   = 0; //??????????????
    unsigned char lastBits = 0; //????????��????
    unsigned char irq;

    Clear_FIFO();
    Write_Reg(CommandReg, Idle);
    Write_Reg(WaterLevelReg, 0x20); //????FIFOLevel=32???
    Write_Reg(ComIrqReg, 0x7F);     //???IRQ???

    if (Command == MFAuthent) {
        Write_FIFO(InLenByte, pInData);   //??????????
        Set_BitMask(BitFramingReg, 0x80); //????????
    }

    Set_BitMask(TModeReg, 0x80); //????????????
    Write_Reg(CommandReg, Command);

    while (1) //????��??��???
    {
        irq = Read_Reg(ComIrqReg); //????��???
        if (irq & 0x01)            //TimerIRq  ???????????
        {
            result = ERROR;
            break;
        }

        if (Command == MFAuthent) {
            if (irq & 0x10) //IdelIRq  command?????????��??????????
            {
                result = OK;
                break;
            }
        }

        if (Command == Transmit) {
            if ((irq & 0x04) && (InLenByte > 0)) //LoAlertIrq+?????????????0
            {
                if (InLenByte < 32) {
                    Write_FIFO(InLenByte, pInData);
                    InLenByte = 0;
                } else {
                    Write_FIFO(32, pInData);
                    InLenByte = InLenByte - 32;
                    pInData   = pInData + 32;
                }
                Write_Reg(ComIrqReg, 0x04);       //???LoAlertIrq
                Set_BitMask(BitFramingReg, 0x80); //????????
            }

            if ((irq & 0x40) && (InLenByte == 0)) //TxIRq
            {
                result = OK;
                break;
            }
        }

        if (Command == Transceive) {
            if ((irq & 0x04) && (InLenByte > 0)) //LoAlertIrq+?????????????0
            {
                if (InLenByte > 32) {
                    Write_FIFO(32, pInData);
                    InLenByte = InLenByte - 32;
                    pInData   = pInData + 32;
                } else {
                    Write_FIFO(InLenByte, pInData);
                    InLenByte = 0;
                }
                Set_BitMask(BitFramingReg, 0x80); //????????
                Write_Reg(ComIrqReg, 0x04);       //???LoAlertIrq
            }
            if (irq & 0x08) //HiAlertIRq
            {
                if ((irq & 0x40) && (InLenByte == 0) && (Read_Reg(FIFOLevelReg) > 32)) //TxIRq
                {
                    Read_FIFO(32, pOutData + rx_len); //????FIFO????
                    rx_len = rx_len + 32;
                    Write_Reg(ComIrqReg, 0x08); //???? HiAlertIRq
                }
            }
            if ((irq & 0x20) && (InLenByte == 0)) //RxIRq=1
            {
                result = OK;
                break;
            }
        }
    }

    //    if (Read_Reg(ErrorReg)&0x0F)
    //        {
    // 			result = ERROR;
    //        }
    //        else
    {
        if (Command == Transceive) {
            rx_temp = Read_Reg(FIFOLevelReg);
            Read_FIFO(rx_temp, pOutData + rx_len); //????FIFO????
            rx_len = rx_len + rx_temp;             //??????????

            lastBits = Read_Reg(ControlReg) & 0x07;

            if (lastBits) {
                *pOutLenBit = (rx_len - 1) * (unsigned int)8 + lastBits;
            } else {
                *pOutLenBit = rx_len * (unsigned int)8;
            }
        }
    }

    Set_BitMask(ControlReg, 0x80); // stop timer now
    Write_Reg(CommandReg, Idle);
    Clear_BitMask(BitFramingReg, 0x80); //??????

    return result;
}

/****************************************************************************************/
/*?????TypeA_Request																	*/
/*?????TypeA_Request??????															*/
/*????																				*/
/*?????pTagType[0],pTagType[1] =ATQA                                         		*/					
/*       	OK: ??????                                                              	*/
/*	 		ERROR: ???????																*/
/****************************************************************************************/
unsigned char TypeA_Request(unsigned char *pTagType)
{
	unsigned char result,send_buff[1],rece_buff[2];
	unsigned int rece_bitlen;  
	
	Set_BitMask(RxModeReg, 0x08);//???��????
	Clear_BitMask(Status2Reg,0x08);
	Write_Reg(BitFramingReg,0x07);
	Set_BitMask(TxControlReg,0x03);
	send_buff[0] = 0x52;
	result = Pcd_Comm(Transceive,send_buff,1,rece_buff,&rece_bitlen);
	if ((result == OK) && (rece_bitlen == 2*8))
	{    
		*pTagType     = rece_buff[0];
		*(pTagType+1) = rece_buff[1];
	}
 	return result;
}

/****************************************************************************************/
/*?????TypeA_Anticollision																*/
/*?????TypeA_Anticollision????????													*/
/*????selcode =0x93??0x95??0x97														*/
/*?????pSnr[0],pSnr[1],pSnr[2],pSnr[3]pSnr[4] =UID                            		*/					
/*       	OK: ??????                                                              	*/
/*	 		ERROR: ???????																*/
/****************************************************************************************/
unsigned char TypeA_Anticollision(unsigned char selcode,unsigned char *pSnr)
{
    unsigned char result,i,send_buff[2],rece_buff[5];
    unsigned int rece_bitlen;
	
    Clear_BitMask(Status2Reg,0x08);
    Write_Reg(BitFramingReg,0x00);
    Clear_BitMask(CollReg,0x80);
 
    send_buff[0] = selcode;
    send_buff[1] = 0x20;
	
    result = Pcd_Comm(Transceive,send_buff,2,rece_buff,&rece_bitlen);
    if (result == OK)
    {
    	 for (i=0; i<5; i++)
         	*(pSnr+i) = rece_buff[i];
         if (pSnr[4] != (pSnr[0]^pSnr[1]^pSnr[2]^pSnr[3]))
    		result = ERROR;    
    }
  	return result;
}

/****************************************************************************************/
/*?????TypeA_Halt													*/
/*?????TypeA_Halt?????												*/			
/*       	OK: ??????                                                              	*/
/*	 	ERROR: ???????												*/
/****************************************************************************************/
unsigned char TypeA_Halt(void)
{
    unsigned char result,send_buff[2],rece_buff[1];
    unsigned int rece_bitlen;
	
    send_buff[0] = 0x50;
    send_buff[1] = 0x00;
   
   	Write_Reg(BitFramingReg,0x00);
    Clear_BitMask(Status2Reg,0x08);
    result = Pcd_Comm(Transmit,send_buff,2,rece_buff,&rece_bitlen);
	
    return result;
}

unsigned char Set_Rf(unsigned char mode) 
{
    if ((Read_Reg(TxControlReg) & 0x03) == mode)
        return OK;

    if (mode == 0) {
        Clear_BitMask(TxControlReg, 0x03); //关闭TX1，TX2输出
    } else if (mode == 1) {
        Set_BitMask(TxControlReg, 0x01); //仅打开TX1输出
        Clear_BitMask(TxControlReg, 0x02);
    } else if (mode == 2) {
        Clear_BitMask(TxControlReg, 0x01);
        Set_BitMask(TxControlReg, 0x02); //仅打开TX2输出
    } else if (mode == 3) {
        Set_BitMask(TxControlReg, 0x03); //打开TX1，TX2输出
    }

   	usleep(5000);

    printf("Set_Rf mode:%d\n", mode);
    return 0;
}

unsigned char Pcd_ConfigISOType(unsigned char type) 
{
    if (type == 0) //ISO14443_A
    {
		Clear_BitMask(Status2Reg,0x08);
	
		Write_Reg(TReloadLSBReg,30);//tmoLength);// TReloadVal = 'h6a =tmoLength(dec) 
		Write_Reg(TReloadMSBReg,0);
		Write_Reg(TModeReg,0x8D);
		Write_Reg(TPrescalerReg,0x3E);
		
		Set_BitMask(ControlReg, 0x10); //ControlReg 0x0C ????reader??
		Set_BitMask(TxAutoReg, 0x40);  //TxASKReg 0x15 ????100%ASK??��
		Write_Reg(TxModeReg, 0x00);    //TxModeReg 0x12 ????TX CRC??��??TX FRAMING =TYPE A
		Write_Reg(RxModeReg, 0x00);    //RxModeReg 0x13 ????RX CRC??��??RX FRAMING =TYPE A

        //?????????????????????????
        //????????��?????????????FAE??????????????????��???????
		Write_Reg(RFCfgReg, 0x7f); // bit6-bit4 ????????
        //Write_Reg(GsNReg, 0xFF);     //GsNReg 0x27 ????ON?��
		//Write_Reg(CWGsPReg, 0x3f);
		//Write_Reg(RxThresholdReg, 0x84);//  bit7-bit4 minlevel   bit2-bit0 colllevel

    }

    return OK;
}


/****************************************************************************************/
/*?????TypeA_CardActivate																*/
/*?????TypeA_CardActivate???????														*/
/*????																				*/
/*??????	pTagType[0],pTagType[1] =ATQA 											 	*/
/*	       	pSnr[0],pSnr[1],pSnr[2],pSnr[3]pSnr[4] =UID 		                   		*/
/*	       	pSak[0],pSak[1],pSak[2] =SAK			                            		*/					
/*       	OK: ??????                                                              	*/
/*	 		ERROR: ???????																*/
/****************************************************************************************/
unsigned char TypeA_CardActivate(struct PICC_A_STR *type_a_card)
{
	unsigned char result;
	//printf("TypeA_CardActivate\n");
	result=TypeA_Request(type_a_card->ATQA);//??? Standard	 send request command Standard mode
	if (result==ERROR)
		return ERROR;
		
	if ((type_a_card->ATQA[0]&0xC0)==0x00)	//???UID
	{
		result=TypeA_Anticollision(0x93,type_a_card->UID);//1???????
		if (result==ERROR)
			return ERROR;
			
		type_a_card->UID_len=4;
	}
	
	return result;
}

void set_SerialSpeed(int baud)
{
	if( B115200 == baud ){
		// ???FM175XX UART ???????115200
		Write_Reg(SerialSpeedReg,0x7A); //  0x7A : B115200
		usleep(100000);
		uart_init(B115200);
		usleep(100000);
		printf("read SerialSpeedReg = 0x%x\r\n",Read_Reg(SerialSpeedReg));
	}
}

void read_card(void)
{
    struct PICC_A_STR PICC_A;
	
	while(1)
	{		
		memset(&PICC_A,0,sizeof(PICC_A));
		if( TypeA_CardActivate(&PICC_A) == OK ){
			printf("ReadCard: Type 0x%02x%02x, pSnr %02x %02x %02x %02x\n",
					  PICC_A.ATQA[0], PICC_A.ATQA[1],
					  PICC_A.UID[0], PICC_A.UID[1], PICC_A.UID[2], PICC_A.UID[3]);
		}else{
			printf("not found Mifare card\n");
		}
		TypeA_Halt();		
	}
}

unsigned int system_call(void)
{
 	    char result_buf[MAXLINE], command[MAXLINE];
 	    int rc = 0; // ??????????????
 	    FILE *fp;
 	 
 	    /*?????��?????��??buf*/
 	    snprintf(command, sizeof(command), "echo reset > /tmp/nfc_control");
 	 
 	    /*???????څ????????????????????????*/
 	    fp = popen(command, "r");
 	    if(NULL == fp)
 	    {
 	        perror("popen fail??\n");
 	        exit(1);
 	    }
 	    while(fgets(result_buf, sizeof(result_buf), fp) != NULL)
 	    {
 	        /*?????????????��?????????????��?????*/
 	        if('\n' == result_buf[strlen(result_buf)-1])
 	        {
 	            result_buf[strlen(result_buf)-1] = '\0';
 	        }
 	        printf("command [%s] output [%s] \r\n", command, result_buf);
 	    }
 	 
 	    /*????????????????????????????*/
 	    rc = pclose(fp);
 	    if(-1 == rc)
 	    {
 	        perror("close file fail!\n");
 	        exit(1);
 	    }
 	    else
 	    {
 	        printf("command [%s] status [%d] return [%d]\r\n", command, rc, WEXITSTATUS(rc));
 	    }
 	 
 	    return 0;
}

int uart_init(int baud)
{
	const char *dev ="/dev/ttyS0";
	struct termios  ios;

	printf("open %s\n", dev);

	if( uart_fd>0 ){
		close(uart_fd);
	}

	// O_NOCTTY	????????tty??????
	uart_fd = open( dev, O_RDWR | O_NONBLOCK | O_NOCTTY);//??????????????????  ????????-1
    if ( uart_fd < 0 ){
        printf("Can't Open Serial Port");
        return -1;
    }
    
    if(isatty(uart_fd) != 1)    {//??tty?��????1,?????0
        printf("It is not tty. \n");
        return -1;
    }
	
    tcgetattr( uart_fd, &ios );
    
    ios.c_lflag = 0;//??????,??????????
    ios.c_oflag &= (~ONLCR);//?????NL?????CR-NL??????
    ios.c_cflag &= ~CSTOPB; //??????��??��??????????????????????????????��???????????????????????��???????????��??��????????��??��???��???????

    cfmakeraw(&ios);//????????
	
    if ( cfsetispeed(&ios, baud) == -1 ){	//????????
        printf("Error: failed to set baud rate for %s\n",dev);
        return 0;
    }

    if ( cfsetospeed(&ios, baud) == -1 ){	//??????????
        printf("Error: failed to set baud rate for %s\n",dev);
        return 0;
    }
	
	tcflush(uart_fd, TCIFLUSH);    //??????????
	tcflush(uart_fd, TCOFLUSH);    //??????????
	tcflush(uart_fd, TCIOFLUSH);   //??????????????
    tcsetattr(uart_fd, TCSANOW, &ios);

	return uart_fd;
}

/************************************************************************************************/

static int read_string_from_file(char *str, int str_len, const char *file_name)
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

static int write_string_to_file(const char *str, const char *file_name)
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
static int gpio_set_direction(int gpio, int output)
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
void gm_gpio_set_value(unsigned pin, int val)
{
	char str[10], file_name[50];
	int len, ret;

	len = sprintf(file_name, "/sys/class/gpio/gpio%d", pin);
	file_name[len] = 0;

	if( (access(file_name, F_OK )) == -1 ){
		len = sprintf(str, "%d", pin);
		str[len] = 0;

		ret = write_string_to_file(str, "/sys/class/gpio/export");
		if(ret)
			return ;
	}
	
	len = sprintf(file_name, "/sys/class/gpio/gpio%d/direction", pin);
	file_name[len] = 0;

	memset(str, 0, sizeof(str));
	ret = read_string_from_file(str, sizeof(str), file_name);
	if(ret){
		printf("set gpio%d output %s level failed\n", pin, val?"high":"low");
		return ;
	}
	
	if(!strncmp(str, "in", 2)){
		ret = write_string_to_file("out", file_name);
		if(ret)
			return ;
	}

	len = sprintf(file_name, "/sys/class/gpio/gpio%d/value", pin);
	file_name[len] = 0;

	memset(str, 0, sizeof(str));
	ret = read_string_from_file(str, sizeof(str), file_name);
	if(ret)
		return ;

	if(val){
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
		return ;
}

/************************************************************************************************/
int NFC()
{
    static char m,n;
    int i,j;
    int num;
    int mdelay = 1000; //ms
    int ret = -1;

	printf("nfc test program v0.3.3\n");
	
	gpio_set_direction(33, 1);
	gm_gpio_set_value(33, 0);
	usleep(2 * 1000);
	gm_gpio_set_value(33, 1);

	if(uart_init(B9600) < 0){
		printf("uart_init error\n");
		return -1;
	}

	//#define MODE0_SIZE	821
	//unsigned char _mode0[MODE0_SIZE] = {......}
	int datasent = 0;
	int size_per = 0;
	/*while(1) {
		size_per = write(fd, _mode0+datasent,MODE0_SIZE - datasent);
		printf("write: %d \n",size_per);
		datasent += size_per;
		if(  datasent == MODE0_SIZE ){
			printf("EchoSuppressorUart - Write OK!!\n");
			break;
		}
	}*/
	//system("echo reset > /sys/power/usb_switch_control");
	
	tcflush(uart_fd, TCIFLUSH);    //??????????
	tcflush(uart_fd, TCOFLUSH);    //??????????
	tcflush(uart_fd, TCIOFLUSH);   //??????????????	

	system_call();
	set_SerialSpeed(B115200);
	
	printf("TxControlReg : 0x%x\n",Read_Reg(TxControlReg));
	Pcd_ConfigISOType(0);	
	Set_Rf(3);  
	printf("TxControlReg : 0x%x\n",Read_Reg(TxControlReg));	
	
	read_card();
	//usleep(2000*1000);
	
	/*
	unsigned char recvbuf[MODE0_SIZE] = {0};
	int dataread = 0;
	int size_perread = 0;
	while(1) {
		errno = 0;
		size_perread = read(fd, recvbuf+dataread, MODE0_SIZE - dataread);
		printf("size_perread:%d  errno:%d\n",size_perread,errno);
		if( size_perread > 0 ){
			dataread += size_perread;
		}else{
			goto exit0;
		}
		if(  dataread == MODE0_SIZE ){
			printf("Read OK!!\n");
			break;
		}
		usleep(800*1000);
	}

	if(strncmp( _mode0 , recvbuf, MODE0_SIZE ) == 0 ){
		printf("==\n");
	}
	*/

	
	
	
exit0:

    if( uart_fd >= 0){
        close(uart_fd);
        uart_fd = -1;
    }
    return 0;
}

