#include <config.h>
#include <fp9928_pmic.h>
#include <common.h>
#include <command.h>
#include <asm/arch/mx50.h>
#include <asm/io.h>

struct fp9928_vcom_programming_data {
	int vcom_min_uV;
	int vcom_max_uV;
	int vcom_step_uV;
};

static struct fp9928_vcom_programming_data vcom_data = {
	-2500000,
	-1000000,
	10780,
};
char convert_uV_to_vcom(int uV)
{
	char vcom;
	vcom = 63 + 28 + (((((vcom_data.vcom_max_uV - uV) * 10) 
					/ vcom_data.vcom_step_uV) + 5) / 10);
	//printf("============vcom %0x\n",vcom);
	return vcom;
}
#if 1 /*simulate i2c via gpio*/
void enable_fp9928_poweren(void)
{
	unsigned int reg;
	/* Enable EINK_PW_EN*/
	reg = readl(GPIO3_BASE_ADDR + 0x0);
	reg |= (1<<28);
	writel(reg, GPIO3_BASE_ADDR + 0x0);
}

void disable_fp9928_poweren(void)
{
	unsigned int reg;
	/* Disable EINK_PW_EN*/
	reg = readl(GPIO3_BASE_ADDR + 0x0);
	reg &= ~(1 << 28);
	writel(reg, GPIO3_BASE_ADDR + 0x0);
}

void Delay(void)
{
	unsigned long i;
	for(i=0;i<DELAY;i++);
}

void SCLH_SDAH()
{
	IIC_ESCL_Hi;
	IIC_ESDA_Hi;
	Delay();
}

void SCLH_SDAL()
{
	IIC_ESCL_Hi;
	IIC_ESDA_Lo;
	Delay();
}

void SCLL_SDAH()
{
	IIC_ESCL_Lo;
	IIC_ESDA_Hi;
	Delay();
}

void SCLL_SDAL()
{
	IIC_ESCL_Lo;
	IIC_ESDA_Lo;
	Delay();
}

void IIC_ELow()
{
	SCLL_SDAL();
	SCLH_SDAL();
	SCLH_SDAL();
	SCLL_SDAL();
}

void IIC_EHigh()
{
	SCLL_SDAH();
	SCLH_SDAH();
	SCLH_SDAH();
	SCLL_SDAH();
}

void IIC_EStart()
{
	SCLH_SDAH();
	SCLH_SDAL();
	Delay();
	SCLL_SDAL();
}

void IIC_EEnd()
{
	SCLL_SDAL();
	SCLH_SDAL();
	Delay();
	SCLH_SDAH();
}

static u8  IIC_EAck()
{

	unsigned long ack;

	IIC_ESDA_INP;			// Function <- Input
	IIC_ESCL_Lo;
	Delay();
	IIC_ESCL_Hi;
	Delay();
	//ack = GPD1DAT;
	ack = GPIO6_DR;
	IIC_ESCL_Hi;
	Delay();
	IIC_ESCL_Hi;
	Delay();
	IIC_ESDA_OUTP;			// Function <- Output (SDA)

	ack = (ack>>19)&0x1;
	if(ack!=0)
	{
	printf("Cann't got ACK single.............\n");
	SCLL_SDAL();
	return 0;
	}
	SCLL_SDAL();
	return 1;
}


void IIC0_ESetport(void)
{
	PAD_I2C1_SCL =0x1;
	PAD_I2C1_SDA = 0x1;
	PULL_PAD_I2C1_SCL &= ~(0x1<<7);// Pull Up/Down Disable	SCL, SDA
	PULL_PAD_I2C1_SDA &= ~(0x1<<7);
	IIC_ESCL_Hi;
	IIC_ESDA_Hi;
	IIC_ESCL_OUTP;		// Function <- Output (SCL)
	IIC_ESDA_OUTP;		// Function <- Output (SDA)
	Delay();
}

void IIC_EWrite (unsigned char ChipId, unsigned char IicAddr, unsigned char IicData)
{
	unsigned long i;
	IIC_EStart();

////////////////// write chip id //////////////////
	for(i = 7; i>0; i--)
	{
		if((ChipId >> (i-1)) & 0x0001)
			IIC_EHigh();
		else
			IIC_ELow();
	}

	IIC_ELow();	// write 'W'
	IIC_EAck();	// ACK

////////////////// write reg. addr. //////////////////
	for(i = 8; i>0; i--)
	{
		if((IicAddr >> (i-1)) & 0x0001)
			IIC_EHigh();
		else
			IIC_ELow();
	}

	IIC_EAck();	// ACK

////////////////// write reg. data. //////////////////
	for(i = 8; i>0; i--)
	{
		if((IicData >> (i-1)) & 0x0001)
			IIC_EHigh();
		else
			IIC_ELow();
	}

	IIC_EAck();	// ACK
	IIC_EEnd();
}

 static int get_SDA()
 {
	 return (GPIO6_DR>>19)&1; 
 }
 

static char IIC_ERead(unsigned char ChipId, unsigned char IicAddr)
{
	unsigned long i;
	char temp = 0;

	IIC_EStart();

////////////////// write chip id //////////////////
	for(i = 7; i>0; i--)
	{
		if((ChipId >> (i-1)) & 0x0001)
			IIC_EHigh();
		else
			IIC_ELow();
	}
	IIC_ELow();	// write 'W'
	if(IIC_EAck() == 0)	// ACK
	{
		return -1;
	}

	////////////////// write reg. addr. //////////////////
	for(i = 8; i>0; i--)
	{
		if((IicAddr >> (i-1)) & 0x0001)
			IIC_EHigh();
		else
			IIC_ELow();
	}
	if(IIC_EAck() == 0)	// ACK
	{
		return -1;
	}

	IIC_EStart();//Sending repeat S bit

////////////////// write chip id //////////////////
	for(i = 7; i>0; i--)
	{
		if((ChipId >> (i-1)) & 0x0001)
			IIC_EHigh();
		else
			IIC_ELow();
	}
    	IIC_EHigh();	// write 'R'
	if(IIC_EAck() == 0)	// ACK
	{
		return -1;
	}

    ////////////////// read reg. data. //////////////////
    
    IIC_ESDA_INP;			// Function <- Input
    
    for(i = 0;i < 8; i++)
	{
		IIC_ESCL_Lo;
		Delay();
		Delay();
		IIC_ESCL_Hi;
		Delay();
		Delay();
	        temp <<= 1;
		if(get_SDA() == 1)
		{
			temp += 1;
		}
		else
		{
		
		}
		IIC_ESCL_Hi;
	    Delay();
		Delay();
		IIC_ESCL_Lo;
	    Delay();
		Delay();
    }
	IIC_ESDA_OUTP;		// Function <- Output (SDA)
	IIC_EEnd();
	return temp;
}


void IIC_EWritew(unsigned char ChipId, u16 IicAddr, u16 IicData)
{
	unsigned long i;
	IIC_EStart();
////////////////// write chip id //////////////////
	for(i = 7; i>0; i--)
	{
		if((ChipId >> (i-1)) & 0x0001)
			IIC_EHigh();
		else
			IIC_ELow();
	}

	IIC_ELow();	// write 'W'
	IIC_EAck();	// ACK

////////////////// write reg. addr. //////////////////
	for(i = 16; i>0; i--)
	{
		if((IicAddr >> (i-1)) & 0x0001)
			IIC_EHigh();
		else
			IIC_ELow();
		if (i == 9)
			IIC_EAck(); // ACK
	}

	IIC_EAck();	// ACK

////////////////// write reg. data. //////////////////
	for(i = 16; i>0; i--)
	{
		if((IicData >> (i-1)) & 0x0001)
			IIC_EHigh();
		else
			IIC_ELow();
		if (i == 9)
			IIC_EAck(); // ACK
	}

	IIC_EAck();	// ACK
	IIC_EEnd();
}

void fp9928_write(unsigned char ChipId, u16 IicAddr, u16 IicData)
{
	enable_fp9928_poweren();
	IIC0_ESetport();
	mdelay(20);
	IIC_EWrite(ChipId, IicAddr, IicData);
//	disable_fp9928_poweren();
}

char fp9928_read(unsigned char ChipId, u16 IicAddr)
{
	char tmp;
	enable_fp9928_poweren();
	IIC0_ESetport();
	mdelay(20);
	tmp = IIC_ERead(ChipId,  IicAddr);
//	printf("content of the register[0x%.2x]:0x%x\n", IicAddr, tmp);
//	disable_fp9928_poweren();
	return tmp;
}

char read_temperature(void);
void write_vcom(int uV_value);
int setup_vcom(void);

static int do_dump_fp9928_test(cmd_tbl_t * cmdtp, int flag, int argc, char *argv[])
{
	int temp;
	int item;

	if(argc != 3){
		cmd_usage(cmdtp);
		return 1;
	}

	item = simple_strtol(argv[2], NULL, 0);
	printf("write or read: %c   data: 0x%x\n", *argv[1], item);

	if(*argv[1] == 'r'){
		switch(item){
			case 2:
			 	temp = fp9928_read(0x48,0x02);
			 	break;
			case 1:
			 	temp = fp9928_read(0x48,0x01);
			 	break;
			case 0:
			 	temp = read_temperature();
				printf("^-^^-^%d\n",temp);
			 	break;
			default:
	 			printf("invalid num about reg\n");
				break;
		}
	}
	else if(*argv[1] == 'w'){
		setup_vcom();
//		write_vcom(item);
//		fp9928_write(0x48, 0x02, item);
	}
	return 0;
}

static void do_test_vcom_seting(void)
{
	unsigned int reg;

	reg = readl(GPIO3_BASE_ADDR + 0x0);
	reg |= (1<<28);
	writel(reg, GPIO3_BASE_ADDR + 0x0);

	reg = readl(GPIO3_BASE_ADDR + 0x0);
	reg |= (1 << 27);	//set bit 27 and 28 
	writel(reg, GPIO3_BASE_ADDR + 0x0);
	udelay(500);
}

char read_temperature(void)
{
	char temper_val;
	temper_val = fp9928_read(0x48, 0x00);
	return temper_val;
}


void write_vcom(int uV_value)
{
	char vcom_value;

	vcom_value = convert_uV_to_vcom(uV_value);
	fp9928_write(0x48, 0x02, vcom_value);
}

int setup_vcom(void)
{
	char *buffer;
	int uV_value,def;
	
	//printf("hello\n");
	run_command("mmc read 2 0x70600000 0x2C02 1", 0);

	buffer = (char *)malloc(8);
	if(NULL == buffer)
		return -1;
	memset(buffer, 0x0, 8);

	memcpy(buffer, 0x70600000, 8);
	if(buffer != NULL) {
		*(buffer+8) = '\0';
		def = simple_strtol(buffer, NULL, 0);
	}

	//printf("original def is %d\n",def);

	if(def > -1000000 || def < -2500000) {
		printf("Vcom value not find,use default value: -1250mv\n");
		def = -1250000;
	}
	printf("%s set vcom %duV\n", __func__, def);

	write_vcom(def);
	//printf("Over!\n");
}
#endif
char do_read_temperature_via_i2c()
{
	char temperature;
	i2c_init (CONFIG_SYS_I2C_SPEED, CONFIG_SYS_I2C_SLAVE);
	i2c_read(0x48, 0x0, 1, &temperature, 1);
	//printf("read temperature via i2c, value is 0x%0x\n", temperature);
	return temperature;
}
void setup_vcom_via_i2c()
{
	char *buffer, vcom_value;
	int def;
									
	run_command("mmc read 2 0x70600000 0x2C02 1", 0);
	
	buffer = (char *)malloc(8);
	if(NULL == buffer)
			return -1;
	memset(buffer, 0x0, 8);

	memcpy(buffer, 0x70600000, 8);
	if(buffer != NULL) {
			*(buffer+8) = '\0';
			def = simple_strtol(buffer, NULL, 0);
	}
	//printf("original def is %d\n",def);

	if(def > -1000000 || def < -2500000) {
			printf("Vcom value not find,use default value: -1250mv\n");
			def = -1250000;
	}
	vcom_value = convert_uV_to_vcom(def);
	//printf("%s set vcom value is %0x(%duV)\n", __func__, vcom_value, def);
	i2c_init(CONFIG_SYS_I2C_SPEED, CONFIG_SYS_I2C_SLAVE);
	i2c_write(0x48, 0x2, 1, &vcom_value, 1);
}
/* ----------------------- U-BOOT COMMAND DEFINITION------------------------ */
#if 0
U_BOOT_CMD(
		dump_fp9928_reg, 3, 0, do_dump_fp9928_test,
		"Usage: dump_fp9928_reg r/w num \n ",
		"if read num should be 0/1/2, represent TMST_VALUE,FUNC_ADJUST,VCOM_SETTING register\nelse write data to vcom_setting register"
		);

U_BOOT_CMD(
		test_vcom_settings, 1, 0, do_test_vcom_seting,
		"Usage: test_vcom_settings",
		"NULL"
		);
#endif
U_BOOT_CMD(
		read_temperature_via_i2c, 1, 0, do_read_temperature_via_i2c,
		"Usage: read_temperature_via_i2c",
		"NULL"
		);
