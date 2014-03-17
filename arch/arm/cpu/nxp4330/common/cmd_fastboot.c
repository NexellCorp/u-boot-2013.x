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
#include <asm/byteorder.h>
#include <common.h>
#include <command.h>
#include <malloc.h>
#include <fastboot.h>
#include <mmc.h>
#include <fat.h>
#include <decompress_ext4.h>

/*
#define	debug	printf
*/

#ifndef FASTBOOT_PARTS_DEFAULT
#error "Not default FASTBOOT_PARTS_DEFAULT"
#endif

static const char *const f_parts_default = FASTBOOT_PARTS_DEFAULT;


#define	FASTBOOT_MMC_MAX		2
#define	FASTBOOT_EEPROM_MAX		1
#define	FASTBOOT_NAND_MAX		1
#define	FASTBOOT_MEM_MAX		1

/* device types */
#define	FASTBOOT_DEV_EEPROM		(1<<0)	/*  name "eeprom" */
#define	FASTBOOT_DEV_NAND		(1<<1)	/*  name "nand" */
#define	FASTBOOT_DEV_MMC		(1<<2)	/*  name "mmc" */
#define	FASTBOOT_DEV_MEM		(1<<3)	/*  name "mem" */

/* filesystem types */
#define	FASTBOOT_FS_2NDBOOT		(1<<0)	/*  name "boot" <- bootable */
#define	FASTBOOT_FS_BOOT		(1<<1)	/*  name "boot" <- bootable */
#define	FASTBOOT_FS_RAW			(1<<2)	/*  name "raw" */
#define	FASTBOOT_FS_FAT			(1<<4)	/*  name "fat" */
#define	FASTBOOT_FS_EXT4		(1<<5)	/*  name "ext4" */
#define	FASTBOOT_FS_UBI			(1<<6)	/*  name "ubi" */
#define	FASTBOOT_FS_UBIFS		(1<<7)	/*  name "ubifs" */

#define	FASTBOOT_FS_MASK		(FASTBOOT_FS_EXT4 | FASTBOOT_FS_FAT | FASTBOOT_FS_UBI | FASTBOOT_FS_UBIFS)
#define	FASTBOOT_DEV_PART_MAX	(8)	/* each device max partition max num, if mmc max is 4 */

#define	TCLK_TICK_HZ			(1000000)

/* Use 65 instead of 64
 * null gets dropped
 * strcpy's need the extra byte */
#define	RESP_SIZE				(65)

/*
 *	f_devices[0,1,2..] : mmc / eeprom / nand
 *	|				|
 *	|				write_part
 *	|				|
 *	|				link -> fastboot_part -> fastboot_part  -> ...
 *	|							|
 *	|							.partition = bootloader, boot, system,...
 *	|							.start
 *	|							.length
 *	|							.....
 *	|
 */
struct fastboot_device;
struct fastboot_part {
    char partition[32];
    int dev_type;
    int dev_no;
    uint64_t start;
    uint64_t length;
	unsigned int fs_type;
	unsigned int flags;
	struct fastboot_device *fd;
	struct list_head link;
};

struct fastboot_device {
	char *device;
    int dev_max;
	unsigned int dev_type;
	unsigned int part_type;
	unsigned int fs_support;
	uint64_t parts[FASTBOOT_DEV_PART_MAX][2];
	struct list_head link;
	int (*write_part)(struct fastboot_part *fpart, void *buf, uint64_t length);
};

struct fastboot_fs_type {
	char *name;
	unsigned int fs_type;
};

/* support fs type */
static struct fastboot_fs_type f_part_fs[] = {
	{ "2nd"		, FASTBOOT_FS_2NDBOOT  	},
	{ "boot"	, FASTBOOT_FS_BOOT  	},
	{ "raw"		, FASTBOOT_FS_RAW		},
	{ "fat"		, FASTBOOT_FS_FAT		},
	{ "ext4"	, FASTBOOT_FS_EXT4		},
	{ "ubi"		, FASTBOOT_FS_UBI		},
	{ "ubifs"	, FASTBOOT_FS_UBIFS		},
};

/* Reserved partition names
 *
 *  NOTE :
 * 		Each command must be ended with ";"
 *
 *	partmap :
 * 			flash= <device>,<devno> : <partition> : <fs type> : <start>,<length> ;
 *		EX> flash= nand,0:bootloader:boot:0x300000,0x400000;
 *
 *	env :
 * 			<command name> = " command arguments ";
 * 		EX> bootcmd	= "tftp 42000000 uImage";
 *
 *	cmd :
 * 			" command arguments ";
 * 		EX> "tftp 42000000 uImage";
 *
 */

static const char *f_reserve_part[] = {
	[0] = "partmap",			/* fastboot partition */
	[1] = "mem",				/* download only */
	[2] = "env",				/* u-boot environment setting */
	[3] = "cmd",				/* command run */
};

/*
 * device partition functions
 */
static int get_parts_from_lists(struct fastboot_part *fpart, uint64_t (*parts)[2], int *count);
static void print_dev_parts(struct fastboot_device *fd);

#ifdef CONFIG_CMD_MMC
extern ulong mmc_bwrite(int dev_num, lbaint_t start, lbaint_t blkcnt, const void *src);
extern int mmc_get_part_table(block_dev_desc_t *desc, uint64_t (*parts)[2], int *count);

static inline int mmc_make_parts(int dev, uint64_t (*parts)[2], int count)
{
	char cmd[128];
	int i = 0, l = 0, p = 0;

	l = sprintf(cmd, "fdisk %d %d:", dev, count);
	p = l;
	for (i= 0; count > i; i++) {
		l = sprintf(&cmd[p], " 0x%llx:0x%llx", parts[i][0], parts[i][1]);
		p += l;
	}
	cmd[p] = 0;
	printf("%s\n", cmd);

	/* "fdisk <dev no> [part table counts] <start:length> <start:length> ...\n" */
	return run_command(cmd, 0);
}

static int mmc_part_write(struct fastboot_part *fpart, void *buf, uint64_t length)
{
	block_dev_desc_t *desc;
	struct fastboot_device *fd = fpart->fd;
	int dev = fpart->dev_no;
	lbaint_t blk, cnt;
	int blk_size = 512;
	char cmd[32];
	int i = 0, ret = 0;

	sprintf(cmd, "mmc dev %d", dev);

	debug("** mmc.%d partition %s (%s)**\n",
		dev, fpart->partition, fpart->fs_type&FASTBOOT_FS_EXT4?"FS":"Image");

	/* set mmc devicee */
	if (0 > get_device("mmc", simple_itoa(dev), &desc)) {
    	if (0 > run_command(cmd, 0))
    		return -1;
    	if (0 > run_command("mmc rescan", 0))
    		return -1;
	}

	if (0 > run_command(cmd, 0))	/* mmc device */
		return -1;

	if (fpart->fs_type == FASTBOOT_FS_2NDBOOT ||
		fpart->fs_type == FASTBOOT_FS_BOOT) {
		char args[64];
		int l = 0, p = 0;

		if (fpart->fs_type == FASTBOOT_FS_2NDBOOT)
			p = sprintf(args, "update_mmc %d 2ndboot", dev);
		else
			p = sprintf(args, "update_mmc %d boot", dev);

		l = sprintf(&args[p], " 0x%x 0x%llx 0x%llx", (unsigned int)buf, fpart->start, length);
		p += l;
		args[p] = 0;

		return run_command(args, 0); /* update_mmc [dev no] <type> 'mem' 'addr' 'length' [load addr] */
	}

	if (fpart->fs_type & FASTBOOT_FS_MASK) {
		uint64_t parts[4][2] = { {0,0}, };
		int num = 0;

		if (0 > mmc_get_part_table(desc, parts, &num))
			return -1;

		for (i = 0; num > i; i++) {
			if (parts[i][0] == fpart->start &&
				parts[i][1] == fpart->length)
				break;
			/* when last partition set value is zero,
			   set avaliable length */
			if ((num-1) == i &&
				0 == fpart->length) {
				fpart->length = parts[i][1];
				break;
			}
		}

		if (i == num) {	/* new partition */
			printf("Warn  : [%s] invalid, make new partitions ....\n", fpart->partition);
			print_dev_parts(fpart->fd);

			get_parts_from_lists(fpart, parts, &num);
			ret = mmc_make_parts(dev, parts, num);
			if (0 > ret) {
				printf("** Fail make partition : %s.%d %s**\n",
					fd->device, dev, fpart->partition);
				return -1;
			}
		}
	}

 	if ((fpart->fs_type & FASTBOOT_FS_EXT4) &&
 		(0 == check_compress_ext4((char*)buf, fpart->length))) {
		debug("write compressed ext4 ...\n");
		return write_compressed_ext4((char*)buf, fpart->start/blk_size);
	}

	blk = fpart->start/blk_size ;
	cnt = (length/blk_size) + ((length & (blk_size-1)) ? 1 : 0);

	printf("write image to 0x%llx(0x%x), 0x%llx(0x%x)\n",
		fpart->start, (unsigned int)blk, length, (unsigned int)blk);

	ret = mmc_bwrite(dev, blk, cnt, buf);

	return (0 > ret ? ret : 0);
}
#endif
#ifdef CONFIG_CMD_EEPROM
static int eeprom_part_write(struct fastboot_part *fpart, void *buf, uint64_t length)
{
	char args[64];
	int l = 0, p = 0;

	p = sprintf(args, "update_eeprom ");

	if (fpart->fs_type & FASTBOOT_FS_BOOT)
		l = sprintf(&args[p], "%s", "uboot");
	else if (fpart->fs_type & FASTBOOT_FS_2NDBOOT)
		l = sprintf(&args[p], "%s", "2ndboot");
	else
		l = sprintf(&args[p], "%s", "raw");

	p += l;
	l = sprintf(&args[p], " 0x%x 0x%llx 0x%llx", (unsigned int)buf, fpart->start, length);
	p += l;
	args[p] = 0;

	debug("%s\n", args);

	return run_command(args, 0);	/* "update_eeprom <type> <mem> <addr> <length> ...\n" */
}
#endif

#ifdef CONFIG_CMD_NAND
static int nand_part_write(struct fastboot_part *fpart, void *buf, uint64_t length)
{
	char args1[64], args2[64];
	int l = 0, p = 0;


	/*
	 * nand standalone
	 *		2ndboot,3rdboot
	 *			"update_nand write         0x50000000 0x0        0x20000" 
	 *
	 * normal
	 *		raw image
	 *			"nand        write         0x50000000 0x400000   0x100000"
	 *
	 *		ubi image
	 *			"nand        write.trimffs 0x50000000 0x20000000 0x20000000"
	 */
	if ((fpart->fs_type & FASTBOOT_FS_2NDBOOT) || (fpart->fs_type & FASTBOOT_FS_BOOT))
		p = sprintf(args1, "update_nand ");
	else
		p = sprintf(args1, "nand ");

	l = sprintf(&args1[p], "%s", "erase");
	p += l;
	l = sprintf(&args1[p], " 0x%llx 0x%llx", fpart->start, fpart->length);
	p += l;
	args1[p] = 0;

	run_command(args1, 0);


	l = 0, p = 0;
	if ((fpart->fs_type & FASTBOOT_FS_2NDBOOT) || (fpart->fs_type & FASTBOOT_FS_BOOT))
		p = sprintf(args2, "update_nand ");
	else
		p = sprintf(args2, "nand ");


	if (fpart->fs_type & FASTBOOT_FS_UBI)
		l = sprintf(&args2[p], "%s", "write.trimffs");
	else
		l = sprintf(&args2[p], "%s", "write");
	p += l;

	l = sprintf(&args2[p], " 0x%x 0x%llx 0x%llx", (unsigned int)buf, fpart->start, length);
	p += l;
	args2[p] = 0;

	debug("%s\n", args1);
	debug("%s\n", args2);

	return run_command(args2, 0);
}
#endif

static struct fastboot_device f_devices[] = {
	{
		.device 	= "eeprom",
		.dev_max	= FASTBOOT_EEPROM_MAX,
		.dev_type	= FASTBOOT_DEV_EEPROM,
		.fs_support	= (FASTBOOT_FS_2NDBOOT | FASTBOOT_FS_BOOT | FASTBOOT_FS_RAW),
	#ifdef CONFIG_CMD_EEPROM
		.write_part	= eeprom_part_write,
	#endif
	},
	{
		.device 	= "nand",
		.dev_max	= FASTBOOT_NAND_MAX,
		.dev_type	= FASTBOOT_DEV_NAND,
		.fs_support	= (FASTBOOT_FS_2NDBOOT | FASTBOOT_FS_BOOT | FASTBOOT_FS_RAW | FASTBOOT_FS_UBI),
	#ifdef CONFIG_CMD_NAND
		.write_part	= nand_part_write,
	#endif
	},
	{
		.device 	= "mmc",
		.dev_max	= FASTBOOT_MMC_MAX,
		.dev_type	= FASTBOOT_DEV_MMC,
		.part_type	= PART_TYPE_DOS,
		.fs_support	= (FASTBOOT_FS_2NDBOOT | FASTBOOT_FS_BOOT | FASTBOOT_FS_RAW | FASTBOOT_FS_FAT | FASTBOOT_FS_EXT4),
	#ifdef CONFIG_CMD_MMC
		.write_part	= &mmc_part_write,
	#endif
	},
};

#define	FASTBOOT_DEV_SIZE	ARRAY_SIZE(f_devices)

/*
 *
 * FASTBOOT COMMAND PARSE
 *
 */
static inline void parse_comment(const char *str, const char **ret)
{
	const char *p = str, *r;

	do {
		if (!(r = strchr(p, '#')))
			break;
		r++;

		if (!(p = strchr(r, '\n'))) {
			printf("---- not end comments '#' ----\n");
			break;
		}
		p++;
	} while (1);

	/* for next */
	*ret = p;
}

static inline int parse_string(const char *s, const char *e, char *b, int len)
{
	int l, a = 0;

	do { while (0x20 == *s || 0x09 == *s || 0x0a == *s) { s++; } } while(0);

	if (0x20 == *(e-1) || 0x09 == *(e-1))
		do { e--; while (0x20 == *e || 0x09 == *e) { e--; }; a = 1; } while(0);

	l = (e - s + a);
	if (l > len) {
		printf("-- Not enough buffer %d for string len %d [%s] --\n", len, l, s);
		return -1;
	}

	strncpy(b, s, l);
	b[l] = 0;

	return l;
}

static inline void sort_string(char *p, int len)
{
	int i, j;
	for (i = 0, j = 0; len > i; i++) {
		if (0x20 != p[i] && 0x09 != p[i] && 0x0A != p[i])
			p[j++] = p[i];
	}
	p[j] = 0;
}

static int parse_map_device(const char *parts, const char **ret,
			struct fastboot_device **fdev, struct fastboot_part *fpart)
{
	struct fastboot_device *fd = *fdev;
	const char *p, *id, *c;
	char str[32];
	int i = 0, id_len;

	if (ret)
		*ret = NULL;

	id = p = parts;
	if (!(p = strchr(id, ':'))) {
		printf("no <dev-id> identifier\n");
		return 1;
	}
	id_len = p - id;

	/* for next */
	p++, *ret = p;

	c = strchr(id, ',');
	parse_string(id, c, str, sizeof(str));

	for (i = 0; FASTBOOT_DEV_SIZE > i; i++, fd++) {
		if (strcmp(fd->device, str) == 0) {
			/* add to device */
			list_add_tail(&fpart->link, &fd->link);
			*fdev = fd;

			/* dev no */
			debug("device: %s", fd->device);
			if (!(p = strchr(id, ','))) {
				printf("no <dev-no> identifier\n");
				return -1;
			}
			p++;
	#if (0)
			/* dev no */
			if (!(c = strchr(p, ','))) {
				printf("no <part-no> identifier\n");
				return -1;
			}

			parse_string(p, c, str, sizeof(str));	/* dev no*/
			fpart->dev_no = simple_strtoul(str, NULL, 10);
			if (fpart->dev_no >= fd->dev_max) {
				printf("** Over dev-no max %s.%d : %d **\n",
					fd->device, fd->dev_max-1, fpart->dev_no);
				return -1;
			}
			debug(".%d", fpart->dev_no);

			c++;
			parse_string(c, c+id_len, str, sizeof(str));	/* dev no*/
			fpart->part_no = simple_strtoul(str, NULL, 10);
			debug(".%d\n", fpart->part_no);
	#else
			parse_string(p, p+id_len, str, sizeof(str));	/* dev no*/
			/* dev no */
			fpart->dev_no = simple_strtoul(str, NULL, 10);
			if (fpart->dev_no >= fd->dev_max) {
				printf("** Over dev-no max %s.%d : %d **\n",
					fd->device, fd->dev_max-1, fpart->dev_no);
				return -1;
			}
			debug(".%d\n", fpart->dev_no);
	#endif
			fpart->fd = fd;
			return 0;
		}
	}

	/* to delete */
	fd = *fdev;
	strcpy(fpart->partition, "unknown");
	list_add_tail(&fpart->link, &fd->link);

	printf("** Can't device parse : %s **\n", parts);
	return -1;
}

static int parse_map_partition(const char *parts, const char **ret,
			struct fastboot_device **fdev, struct fastboot_part *fpart)
{
	struct fastboot_device *fd = f_devices;
	struct fastboot_part *fp;
	struct list_head *entry, *n;
	const char *p, *id;
	char str[32] = { 0, };
	int i = 0;

	if (ret)
		*ret = NULL;

	id = p = parts;
	if (!(p = strchr(id, ':'))) {
		printf("no <name> identifier\n");
		return -1;
	}

	/* for next */
	p++, *ret = p;
	p--; parse_string(id, p, str, sizeof(str));

	for (i = 0; ARRAY_SIZE(f_reserve_part) > i; i++) {
		const char *r =f_reserve_part[i];
		if (!strcmp(r, str)) {
			printf("** Reserved partition name : %s  **\n", str);
			return -1;
		}
	}

	/* check partition */
	for (i = 0; FASTBOOT_DEV_SIZE > i; i++, fd++) {
		struct list_head *head = &fd->link;
		if (list_empty(head))
			continue;
		list_for_each_safe(entry, n, head) {
			fp = list_entry(entry, struct fastboot_part, link);
			if (!strcmp(fp->partition, str)) {
				printf("** Exist partition : %s -> %s **\n",
					fd->device, fp->partition);
				strcpy(fpart->partition, str);
				fpart->partition[strlen(str)] = 0;
				return -1;
			}
		}
	}

	strcpy(fpart->partition, str);
	fpart->partition[strlen(str)] = 0;
	debug("part  : %s\n", fpart->partition);

	return 0;
}

static int parse_map_fs(const char *parts, const char **ret,
		struct fastboot_device **fdev, struct fastboot_part *fpart)
{
	struct fastboot_device *fd = *fdev;
	struct fastboot_fs_type *fs = f_part_fs;
	const char *p, *id;
	char str[16] = { 0, };
	int i = 0;

	if (ret)
		*ret = NULL;

	id = p = parts;
	if (!(p = strchr(id, ':'))) {
		printf("no <dev-id> identifier\n");
		return -1;
	}

	/* for next */
	p++, *ret = p;
	p--; parse_string(id, p, str, sizeof(str));

	for (; ARRAY_SIZE(f_part_fs) > i; i++, fs++) {
		if (strcmp(fs->name, str) == 0) {
			if (!(fd->fs_support & fs->fs_type)) {
				printf("** '%s' not support '%s' fs **\n", fd->device, fs->name);
				return -1;
			}
			fpart->fs_type = fs->fs_type;
			debug("fs    : %s\n", fs->name);
			return 0;
		}
	}

	printf("** Can't fs parse : %s **\n", str);
	return -1;
}

static int parse_map_address(const char *parts, const char **ret,
			struct fastboot_device **fdev, struct fastboot_part *fpart)
{
	const char *p, *id;
	char str[64] = { 0, };
	int id_len;

	if (ret)
		*ret = NULL;

	id = p = parts;
	if (!(p = strchr(id, ';')) && !(p = strchr(id, '\n'))) {
		printf("no <; or '\n'> identifier\n");
		return -1;
	}
	id_len = p - id;

	/* for next */
	p++, *ret = p;

	if (!(p = strchr(id, ','))) {
		printf("no <start> identifier\n");
		return -1;
	}

	parse_string(id, p, str, sizeof(str));
	fpart->start = simple_strtoull(str, NULL, 16);
	debug("start : 0x%llx\n", fpart->start);

	p++;
	parse_string(p, p+id_len, str, sizeof(str));	/* dev no*/
	fpart->length = simple_strtoull(str, NULL, 16);
	debug("length: 0x%llx\n", fpart->length);

	return 0;
}

static int parse_map_head(const char *parts, const char **ret)
{
	const char *p = parts;
	int len = strlen("flash=");

	debug("\n");
	if (!(p = strstr(p, "flash=")))
		return -1;

	*ret = p + len;
	return 0;
}

typedef int (parse_fnc_t) (const char *parts, const char **ret,
						struct fastboot_device **fdev, struct fastboot_part *fpart);

parse_fnc_t *parse_map_seqs[] = {
	parse_map_device,
	parse_map_partition,
	parse_map_fs,
	parse_map_address,
	0,	/* end */
};

static inline void part_lists_init(int init)
{
	struct fastboot_device *fd = f_devices;
	struct fastboot_part *fp;
	struct list_head *entry, *n;
	int i = 0;

	for (i = 0; FASTBOOT_DEV_SIZE > i; i++, fd++) {
		struct list_head *head = &fd->link;

		if (init) {
			INIT_LIST_HEAD(head);
			memset(fd->parts, 0x0, sizeof(FASTBOOT_DEV_PART_MAX*2));
			continue;
		}

		if (list_empty(head))
			continue;

		debug("delete [%s]:", fd->device);
		list_for_each_safe(entry, n, head) {
			fp = list_entry(entry, struct fastboot_part, link);
			debug("%s ", fp->partition);
			list_del(entry);
			free(fp);
		}
		debug("\n");
		INIT_LIST_HEAD(head);
	}
}

static int make_part_lists(const char *ptable_str, int ptable_str_len)
{
	struct fastboot_device *fd = f_devices;
	struct fastboot_part *fp;
	parse_fnc_t **p_fnc_ptr;
	const char *p = ptable_str;
	int len = ptable_str_len;
	int err = -1;

	debug("\n---make_part_lists ---\n");
	part_lists_init(0);

	parse_comment(p, &p);
	sort_string((char*)p, len);

	/* new parts table */
	while (1) {
		fd = f_devices;
		fp = malloc(sizeof(*fp));

		if (!fp) {
			printf("** Can't malloc fastboot part table entry (%d) **\n", sizeof(*fp));
			err = -1;
			break;
		}

		if (parse_map_head(p, &p)) {
			if (err)
				printf("-- unknown parts head: [%s]\n", p);
			break;
		}

		for (p_fnc_ptr = parse_map_seqs; *p_fnc_ptr; ++p_fnc_ptr) {
			if ((*p_fnc_ptr)(p, &p, &fd, fp) != 0) {
				err = -1;
				goto parse_fail;
			}
		}
		err = 0;
	}

parse_fail:
	if (err)
		part_lists_init(0);

	return err;
}

static void print_part_lists(void)
{
	struct fastboot_device *fd = f_devices;
	struct fastboot_part *fp;
	struct list_head *entry, *n;
	int i;

	printf("\nFastboot Partitions:\n");
	for (i = 0; FASTBOOT_DEV_SIZE > i; i++, fd++) {
		struct list_head *head = &fd->link;
		if (list_empty(head))
			continue;
		list_for_each_safe(entry, n, head) {
			fp = list_entry(entry, struct fastboot_part, link);
			printf(" %s.%d: %s, %s : 0x%llx, 0x%llx\n",
				fd->device, fp->dev_no, fp->partition,
				FASTBOOT_FS_MASK&fp->fs_type?"fs":"img", fp->start, fp->length);
		}
	}

	printf("Support fstype:");
	for (i = 0; ARRAY_SIZE(f_part_fs) > i; i++)
		printf(" %s ", f_part_fs[i].name);
	printf("\n");

	printf("Reserved part :");
	for (i = 0; ARRAY_SIZE(f_reserve_part) > i; i++)
		printf(" %s ", f_reserve_part[i]);
	printf("\n\n");
}

static void print_dev_parts(struct fastboot_device *fd)
{
	struct fastboot_part *fp;
	struct list_head *entry, *n;

	printf("Device: %s\n", fd->device);
	struct list_head *head = &fd->link;
	if (!list_empty(head)) {
		list_for_each_safe(entry, n, head) {
			fp = list_entry(entry, struct fastboot_part, link);
			printf(" %s.%d: %s, %s : 0x%llx, 0x%llx\n",
				fd->device, fp->dev_no, fp->partition,
				FASTBOOT_FS_MASK&fp->fs_type?"fs":"img", fp->start, fp->length);
		}
	}
}

static int get_parts_from_lists(struct fastboot_part *fpart, uint64_t (*parts)[2], int *count)
{
	struct fastboot_part *fp = fpart;
	struct fastboot_device *fd = fpart->fd;
	struct list_head *head = &fd->link;
	struct list_head *entry, *n;
	int dev = fpart->dev_no;
	int i = 0;

	if (!parts || !count) {
		printf("-- No partition input params --\n");
		return -1;
	}

	*count = 0;

	if (list_empty(head))
		return 0;

	list_for_each_safe(entry, n, head) {
		fp = list_entry(entry, struct fastboot_part, link);
		if ((FASTBOOT_FS_MASK & fp->fs_type) &&
			(dev == fp->dev_no)) {
			parts[i][0] = fp->start;
			parts[i][1] = fp->length;
			i++;
			debug("%s.%d = %s\n", fd->device, dev, fp->partition);
		}
	}

	*count = i;	/* set part count */
	return 0;
}

/*
 * Display to LCD
 */
#define	ALIAS(fnc)	__attribute__((weak, alias(fnc)))

void fboot_lcd_start (void)				ALIAS("f_lcd_stop");	void f_lcd_start (void) {}
void fboot_lcd_stop  (void)				ALIAS("f_lcd_stop");	void f_lcd_stop  (void) {}
void fboot_lcd_part  (char *p, char *s)	ALIAS("f_lcd_part");	void f_lcd_part  (char *p, char *s){ }
void fboot_lcd_flash (char *p, char *s)	ALIAS("f_lcd_flash");	void f_lcd_flash (char *p, char *s){ }
void fboot_lcd_down  (int ps)			ALIAS("f_lcd_down");	void f_lcd_down  (int ps){ }
void fboot_lcd_status(char *s)			ALIAS("f_lcd_status");	void f_lcd_status(char *s){ }

/*
 *
 * FASTBOOT USB CONTROL
 *
 */
struct f_trans_stat {
	unsigned long long image_size;	/* Image size */
	unsigned long long down_bytes;	/* Downloaded size */
	int down_percent;
	unsigned int error;
};
static struct f_trans_stat f_status;

typedef struct cmd_fastboot_interface f_cmd_inf;
static int fboot_rx_handler(const unsigned char *buffer, unsigned int length);
static void fboot_reset_handler(void);

static f_cmd_inf f_interface = {
	.rx_handler = fboot_rx_handler,
	.reset_handler = fboot_reset_handler,
	.product_name = NULL,
	.serial_no = NULL,
	.transfer_buffer = (unsigned char *)CFG_FASTBOOT_TRANSFER_BUFFER,
	.transfer_buffer_size = CFG_FASTBOOT_TRANSFER_BUFFER_SIZE,
};

static int parse_env_head(const char *env, const char **ret, char *str, int len)
{
	const char *p = env, *r = p;

	parse_comment(p, &p);
	if (!(r = strchr(p, '=')))
		return -1;

	if (0 > parse_string(p, r, str, len))
		return -1;

	if (!(r = strchr(r, '"'))){
		printf("no <\"> identifier\n");
		return -1;
	}

	r++; *ret = r;
	return 0;
}

static int parse_env_end(const char *env, const char **ret, char *str, int len)
{
	const char *p = env;
	const char *r = p;

	if (!(r = strchr(p, '"'))) {
		printf("no <\"> end identifier\n");
		return -1;
	}

	if (0 > parse_string(p, r, str, len))
		return -1;

	r++;
	if (!(r = strchr(p, ';')) &&
		!(r = strchr(p, '\n'))) {
		printf("no <;> exit identifier\n");
		return -1;
	}

	/* for next */
	r++, *ret = r;
	return 0;
}

static int fboot_set_env(const char *str, int len)
{
	const char *p = str;
	char cmd[32];
	char arg[1024];
	int err = -1;

	debug("---fboot_set_env---\n");
	while (1) {
		if (parse_env_head(p, &p, cmd, sizeof(cmd)))
			break;

		if (parse_env_end(p, &p, arg, sizeof(arg)))
			break;

		printf("%s=%s\n", cmd, arg);
		setenv(cmd, (char *)arg);
		saveenv();
		err = 0;
	}
	return err;
}

static int parse_cmd(const char *cmd, const char **ret, char *str, int len)
{
	const char *p = cmd, *r = p;

	parse_comment(p, &p);
	if (!(p = strchr(p, '"')))
		return -1;
	p++;

	if (!(r = strchr(p, '"')))
		return -1;

	if (0 > parse_string(p, r, str, len))
		return -1;
	r++;

	if (!(r = strchr(p, ';')) &&
		!(r = strchr(p, '\n'))) {
		printf("no <;> exit identifier\n");
		return -1;
	}

	/* for next */
	r++, *ret = r;
	return 0;
}

static int fboot_run_cmd(const char *str, int len)
{
	const char *p = str;
	char cmd[128];
	int err = -1;

	debug("---fboot_run_cmd---\n");
	while (1) {
		if (parse_cmd(p, &p, cmd, sizeof(cmd)))
			break;

		printf("Run [%s]\n", cmd);
		err = run_command(cmd, 0);
		if (0 > err)
			break;
	}
	return err;
}

static int fboot_tx_status(const char *resp, unsigned int len, unsigned int sync)
{
	debug("reaponse -> %s\n", resp);
	fastboot_tx_status(resp, len, sync);
	return 0;
}

static int fboot_cmd_reboot(const char *cmd, f_cmd_inf *inf, struct f_trans_stat *fst)
{
	fboot_tx_status("OKAY", strlen("OKAY"), FASTBOOT_TX_SYNC);
	return do_reset (NULL, 0, 0, NULL);
}

static int fboot_cmd_getvar(const char *cmd, f_cmd_inf *inf, struct f_trans_stat *fst)
{
	char resp[RESP_SIZE] = "OKAY";
	char *s = (char*)cmd;
	char *p = resp + strlen(resp);
	debug("getvar = %s\n", cmd);

	if (!strncmp(cmd, "partition-type:", strlen("partition-type:"))) {
		s += strlen("partition-type:");
		printf("\nReady : [%s]\n", s);
		fboot_lcd_part(s, "wait...");
		goto var_done;
	}

	if (!strncmp(cmd, "version", strlen("version"))) {
		strcpy(p, FASTBOOT_VERSION);
		goto var_done;
	}

	if (!strncmp(cmd, "product", strlen("product"))) {
		if (inf->product_name)
			strcpy(p, inf->product_name);
		goto var_done;
	}

	if (!strncmp(cmd, "serialno", strlen("serialno"))) {
		if (inf->serial_no)
			strcpy(p, inf->serial_no);
		goto var_done;
	}

	if (!strncmp(cmd, "max-download-size", strlen("max-download-size"))) {
		if (inf->transfer_buffer_size)
			sprintf(p, "%08x", inf->transfer_buffer_size);
		goto var_done;
	}

	fastboot_getvar(cmd, p);

var_done:
	return fboot_tx_status(resp, strlen(resp), FASTBOOT_TX_ASYNC);
}

static int fboot_cmd_download(const char *cmd, f_cmd_inf *inf, struct f_trans_stat *fst)
{
	char resp[RESP_SIZE] = "OKAY";
	unsigned int len;
	unsigned int clear = (unsigned int)inf->transfer_buffer;

	fst->image_size = simple_strtoull (cmd, NULL, 16);
	fst->down_bytes = 0;
	fst->error = 0;
	fst->down_percent = -1;

	len = (fst->image_size & ~0x3) + 4;
	clear += fst->image_size;
	clear &= ~0x3;

	memset((char*)clear, 0x0, 16);	/* clear buffer for string parsing */

	printf("Starting download of %lld bytes\n", fst->image_size);

	if (0 == fst->image_size) {
		sprintf(resp, "FAIL data invalid size");	/* bad user input */
		printf("-- Fail, data invalid size --\n");
	} else if (fst->image_size > inf->transfer_buffer_size) {
		sprintf(resp, "FAIL data too large");
		printf("-- Fail, data too large buf[%d], image[%lld] --\n",
			inf->transfer_buffer_size, fst->image_size);
		fst->image_size = 0;
	} else {
		/* The default case, the transfer fits
		   completely in the interface buffer */
		sprintf(resp, "DATA%08llx", fst->image_size);
	}

	return fboot_tx_status(resp, strlen(resp), FASTBOOT_TX_ASYNC);
}

static int fboot_cmd_flash(const char *cmd, f_cmd_inf *inf, struct f_trans_stat *fst)
{
	struct fastboot_device *fd = f_devices;
	struct fastboot_part *fp;
	struct list_head *entry, *n;
	char resp[RESP_SIZE] = "OKAY";
	int i = 0, err = 0;

	printf("Flash : [%s]\n", cmd);
	fboot_lcd_flash((char*)cmd, "flashing");

	if (fst->down_bytes == 0) {
		sprintf(resp, "FAIL no image downloaded");
		printf("Fail : image not dowloaded !!!\n");
		return fboot_tx_status(resp, strlen(resp), FASTBOOT_TX_ASYNC);
	}

	/* new partition map */
	if (!strcmp("partmap", cmd)) {
		const char *p = (const char *)inf->transfer_buffer;

		if (0 > make_part_lists(p, strlen(p))) {
			sprintf(resp, "FAIL partition map parse");
			printf("-- Fail, partition map parse --\n");
			goto err_flash;
		}
		print_part_lists();
		parse_comment(p, &p);

		if (0 == setenv("fastboot", (char *)p) &&
			0 == saveenv());
			goto done_flash;

	/* set environments */
	} else if (!strcmp("env", cmd)) {
		char *p = (char *)inf->transfer_buffer;

		if(0 > fboot_set_env(p, fst->down_bytes)){
			sprintf(resp, "FAIL environment parse");
			printf("-- Fail, env partition parse --\n");
			goto err_flash;
		}
		goto done_flash;

	/* run command */
	} else if (!strcmp("cmd", cmd)) {
		char *p = (char *)inf->transfer_buffer;

		if(0 > fboot_run_cmd(p, fst->down_bytes)){
			sprintf(resp, "FAIL cmd parse");
			printf("-- Fail, cmd partition parse --\n");
			goto err_flash;
		}
		goto done_flash;

	/* memory partition : do nothing */
	} else if (0 == strcmp("mem", cmd)) {
		goto done_flash;
	}

	/* flash to partition */
	for (i = 0; FASTBOOT_DEV_SIZE > i; i++, fd++) {
		struct list_head *head = &fd->link;
		if (list_empty(head))
			continue;

		list_for_each_safe(entry, n, head) {
			fp = list_entry(entry, struct fastboot_part, link);
			if (!strcmp(fp->partition, cmd)) {

				if ((fst->down_bytes > fp->length) && (fp->length != 0)) {
					sprintf(resp, "FAIL image too large for partition");
					printf("-- Fail: image too large for %s part length %lld --\n",
						fp->partition, fp->length);
					goto err_flash;
				}

				if ((fd->dev_type != FASTBOOT_DEV_MEM) &&
					fd->write_part) {
					char *p = (char *)inf->transfer_buffer;
					if (0 > fd->write_part(fp, p, fst->down_bytes)) {
						sprintf(resp, "FAIL to flash partition");
						printf("-- Fail, flash %s partition %s --\n", fd->device, fp->partition);
					}
				}

				goto done_flash;
			}
		}
	}

err_flash:
	err = -1;
	sprintf(resp, "FAIL partition does not exist");
	printf("-- Fail: %s partition does not exist --\n", cmd);

done_flash:
	printf("Flash : [%s] %s\n", cmd, 0 > err ? "FAIL":"DONE");
	fboot_lcd_flash((char*)cmd, 0 > err ? "fail":"done");

	return fboot_tx_status(resp, strlen(resp), FASTBOOT_TX_ASYNC);
}

static int fboot_cmd_boot(const char *cmd, f_cmd_inf *inf, struct f_trans_stat *fst)
{
	char resp[RESP_SIZE] = "FAIL";
	printf("*** Not IMPLEMENT ***\n");
	return fboot_tx_status(resp, strlen(resp), FASTBOOT_TX_SYNC);
}

static int fboot_cmd_format(const char *cmd, f_cmd_inf *inf, struct f_trans_stat *fst)
{
	char resp[RESP_SIZE] = "FAIL";
	printf("*** Not IMPLEMENT ***\n");
	return fboot_tx_status(resp, strlen(resp), FASTBOOT_TX_ASYNC);
}

static int fboot_cmd_erase(const char *cmd, f_cmd_inf *inf, struct f_trans_stat *fst)
{
	char resp[RESP_SIZE] = "FAIL";
	printf("*** Not IMPLEMENT ***\n");
	return fboot_tx_status(resp, strlen(resp), FASTBOOT_TX_ASYNC);
}

static int fboot_cmd_oem(const char *cmd, f_cmd_inf *inf, struct f_trans_stat *fst)
{
	char resp[RESP_SIZE] = "INFO unknown OEM command";
	fboot_tx_status(resp, strlen(resp), FASTBOOT_TX_ASYNC);

	sprintf(resp,"OKAY");
	fboot_tx_status(resp, strlen(resp), FASTBOOT_TX_ASYNC);
	return 0;
}

static void fboot_reset_handler(void)
{
	struct f_trans_stat *fst = &f_status;
	fst->image_size = 0;
	fst->down_bytes = 0;
	fst->error = 0;
	fst->down_percent = -1;
}

struct f_cmd_fnc_t {
	char *command;
	int (*fnc_t)(const char *cmd, f_cmd_inf *inf, struct f_trans_stat *fst);
};

static struct f_cmd_fnc_t f_sequence [] = {
	{ "reboot"		, fboot_cmd_reboot		},
	{ "getvar:"		, fboot_cmd_getvar		},
	{ "download:"	, fboot_cmd_download	},
	{ "flash:"		, fboot_cmd_flash		},
	{ "boot"		, fboot_cmd_boot		},
	{ "format:"		, fboot_cmd_format		},
	{ "erase:"		, fboot_cmd_erase		},
	{ "oem"			, fboot_cmd_oem			},
};
#define	FASTBOOT_CMD_SIZE	ARRAY_SIZE(f_sequence)

static int fboot_rx_handler(const unsigned char *buffer, unsigned int length)
{
	f_cmd_inf *inf = &f_interface;
	struct f_trans_stat *fst = &f_status;
	int i = 0, ret = 0;

	/*
	 * Command
	 */
	if (!fst->image_size) {
		const char *cmd = (char *)buffer;
		debug("[CMD = %s]\n", cmd);
		for (i = 0; FASTBOOT_CMD_SIZE > i; i++) {
			struct f_cmd_fnc_t *fptr = &f_sequence[i];
			const char *str = fptr->command;
			int len = strlen(str);
			if (!strncmp(cmd, str, len) &&
				fptr->fnc_t)
			{
				fptr->fnc_t(cmd+len, inf, fst);
				break;
			}
		}
		memset((void*)buffer, 0, length);	/* clear buffer */

		if (FASTBOOT_CMD_SIZE == i) {
			printf("-- unknown fastboot cmd [%s] --\n", cmd);
			fboot_tx_status("ERROR", strlen("ERROR"), FASTBOOT_TX_ASYNC);
		}
	/*
	 * Download
	 */
	} else {
		if (length) {
			char resp[RESP_SIZE];
			unsigned long long len = (fst->image_size - fst->down_bytes);
			unsigned long long n = fst->down_bytes * 100ULL;
			int percent;

			do_div(n, fst->image_size);
			percent = (int)n;

			if (len > length)
				len = length;

			memcpy(inf->transfer_buffer+fst->down_bytes, buffer, len);
			fst->down_bytes += len;

			/* transfer is done */
			if (fst->down_bytes >= fst->image_size) {

				(fst->down_percent > 0) ? printf ("\n"): 0;
				printf ("downloading of %lld bytes to 0x%x (0x%x) finished\n",
					fst->down_bytes, (uint)inf->transfer_buffer, inf->transfer_buffer_size);

				if (fst->error)
					sprintf(resp, "ERROR");
				else
					sprintf(resp, "OKAY");

				fst->image_size = 0;
				fboot_tx_status(resp, strlen(resp), FASTBOOT_TX_ASYNC);

				fboot_lcd_down(100);
			} else {
				if (percent != fst->down_percent) {
					fst->down_percent = percent;
					printf("\rdownloading %lld -- %3d%% complete.", fst->down_bytes, percent);
					fboot_lcd_down(percent);
				}
			}

		} else {
			/* Ignore empty buffers */
			printf ("Warning empty download buffer\n");
			printf ("Ignoring\n");
		}
	}

	return ret;
}

static int do_fastboot(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	static int inited = 0;
	f_cmd_inf *inf = &f_interface;
	const char *p;
	unsigned int tclk = TCLK_TICK_HZ;
	int timeout = 0, f_connect = 0;
	int err;

	p = getenv("fastboot");
	if (NULL == p) {
		printf("*** Warning use default fastboot commands ***\n");
		p = f_parts_default;

		sort_string((char*)p, strlen(p));
		setenv("fastboot", (char *)p);
		saveenv();
	}

	if (!inited) {
		part_lists_init(1);
		err = make_part_lists(p, strlen(p));
		if (0 > err)
			return err;

		inited = 1;
	}

	if (argc > 1) {
		if (!strcmp(argv[1], "-l")) {
			print_part_lists();
			return 0;
		} else {
			timeout = simple_strtol(argv[1], NULL, 10);
		}
	}

	print_part_lists();
	fboot_lcd_start();

	do {
		/* reset */
		f_connect = 0;
		if (0 == fastboot_init(inf)) {
			unsigned int curr_time = (get_ticks()/tclk);
			unsigned int end_time = curr_time + timeout;

			printf("------------------------------------------\n");
			while (1) {
				int status = fastboot_poll();
				if (timeout)
					curr_time = (get_ticks()/tclk);

				if (status != FASTBOOT_OK) {
					if (ctrlc()) {
						printf("Fastboot ended by user\n");
						fboot_lcd_status("exit");
						f_connect = 0;
						break;
					}
				}

				if (FASTBOOT_ERROR == status) {
					printf("Fastboot error \n");
					fboot_lcd_status("error!!!");
					break;
				}
				else if (FASTBOOT_DISCONNECT == status) {
					f_connect = 1;
					printf("Fastboot disconnect detected\n");
					fboot_lcd_status("disconnect...");
					break;
				}
				else if ((FASTBOOT_INACTIVE == status) && timeout) {
					if (curr_time >= end_time) {
						printf("Fastboot inactivity detected\n");
						fboot_lcd_status("inactivity...");
						break;
					}
				}
				else {
					/* Something happened */
					/* Update the timeout endtime */
					if (timeout &&
						end_time != (curr_time+timeout)) {
						debug("Fastboot update inactive timeout (%d->", end_time);
						end_time = curr_time;
						end_time += timeout;
						debug("%d)\n", end_time);
					}
				}
			} /* while (1) */
		}

		fastboot_shutdown();
		mdelay(10);

	} while (f_connect);

	fboot_lcd_stop();
	return 0;
}


U_BOOT_CMD(
	fastboot,	4,	1,	do_fastboot,
	"fastboot- use USB Fastboot protocol\n",
	"[inactive timeout]\n"
	"    - Run as a fastboot usb device.\n"
	"    - The optional inactive timeout is the decimal seconds before\n"
	"    - the normal console resumes\n"
	"fastboot -n [inactive timeout]\n"
	"    - reflect new partition environments and Run.\n"
	"fastboot -1 \n"
	"    - Print current fastboot partition map table.\n"
	"fastboot -p \n"
	"    - Print current fastboot support device list.\n"
);



