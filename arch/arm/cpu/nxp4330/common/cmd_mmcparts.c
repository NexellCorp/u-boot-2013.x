/*
 * (C) Copyright 2009 Nexell Co.,
 * jung hyun kim<jhkim@nexell.co.kr>
 *
 * Configuation settings for the Nexell board.
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

#include <common.h>
#include <command.h>
#include <mmc.h>

/*
#define	debug	printf
*/

#define DOS_PART_DISKSIG_OFFSET	0x1b8
#define DOS_PART_TBL_OFFSET	0x1be
#define DOS_PART_MAGIC_OFFSET	0x1fe
#define DOS_PBR_FSTYPE_OFFSET	0x36
#define DOS_PBR32_FSTYPE_OFFSET	0x52
#define DOS_PBR_MEDIA_TYPE_OFFSET	0x15
#define DOS_MBR	0
#define DOS_PBR	1

#define MMC_BLOCK_SIZE		(512)

typedef struct dos_partition {
	unsigned char boot_ind;		/* 0x80 - active			*/
	unsigned char head;		/* starting head			*/
	unsigned char sector;		/* starting sector			*/
	unsigned char cyl;		/* starting cylinder			*/
	unsigned char sys_ind;		/* What partition type			*/
	unsigned char end_head;		/* end head				*/
	unsigned char end_sector;	/* end sector				*/
	unsigned char end_cyl;		/* end cylinder				*/
	unsigned char start4[4];	/* starting sector counting from 0	*/
	unsigned char size4[4];		/* nr of sectors in partition		*/
} dos_partition_t;

/* Convert char[4] in little endian format to the host format integer
 */
static inline void int_to_le32(unsigned char *le32, unsigned int blocks)
{
	le32[3] = (blocks >> 24) & 0xff;
	le32[2] = (blocks >> 16) & 0xff;
	le32[1] = (blocks >>  8) & 0xff;
	le32[0] = (blocks >>  0) & 0xff;
}

static inline int le32_to_int(unsigned char *le32)
{
    return ((le32[3] << 24) +
	    (le32[2] << 16) +
	    (le32[1] << 8) +
	     le32[0]
	   );
}

static int mmc_put_part_mbr(block_dev_desc_t *desc, unsigned char *buffer)
{
	if (!desc || !buffer)
		return -1;

	return desc->block_write(desc->dev, 0, 1, (const void *)buffer);
}

int mmc_get_part_table(block_dev_desc_t *desc, uint64_t (*parts)[2], int *count)
{
	unsigned char buffer[MMC_BLOCK_SIZE];
	dos_partition_t *pt = (dos_partition_t *) (buffer + DOS_PART_TBL_OFFSET);
	int i = 0, max_part = 4;

	if (!parts || !count) {
		printf("-- Fail: partition input params --\n");
		return -1;
	}

	*count = 0;

	if (0 > desc->block_read(desc->dev, 0, 1, (void *)buffer)) {
		printf ("** Error read mmc.%d partition info **\n", desc->dev);
		return -1;
	}

	if (buffer[DOS_PART_MAGIC_OFFSET] != 0x55 ||
		buffer[DOS_PART_MAGIC_OFFSET + 1] != 0xAA) {
		printf ("** Not find partition magic number **\n");
		return -1;
	}

	for (i = 0; max_part > i; i++, pt++) {
		parts[i][0] = (uint64_t)le32_to_int(pt->start4) * MMC_BLOCK_SIZE;
		parts[i][1] = (uint64_t)le32_to_int(pt->size4)  * MMC_BLOCK_SIZE;
		if (parts[i][0] && parts[i][1])
			*count = (i+1);
		debug("part.%d=\t0x%llx \t~ \t0x%llx\n", i, parts[i][0], parts[i][1]);
	}
	return 0;
}

int mmc_make_part_mbr(block_dev_desc_t *desc, uint64_t (*parts)[2], int count,
				unsigned int part_type)
{
	dos_partition_t *pt;
	unsigned char buffer[MMC_BLOCK_SIZE];
	uint64_t avalible = desc->lba;
	int i = 0;

	printf("--- Create mmc.%d partitions %d ---\n", desc->dev, count);
	if (part_type != PART_TYPE_DOS) {
		printf ("** Support only DOS PARTITION **\n");
		return -1;
	}

	if (1 > count || count > 4) {
		printf ("** Can't make partition tables %d (1 ~ 4) **\n", count);
		return -1;
	}

	printf("Total = %lld * %d (%d.%d G) \n", avalible,
	(int)desc->blksz,
	(int)((avalible*desc->blksz)/(1024*1024*1024)),
	(int)((avalible*desc->blksz)%(1024*1024*1024)));

	memset(buffer, 0x0, sizeof(buffer));
	buffer[DOS_PART_MAGIC_OFFSET] = 0x55;
	buffer[DOS_PART_MAGIC_OFFSET + 1] = 0xAA;

	pt = (dos_partition_t *) (buffer + DOS_PART_TBL_OFFSET);
	for (i = 0; count > i; i++, pt++) {
		int head = 1, sector = 1, cys = 0;
		int end_head = 254, end_sector = 63, end_cys = 1023;
		unsigned int start = (unsigned int)(parts[i][0] / desc->blksz);
		unsigned int length =(unsigned int)(parts[i][1] / desc->blksz);

		pt->boot_ind = 0x00; /* no bootable */
		pt->sys_ind = 0x83;	 /* linux partition */

		/* default CHS */
		pt->head = (unsigned char)head;
		pt->sector = (unsigned char)(sector + ((cys & 0x00000300) >> 2) );
		pt->cyl = (unsigned char)(cys & 0x000000FF);
		pt->end_head = (unsigned char)end_head;
		pt->end_sector = (unsigned char)(end_sector + ((end_cys & 0x00000300) >> 2) );
		pt->end_cyl = (unsigned char)(end_cys & 0x000000FF);

		/* prevent MBR zone */
		if (i == 0)
			avalible -= start;

		if (0 == length)
			length = avalible;

		if (!start) {
			printf("-- Fail: part %d start 0x%llx is in MBR zone (0x200) --\n",
				i, parts[i][0]);
			return -1;
		}

		if (0 == length || length > avalible) {
			printf("-- Fail: part %d invalid length 0x%llx, avaliable (0x%llx) --\n",
				i, parts[i][1], (uint64_t)(avalible*MMC_BLOCK_SIZE));
			return -1;
		}

		int_to_le32(pt->start4, start);
		int_to_le32(pt->size4, length);
		printf("part.%d=\t0x%llx \t~ \t0x%llx\n", i, parts[i][0], (uint64_t)length * MMC_BLOCK_SIZE);

		avalible -= length;
		if (0 == avalible)
			break;
	}
	printf("--- Done mmc partition ---\n");

	return mmc_put_part_mbr(desc, buffer);
}

/*
 * cmd : fdisk 0 n, s1:size, s2:size, s3:size, s4:size
 */
static int do_fdisk(cmd_tbl_t *cmdtp, int flag, int argc, char* const argv[])
{
	block_dev_desc_t *desc;
	int i = 0, ret;

	if (2 > argc)
		return CMD_RET_USAGE;

	ret = get_device("mmc", argv[1], &desc);
	if (ret < 0) {
		printf ("** Not find device mmc.%s **\n", argv[1]);
		return 1;
	}

	if (2 == argc) {
		print_part(desc);
		dev_print(desc);
	} else {
		uint64_t parts[4][2];
		int count = 1;

		count = simple_strtoul(argv[2], NULL, 10);
		if (1 > count || count > 4) {
			printf ("** Invalid partition table count %d (1 ~ 4) **\n", count);
			return -1;
		}

		for (i = 0; count > i; i++) {
			const char *p = argv[i+3];
			parts[i][0] = simple_strtoull(p, NULL, 16);	/* start */
			if (!(p = strchr(p, ':'))) {
				printf("no <0x%llx:length> identifier\n", parts[i][0]);
				return -1;
			}
			p++;
			parts[i][1] = simple_strtoull(p, NULL, 16);		/* end */
			debug("part[%d] 0x%llx:0x%llx\n", i, parts[i][0], parts[i][1]);
		}

		mmc_make_part_mbr(desc, parts, count, PART_TYPE_DOS);
	}
	return 0;
}

U_BOOT_CMD(
	fdisk, 8, 1, do_fdisk,
	"list or create ms-dos partition tables",
	"<dev no>\n"
	"	- list partition table info\n"
	"fdisk <dev no> [part table counts] <start:length> <start:length> ...\n"
	"	- Note. each arguments seperated with space"
	"	- Create partition table info\n"
	"	- All numeric parameters are assumed to be hex.\n"
	"	- start and length is offset.\n"
	"	- If the length is zero, uses the remaining.\n"
);

