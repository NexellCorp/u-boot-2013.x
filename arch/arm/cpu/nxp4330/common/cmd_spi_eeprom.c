/*
 * (C) Copyright 2000-2003
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/*
 * Misc boot support
 */
#include <common.h>
#include <command.h>
#include <malloc.h>
#include <spi.h>
#include "secondboot.h"

extern int  eeprom_write (unsigned dev_addr, unsigned offset,
               uchar *buffer, unsigned cnt);

extern ssize_t spi_write (uchar *addr, int alen, uchar *buffer, int len);

#define POLY 0x04C11DB7L 

struct BOOTINFO {
	int				loadsize;
	unsigned int	loadaddr;
	unsigned int	jumpaddr;
};


unsigned int get_fcs(unsigned int fcs, unsigned char data) 
{ 
	register int i;
	fcs ^= (unsigned int)data; 
	for(i=0; i<8; i++) 
	{ 
	 	if(fcs & 0x01) fcs ^= POLY; fcs >>= 1;
 	} 
	return fcs; 
}

int parse_nsih(char *addr, int size)
{
	char ch;
	int writesize, skipline = 0, line, bytesize, i;
	unsigned int writeval;

	struct BOOTINFO *pinfo = NULL;
	char *base = addr;
	char  buffer[512] = { 0, };

	bytesize  = 0;
	writeval  = 0;
	writesize = 0;
	skipline  = 0;
	line = 0;

	while (1) {

		ch = *addr++;
		if (0 >= size)
			break;

		if (skipline == 0) {
			if (ch >= '0' && ch <= '9') {
				writeval  = writeval * 16 + ch - '0';
				writesize += 4;
			} else if (ch >= 'a' && ch <= 'f') {
				writeval  = writeval * 16 + ch - 'a' + 10;
				writesize += 4;
			} else if (ch >= 'A' && ch <= 'F') {
				writeval  = writeval * 16 + ch - 'A' + 10;
				writesize += 4;
			} else {
				if (writesize == 8 || writesize == 16 || writesize == 32) {
					for (i=0 ; i<writesize/8 ; i++) {
						buffer[bytesize] = (unsigned char)(writeval & 0xFF);
						bytesize++;
						writeval >>= 8;
					}
				} else {
					if (writesize != 0)
						printf("parse nsih : Error at %d line.\n", line+1);
				}

				writesize = 0;
				skipline = 1;
			}
		}

		if (ch == '\n') {
			line++;
			skipline = 0;
			writeval = 0;
		}

		size--;
	}

	pinfo = (struct BOOTINFO *)&buffer[0x44];

	pinfo->loadsize	= (int)CONFIG_UBOOT_SIZE;
	pinfo->loadaddr	= (U32)_TEXT_BASE;
	pinfo->jumpaddr = (U32)_TEXT_BASE;

	memcpy(base, buffer, sizeof(buffer));

	printf(" parse nsih : %d line processed\n", line+1);
	printf(" parse nsih : %d bytes generated.\n\n", bytesize);

	return bytesize;
}

int do_update(cmd_tbl_t * cmdtp, int flag, int argc, char * const argv[])
{
	char *cmd = argv[1];
	unsigned int addr = 0, offset = 0;
	U8 offs[3] = { 0, };
	int size = 0, maxsize = 0;

	int s_size = 0, s_addr =0;
	int i=0;
	unsigned int crc = 0;
	if (0 != strcmp(cmd, "2ndboot") &&
		0 != strcmp(cmd, "nsih")    &&
		0 != strcmp(cmd, "uboot")	&&
		0 != strcmp(cmd, "2nd") &&
		0 != strcmp(cmd, "clean") )
		goto usage;

	/*
	 * update eeprom data
	 */
	if (0 != strcmp(cmd, "clean")) {

		if (4 > argc)
			goto usage;

		addr = simple_strtoul(argv[2], NULL, 16);
		size = simple_strtoul(argv[3], NULL, 16);

		s_addr = simple_strtoul(argv[4], NULL, 16);
		s_size = simple_strtoul(argv[5], NULL, 16);

		if (strcmp(cmd, "2ndboot") == 0) {

		U8 *buf = (uchar *)malloc(CONFIG_2STBOOT_SIZE );


		offset  = CONFIG_NSIH_OFFSET;
		maxsize = CONFIG_NSIH_SIZE;
		
		size    = parse_nsih((char*)addr, size);
			printf(" eeprom update nsih 0x%08x, mem 0x%08x, size %d \n", offset, addr, size);
			if (512 != size) {
				printf(" fail nsih parse, invalid nsih headers ...\n");
			return 1;
		}

		memcpy((uchar *)buf,(uchar *)addr,CONFIG_NSIH_SIZE);
		memcpy(buf+CONFIG_NSIH_SIZE, (uchar *)s_addr, s_size);
	
		for (i = 0; i<CONFIG_2STBOOT_SIZE-16; i++ )
			crc = get_fcs(crc,  buf[i]);

		memcpy(buf + CONFIG_2STBOOT_SIZE -16, &crc,4);

		offs[0] = 0;
		offs[1] = 0;
		offs[2] = 0;

		spi_init_f();


		spi_write(offs, 3, (uchar*)buf, CONFIG_2STBOOT_SIZE);
		free(buf);		
		printf(" eeprom update 2nd boot 0x%08x, mem 0x%08x, size %d\n", offset, addr, CONFIG_2STBOOT_SIZE);
		}	
		
		else if (strcmp(cmd, "uboot") == 0) {

		struct NX_SecondBootInfo *bootheader = (struct NX_SecondBootInfo *)malloc(sizeof(struct NX_SecondBootInfo));

		memset(bootheader, 0xff,512);

		maxsize = CONFIG_UBOOT_SIZE;
		offset  = CONFIG_UBOOT_OFFSET;

		spi_init_f();

		U8 *buf = (uchar *)malloc(CONFIG_UBOOT_SIZE);
		
		memcpy(buf, (uchar *)addr, size);
		crc=0;
		for (i = 0; i<size; i++ )
			crc = get_fcs(crc , buf[i]);

		bootheader->DBI.SPIBI.CRC32	= crc;
		bootheader->LOADSIZE	= (int)size; 
		bootheader->LOADADDR	= (U32)_TEXT_BASE;
		bootheader->LAUNCHADDR	= (U32)_TEXT_BASE;
		bootheader->SIGNATURE	= HEADER_ID;

		memcpy(buf, (uchar *)bootheader, 512);
		memcpy(buf+512, (uchar *)addr, size);

		offset = CONFIG_UBOOT_OFFSET;

		offs[0] = (offset >>  16);
		offs[1] = (offset >>   8);
		offs[2] = (offset & 0xFF);
		
		spi_write(offs, 3, (uchar*)buf, CONFIG_UBOOT_SIZE );

		free(buf);
		free(bootheader);

		printf(" eeprom update u-boot 0x%08x, mem 0x%08x, size %d\n", offset, addr, size);
		return 0;
		}

		else if (strcmp(cmd, "2nd") == 0) {
		maxsize = CONFIG_2STBOOT_SIZE;
		offset  = CONFIG_NSIH_OFFSET;
		struct BOOTINFO *pinfo = NULL;

		U8 *buf = (uchar *)malloc(CONFIG_2STBOOT_SIZE );

		memset(buf, 0xff,CONFIG_2STBOOT_SIZE);

		memcpy(buf, (uchar *)addr, size);
		pinfo = (struct BOOTINFO *)&buf[0x44];

		pinfo->loadsize	= (int)CONFIG_UBOOT_SIZE;
		pinfo->loadaddr	= (U32)_TEXT_BASE;
		pinfo->jumpaddr = (U32)_TEXT_BASE;
		
		for (i = 0; i<CONFIG_2STBOOT_SIZE-16; i++ )
			crc = get_fcs(crc ,  buf[i]);

		memcpy(buf + CONFIG_2STBOOT_SIZE-16 , &crc, 4);

		offset = CONFIG_NSIH_OFFSET;

		offs[0] = (offset >>  16);
		offs[1] = (offset >>   8);
		offs[2] = (offset & 0xFF);

		spi_init_f();
		
		spi_write(offs, 3, (uchar*)buf, CONFIG_2STBOOT_SIZE);

		free(buf);

			printf(" eeprom update u-boot 0x%08x, mem 0x%08x, size %d\n", offset, addr, size);
		}  else {
			cmd_usage(cmdtp);
			return 1;
		}

		if (size > maxsize) {
			printf(" error input size %d is over the max part size %d\n", size, maxsize);
			goto usage;
		}

		
	/*
	 * clean eeprom data
	 */
	}

	
#if defined(CONFIG_SPI) && defined(CONFIG_ENV_IS_IN_EEPROM)
	else {
		if (3 > argc)
			goto usage;

		cmd = argv[2];

		if (strcmp(cmd, "env") == 0) {
			char *buf = malloc(CONFIG_ENV_SIZE);

			offset = CONFIG_ENV_OFFSET;
			size   = CONFIG_ENV_SIZE;

			/* clear buffer */
			memset((void*)buf, 0, CONFIG_ENV_SIZE);

			offs[0] = (offset >>  16);
			offs[1] = (offset >>   8);
			offs[2] = (offset & 0xFF);

			spi_write(offs, 3 , (uchar*)buf, size);
			free(buf);
		} else {
			cmd_usage(cmdtp);
			return 1;
		}
	}
#endif
	return 0;

usage:
	cmd_usage(cmdtp);
	return 1;
}



U_BOOT_CMD(
	update, CONFIG_SYS_MAXARGS, 1,	do_update,
	"update eeprom data",
	"uboot 'addr' 'cnt' (hex)\n"
	"    - update uboot loder (cnt max 240 Kbyte)\n"
	"update 2ndboot  'nsih addr' 'nsih cnt' (hex) '2ndboot addr' '2nd cnt' (hex)\n"
	"    - update nsih file  2ndboot (cnt max 16 Kbyte)\n"
	
#if defined(CONFIG_SPI) && defined(CONFIG_ENV_IS_IN_EEPROM)
	"update clean env\n"
	"    - clear environment data\n"
#endif
);




