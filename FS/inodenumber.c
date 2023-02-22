#define _LARGEFILE64_SOURCE
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <ext2fs/ext2_fs.h>
#include <ext2fs/ext2_types.h>

#define DEF_DEV "/dev/sdb"
#define DEF_INODE 2
#define SZ_BOOT_BLK 1024

unsigned int block_size, total_blocks, total_inodes;
unsigned int inodes_per_group, blocks_per_group;
const unsigned int sz_sup_blk = sizeof(struct ext2_super_block), 
      sz_grp_desc = sizeof(struct ext2_group_desc);
unsigned int sz_inode;

void display_super_block(struct ext2_super_block *);
void display_group_desc(struct ext2_group_desc *);
void display_inode(struct ext2_inode *);
bool read_inode(int , unsigned int , struct ext2_inode *);
bool read_group_desc(int , unsigned int , struct ext2_group_desc *);
bool read_super_block(int , struct ext2_super_block *);

bool read_super_block(int fd_dev, struct ext2_super_block *sup_blk)
{
	bool flag;
	ssize_t rd_cnt;
	long int offset;
	offset = (long int) SZ_BOOT_BLK;

	flag = false;
	if (sup_blk)
	{
		if (lseek64(fd_dev, offset, SEEK_SET) != -1)
		{
			if ((rd_cnt = read(fd_dev, sup_blk, sz_sup_blk))
					== sz_sup_blk)
			{
				flag = true;

				block_size = 1024 << sup_blk->s_log_block_size;
				total_inodes = sup_blk->s_inodes_count;
				total_blocks = sup_blk->s_blocks_count;
				inodes_per_group = sup_blk->s_inodes_per_group;
				blocks_per_group = sup_blk->s_blocks_per_group;

				/* printf("read_super_block: rd_cnt = %u\n", rd_cnt); */
			}
			else
				perror("read() : failed to read\n");
		}
		else
			perror("lseek64() : failed to seek\n");
	}
	return flag;
}

bool read_group_desc(int fd_dev, unsigned int idx, struct ext2_group_desc *grp_desc)
{
	bool flag;
	ssize_t rd_cnt;
	long int offset;
	offset = (long int) idx * sz_grp_desc + block_size; 

	flag = false;
	if (grp_desc)
	{
		if (lseek64(fd_dev, offset,
					SEEK_SET) != -1)
		{
			if ((rd_cnt = read(fd_dev, grp_desc,
							sz_grp_desc))
					== sz_grp_desc)
			{
				flag = true;
				/* printf("read_group_desc: rd_cnt = %u\n", rd_cnt); */
			}
			else
				perror("read() : failed to read\n");
		}
		else
			perror("lseek64() : failed to seek\n");
	}
	return flag;
}

bool read_inode(int fd_dev, unsigned int n_inode, struct ext2_inode *inode)
{
	bool flag;
	ssize_t rd_cnt;
	long int g_offset, l_offset;
	struct ext2_group_desc grp_desc;
	unsigned int grp_desc_idx, local_inode_idx;

	flag = false;
	if (inode && fd_dev > 0 
			&& n_inode > 0)
	{
		grp_desc_idx = (n_inode - 1) / inodes_per_group;	
		local_inode_idx = (n_inode - 1) % inodes_per_group;

		/* read the group descriptor required */
		if (read_group_desc(fd_dev, grp_desc_idx, &grp_desc))
		{
			/*
			   printf("grp_desc_idx = %u, local_inode_idx = %u\n",
			   grp_desc_idx, local_inode_idx);
			   display_group_desc(&grp_desc);
			 */

			/* seek to inode table in required block group */
			g_offset = (long int) grp_desc.bg_inode_table * block_size;

			if (lseek64(fd_dev, g_offset, SEEK_SET) != -1)
			{
				/* printf("inode_table_offset = %ld\n", g_offset); */

				/* read the inode by using local inode index */ 
				l_offset = (long int) local_inode_idx * sz_inode;

				if (lseek64(fd_dev, l_offset, SEEK_CUR) != -1)
				{
					/* printf("inode_offset_relative = %ld\n",
					   l_offset); */
					if ((rd_cnt = read(fd_dev, inode,
									sz_inode))
							== sz_inode)
					{
						flag = true;
						/* printf("read_inode: rd_cnt = %u\n", rd_cnt); */
					}
					else
						perror("read() : failed to read\n");
				}
				else
					perror("lseek64() : failed to seek\n");
			}
			else
				perror("lseek64() : failed to seek\n");
		}
		else
			fprintf(stderr, "read_group_desc() : failed to read group descriptor\n");
	}
	return flag;
}

void display_inode(struct ext2_inode *inode)
{
	if (inode)
	{
		printf("===========================================================\n");
		printf("mode		= %10o\n",
				inode->i_mode);
		printf("uid		= %10u\n",
				inode->i_uid);
		printf("gid		= %10u\n",
				inode->i_gid);
		printf("size		= %10u\n",
				inode->i_size);
		printf("atime		= %10x\n",
				inode->i_atime);
		printf("ctime		= %10x\n",
				inode->i_ctime);
		printf("mtime		= %10x\n",
				inode->i_mtime);
		printf("dtime		= %10x\n",
				inode->i_dtime);
		printf("links_count	= %10u\n",
				inode->i_links_count);
		printf("blocks		= %10u\n",
				inode->i_blocks);
		printf("flags		= %10x\n",
				inode->i_flags);
		printf("file_acl	= %10u\n",
				inode->i_file_acl);
		for (int i = 0; i < EXT2_N_BLOCKS; i++)
		{
			printf("block[%2u] 	= %10u\n", 
					i, inode->i_block[i]);
		}
		printf("===========================================================\n");
	}
	return ;
}

void display_group_desc(struct ext2_group_desc *grp_desc)
{
	if (grp_desc)
	{
		printf("===========================================================\n");
		printf("block_bitmap 		= %10u\n", 
				grp_desc->bg_block_bitmap); 
		printf("inode_bitmap 		= %10u\n", 
				grp_desc->bg_inode_bitmap); 
		printf("inode_table 		= %10u\n", 
				grp_desc->bg_inode_table); 
		printf("free_blocks_count 	= %10u\n", 
				grp_desc->bg_free_blocks_count);
		printf("free_inodes_count 	= %10u\n",
				grp_desc->bg_free_inodes_count);
		printf("used_dirs_count 	= %10u\n",
				grp_desc->bg_used_dirs_count);
		printf("===========================================================\n");
	}
	return ;
}

void display_super_block(struct ext2_super_block *sup_blk)
{
	if(sup_blk)
	{
		printf("===========================================================\n");
		printf("magic_signature 	= %10X\n",
				sup_blk->s_magic);
		printf("inodes_count 		= %10u\n",
				sup_blk->s_inodes_count);
		printf("blocks_count 		= %10u\n",
				sup_blk->s_blocks_count);
		printf("log_block_size 		= %10u\n", 
				sup_blk->s_log_block_size);
		printf("block_size 		= %10u\n",
				1024 << sup_blk->s_log_block_size);
		printf("inodes_per_group 	= %10u\n",
				sup_blk->s_inodes_per_group);
		printf("blocks_per_group 	= %10u\n",
				sup_blk->s_blocks_per_group);
		printf("first_inode		= %10u\n",
				sup_blk->s_first_ino);
		printf("inode_size		= %10u\n",
				sup_blk->s_inode_size);
		printf("block_group_number	= %10u\n",
				sup_blk->s_block_group_nr);
		printf("volume_name		= %s\n",
				sup_blk->s_volume_name);
		printf("===========================================================\n");

	}
	return ;
}

int main(const int argc, const char *argv[])
{
	char *dev;
	ssize_t rd_cnt;	
	unsigned int fd_dev, n_inode, blk_grp_cnt;

	struct stat statbuf;

	struct ext2_super_block sup_blk;
	struct ext2_group_desc grp_desc;
	struct ext2_inode inode;

	memset(&sup_blk, 0, sz_sup_blk);
	memset(&grp_desc, 0, sz_grp_desc);
	memset(&inode, 0, sz_inode);

	if ((argc > 1) && !stat((dev = argv[1]), &statbuf)
			&& ((n_inode = atol(argv[2])) > 0))
		;
	else
	{
		dev = DEF_DEV;
		n_inode = DEF_INODE;
	}

	/* opening the block device to read */
	if ((fd_dev = open(dev, O_RDONLY)) != -1)
	{
		/* reading super block from first block group */
		read_super_block(fd_dev, &sup_blk);
		/* display_super_block(&sup_blk);*/

		sz_inode = sup_blk.s_inode_size;
		if ((blk_grp_cnt = (total_inodes / inodes_per_group))
				== (total_blocks / blocks_per_group))
		{
			/*
			   for (unsigned int idx = 0; idx < blk_grp_cnt; idx++)
			   {
			   read_group_desc(fd_dev, idx, &grp_desc);
			   printf("group %d :\n", idx);
			   display_group_desc(&grp_desc);
			   }
			 */
			/*
			   for (unsigned int idx = sup_blk.s_first_ino; idx <= total_inodes; idx++)
			   {
			   read_inode(fd_dev, idx, &inode);
			   printf("INODE (%u) :\n", idx);
			   display_inode(&inode);
			   }
			 */
			read_inode(fd_dev, n_inode, &inode);
			printf("inode %u:\n", n_inode);
			display_inode(&inode);

		}
		else
			fprintf(stderr, "validation failed, file system broken\n");
	}
	else
		perror("open() : failed to open block device\n");
	return 0;
}
