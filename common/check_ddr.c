/****************************************************************************************
 *  File:   check_ddr.c
 *  Date:   2011-12-05
 *  Description:
 *				The purpose of this file is to test SDRAM address and data line integrity
 				1.Add sdram_phy_addr_diag (to confirm the address wire is OK)
 				2.Add sdram_pattern_diag	(to confirm the data address wire is OK)
 *     			3.check_file_WR (to confirm write and read data OK) 
 *---------------------------------------------------------------------------------------
 *  History:
 *  	2008.2.26  roy/evan    			V0.1
 *	    2011.12.05  Viola    			V0.2
*****************************************************************************************/
#include <common.h>
#include <command.h>
#include <mmc.h>
#include <linux/types.h>
#include <linux/err.h>

#define CRC32_RESIDUE 0xdebb20e3UL
/*****************************************************************************************
	* Routine:	sdram_phy_addr_diag( unsigned long base, unsigned long size )
 	* Purpose:	Test SDRAM address and data line integrity
 	* Parameters:	base  physical base address of SDRAM
 	*				size  size in bytes of SDRAM

 	* Write the physical address at each of the following memory locations to assure
 	*	there aren¡¦t any mirroring issues.  
 	*	The test needs to do a complete write pass through SDRAM before reading 
 	*	back to verify.
 	*
 	*	The memory locations to write to :
 	*	0, 4, 8, 16, 32, 64, 128, 256, 512, 1024, and then every 1k through remainder of SDRAM.
	* Returns:	integer value indicating test result, which will be one of the following:
 	*		0  Pass
 	*		1  Failed mirror test
 	* Pass Criteria:	
 	*			1) 	Each address read back matched the physical address from which 
 	*											it was read.
 	* Assumptions: 
 	* Side Effects:	
********************************************************************************************/
static int sdram_phy_addr_diag (int base,int size)
{
	volatile unsigned int	addr,check_addr;
	int	i,end,offset;
	end=base+size;
		
	for(check_addr=base,i=0;check_addr<end;i++,check_addr+=1024) 
	{
		//printf("Address 0x%x\n",check_addr);
		if(0==i%(2*1024))
		printf("Address 0x%x 's content is 0x%x\n",check_addr,*(unsigned int *)check_addr);
		
		addr=(check_addr);//0 offset 1k in size
		*(unsigned int *)addr = addr;
		for (offset=4; offset<=1024;offset <<=1) {  //4-1024 offset in 1k size
			addr=((check_addr+offset));				
			*(unsigned int *)addr = addr;
		}
	}		

	for(i=0,check_addr=base;check_addr<end;i++,check_addr+=1024)
	{
		if(0==i%(2*1024))
		printf("Check address 0x%x 's content is 0x%x\n",check_addr,*(unsigned int *)check_addr);
		
		addr=(check_addr);//0 offset 1k in size
		if(*(unsigned int *)addr != addr)
		{
			printf("fail at address 0x%x 's content is 0x%x\n",check_addr,*(unsigned int *)check_addr);
			return 1;
		}
		for (offset=4; offset<=1024;offset <<=1) {  //4-1024 offset in 1k size
				addr=(check_addr+offset);			
				if(*(unsigned int *)addr != addr)
				{
					printf("fail at address 0x%x 's content is 0x%x\n",check_addr,*(unsigned int *)check_addr);
				  return 1;
				}
		}
	}	
	return 0;
}
/*****************************************************************************************
	* Routine:	sdram_pattern_diag (int base,int size)
 	* Purpose:	Test SDRAM address and data line integrity
 	* Parameters:	base  physical base address of SDRAM
 	*							size  size in bytes of SDRAM
 	* Details:	1)  Verify address and data lines by writing the following test data pattern 
 	*	throughout SDRAM:
 	*
 	*		0xAAAAAAAA 			// address line can be driven high and low
 	*		0x55555555 			// address line can be driven high and low
 	*		0x00000000			// contiguous bits clear
 	*		0xFFFFFFFF			// contiguous bits set
 	*
 	*	The memory locations to write to :
 	*	0, 4, 8, 16, 32, 64, 128, 256, 512, 1024, and then every 1k through remainder of SDRAM.
	* Returns:	integer value indicating test result, which will be one of the following:
 	*		0  Pass
 	*		1  Failed 0xAAAAAAAA pattern test
 	*		2  Failed 0x55555555 pattern test
 	*		3  Failed 0x0 pattern test
 	*		4  Failed 0xFFFFFFFF pattern test
 	*		5  Failed mirror test
 	* Pass Criteria:	No bits were dropped in the bit pattern
 	* Assumptions: 	
 	* Side Effects:	
********************************************************************************************/
static int sdram_pattern_diag (int base,int size)
{
	volatile unsigned int	*addr;
	volatile unsigned int	offset,end,check_addr;
	volatile unsigned int	val;
	unsigned int	readback;
	unsigned int rcode;
	int	i;
	
	static const unsigned int bitpattern[] = {
		0xAAAAAAAA,
		0x55555555,
		0x00000000,
		0xFFFFFFFF
	};
	
	if(!(base&0x70000000))
		{base=0x74100000;size=(0x78000000-0x74100000);}
	printf("check SDRAM from 0x%x to 0x%x\n",base,base+size);

	end =base+size;
	rcode=0;

	for (i=0;i<4;i++) {
		val=bitpattern[i];
		printf ("\rPattern %X  Writing...",val);

		for(check_addr=base;check_addr<end;check_addr +=1024){
		
			if(0==check_addr%(4*1024*1024))
				printf("\rwritting @0x%8x with 0x%8x",check_addr,val);

			addr=(unsigned int *)(check_addr);//0 offset 1k in size
			*addr = val;
			for (offset=4; offset<=1024 && addr<end;offset <<=1) {  //4-1024 offset in 1k size
				addr=(unsigned int *)(check_addr+offset);				
				*addr = val;
			}
		}

		printf ("Reading...\n");

		for(check_addr=base;check_addr<end;check_addr +=1024){

			if(0==check_addr%(4*1024*1024))
				printf("\rreading @0x%8x with 0x%8x",check_addr,val);
			
			addr=(unsigned int *)(check_addr);//0 offset 1k in size
			readback=(unsigned int)*addr;
			printf("readback %x\n",readback);
			if (readback != val) {
				rcode =i+1;
			}
			for (offset=4; offset<=1024 && addr<end;offset <<=1) {  //4-1024 offset in 1k size
				addr=(unsigned int *)(check_addr+offset);
				readback=*addr;
				if (readback != val) {
					rcode =i+1;
					}
				}
			}		
		}
		putc('\n');
//		printf("rcode%d*****\n",rcode);
	return rcode;
}
static check_file_crc(unsigned int base, unsigned int size)
{
	volatile unsigned int check_addr = 0x70300000;
	unsigned int blk_start, blk_cnt, n;
	int result = 0;
	int mmc_dev = get_mmc_env_devno();
	struct mmc *mmc = find_mmc_device(mmc_dev);

	if (!mmc) {
			printf("MMC Device %d not found\n",	mmc_dev);
			return -1;
	}

	if (mmc_init(mmc)) {
			puts("MMC init failed\n");
			return -1;
	}

	memset(check_addr, 0, size);
	blk_start = ALIGN(base, mmc->read_bl_len) / mmc->read_bl_len;
	blk_cnt   = ALIGN(size, mmc->read_bl_len) / mmc->read_bl_len;
	n = mmc->block_dev.block_read(mmc_dev, blk_start,
								blk_cnt, (u_char *)check_addr);
	flush_cache((volatile unsigned int)check_addr, blk_cnt * mmc->read_bl_len);

	*((unsigned long *)(check_addr+size-4)) = crc32(0L, (unsigned char *)check_addr, size - sizeof(unsigned long));
	unsigned long computed_re = ~crc32(0L,(unsigned char *)check_addr,size);
	result=CRC32_RESIDUE==computed_re;
	if(result)
		printf("file write and read check passed\n");
	else
		printf("file write and read check failed\n");
	return result;
}
static int check_file_WR(cmd_tbl_t *cmdtp, int flag, int argc,char *argv[])
{
	unsigned int base, size;
	if(argc!=2){
		cmd_usage(cmdtp);
		return 1;
	}
	int cmp_value = strcmp("uboot",argv[1]);
	if(!cmp_value){
		base = ANDROID_BOOTLOADER_OFFSET;
		size = ANDROID_BOOTLOADER_SIZE;
	}
	else
		if(!strcmp("kernel",argv[1])){
			base = ANDROID_KERNEL_OFFSET;
			size = ANDROID_KERNEL_SIZE;
		}
		else
			if(!strcmp("logo1",argv[1])){
				base = ANDROID_LOGO_OFFSET;
				size = ANDROID_LOGO_SIZE;
			}
			else
				if(!strcmp("logo2",argv[1])){
					base = ANDROID_LOGO2_OFFSET;
					size = ANDROID_LOGO2_SIZE;
				}
				else
					if(!strcmp("waveform",argv[1])){
						base = ANDROID_WAVEFORM_OFFSET;
						size = ANDROID_WAVEFORM_SIZE;
					}
					else{
						printf("no avialible file to check for DDR\n");
						return 1;
					}
	check_file_crc(base,size);
	return 0;

}

static int sdram_diag (cmd_tbl_t *cmdtp, int flag, int argc,int *argv[])
{
	int base,size;
	int ret;	
	if(argc != 3){
		cmd_usage(cmdtp);
		return 1;
	}

	base = simple_strtoul(argv[1], NULL, 16);
	size = simple_strtoul(argv[2], NULL, 16);
	printf("base%08x\tsize%08x\n",base,size);

	ret = sdram_pattern_diag(base,size);
	if(!ret)
		printf("pattern test OK\n");
	else
		switch(ret){
			case 1: 
				printf("pattern AAAAAAAA test FAILED\n");
				return ret;
			case 2:
				printf("pattern 55555555 test FAILED\n");
				return ret;
			case 3:
				printf("pattern 00000000 test FAILED\n");
				return ret;
			case 4:
				printf("pattern FFFFFFFF test FAILED\n");
				return ret;
		}

	if(!sdram_phy_addr_diag(base,size))
		printf("addr test OK\n");
	else{
		printf("addr test FAILED\n");
		return 1;
	}
	return 0;	
}

U_BOOT_CMD(
		test_ddr, 3, 0, sdram_diag,
		"test_ddr test_addr_base test_size",
		"to test the ddr zone"
		);

U_BOOT_CMD(
		check_file_crc, 2, 0, check_file_WR,
		"check_file_crc filename",
		"filename could be uboot/kernel/logo1/logo2/waveform"
		);

