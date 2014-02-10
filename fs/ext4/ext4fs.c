/*
 * (C) Copyright 2011 - 2012 Samsung Electronics
 * EXT4 filesystem implementation in Uboot by
 * Uma Shankar <uma.shankar@samsung.com>
 * Manjunatha C Achar <a.manjunatha@samsung.com>
 *
 * ext4ls and ext4load : Based on ext2 ls and load support in Uboot.
 *		       Ext4 read optimization taken from Open-Moko
 *		       Qi bootloader
 *
 * (C) Copyright 2004
 * esd gmbh <www.esd-electronics.com>
 * Reinhard Arlt <reinhard.arlt@esd-electronics.com>
 *
 * based on code from grub2 fs/ext2.c and fs/fshelp.c by
 * GRUB  --  GRand Unified Bootloader
 * Copyright (C) 2003, 2004  Free Software Foundation, Inc.
 *
 * ext4write : Based on generic ext4 protocol.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <common.h>
#include <malloc.h>
#include <ext_common.h>
#include <ext4fs.h>
#include <linux/stat.h>
#include <linux/time.h>
#include <asm/byteorder.h>
#include <div64.h>
#include "ext4_common.h"

int ext4fs_symlinknest;
struct ext_filesystem ext_fs;

struct ext_filesystem *get_fs(void)
{
	return &ext_fs;
}

void ext4fs_free_node(struct ext2fs_node *node, struct ext2fs_node *currroot)
{
	if ((node != &ext4fs_root->diropen) && (node != currroot))
		free(node);
}

/*
 * Taken from openmoko-kernel mailing list: By Andy green
 * Optimized read file API : collects and defers contiguous sector
 * reads into one potentially more efficient larger sequential read action
 */
int ext4fs_read_file(struct ext2fs_node *node, int pos,
		unsigned int len, char *buf)
{
	int i;
	int blockcnt;
	int log2blocksize = LOG2_EXT2_BLOCK_SIZE(node->data);
	int blocksize = 1 << (log2blocksize + DISK_SECTOR_BITS);
	unsigned int filesize = __le32_to_cpu(node->inode.size);
	int previous_block_number = -1;
	int delayed_start = 0;
	int delayed_extent = 0;
	int delayed_skipfirst = 0;
	int delayed_next = 0;
	char *delayed_buf = NULL;
	short status;

	/* Adjust len so it we can't read past the end of the file. */
	if (len > filesize)
		len = filesize;

	blockcnt = ((len + pos) + blocksize - 1) / blocksize;

	for (i = pos / blocksize; i < blockcnt; i++) {
		int blknr;
		int blockoff = pos % blocksize;
		int blockend = blocksize;
		int skipfirst = 0;
		blknr = read_allocated_block(&(node->inode), i);
		if (blknr < 0)
			return -1;

		blknr = blknr << log2blocksize;

		/* Last block.  */
		if (i == blockcnt - 1) {
			blockend = (len + pos) % blocksize;

			/* The last portion is exactly blocksize. */
			if (!blockend)
				blockend = blocksize;
		}

		/* First block. */
		if (i == pos / blocksize) {
			skipfirst = blockoff;
			blockend -= skipfirst;
		}
		if (blknr) {
			int status;

			if (previous_block_number != -1) {
				if (delayed_next == blknr) {
					delayed_extent += blockend;
					delayed_next += blockend >> SECTOR_BITS;
				} else {	/* spill */
					status = ext4fs_devread(delayed_start,
							delayed_skipfirst,
							delayed_extent,
							delayed_buf);
					if (status == 0)
						return -1;
					previous_block_number = blknr;
					delayed_start = blknr;
					delayed_extent = blockend;
					delayed_skipfirst = skipfirst;
					delayed_buf = buf;
					delayed_next = blknr +
						(blockend >> SECTOR_BITS);
				}
			} else {
				previous_block_number = blknr;
				delayed_start = blknr;
				delayed_extent = blockend;
				delayed_skipfirst = skipfirst;
				delayed_buf = buf;
				delayed_next = blknr +
					(blockend >> SECTOR_BITS);
			}
		} else {
			if (previous_block_number != -1) {
				/* spill */
				status = ext4fs_devread(delayed_start,
							delayed_skipfirst,
							delayed_extent,
							delayed_buf);
				if (status == 0)
					return -1;
				previous_block_number = -1;
			}
			memset(buf, 0, blocksize - skipfirst);
		}
		buf += blocksize - skipfirst;
	}
	if (previous_block_number != -1) {
		/* spill */
		status = ext4fs_devread(delayed_start,
					delayed_skipfirst, delayed_extent,
					delayed_buf);
		if (status == 0)
			return -1;
		previous_block_number = -1;
	}

	return len;
}

int ext4fs_ls(const char *dirname)
{
	struct ext2fs_node *dirnode;
	int status;

	if (dirname == NULL)
		return 0;

	status = ext4fs_find_file(dirname, &ext4fs_root->diropen, &dirnode,
				  FILETYPE_DIRECTORY);
	if (status != 1) {
		printf("** Can not find directory. **\n");
		return 1;
	}

	ext4fs_iterate_dir(dirnode, NULL, NULL, NULL);
	ext4fs_free_node(dirnode, &ext4fs_root->diropen);

	return 0;
}

int ext4fs_read(char *buf, unsigned len)
{
	if (ext4fs_root == NULL || ext4fs_file == NULL)
		return 0;

	return ext4fs_read_file(ext4fs_file, 0, len, buf);
}

#if defined(CONFIG_EXT4_WRITE)
static void ext4fs_update(void)
{
	short i;
	ext4fs_update_journal();
	struct ext_filesystem *fs = get_fs();

	/* update  super block */
	put_ext4((uint64_t)(SUPERBLOCK_SIZE),
		 (struct ext2_sblock *)fs->sb, (uint32_t)SUPERBLOCK_SIZE);

	/* update block groups */
	for (i = 0; i < fs->no_blkgrp; i++) {
		fs->bgd[i].bg_checksum = ext4fs_checksum_update(i);
		put_ext4((uint64_t)(fs->bgd[i].block_id * fs->blksz),
			 fs->blk_bmaps[i], fs->blksz);
	}

	/* update inode table groups */
	for (i = 0; i < fs->no_blkgrp; i++) {
		put_ext4((uint64_t) (fs->bgd[i].inode_id * fs->blksz),
			 fs->inode_bmaps[i], fs->blksz);
	}

	/* update the block group descriptor table */
	put_ext4((uint64_t)(fs->gdtable_blkno * fs->blksz),
		 (struct ext2_block_group *)fs->gdtable,
		 (fs->blksz * fs->no_blk_pergdt));

	ext4fs_dump_metadata();

	gindex = 0;
	gd_index = 0;
}

int ext4fs_get_bgdtable(void)
{
	int status;
	int grp_desc_size;
	struct ext_filesystem *fs = get_fs();
	grp_desc_size = sizeof(struct ext2_block_group);
	fs->no_blk_pergdt = (fs->no_blkgrp * grp_desc_size) / fs->blksz;
	if ((fs->no_blkgrp * grp_desc_size) % fs->blksz)
		fs->no_blk_pergdt++;

	/* allocate memory for gdtable */
	fs->gdtable = zalloc(fs->blksz * fs->no_blk_pergdt);
	if (!fs->gdtable)
		return -ENOMEM;
	/* read the group descriptor table */
	status = ext4fs_devread(fs->gdtable_blkno * fs->sect_perblk, 0,
				fs->blksz * fs->no_blk_pergdt, fs->gdtable);
	if (status == 0)
		goto fail;

	if (ext4fs_log_gdt(fs->gdtable)) {
		printf("Error in ext4fs_log_gdt\n");
		return -1;
	}

	return 0;
fail:
	free(fs->gdtable);
	fs->gdtable = NULL;

	return -1;
}

static void delete_single_indirect_block(struct ext2_inode *inode)
{
	struct ext2_block_group *bgd = NULL;
	static int prev_bg_bmap_idx = -1;
	long int blknr;
	int remainder;
	int bg_idx;
	int status;
	unsigned int blk_per_grp = ext4fs_root->sblock.blocks_per_group;
	struct ext_filesystem *fs = get_fs();
	char *journal_buffer = zalloc(fs->blksz);
	if (!journal_buffer) {
		printf("No memory\n");
		return;
	}
	/* get  block group descriptor table */
	bgd = (struct ext2_block_group *)fs->gdtable;

	/* deleting the single indirect block associated with inode */
	if (inode->b.blocks.indir_block != 0) {
		debug("SIPB releasing %u\n", inode->b.blocks.indir_block);
		blknr = inode->b.blocks.indir_block;
		if (fs->blksz != 1024) {
			bg_idx = blknr / blk_per_grp;
		} else {
			bg_idx = blknr / blk_per_grp;
			remainder = blknr % blk_per_grp;
			if (!remainder)
				bg_idx--;
		}
		ext4fs_reset_block_bmap(blknr, fs->blk_bmaps[bg_idx], bg_idx);
		bgd[bg_idx].free_blocks++;
		fs->sb->free_blocks++;
		/* journal backup */
		if (prev_bg_bmap_idx != bg_idx) {
			status =
			    ext4fs_devread(bgd[bg_idx].block_id *
					   fs->sect_perblk, 0, fs->blksz,
					   journal_buffer);
			if (status == 0)
				goto fail;
			if (ext4fs_log_journal
			    (journal_buffer, bgd[bg_idx].block_id))
				goto fail;
			prev_bg_bmap_idx = bg_idx;
		}
	}
fail:
	free(journal_buffer);
}

static void delete_double_indirect_block(struct ext2_inode *inode)
{
	int i;
	short status;
	static int prev_bg_bmap_idx = -1;
	long int blknr;
	int remainder;
	int bg_idx;
	unsigned int blk_per_grp = ext4fs_root->sblock.blocks_per_group;
	unsigned int *di_buffer = NULL;
	unsigned int *DIB_start_addr = NULL;
	struct ext2_block_group *bgd = NULL;
	struct ext_filesystem *fs = get_fs();
	char *journal_buffer = zalloc(fs->blksz);
	if (!journal_buffer) {
		printf("No memory\n");
		return;
	}
	/* get the block group descriptor table */
	bgd = (struct ext2_block_group *)fs->gdtable;

	if (inode->b.blocks.double_indir_block != 0) {
		di_buffer = zalloc(fs->blksz);
		if (!di_buffer) {
			printf("No memory\n");
			return;
		}
		DIB_start_addr = (unsigned int *)di_buffer;
		blknr = inode->b.blocks.double_indir_block;
		status = ext4fs_devread(blknr * fs->sect_perblk, 0, fs->blksz,
					(char *)di_buffer);
		for (i = 0; i < fs->blksz / sizeof(int); i++) {
			if (*di_buffer == 0)
				break;

			debug("DICB releasing %u\n", *di_buffer);
			if (fs->blksz != 1024) {
				bg_idx = (*di_buffer) / blk_per_grp;
			} else {
				bg_idx = (*di_buffer) / blk_per_grp;
				remainder = (*di_buffer) % blk_per_grp;
				if (!remainder)
					bg_idx--;
			}
			ext4fs_reset_block_bmap(*di_buffer,
					fs->blk_bmaps[bg_idx], bg_idx);
			di_buffer++;
			bgd[bg_idx].free_blocks++;
			fs->sb->free_blocks++;
			/* journal backup */
			if (prev_bg_bmap_idx != bg_idx) {
				status = ext4fs_devread(bgd[bg_idx].block_id
							* fs->sect_perblk, 0,
							fs->blksz,
							journal_buffer);
				if (status == 0)
					goto fail;

				if (ext4fs_log_journal(journal_buffer,
							bgd[bg_idx].block_id))
					goto fail;
				prev_bg_bmap_idx = bg_idx;
			}
		}

		/* removing the parent double indirect block */
		blknr = inode->b.blocks.double_indir_block;
		if (fs->blksz != 1024) {
			bg_idx = blknr / blk_per_grp;
		} else {
			bg_idx = blknr / blk_per_grp;
			remainder = blknr % blk_per_grp;
			if (!remainder)
				bg_idx--;
		}
		ext4fs_reset_block_bmap(blknr, fs->blk_bmaps[bg_idx], bg_idx);
		bgd[bg_idx].free_blocks++;
		fs->sb->free_blocks++;
		/* journal backup */
		if (prev_bg_bmap_idx != bg_idx) {
			memset(journal_buffer, '\0', fs->blksz);
			status = ext4fs_devread(bgd[bg_idx].block_id *
						fs->sect_perblk, 0, fs->blksz,
						journal_buffer);
			if (status == 0)
				goto fail;

			if (ext4fs_log_journal(journal_buffer,
						bgd[bg_idx].block_id))
				goto fail;
			prev_bg_bmap_idx = bg_idx;
		}
		debug("DIPB releasing %ld\n", blknr);
	}
fail:
	free(DIB_start_addr);
	free(journal_buffer);
}

static void delete_triple_indirect_block(struct ext2_inode *inode)
{
	int i, j;
	short status;
	static int prev_bg_bmap_idx = -1;
	long int blknr;
	int remainder;
	int bg_idx;
	unsigned int blk_per_grp = ext4fs_root->sblock.blocks_per_group;
	unsigned int *tigp_buffer = NULL;
	unsigned int *tib_start_addr = NULL;
	unsigned int *tip_buffer = NULL;
	unsigned int *tipb_start_addr = NULL;
	struct ext2_block_group *bgd = NULL;
	struct ext_filesystem *fs = get_fs();
	char *journal_buffer = zalloc(fs->blksz);
	if (!journal_buffer) {
		printf("No memory\n");
		return;
	}
	/* get block group descriptor table */
	bgd = (struct ext2_block_group *)fs->gdtable;

	if (inode->b.blocks.triple_indir_block != 0) {
		tigp_buffer = zalloc(fs->blksz);
		if (!tigp_buffer) {
			printf("No memory\n");
			return;
		}
		tib_start_addr = (unsigned int *)tigp_buffer;
		blknr = inode->b.blocks.triple_indir_block;
		status = ext4fs_devread(blknr * fs->sect_perblk, 0, fs->blksz,
					(char *)tigp_buffer);
		for (i = 0; i < fs->blksz / sizeof(int); i++) {
			if (*tigp_buffer == 0)
				break;
			debug("tigp buffer releasing %u\n", *tigp_buffer);

			tip_buffer = zalloc(fs->blksz);
			if (!tip_buffer)
				goto fail;
			tipb_start_addr = (unsigned int *)tip_buffer;
			status = ext4fs_devread((*tigp_buffer) *
						fs->sect_perblk, 0, fs->blksz,
						(char *)tip_buffer);
			for (j = 0; j < fs->blksz / sizeof(int); j++) {
				if (*tip_buffer == 0)
					break;
				if (fs->blksz != 1024) {
					bg_idx = (*tip_buffer) / blk_per_grp;
				} else {
					bg_idx = (*tip_buffer) / blk_per_grp;

					remainder = (*tip_buffer) % blk_per_grp;
					if (!remainder)
						bg_idx--;
				}

				ext4fs_reset_block_bmap(*tip_buffer,
							fs->blk_bmaps[bg_idx],
							bg_idx);

				tip_buffer++;
				bgd[bg_idx].free_blocks++;
				fs->sb->free_blocks++;
				/* journal backup */
				if (prev_bg_bmap_idx != bg_idx) {
					status =
					    ext4fs_devread(
							bgd[bg_idx].block_id *
							fs->sect_perblk, 0,
							fs->blksz,
							journal_buffer);
					if (status == 0)
						goto fail;

					if (ext4fs_log_journal(journal_buffer,
							       bgd[bg_idx].
							       block_id))
						goto fail;
					prev_bg_bmap_idx = bg_idx;
				}
			}
			free(tipb_start_addr);
			tipb_start_addr = NULL;

			/*
			 * removing the grand parent blocks
			 * which is connected to inode
			 */
			if (fs->blksz != 1024) {
				bg_idx = (*tigp_buffer) / blk_per_grp;
			} else {
				bg_idx = (*tigp_buffer) / blk_per_grp;

				remainder = (*tigp_buffer) % blk_per_grp;
				if (!remainder)
					bg_idx--;
			}
			ext4fs_reset_block_bmap(*tigp_buffer,
						fs->blk_bmaps[bg_idx], bg_idx);

			tigp_buffer++;
			bgd[bg_idx].free_blocks++;
			fs->sb->free_blocks++;
			/* journal backup */
			if (prev_bg_bmap_idx != bg_idx) {
				memset(journal_buffer, '\0', fs->blksz);
				status =
				    ext4fs_devread(bgd[bg_idx].block_id *
						   fs->sect_perblk, 0,
						   fs->blksz, journal_buffer);
				if (status == 0)
					goto fail;

				if (ext4fs_log_journal(journal_buffer,
							bgd[bg_idx].block_id))
					goto fail;
				prev_bg_bmap_idx = bg_idx;
			}
		}

		/* removing the grand parent triple indirect block */
		blknr = inode->b.blocks.triple_indir_block;
		if (fs->blksz != 1024) {
			bg_idx = blknr / blk_per_grp;
		} else {
			bg_idx = blknr / blk_per_grp;
			remainder = blknr % blk_per_grp;
			if (!remainder)
				bg_idx--;
		}
		ext4fs_reset_block_bmap(blknr, fs->blk_bmaps[bg_idx], bg_idx);
		bgd[bg_idx].free_blocks++;
		fs->sb->free_blocks++;
		/* journal backup */
		if (prev_bg_bmap_idx != bg_idx) {
			memset(journal_buffer, '\0', fs->blksz);
			status = ext4fs_devread(bgd[bg_idx].block_id *
						fs->sect_perblk, 0, fs->blksz,
						journal_buffer);
			if (status == 0)
				goto fail;

			if (ext4fs_log_journal(journal_buffer,
						bgd[bg_idx].block_id))
				goto fail;
			prev_bg_bmap_idx = bg_idx;
		}
		debug("tigp buffer itself releasing %ld\n", blknr);
	}
fail:
	free(tib_start_addr);
	free(tipb_start_addr);
	free(journal_buffer);
}

static int ext4fs_delete_file(int inodeno)
{
	struct ext2_inode inode;
	short status;
	int i;
	int remainder;
	long int blknr;
	int bg_idx;
	int ibmap_idx;
	char *read_buffer = NULL;
	char *start_block_address = NULL;
	unsigned int no_blocks;

	static int prev_bg_bmap_idx = -1;
	unsigned int inodes_per_block;
	long int blkno;
	unsigned int blkoff;
	unsigned int blk_per_grp = ext4fs_root->sblock.blocks_per_group;
	unsigned int inode_per_grp = ext4fs_root->sblock.inodes_per_group;
	struct ext2_inode *inode_buffer = NULL;
	struct ext2_block_group *bgd = NULL;
	struct ext_filesystem *fs = get_fs();
	char *journal_buffer = zalloc(fs->blksz);
	if (!journal_buffer)
		return -ENOMEM;
	/* get the block group descriptor table */
	bgd = (struct ext2_block_group *)fs->gdtable;
	status = ext4fs_read_inode(ext4fs_root, inodeno, &inode);
	if (status == 0)
		goto fail;

	/* read the block no allocated to a file */
	no_blocks = inode.size / fs->blksz;
	if (inode.size % fs->blksz)
		no_blocks++;

	if (le32_to_cpu(inode.flags) & EXT4_EXTENTS_FL) {
		struct ext2fs_node *node_inode =
		    zalloc(sizeof(struct ext2fs_node));
		if (!node_inode)
			goto fail;
		node_inode->data = ext4fs_root;
		node_inode->ino = inodeno;
		node_inode->inode_read = 0;
		memcpy(&(node_inode->inode), &inode, sizeof(struct ext2_inode));

		for (i = 0; i < no_blocks; i++) {
			blknr = read_allocated_block(&(node_inode->inode), i);
			if (fs->blksz != 1024) {
				bg_idx = blknr / blk_per_grp;
			} else {
				bg_idx = blknr / blk_per_grp;
				remainder = blknr % blk_per_grp;
				if (!remainder)
					bg_idx--;
			}
			ext4fs_reset_block_bmap(blknr, fs->blk_bmaps[bg_idx],
						bg_idx);
			debug("EXT4_EXTENTS Block releasing %ld: %d\n",
			      blknr, bg_idx);

			bgd[bg_idx].free_blocks++;
			fs->sb->free_blocks++;

			/* journal backup */
			if (prev_bg_bmap_idx != bg_idx) {
				status =
				    ext4fs_devread(bgd[bg_idx].block_id *
						   fs->sect_perblk, 0,
						   fs->blksz, journal_buffer);
				if (status == 0)
					goto fail;
				if (ext4fs_log_journal(journal_buffer,
							bgd[bg_idx].block_id))
					goto fail;
				prev_bg_bmap_idx = bg_idx;
			}
		}
		if (node_inode) {
			free(node_inode);
			node_inode = NULL;
		}
	} else {

		delete_single_indirect_block(&inode);
		delete_double_indirect_block(&inode);
		delete_triple_indirect_block(&inode);

		/* read the block no allocated to a file */
		no_blocks = inode.size / fs->blksz;
		if (inode.size % fs->blksz)
			no_blocks++;
		for (i = 0; i < no_blocks; i++) {
			blknr = read_allocated_block(&inode, i);
			if (fs->blksz != 1024) {
				bg_idx = blknr / blk_per_grp;
			} else {
				bg_idx = blknr / blk_per_grp;
				remainder = blknr % blk_per_grp;
				if (!remainder)
					bg_idx--;
			}
			ext4fs_reset_block_bmap(blknr, fs->blk_bmaps[bg_idx],
						bg_idx);
			debug("ActualB releasing %ld: %d\n", blknr, bg_idx);

			bgd[bg_idx].free_blocks++;
			fs->sb->free_blocks++;
			/* journal backup */
			if (prev_bg_bmap_idx != bg_idx) {
				memset(journal_buffer, '\0', fs->blksz);
				status = ext4fs_devread(bgd[bg_idx].block_id
							* fs->sect_perblk,
							0, fs->blksz,
							journal_buffer);
				if (status == 0)
					goto fail;
				if (ext4fs_log_journal(journal_buffer,
						bgd[bg_idx].block_id))
					goto fail;
				prev_bg_bmap_idx = bg_idx;
			}
		}
	}

	/* from the inode no to blockno */
	inodes_per_block = fs->blksz / fs->inodesz;
	ibmap_idx = inodeno / inode_per_grp;

	/* get the block no */
	inodeno--;
	blkno = __le32_to_cpu(bgd[ibmap_idx].inode_table_id) +
		(inodeno % __le32_to_cpu(inode_per_grp)) / inodes_per_block;

	/* get the offset of the inode */
	blkoff = ((inodeno) % inodes_per_block) * fs->inodesz;

	/* read the block no containing the inode */
	read_buffer = zalloc(fs->blksz);
	if (!read_buffer)
		goto fail;
	start_block_address = read_buffer;
	status = ext4fs_devread(blkno * fs->sect_perblk,
				0, fs->blksz, read_buffer);
	if (status == 0)
		goto fail;

	if (ext4fs_log_journal(read_buffer, blkno))
		goto fail;

	read_buffer = read_buffer + blkoff;
	inode_buffer = (struct ext2_inode *)read_buffer;
	memset(inode_buffer, '\0', sizeof(struct ext2_inode));

	/* write the inode to original position in inode table */
	if (ext4fs_put_metadata(start_block_address, blkno))
		goto fail;

	/* update the respective inode bitmaps */
	inodeno++;
	ext4fs_reset_inode_bmap(inodeno, fs->inode_bmaps[ibmap_idx], ibmap_idx);
	bgd[ibmap_idx].free_inodes++;
	fs->sb->free_inodes++;
	/* journal backup */
	memset(journal_buffer, '\0', fs->blksz);
	status = ext4fs_devread(bgd[ibmap_idx].inode_id *
				fs->sect_perblk, 0, fs->blksz, journal_buffer);
	if (status == 0)
		goto fail;
	if (ext4fs_log_journal(journal_buffer, bgd[ibmap_idx].inode_id))
		goto fail;

	ext4fs_update();
	ext4fs_deinit();

	if (ext4fs_init() != 0) {
		printf("error in File System init\n");
		goto fail;
	}

	free(start_block_address);
	free(journal_buffer);

	return 0;
fail:
	free(start_block_address);
	free(journal_buffer);

	return -1;
}

int ext4fs_init(void)
{
	short status;
	int i;
	unsigned int real_free_blocks = 0;
	struct ext_filesystem *fs = get_fs();

	/* populate fs */
	fs->blksz = EXT2_BLOCK_SIZE(ext4fs_root);
	fs->inodesz = INODE_SIZE_FILESYSTEM(ext4fs_root);
	fs->sect_perblk = fs->blksz / SECTOR_SIZE;

	/* get the superblock */
	fs->sb = zalloc(SUPERBLOCK_SIZE);
	if (!fs->sb)
		return -ENOMEM;
	if (!ext4fs_devread(SUPERBLOCK_SECTOR, 0, SUPERBLOCK_SIZE,
			(char *)fs->sb))
		goto fail;

	/* init journal */
	if (ext4fs_init_journal())
		goto fail;

	/* get total no of blockgroups */
	fs->no_blkgrp = (uint32_t)ext4fs_div_roundup(
			(ext4fs_root->sblock.total_blocks -
			ext4fs_root->sblock.first_data_block),
			ext4fs_root->sblock.blocks_per_group);

	/* get the block group descriptor table */
	fs->gdtable_blkno = ((EXT2_MIN_BLOCK_SIZE == fs->blksz) + 1);
	if (ext4fs_get_bgdtable() == -1) {
		printf("Error in getting the block group descriptor table\n");
		goto fail;
	}
	fs->bgd = (struct ext2_block_group *)fs->gdtable;

	/* load all the available bitmap block of the partition */
	fs->blk_bmaps = zalloc(fs->no_blkgrp * sizeof(char *));
	if (!fs->blk_bmaps)
		goto fail;
	for (i = 0; i < fs->no_blkgrp; i++) {
		fs->blk_bmaps[i] = zalloc(fs->blksz);
		if (!fs->blk_bmaps[i])
			goto fail;
	}

	for (i = 0; i < fs->no_blkgrp; i++) {
		status =
		    ext4fs_devread(fs->bgd[i].block_id * fs->sect_perblk, 0,
				   fs->blksz, (char *)fs->blk_bmaps[i]);
		if (status == 0)
			goto fail;
	}

	/* load all the available inode bitmap of the partition */
	fs->inode_bmaps = zalloc(fs->no_blkgrp * sizeof(unsigned char *));
	if (!fs->inode_bmaps)
		goto fail;
	for (i = 0; i < fs->no_blkgrp; i++) {
		fs->inode_bmaps[i] = zalloc(fs->blksz);
		if (!fs->inode_bmaps[i])
			goto fail;
	}

	for (i = 0; i < fs->no_blkgrp; i++) {
		status = ext4fs_devread(fs->bgd[i].inode_id * fs->sect_perblk,
					0, fs->blksz,
					(char *)fs->inode_bmaps[i]);
		if (status == 0)
			goto fail;
	}

	/*
	 * check filesystem consistency with free blocks of file system
	 * some time we observed that superblock freeblocks does not match
	 * with the  blockgroups freeblocks when improper
	 * reboot of a linux kernel
	 */
	for (i = 0; i < fs->no_blkgrp; i++)
		real_free_blocks = real_free_blocks + fs->bgd[i].free_blocks;
	if (real_free_blocks != fs->sb->free_blocks)
		fs->sb->free_blocks = real_free_blocks;

	return 0;
fail:
	ext4fs_deinit();

	return -1;
}

void ext4fs_deinit(void)
{
	int i;
	struct ext2_inode inode_journal;
	struct journal_superblock_t *jsb;
	long int blknr;
	struct ext_filesystem *fs = get_fs();

	/* free journal */
	char *temp_buff = zalloc(fs->blksz);
	if (temp_buff) {
		ext4fs_read_inode(ext4fs_root, EXT2_JOURNAL_INO,
				  &inode_journal);
		blknr = read_allocated_block(&inode_journal,
					EXT2_JOURNAL_SUPERBLOCK);
		ext4fs_devread(blknr * fs->sect_perblk, 0, fs->blksz,
			       temp_buff);
		jsb = (struct journal_superblock_t *)temp_buff;
		jsb->s_start = cpu_to_be32(0);
		put_ext4((uint64_t) (blknr * fs->blksz),
			 (struct journal_superblock_t *)temp_buff, fs->blksz);
		free(temp_buff);
	}
	ext4fs_free_journal();

	/* get the superblock */
	ext4fs_devread(SUPERBLOCK_SECTOR, 0, SUPERBLOCK_SIZE, (char *)fs->sb);
	fs->sb->feature_incompat &= ~EXT3_FEATURE_INCOMPAT_RECOVER;
	put_ext4((uint64_t)(SUPERBLOCK_SIZE),
		 (struct ext2_sblock *)fs->sb, (uint32_t)SUPERBLOCK_SIZE);
	free(fs->sb);
	fs->sb = NULL;

	if (fs->blk_bmaps) {
		for (i = 0; i < fs->no_blkgrp; i++) {
			free(fs->blk_bmaps[i]);
			fs->blk_bmaps[i] = NULL;
		}
		free(fs->blk_bmaps);
		fs->blk_bmaps = NULL;
	}

	if (fs->inode_bmaps) {
		for (i = 0; i < fs->no_blkgrp; i++) {
			free(fs->inode_bmaps[i]);
			fs->inode_bmaps[i] = NULL;
		}
		free(fs->inode_bmaps);
		fs->inode_bmaps = NULL;
	}


	free(fs->gdtable);
	fs->gdtable = NULL;
	fs->bgd = NULL;
	/*
	 * reinitiliazed the global inode and
	 * block bitmap first execution check variables
	 */
	fs->first_pass_ibmap = 0;
	fs->first_pass_bbmap = 0;
	fs->curr_inode_no = 0;
	fs->curr_blkno = 0;
}

static int ext4fs_write_file(struct ext2_inode *file_inode,
			     int pos, unsigned int len, char *buf)
{
	int i;
	int blockcnt;
	int log2blocksize = LOG2_EXT2_BLOCK_SIZE(ext4fs_root);
	unsigned int filesize = __le32_to_cpu(file_inode->size);
	struct ext_filesystem *fs = get_fs();
	int previous_block_number = -1;
	int delayed_start = 0;
	int delayed_extent = 0;
	int delayed_next = 0;
	char *delayed_buf = NULL;

	/* Adjust len so it we can't read past the end of the file. */
	if (len > filesize)
		len = filesize;

	blockcnt = ((len + pos) + fs->blksz - 1) / fs->blksz;

	for (i = pos / fs->blksz; i < blockcnt; i++) {
		long int blknr;
		int blockend = fs->blksz;
		int skipfirst = 0;
		blknr = read_allocated_block(file_inode, i);
		if (blknr < 0)
			return -1;

		blknr = blknr << log2blocksize;

		if (blknr) {
			if (previous_block_number != -1) {
				if (delayed_next == blknr) {
					delayed_extent += blockend;
					delayed_next += blockend >> SECTOR_BITS;
				} else {	/* spill */
					put_ext4((uint64_t) (delayed_start *
							     SECTOR_SIZE),
						 delayed_buf,
						 (uint32_t) delayed_extent);
					previous_block_number = blknr;
					delayed_start = blknr;
					delayed_extent = blockend;
					delayed_buf = buf;
					delayed_next = blknr +
					    (blockend >> SECTOR_BITS);
				}
			} else {
				previous_block_number = blknr;
				delayed_start = blknr;
				delayed_extent = blockend;
				delayed_buf = buf;
				delayed_next = blknr +
				    (blockend >> SECTOR_BITS);
			}
		} else {
			if (previous_block_number != -1) {
				/* spill */
				put_ext4((uint64_t) (delayed_start *
						     SECTOR_SIZE), delayed_buf,
					 (uint32_t) delayed_extent);
				previous_block_number = -1;
			}
			memset(buf, 0, fs->blksz - skipfirst);
		}
		buf += fs->blksz - skipfirst;
	}
	if (previous_block_number != -1) {
		/* spill */
		put_ext4((uint64_t) (delayed_start * SECTOR_SIZE),
			 delayed_buf, (uint32_t) delayed_extent);
		previous_block_number = -1;
	}

	return len;
}

int ext4fs_write(const char *fname, unsigned char *buffer,
					unsigned long sizebytes)
{
	int ret = 0;
	struct ext2_inode *file_inode = NULL;
	unsigned char *inode_buffer = NULL;
	int parent_inodeno;
	int inodeno;
	time_t timestamp = 0;

	uint64_t bytes_reqd_for_file;
	unsigned int blks_reqd_for_file;
	unsigned int blocks_remaining;
	int existing_file_inodeno;
	char *temp_ptr = NULL;
	long int itable_blkno;
	long int parent_itable_blkno;
	long int blkoff;
	struct ext2_sblock *sblock = &(ext4fs_root->sblock);
	unsigned int inodes_per_block;
	unsigned int ibmap_idx;
	struct ext_filesystem *fs = get_fs();
	ALLOC_CACHE_ALIGN_BUFFER(char, filename, 256);
	memset(filename, 0x00, sizeof(filename));

	g_parent_inode = zalloc(sizeof(struct ext2_inode));
	if (!g_parent_inode)
		goto fail;

	if (ext4fs_init() != 0) {
		printf("error in File System init\n");
		return -1;
	}
	inodes_per_block = fs->blksz / fs->inodesz;
	parent_inodeno = ext4fs_get_parent_inode_num(fname, filename, F_FILE);
	if (parent_inodeno == -1)
		goto fail;
	if (ext4fs_iget(parent_inodeno, g_parent_inode))
		goto fail;
	/* check if the filename is already present in root */
	existing_file_inodeno = ext4fs_filename_check(filename);
	if (existing_file_inodeno != -1) {
		ret = ext4fs_delete_file(existing_file_inodeno);
		fs->first_pass_bbmap = 0;
		fs->curr_blkno = 0;

		fs->first_pass_ibmap = 0;
		fs->curr_inode_no = 0;
		if (ret)
			goto fail;
	}
	/* calucalate how many blocks required */
	bytes_reqd_for_file = sizebytes;
	blks_reqd_for_file = lldiv(bytes_reqd_for_file, fs->blksz);
	if (do_div(bytes_reqd_for_file, fs->blksz) != 0) {
		blks_reqd_for_file++;
		debug("total bytes for a file %u\n", blks_reqd_for_file);
	}
	blocks_remaining = blks_reqd_for_file;
	/* test for available space in partition */
	if (fs->sb->free_blocks < blks_reqd_for_file) {
		printf("Not enough space on partition !!!\n");
		goto fail;
	}

	ext4fs_update_parent_dentry(filename, &inodeno, FILETYPE_REG);
	/* prepare file inode */
	inode_buffer = zalloc(fs->inodesz);
	if (!inode_buffer)
		goto fail;
	file_inode = (struct ext2_inode *)inode_buffer;
	file_inode->mode = S_IFREG | S_IRWXU |
	    S_IRGRP | S_IROTH | S_IXGRP | S_IXOTH;
	/* ToDo: Update correct time */
	file_inode->mtime = timestamp;
	file_inode->atime = timestamp;
	file_inode->ctime = timestamp;
	file_inode->nlinks = 1;
	file_inode->size = sizebytes;

	/* Allocate data blocks */
	ext4fs_allocate_blocks(file_inode, blocks_remaining,
			       &blks_reqd_for_file);
	file_inode->blockcnt = (blks_reqd_for_file * fs->blksz) / SECTOR_SIZE;

	temp_ptr = zalloc(fs->blksz);
	if (!temp_ptr)
		goto fail;
	ibmap_idx = inodeno / ext4fs_root->sblock.inodes_per_group;
	inodeno--;
	itable_blkno = __le32_to_cpu(fs->bgd[ibmap_idx].inode_table_id) +
			(inodeno % __le32_to_cpu(sblock->inodes_per_group)) /
			inodes_per_block;
	blkoff = (inodeno % inodes_per_block) * fs->inodesz;
	ext4fs_devread(itable_blkno * fs->sect_perblk, 0, fs->blksz, temp_ptr);
	if (ext4fs_log_journal(temp_ptr, itable_blkno))
		goto fail;

	memcpy(temp_ptr + blkoff, inode_buffer, fs->inodesz);
	if (ext4fs_put_metadata(temp_ptr, itable_blkno))
		goto fail;
	/* copy the file content into data blocks */
	if (ext4fs_write_file(file_inode, 0, sizebytes, (char *)buffer) == -1) {
		printf("Error in copying content\n");
		goto fail;
	}
	ibmap_idx = parent_inodeno / ext4fs_root->sblock.inodes_per_group;
	parent_inodeno--;
	parent_itable_blkno = __le32_to_cpu(fs->bgd[ibmap_idx].inode_table_id) +
	    (parent_inodeno %
	     __le32_to_cpu(sblock->inodes_per_group)) / inodes_per_block;
	blkoff = (parent_inodeno % inodes_per_block) * fs->inodesz;
	if (parent_itable_blkno != itable_blkno) {
		memset(temp_ptr, '\0', fs->blksz);
		ext4fs_devread(parent_itable_blkno * fs->sect_perblk,
			       0, fs->blksz, temp_ptr);
		if (ext4fs_log_journal(temp_ptr, parent_itable_blkno))
			goto fail;

		memcpy(temp_ptr + blkoff, g_parent_inode,
			sizeof(struct ext2_inode));
		if (ext4fs_put_metadata(temp_ptr, parent_itable_blkno))
			goto fail;
		free(temp_ptr);
	} else {
		/*
		 * If parent and child fall in same inode table block
		 * both should be kept in 1 buffer
		 */
		memcpy(temp_ptr + blkoff, g_parent_inode,
		       sizeof(struct ext2_inode));
		gd_index--;
		if (ext4fs_put_metadata(temp_ptr, itable_blkno))
			goto fail;
		free(temp_ptr);
	}
	ext4fs_update();
	ext4fs_deinit();

	fs->first_pass_bbmap = 0;
	fs->curr_blkno = 0;
	fs->first_pass_ibmap = 0;
	fs->curr_inode_no = 0;
	free(inode_buffer);
	free(g_parent_inode);
	g_parent_inode = NULL;

	return 0;
fail:
	ext4fs_deinit();
	free(inode_buffer);
	free(g_parent_inode);
	g_parent_inode = NULL;

	return -1;
}
#endif

/*
 * Format device by ext2.
 */
struct super_block {
	__u8 total_inodes[4];
	__u8 total_blocks[4];
	__u8 reserved_blocks[4];
	__u8 free_blocks[4];
	__u8 free_inodes[4];
	__u8 first_data_block[4];
	__u8 log2_block_size[4];
	__u8 log2_fragment_size[4];
	__u8 blocks_per_group[4];
	__u8 fragments_per_group[4];
	__u8 inodes_per_group[4];
	__u8 mtime[4];
	__u8 wtime[4];
	__u8 mnt_count[2];
	__u8 max_mnt_count[2];
	__u8 magic[2];
	__u8 fs_state[2];
	__u8 error_handling[2];
	__u8 minor_revision_level[2];
	__u8 lastcheck[4];
	__u8 checkinterval[4];
	__u8 creator_os[4];
	__u8 revision_level[4];
	__u8 uid_reserved[2];
	__u8 gid_reserved[2];
	__u8 first_inode[4];
	__u8 inode_size[2];
	__u8 block_group_number[2];
	__u8 feature_compatibility[4];
	__u8 feature_incompat[4];
	__u8 feature_ro_compat[4];
	__u8 unique_id[16];
	__u8 volume_name[16];
	__u8 last_mounted_on[64];
	__u8 compression_info[4];
	__u8 prealloc_blk_cnt;
	__u8 prealloc_dir_cnt;
	__u8 reserved_gtd_blk[2];
	__u8 journal_uuid[16];
	__u8 journal_inode_num[4];
	__u8 journal_device[4];
	__u8 orphan_inode_list[4];
	__u8 hash_seed[16];
	__u8 hash_version;
	__u8 journal_backup_type;
	__u8 gtd_size[2];
	__u8 mount_option[4];
	__u8 first_meta_blk[4];
	__u8 mkfs_time[4];
	__u8 journal_blocks[68];
	__u8 blocks_cnt_hi[4];
	__u8 reserved_blk_cnt_hi[4];
	__u8 free_blocks_hi[4];
	__u8 min_extra_isize[2];
	__u8 want_extra_isize[2];
	__u8 flags[4];
	__u8 raid_stride[2];
	__u8 mmp_interval[2];
	__u8 mmp_block[8];
	__u8 raid_stripe_width[4];
};

struct group_desc_table {
	__u8 start_blkbit_addr[4];
	__u8 start_indbit_addr[4];
	__u8 start_inode_table[4];
	__u8 free_blk_cnt[2];
	__u8 free_inode_cnt[2];
	__u8 directories_cnt[2];
	__u8 padding[2];
	__u8 reserved[12];
};

struct inode_desc {
	__u8 file_mode[2];
	__u8 uid[2];
	__u8 size_byte[4];
	__u8 access_time[4];
	__u8 change_time[4];
	__u8 modify_time[4];
	__u8 deletion_time[4];
	__u8 group_id[2];
	__u8 link_count[2];
	__u8 block_count[4];
	__u8 flags[4];
	__u8 os_description1[4];
	__u8 block_pointers[60];
	__u8 generation_num[4];
	__u8 file_acl[4];
	__u8 directory_acl[4];
	__u8 address_fragmentation[4];
	__u8 os_description2[12];
};

#define mk1(p, x)	\
 	(p) = (__u8)(x)

#define mk2(p, x)		\
	(p)[0] = (__u8)(x),	\
	(p)[1] = (__u8)((x) >> 010)

#define mk4(p, x)			\
	(p)[0] = (__u8)(x),		\
	(p)[1] = (__u8)((x) >> 010),	\
	(p)[2] = (__u8)((x) >> 020),	\
	(p)[3] = (__u8)((x) >> 030)

#define SEC_PER_BLOCK	8

unsigned int get_random_val(unsigned int next)
{
	next = next * 1103515245 + 12345;
	return((unsigned)(next/65536) % 65536);
}

unsigned int default_journal_size(uint32_t block_cnt)
{
	if (block_cnt < 2048)
		return -1;
	if (block_cnt < 32768)
		return (1024);
	if (block_cnt < 256 * 1024)
		return (4096);
	if (block_cnt < 512 * 1024)
		return (8192);
	if (block_cnt < 1024 * 1024)
		return (16384);
	return (32768);
}

int ext2fs_format(block_dev_desc_t *dev_desc, int part_no, char set_journaling)
{
	
	unsigned char buffer[SECTOR_SIZE];
	disk_partition_t info;

	if (!dev_desc->block_read)
		return -1;

	/* check if we have a MBR (on floppies we have only a PBR) */
	if (dev_desc->block_read (dev_desc->dev, 0, 1, (ulong *) buffer) != 1) {
		printf ("** Can't read from device %d **\n", dev_desc->dev);
		return -1;
	}
	if (buffer[0x1fe] != 0x55 ||
		buffer[0x1fe + 1] != 0xaa) {
		printf("** MBR is broken **\n");
		/* no signature found */
		return -1;
	}

	/* First we assume, there is a MBR */
	if (!get_partition_info (dev_desc, part_no, &info)) {
		printf ("** Partition%d is not ext2 file-system %d **\n",
				part_no, dev_desc->dev);
	}

	printf("Partition%d: Start Address(0x%x), Size(0x%x)\n", part_no, info.start, info.size);

	/* Write Super Block */
	int blk_group_num = info.size / 0x40000;
	if (info.size % 0x40000)
		blk_group_num += 1;
	
	int bgcnt;
	uint32_t start_block = 0;
	uint8_t *img; /* Super Block Image */
	uint32_t uuid[4];
	img = malloc(sizeof(__u8) * 4096);
	if (img == NULL) {
		printf("Can't make img buffer~~!!\n");
		return -1;
	}
	uint8_t *img2; /* Group Descriptor Table */
	img2 = malloc(sizeof(__u8) * 4096);
	if (img2 == NULL) {
		printf("Can't make img2 buffer~~!!\n");
		return -1;
	}
	uint8_t *reserve_img;
	reserve_img = malloc(sizeof(__u8) * 4096);
	if (reserve_img == NULL) {
		printf("Can't make reserve_img buffer~~!!\n");
		return -1;
	}

	uint8_t *img3; /* Block Bitmap */
	img3 = malloc(sizeof(__u8) * 4096);
	if (img3 == NULL) {
		printf("Can't make img3 buffer~~!!\n");
		return -1;
	}
	uint8_t *img4;
	img4 = malloc(sizeof(__u8) * 4096);
	if (img4 == NULL) {
		printf("Can't make img4 buffer~~!!\n");
		return -1;
	}
	uint8_t *zerobuf = malloc(sizeof(__u8) * 4096 * 80);
	if (zerobuf == NULL) {
		printf("Can't make zero buffer~~!!\n");
		return -1;
	}

	uint8_t *img5;
	img5 = malloc(sizeof(__u8) * 2048);
	if (img5 == NULL) {
		printf("Can't make img5 buffer~~!!\n");
		return -1;
	}
	uint8_t *rootdata = malloc(sizeof(__u8) * 4096);
	if (rootdata == NULL) {
		printf("Can't make rootdata buffer~~!!\n");
		return -1;
	}
	uint8_t *inode_data = malloc(sizeof(__u8) * 4096);
	if (inode_data == NULL) {
		printf("Can't make inodedata buffer~~!!\n");
		return -1;
	}

	uint8_t *inode_data2 = malloc(sizeof(__u8) * 4096);
	if (inode_data2 == NULL) {
		printf("Can't make inodedata2 buffer~~!!\n");
		return -1;
	}

	printf("Start ext2format...\n");
	for (bgcnt = 0; bgcnt < blk_group_num; bgcnt++) {
		printf("Wirte %d/%dblock-group\n",bgcnt,  blk_group_num);
		int i = 0;
		int rsv_blk_cnt = 0;
		struct super_block *sblock;
		memset(img, 0x0, sizeof(__u8) * 4096);
		if (bgcnt == 0)
			sblock = (struct super_block *)(img + 1024);
		else
			sblock = (struct super_block *)img;

		uint32_t total_blocks = info.size / SEC_PER_BLOCK; /* 4KB block */
		mk4(sblock->total_blocks, total_blocks); /* block count */
		uint32_t total_inodes = (total_blocks / 0x20);
		if (total_blocks % 0x20) {
			total_inodes += 1;
			/* Fix Me... it's just temp code */
			/* Check... 'total_blocks % 0x20'*/
			if (total_blocks > 0x8000 && total_blocks < 0x10000) {
				total_inodes += 1;
			}
		}
		total_inodes *= 0x20;
			
		if (total_blocks > 0x3E800) /* if size > 1GB */
			total_inodes /= 2;
		
		uint32_t inodes_per_group = ((total_inodes -1) / blk_group_num) + 1;
		inodes_per_group = (inodes_per_group / 0x20) + 1;
		inodes_per_group *= 0x20;

		if (total_blocks > 0x8000)
			mk4(sblock->inodes_per_group, inodes_per_group); /* inode for group */
		else
			mk4(sblock->inodes_per_group, total_inodes); /* inode for group */
			
		uint32_t reserved_blocks = total_blocks * 5 / 100;
		mk4(sblock->reserved_blocks, reserved_blocks); /* reserved block count */
		mk4(sblock->first_data_block, 0); /* frist data block */
		mk4(sblock->log2_block_size, 2); /* log block size */
		mk4(sblock->log2_fragment_size, 2); /* log fragmentation size */
		/* 8(1bit) * 4096(1block:4K) */
		mk4(sblock->blocks_per_group, 0x8000); /* block per group */
		uint32_t blocks_per_group = 0x8000;
		mk4(sblock->fragments_per_group, 0x8000); /* fragmentation per group */
		mk4(sblock->mtime, 0); /* mtime */
		mk4(sblock->wtime, 0x33); /* wtime : 0x33 means nothing*/
		mk2(sblock->mnt_count, 0); /* mount count*/
		mk2(sblock->max_mnt_count, 0x20); /* max mount count */
		mk2(sblock->magic, EXT2_MAGIC); /* magic signature */
		mk2(sblock->fs_state, 1); /* state (1:valid, 2:error) */
		mk2(sblock->error_handling, 1); /* errors */
		mk2(sblock->minor_revision_level, 0); /* minor version */
		mk4(sblock->lastcheck, 0); /* last check time */
		mk4(sblock->checkinterval, 0xed4e00); /* check interval */
		mk4(sblock->creator_os, 0); /* creator os */
		mk4(sblock->revision_level, 1); /* major version */
		mk2(sblock->uid_reserved, 0); /* UID that can use reserved blocks */
		mk2(sblock->gid_reserved, 0); /* GID that can use reserved blocks */
		mk4(sblock->first_inode, 0xb); /* first non-reserved inode */
		mk2(sblock->inode_size, 0x80); /* inode structure size */
		mk2(sblock->block_group_number, 0); /* block group num */
		uint32_t feature_compatibility = 0x30; /* default */
		if (set_journaling) {
			feature_compatibility += 0x4; /* 0x4 means HAS_JOURNAL */
		}
		mk4(sblock->feature_compatibility, feature_compatibility); /* compatible feature */
		mk4(sblock->feature_incompat, 0x2); /* incompatible feature */
		mk4(sblock->feature_ro_compat, 0x3); /* read-only feature */
		if (bgcnt == 0) {
			/* Set Unique ID */
			uuid[0] = get_random_val(get_timer(0));
			uuid[0] |= (get_random_val(get_timer(0)) << 16);
			uuid[1] = get_random_val(get_timer(0));
			uuid[1] |= (get_random_val(get_timer(0)) << 16);
			uuid[2] = get_random_val(get_timer(0));
			uuid[2] |= (get_random_val(get_timer(0)) << 16);
			uuid[3] = get_random_val(get_timer(0));
			uuid[3] |= (get_random_val(get_timer(0)) << 16);
		}
		mk4(sblock->unique_id, uuid[0]); /* UUID filesystem ID */
		mk4(sblock->unique_id + 4, uuid[1]); /* UUID filesystem ID */
		mk4(sblock->unique_id + 8, uuid[2]); /* UUID filesystem ID */
		mk4(sblock->unique_id + 12, uuid[3]); /* UUID filesystem ID */
		mk4(sblock->compression_info, 0); /* algorithm usage bitmap */
	
		uint32_t reserved_gdt_blk = total_blocks >> 12;
		mk2(sblock->reserved_gtd_blk, reserved_gdt_blk); /* reserved gtd blocks */
		if (set_journaling) {
			/* 8th inode is reserved for journaling */
			mk4(sblock->journal_inode_num, 8);
			sblock->journal_backup_type = 1;
		}
		mk2(sblock->hash_seed, get_random_val(get_timer(0))); /* Hash Seed */
		mk2(sblock->hash_seed + 2, get_random_val(get_timer(0)));
		mk2(sblock->hash_seed + 4, get_random_val(get_timer(0)));
		mk2(sblock->hash_seed + 6, get_random_val(get_timer(0)));
		mk2(sblock->hash_seed + 8, get_random_val(get_timer(0)));
		mk2(sblock->hash_seed + 10, get_random_val(get_timer(0)));
		mk2(sblock->hash_seed + 12, get_random_val(get_timer(0)));
		mk2(sblock->hash_seed + 14, get_random_val(get_timer(0)));
		sblock->hash_version = 0x2;
		mk4(sblock->mkfs_time, 0x33); /* 0x33 means nothing */

		/* Write Group Descriptor Table */
		struct group_desc_table *desc_table;
		memset(img2, 0x0, sizeof(__u8) * 4096);
	
		int offset = 0;
		uint32_t free_blk_cnt = 0;
		uint32_t free_inode_cnt = 0;
		uint32_t free_blocks = 0;
		uint32_t free_inodes = 0;
		uint32_t start_blkbit_addr = 0;
		uint32_t start_indbit_addr = 0;
		uint32_t start_indtable_addr = 0;
	
		uint32_t current_free_blk = 0;
		uint32_t reserved_blk_cnt = 0;
		uint32_t normal_used_blk = 0;
		uint32_t default_used_blk = 0;
		uint32_t first_inode_table_addr = 0;
		uint32_t current_blkbit_addr = 0;
		uint32_t current_indbit_addr = 0;
		uint32_t current_indtable_addr= 0;

		char have_rev_gdt = 0;
		int remain_blocks = 0;
		int jnl_blocks = 0;
		int32_t jnldata_start_addr = 0;
		/* second group inode table offset */
		int32_t seblk_offset = 0;
		/* Starting block address of block bitmap */
		for (i = 0; i < blk_group_num; i++) {
			char is_revgdt = 0;
			/* 0 group description table need to change for default
			 * directory.
			 */
			start_blkbit_addr = i * blocks_per_group;
			start_indbit_addr = 1 + i * blocks_per_group;
			start_indtable_addr = 2 + i * blocks_per_group;
			desc_table = (struct group_desc_table *)(img2 + offset);	
			
			/* last block is not full size */
			if (i == blk_group_num - 1) {
				//uint32_t remain_blocks = total_blocks -
				//			(blocks_per_group * i);
				uint32_t remain_blocks = total_blocks % 0x8000;
				free_blk_cnt = remain_blocks - 4 -
					(inodes_per_group * 128 / 4096) + 2;
				free_inode_cnt = inodes_per_group;
			} else {
				/* 
				 * 0x8000 : total blocks per group
				 * 4 : super block(1), group descriptor table(1),
				 *     block bitmap(1), inode bitmap(1)
				 * (inodes_per_group * 128 / 4096) : inode table
				 * 2 : ?? (block start from 0??)
				 *     reserved block include superblock&gdt
				 */
				free_blk_cnt = 0x8000 - 4 - (inodes_per_group * 128 / 4096) + 2;
				free_inode_cnt = inodes_per_group;
			}
	
			if (i == 0) {
				is_revgdt = 1;
				reserved_blk_cnt++;
				/* For default creation */
				free_blk_cnt -= 6;
				if (total_blocks > 0x8000) /* if size > 128MB */
					free_inode_cnt -= 0xb;
				else
					free_inode_cnt -= 0x2b;
				
				start_blkbit_addr += reserved_gdt_blk + 2;
				start_indbit_addr += reserved_gdt_blk + 2;
				start_indtable_addr += reserved_gdt_blk + 2;
				free_blk_cnt -= (reserved_gdt_blk + 2);

				first_inode_table_addr = start_indtable_addr;
			}

			/* delete journaling block from free blocks */
			if (set_journaling) {
				if (i == 0) {
					jnl_blocks = default_journal_size(total_blocks);
					/* 1024 means number of pointing by 1blcok (4096/4) */
					jnl_blocks += (jnl_blocks / 1024);
					if (jnl_blocks >= 4096) {
						/* 
						 * 2 blocks are needed to use double
						 * indirect pointer
						 */
						jnl_blocks += 2;
					}
					printf("Reserved blocks for jounaling : %d\n", jnl_blocks);
					if (free_blk_cnt >= jnl_blocks) {
						free_blk_cnt -= jnl_blocks;
					}
					else {
						remain_blocks = jnl_blocks -
							free_blk_cnt;
						free_blk_cnt = 0;
					}
				} else {
					
					if (remain_blocks > 0) {
						free_blk_cnt -= remain_blocks;
						remain_blocks = 0;
					}
				}
			}
	
			/* 
			 * Keep reserved region.???
			 * If card is higher than 16GB, below codes must be fixed. 
			 * Please excute mke2fs on linux and check messages 
			 * 'superblock backups stored on blocks:'. 
			 */
			if (i < 10) {
				if (i % 2) {
					is_revgdt  = 1;
					reserved_blk_cnt++;
					start_blkbit_addr += reserved_gdt_blk + 2;
					start_indbit_addr += reserved_gdt_blk + 2;
					start_indtable_addr += reserved_gdt_blk + 2;
					free_blk_cnt -= (reserved_gdt_blk + 2);
					rsv_blk_cnt++;
				}
			} else if (i > 20 && i < 30) {
				if (i == 25 || i == 27) {
					is_revgdt = 1;
					reserved_blk_cnt++;
					start_blkbit_addr += reserved_gdt_blk + 2;
					start_indbit_addr += reserved_gdt_blk + 2;
					start_indtable_addr += reserved_gdt_blk + 2;
					free_blk_cnt -= (reserved_gdt_blk + 2);
					rsv_blk_cnt++;
				}
			} else if (i > 40 && i < 50) {
				if (i == 49) {
					is_revgdt = 1;
					reserved_blk_cnt++;
					start_blkbit_addr += reserved_gdt_blk + 2;
					start_indbit_addr += reserved_gdt_blk + 2;
					start_indtable_addr += reserved_gdt_blk + 2;
					free_blk_cnt -= (reserved_gdt_blk + 2);
					rsv_blk_cnt++;
				}
			}
	
			free_blocks += free_blk_cnt;
			free_inodes += free_inode_cnt;
	
			mk4(desc_table->start_blkbit_addr, start_blkbit_addr);
			mk4(desc_table->start_indbit_addr, start_indbit_addr);
			mk4(desc_table->start_inode_table, start_indtable_addr);
			mk2(desc_table->free_blk_cnt, free_blk_cnt);
			
			if (i == 1) {
				seblk_offset = start_indtable_addr - 0x8000;
			}
			if (i == bgcnt) { //original code
			//if (i == 0) { //first block 
			//if (i == blk_group_num -1) { // last block
				current_free_blk = free_blk_cnt;
				current_blkbit_addr = start_blkbit_addr;
				current_indbit_addr = start_indbit_addr;
				current_indtable_addr = start_indtable_addr;
				if (is_revgdt)
					have_rev_gdt = 1;
				
			}
			mk2(desc_table->free_inode_cnt, free_inode_cnt);
			if (i == 0)
				mk2(desc_table->directories_cnt, 2);
			else
				mk2(desc_table->directories_cnt, 0);
			offset += sizeof(struct group_desc_table);
		}	

		if (set_journaling) {

			/* set journal blocks 
			 * journal data start address =
			 * next of first inode_table block
			 * + (size of inode table)
			 * + 6 (size of root & reserved data region)
			 */
			if (total_blocks > 0x8000) { /* if size > 128MB */
				jnldata_start_addr = first_inode_table_addr
						+ (inodes_per_group * 128 / 4096) + 6;
			}
			else {
				jnldata_start_addr = first_inode_table_addr
						+ (inodes_per_group * 128 / 4096) + 5;
			}

			mk4(sblock->journal_blocks, jnldata_start_addr);
			mk4(sblock->journal_blocks + 4, jnldata_start_addr + 1);
			mk4(sblock->journal_blocks + 8, jnldata_start_addr + 2);
			mk4(sblock->journal_blocks + 12, jnldata_start_addr + 3);
			mk4(sblock->journal_blocks + 16, jnldata_start_addr + 4);
			mk4(sblock->journal_blocks + 20, jnldata_start_addr + 5);
			mk4(sblock->journal_blocks + 24, jnldata_start_addr + 6);
			mk4(sblock->journal_blocks + 28, jnldata_start_addr + 7);
			mk4(sblock->journal_blocks + 32, jnldata_start_addr + 8);
			mk4(sblock->journal_blocks + 36, jnldata_start_addr + 9);
			mk4(sblock->journal_blocks + 40, jnldata_start_addr + 10);
			mk4(sblock->journal_blocks + 44, jnldata_start_addr + 11);
			mk4(sblock->journal_blocks + 48, jnldata_start_addr + 12);
			
			int default_jnl_blocks = default_journal_size(total_blocks);
			uint32_t temp_val;
			if (default_jnl_blocks == 1024)
				temp_val = 0x400000;
			else if (default_jnl_blocks == 4096)
				temp_val = 0x1000000;
			else if (default_jnl_blocks == 8192)
				temp_val = 0x2000000;
			else if (default_jnl_blocks == 16384)
				temp_val = 0x4000000;
			else if (default_jnl_blocks == 32768)
				temp_val = 0x8000000;
			mk4(sblock->journal_blocks + 64, temp_val);

			/* 400 : 0x400(1024) means amount of pointing by 1block
			 *       4096(1block) / 4 = 1024
			 */
			if (default_jnl_blocks != 1024)
				mk4(sblock->journal_blocks + 52, jnldata_start_addr + 0x400 + 13);
		}
		mk4(sblock->free_blocks, free_blocks); /* free block count */
		mk4(sblock->free_inodes, free_inodes); /* free inode count */
		free_inodes += 0x10;
		free_inodes &= 0xfffffff0;
		mk4(sblock->total_inodes, free_inodes); /* inode count */

		/* Write super-block to mmc */
		start_block = info.start + (bgcnt * 0x8000 * 4096 / 512);
		printf("Start write addr : 0x%x\n",start_block);
		if (have_rev_gdt) {
			if (dev_desc->block_write(dev_desc->dev, start_block, 8, (ulong *)img) != 8) {
				printf("Can't write Superblock(%d)~~~!!!\n", bgcnt);
			}
		}

		/* Write Group Descriptor Table */
		if (have_rev_gdt) {
			start_block += 8;
			if (dev_desc->block_write(dev_desc->dev, start_block, 8,
				(ulong *)img2) != 8) {
				printf("Can't write Descriptor Table(%d)~~~!!!\n", bgcnt);
			}
		}

		/* Write reserved region */
		if (bgcnt == 0) {
			memset(reserve_img, 0x0, sizeof(__u8) * 4096);

			int rev_cnt;
			for (rev_cnt = 2; rev_cnt <= reserved_gdt_blk +
				1; rev_cnt++) {
				start_block += 8;
				if (rsv_blk_cnt >= 1)
					mk4(reserve_img, 0x8000 + rev_cnt);
				if (rsv_blk_cnt >= 2)
					mk4(reserve_img + 4, 0x18000 + rev_cnt);
				if (rsv_blk_cnt >= 3)
					mk4(reserve_img + 8, 0x28000 + rev_cnt);
				if (rsv_blk_cnt >= 4)
					mk4(reserve_img + 12, 0x38000 + rev_cnt);
				if (rsv_blk_cnt >= 5)
					mk4(reserve_img + 16, 0x48000 + rev_cnt);
				if (rsv_blk_cnt >= 6)
					mk4(reserve_img + 20, 0xc8000 + rev_cnt);
				if (rsv_blk_cnt >= 7)
					mk4(reserve_img + 24, 0xd8000 + rev_cnt);
				if (rsv_blk_cnt >= 8)
					mk4(reserve_img + 28, 0x188000 + rev_cnt);
				if (dev_desc->block_write(dev_desc->dev, start_block, 8,
					(ulong *)reserve_img) != 8) {
					printf("Can't write reserve(%d)~~~!!!\n", bgcnt);
				}
			}
		}

		/* Write Block Bitmap */
		uint8_t *blk_bitmap;
		int used_blocks;
		int set_full;
		int j;
		int set_val;
		uint32_t bitmap_val;
		int empty_blocks;
	
		
		if (bgcnt != blk_group_num - 1) {
			memset(img3, 0x00, sizeof(__u8) * 4096);

			used_blocks = 0x8000 - current_free_blk;
			set_full = used_blocks / 0x20;
			set_val = used_blocks % 0x20;
			bitmap_val = 0;
	
			for (i = 0;i <= set_full;i++) {
				blk_bitmap = (uint32_t *)(img3 + (4 * i));
				if(i == set_full) {
					for (j = 0;j <= set_val - 1;j++) {
						bitmap_val |= 1 << j;
					}
					mk4(blk_bitmap, bitmap_val);
				} else {
					mk4(blk_bitmap, 0xffffffff);
				}
			}
		} else {
			memset(img3, 0xff, sizeof(__u8) * 4096);

			used_blocks = 0x8000 - (current_free_blk + (0x8000 -
					(total_blocks % 0x8000)));

			set_full = used_blocks / 0x20;
			set_val = used_blocks % 0x20;

			bitmap_val = 0x0;
			//==> write empty blocks
			empty_blocks = (current_free_blk + used_blocks) / 32;
			int temp = (current_free_blk + used_blocks) % 32;
			for (i = 0; i <= empty_blocks; i++) {
				blk_bitmap = (uint32_t *)(img3 + (4 * i));
				if (i == empty_blocks ) {
					for (j = temp;j < 32; j++) {
						bitmap_val |= 1 << j;
					}
					mk4(blk_bitmap, bitmap_val);
				} else {
					mk4(blk_bitmap, 0x00);
				}
			}

			bitmap_val = 0;
			for (i = 0;i <= set_full;i++) {
				blk_bitmap = (uint32_t *)(img3 + (4 * i));
				if(i == set_full) {
					for (j = 0;j <= set_val - 1;j++) {
						bitmap_val |= 1 << j;
					}
					mk4(blk_bitmap, bitmap_val);
				} else {
					mk4(blk_bitmap, 0xffffffff);
				}
			}
		}

		/* Write block bitmap */
		start_block = (current_blkbit_addr * 8) + info.start;

		if (dev_desc->block_write(dev_desc->dev, start_block, 8,
			(ulong *)img3) != 8) {
			printf("Can't write Descriptor Table(%d)~~~!!!\n", bgcnt);
		}

		/* Write inode bitmap */
		uint8_t *inode_bitmap;
	
		memset(img4, 0xff, sizeof(__u8) * 4096);
	
		int empty_inode = inodes_per_group;
		int set_empty = empty_inode / 0x20;
		
		// Fix me~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		if (total_blocks < 0x8000) /* if size < 128MB */
			set_empty -= 1;
		
		for (i = 0;i < set_empty;i++) {
			inode_bitmap = (uint32_t *)(img4 + (4 * i));
			mk4(inode_bitmap, 0);
		}

		if (bgcnt == 0) {
			inode_bitmap = (uint32_t *)img4;
			mk4(inode_bitmap, 0x7ff);
		}

		/* Write inode bitmap */
		start_block = (current_indbit_addr * 8) + info.start;
		if (dev_desc->block_write(dev_desc->dev, start_block, 8,
			(ulong *)img4) != 8) {
			printf("Can't write inode bitmap(%d)~~~!!!\n", bgcnt);
		}
		
		/* Write Inode Table & data */
		start_block = (current_indtable_addr * 8) + info.start;
		//Erase to 0x00 to initialize inode table.....
		memset(zerobuf, 0x00, sizeof(__u8) * 4096 * 80);
		int indt_blknum = (inodes_per_group * 128 / 4096) + 1;
		printf("Erase inode table(%d) - 0x%x", bgcnt, start_block);
		for (j = 0; j < (indt_blknum /80) + 4; j++) { // 2 means root + a 
			start_block += j * 8;
			if (dev_desc->block_write(dev_desc->dev,
				start_block, 640, (ulong *)zerobuf) != 640) {
				printf("Can't erase inode table(%d)~~~!!!\n", bgcnt);
			}
			printf(".");
		}
		printf("\n");

		struct inode_desc *inode;
	
		memset(img5, 0x00, sizeof(__u8) * 2048);
	
		// 1st inode : bad inode
		inode = (struct inode *)img5;
		mk4(inode->access_time, 0x33); /* 0x33 means nothing */
		mk4(inode->change_time, 0x33); /* 0x33 means nothing */
		mk4(inode->modify_time, 0x33); /* 0x33 means nothing */
	
		// 2nd inode : Root inode
		inode = (struct inode *)(img5 + sizeof(struct inode_desc));
		mk2(inode->file_mode, 0x41ed);
		mk4(inode->size_byte, 0x1000);
		mk4(inode->access_time, 0x33); /* 0x33 means nothing */
		mk4(inode->change_time, 0x33); /* 0x33 means nothing */
		mk4(inode->modify_time, 0x33); /* 0x33 means nothing */
		mk2(inode->link_count, 0x3);
		mk2(inode->block_count, 0x8);
		/* root data block addr = 
		 * inode_table_addr + inode table size
		 */
		uint32_t root_data_blk = (inodes_per_group * 128) / 4096;
		if (total_blocks > 0x8000) /* if size > 128MB */
			root_data_blk += first_inode_table_addr; 
		else
			root_data_blk += first_inode_table_addr - 1; 

		mk4(inode->block_pointers, root_data_blk);
		// Write root directory in data region.
		if (bgcnt == 0) {
			/* root data */
			memset(rootdata, 0x00, sizeof(__u8) * 4096);
			mk4(rootdata, 0x2);
			mk4(rootdata + 4, 0x201000c);
			mk4(rootdata + 8, 0x2e);
			mk4(rootdata + 12, 0x2);
			mk4(rootdata + 16, 0x202000c);
			mk4(rootdata + 20, 0x2e2e);
			mk4(rootdata + 24, 0xb);
			mk4(rootdata + 28, 0x20a0fe8);
			mk4(rootdata + 32, 0x74736f6c);
			mk4(rootdata + 36, 0x756f662b);
			mk4(rootdata + 40, 0x646e);
			if (dev_desc->block_write(dev_desc->dev,
				(root_data_blk * 8) + info.start, 8,
				(ulong *)rootdata) != 8) {
				printf("Can't write rootdata~~~!!!\n");
			}

			/* root + 1 data */
			root_data_blk++;
			memset(inode_data, 0x00, sizeof(__u8) * 4096);
			mk4(inode_data, 0xb);
			mk4(inode_data + 4, 0x201000c);
			mk4(inode_data + 8, 0x2e);
			mk4(inode_data + 12, 0x2);
			mk4(inode_data + 16, 0x2020ff4);
			mk4(inode_data + 20, 0x2e2e);
			if (dev_desc->block_write(dev_desc->dev,
				(root_data_blk * 8) + info.start, 8,
				(ulong*)inode_data) != 8) {
				printf("Can't write root+1~~~!!!\n");
			}
			/* root + 2 data */
			root_data_blk++;
			memset(inode_data, 0x00, sizeof(__u8) * 4096);
			mk4(inode_data + 4, 0x1000);
			if (dev_desc->block_write(dev_desc->dev,
				(root_data_blk * 8) + info.start, 8,
				(ulong*)inode_data) != 8) {
				printf("Can't write root+1~~~!!!\n");
			}

			/* root + 3 data */
			root_data_blk++;
			memset(inode_data, 0x00, sizeof(__u8) * 4096);
			mk4(inode_data + 4, 0x1000);
			if (dev_desc->block_write(dev_desc->dev,
				(root_data_blk * 8) + info.start, 8,
				(ulong*)inode_data) != 8) {
				printf("Can't write root+1~~~!!!\n");
			}

			/* root + 4 data */
			root_data_blk++;
			memset(inode_data, 0x00, sizeof(__u8) * 4096);
			mk4(inode_data + 4, 0x1000);
			if (dev_desc->block_write(dev_desc->dev,
				(root_data_blk * 8) + info.start, 8,
				(ulong*)inode_data) != 8) {
				printf("Can't write root+1~~~!!!\n");
			}
		}

		// 3 ~ 6th inode : empty
		// 7th inode : reserved_gdt_blocks
		inode = (struct inode *)(img5 + sizeof(struct inode_desc) * 6);
		mk2(inode->file_mode, 0x8180);
		mk4(inode->size_byte, 0x40c000);
		mk4(inode->access_time, 0x33); /* 0x33 means nothing */
		mk4(inode->change_time, 0x33); /* 0x33 means nothing */
		mk4(inode->modify_time, 0x33); /* 0x33 means nothing */
		mk2(inode->link_count, 0x1);
		uint32_t inode7_cnt;
		// 4096 * 512 ==> changing from ext2block to mmc block....
		inode7_cnt = ((reserved_gdt_blk * reserved_blk_cnt) + 1) * 4096 / 512;
		mk2(inode->block_count, inode7_cnt);
		root_data_blk ++; 
		mk4(inode->block_pointers + 52, root_data_blk);
		mk4(inode->directory_acl, 1);

		// Write 7th inode data
		if (bgcnt == 0) {
			memset(inode_data, 0x00, sizeof(__u8) * 4096);
			for(j=0; j< current_blkbit_addr - 1; j++) {
				if ( j != 0)
					mk4(inode_data + (j * 4), j + 1);
			}
			if (dev_desc->block_write(dev_desc->dev, 
				(root_data_blk * 8) + info.start, 8,
				(ulong*)inode_data) != 8) {
				printf("Can't write 7th inode~~~!!!\n");
			}
		}
		
		if (set_journaling) {
			// 8th inode : reserved for journaling
			inode = (struct inode *)(img5 + sizeof(struct
					inode_desc) * 7);
			mk2(inode->file_mode, 0x8180);
			uint32_t indirect_pointer_addr = 0;
			uint32_t d_indirect_pointer_addr = 0;
			int default_jnl_blocks = default_journal_size(total_blocks);
			uint32_t temp_val;
			if (default_jnl_blocks == 1024)
				temp_val = 0x400000;
			else if (default_jnl_blocks == 4096)
				temp_val = 0x1000000;
			else if (default_jnl_blocks == 8192)
				temp_val = 0x2000000;
			else if (default_jnl_blocks == 16384)
				temp_val = 0x4000000;
			else if (default_jnl_blocks == 32768)
				temp_val = 0x8000000;
			mk4(inode->size_byte, temp_val);
			mk4(inode->change_time, 0x33);
			mk4(inode->modify_time, 0x33);
			mk2(inode->link_count, 0x1);
			// set journaling blocks by 512 per 1block.
			int inode_jnl_blocks = jnl_blocks * 4096 / 512;
			if (total_blocks > 0x8000) /* if size > 128MB */
				mk4(inode->block_count, inode_jnl_blocks);
			else
				mk4(inode->block_count, inode_jnl_blocks + 8);

			// set journaling block address
			mk4(inode->block_pointers, jnldata_start_addr);
			mk4(inode->block_pointers + 4, jnldata_start_addr + 1);
			mk4(inode->block_pointers + 8, jnldata_start_addr + 2);
			mk4(inode->block_pointers + 12, jnldata_start_addr + 3);
			mk4(inode->block_pointers + 16, jnldata_start_addr + 4);
			mk4(inode->block_pointers + 20, jnldata_start_addr + 5);
			mk4(inode->block_pointers + 24, jnldata_start_addr + 6);
			mk4(inode->block_pointers + 28, jnldata_start_addr + 7);
			mk4(inode->block_pointers + 32, jnldata_start_addr + 8);
			mk4(inode->block_pointers + 36, jnldata_start_addr + 9);
			mk4(inode->block_pointers + 40, jnldata_start_addr + 10);
			mk4(inode->block_pointers + 44, jnldata_start_addr + 11);
			indirect_pointer_addr = jnldata_start_addr + 12;
			mk4(inode->block_pointers + 48, indirect_pointer_addr);

			/* If journaling data blocks are bigger than 4096, it
			 * use double indirect pointer (block13)
			 */
			if (default_jnl_blocks >= 4096) {
				/* 400 : 0x400(1024) means amount of pointing by 1block
				 *       4096(1block) / 4 = 1024
				 */
				d_indirect_pointer_addr = jnldata_start_addr
							+ 0x400 + 13;
				mk4(inode->block_pointers + 52,
					d_indirect_pointer_addr);
			}
			// Write 8th inode data
			if (bgcnt == 0) {
				memset(inode_data, 0x00, sizeof(__u8) * 4096);
				mk4(inode_data, 0x98393bc0);
				mk4(inode_data + 4, 0x4000000);
				mk4(inode_data + 12, 0x100000);
				temp_val >>= 4;
				mk4(inode_data + 16, temp_val);
				mk4(inode_data + 20, 0x1000000);
				mk4(inode_data + 24, 0x1000000);
				// set UUID
				mk4(inode_data + 48, uuid[0]);
				mk4(inode_data + 52, uuid[1]);
				mk4(inode_data + 56, uuid[2]);
				mk4(inode_data + 60, uuid[3]);
				mk4(inode_data + 64, 0x1000000);

				//write data to sd/mmc
				if (dev_desc->block_write(dev_desc->dev, 
					(jnldata_start_addr * 8) + info.start, 8,
					(ulong*)inode_data) != 8) {
					printf("Can't write 8th inode~~~!!!\n");
				}

				// Write indirect pointer value
				// 12 means num of default direct pointer
				uint32_t remain_blocks = jnl_blocks - 12;
				memset(inode_data, 0x00, sizeof(__u8) * 4096);
				for (j = 0; j < 1024; j++) {
					mk4(inode_data + (j * 4),
						indirect_pointer_addr + 1 + j);
					remain_blocks--;
					if (remain_blocks == 0)
						break;
				}
				//write data to sd/mmc
				if (dev_desc->block_write(dev_desc->dev, 
					(indirect_pointer_addr * 8) + info.start, 8,
					(ulong*)inode_data) != 8) {
					printf("Can't write 8th inode~~~!!!\n");
				}

				//Write double indirect pointer
				if (remain_blocks != 0) {
					memset(inode_data, 0x00, sizeof(__u8) * 4096);
					int k;
					int needed_blocks = remain_blocks / 1024;
					// Check me==================>
					if (total_blocks > 0x8000) /* if size > 128MB */
						if (remain_blocks % 1024)
							needed_blocks += 1;
					//<==========================
					remain_blocks -= (needed_blocks + 2);

					for (j=0;j < needed_blocks; j++) {
						int now_val =
						d_indirect_pointer_addr + 1 + (j * 0x400) + j;
						mk4(inode_data + (j * 4), now_val);
						
						memset(inode_data2, 0x00, sizeof(__u8) * 4096);
						for (k = 0; k < 1024; k++) {
							uint32_t tmp_val = now_val + 1 + k;
							if (tmp_val >= 0x8000) {
								tmp_val += 
								seblk_offset + (inodes_per_group * 128 / 4096);
							}
							mk4(inode_data2 + (k * 4), tmp_val);
							remain_blocks--;
							if(remain_blocks == 0)
								break;
						}
						//write data to sd/mmc
						if (dev_desc->block_write(dev_desc->dev, 
							(now_val * 8) + info.start, 8,
							(ulong*)inode_data2) != 8) {
							printf("Can't  inodeval~~~!!!\n");
						}
					}
					//write data to sd/mmc
					printf("d_indirect_point:0x%x\n",
						(d_indirect_pointer_addr * 8) + info.start);
					if (dev_desc->block_write(dev_desc->dev, 
						(d_indirect_pointer_addr * 8) + info.start, 8,
						(ulong*)inode_data) != 8) {
						printf("Can't write 8th inode~~~!!!\n");
					}
				}
			}
		}

		// 9  ~ 10th inode : empty
		// 11th inode : first inode without reserved
		//              It point next of root(lost+found)
		inode = (struct inode *)(img5 + sizeof(struct inode_desc) * 10);
		mk2(inode->file_mode, 0x41c0);
		mk4(inode->size_byte, 0x4000);
		mk4(inode->access_time, 0x33); /* 0x33 means nothing */
		mk4(inode->change_time, 0x33); /* 0x33 means nothing */
		mk4(inode->modify_time, 0x33); /* 0x33 means nothing */
		mk2(inode->link_count, 0x2);
		mk2(inode->block_count, 0x20);
		root_data_blk -= 4; 
		mk4(inode->block_pointers, root_data_blk);
		mk4(inode->block_pointers + 4, root_data_blk + 1);
		mk4(inode->block_pointers + 8, root_data_blk + 2);
		mk4(inode->block_pointers + 12, root_data_blk + 3);

		if (bgcnt == 0) {
			/* Write inode table */
			start_block = (current_indtable_addr * 8) + info.start;
			if (dev_desc->block_write(dev_desc->dev, start_block, 4,
				(ulong *)img5) != 4) {
				printf("Can't write inode table(%d)~~~!!!\n", bgcnt);
			}
		}
	}
	free(img);
	free(img2);
	free(reserve_img);
	free(img3);
	free(img4);
	free(zerobuf);
	free(img5);
	free(rootdata);
	free(inode_data);

	return 0;
}
