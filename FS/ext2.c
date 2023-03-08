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

#define DEF_DEV "/dev/sda9"
#define DEF_INODE 2

#define SZ_BOOT_BLK 1024

#define MOD_INODE 0
#define MOD_DATA 1

unsigned int block_size, total_blocks, total_inodes;
unsigned int inodes_per_group, blocks_per_group;
const unsigned int sz_sup_blk = sizeof(struct ext2_super_block), 
      sz_grp_desc = sizeof(struct ext2_group_desc);
unsigned int sz_inode;

char entry[EXT2_NAME_LEN];

void display_super_block(struct ext2_super_block *);
void display_group_desc(struct ext2_group_desc *);
void display_inode(struct ext2_inode *);
unsigned int read_file(int, struct ext2_inode *, 
		unsigned int (*)(int, void *, size_t));
bool read_block(int , unsigned int, void *, size_t);
bool read_inode(int , unsigned int , struct ext2_inode *);
bool read_group_desc(int , unsigned int , struct ext2_group_desc *);
bool read_super_block(int , struct ext2_super_block *);

unsigned int directory_entry(int, void *, size_t);
unsigned int display_directory(int, void *, size_t);
unsigned int display_block(int , void *, size_t);

unsigned int directory_entry(int fd_dev, void *buffer, size_t size)
{
	extern char entry[];
	unsigned int ret_val, offset;
	struct ext2_dir_entry directory;

	ret_val = 0;
	if (buffer && size > 0)
	{
		offset = 0;
		ret_val = 0;
		while (offset < size && !ret_val)
		{
			memcpy(&directory, buffer + offset, sizeof directory);
			directory.name_len &= 0x00FF;

			if (!directory.name_len)
				break;
			else
			{
				if (!strncmp(directory.name, entry, 
							directory.name_len))
					ret_val = directory.inode;
				else
					offset += directory.rec_len;
			}
		}
	}

	return ret_val;
}

unsigned int display_directory(int fd_dev, void *buffer, size_t size)
{
	unsigned int offset, flag;
	struct ext2_dir_entry directory;

	flag = 1;
	if (buffer && size > 0)
	{
		flag = 0;
		offset = 0;

		/*
		   fprintf(stderr, "===========================================================\n");
		   */
		while (offset < size)
		{
			memcpy(&directory, buffer + offset, sizeof directory);

			directory.name_len &= 0x00FF;
			if (directory.name_len < EXT2_NAME_LEN)
				directory.name[directory.name_len] = '\0';

			if (!directory.name_len)
				break;
			else
			{
				printf("%10u\t%5hu\t%5hu\t%s\n", directory.inode,
						directory.rec_len, directory.name_len,
						directory.name);
				/*printf("%7u %s\n", directory.inode, directory.name);*/
				offset += directory.rec_len;
			}
		}
		/*
		   fprintf(stderr, "===========================================================\n");
		   */
	}

	return flag;
}

unsigned int display_block(int fd_dev, void *buffer, size_t size)
{
	if (buffer && size > 0)
		write(1, buffer, size);
	return 0;
}

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

				/* fprintf(stderr, "read_super_block: rd_cnt = %u\n", rd_cnt); */
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
				/* fprintf(stderr, "read_group_desc: rd_cnt = %u\n", rd_cnt); */
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
			   fprintf(stderr, "grp_desc_idx = %u, local_inode_idx = %u\n",
			   grp_desc_idx, local_inode_idx);
			   display_group_desc(&grp_desc);
			   */

			/* seek to inode table in required block group */
			g_offset = (long int) grp_desc.bg_inode_table * block_size;

			if (lseek64(fd_dev, g_offset, SEEK_SET) != -1)
			{
				/* fprintf(stderr, "inode_table_offset = %ld\n", g_offset); */

				/* read the inode by using local inode index */ 
				l_offset = (long int) local_inode_idx * sz_inode;

				if (lseek64(fd_dev, l_offset, SEEK_CUR) != -1)
				{
					/* fprintf(stderr, "inode_offset_relative = %ld\n",
					   l_offset); */
					if ((rd_cnt = read(fd_dev, inode,
									sz_inode))
							== sz_inode)
					{
						flag = true;
						/* fprintf(stderr, "read_inode: rd_cnt = %u\n", rd_cnt); */
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

bool read_block(int fd_dev, unsigned int blk_nr, void *buffer, size_t size)
{
	bool flag;
	ssize_t rd_cnt;
	long int offset; 

	flag = false;
	if (buffer && blk_nr >= 0 && size > 0)
	{
		offset = (long int) blk_nr * block_size;

		if (lseek64(fd_dev, offset, SEEK_SET) != -1)
		{
			if ((rd_cnt = read(fd_dev, buffer, size))
					== size)
			{
				flag = true;
				fprintf(stderr, "blk_nr = %u\n", blk_nr);
				/*		printf("[%u]\n", blk_nr); */
			}
		}
	}
	return flag;
}

void display_inode(struct ext2_inode *inode)
{
	if (inode)
	{
		fprintf(stderr, "===========================================================\n");
		fprintf(stderr, "mode		= %10o\n",
				inode->i_mode);
		fprintf(stderr, "uid		= %10u\n",
				inode->i_uid);
		fprintf(stderr, "gid		= %10u\n",
				inode->i_gid);
		fprintf(stderr, "size		= %10u\n",
				inode->i_size);
		fprintf(stderr, "atime		= %10x\n",
				inode->i_atime);
		fprintf(stderr, "ctime		= %10x\n",
				inode->i_ctime);
		fprintf(stderr, "mtime		= %10x\n",
				inode->i_mtime);
		fprintf(stderr, "dtime		= %10x\n",
				inode->i_dtime);
		fprintf(stderr, "links_count	= %10u\n",
				inode->i_links_count);
		fprintf(stderr, "blocks		= %10u\n",
				inode->i_blocks);
		fprintf(stderr, "flags		= %10x\n",
				inode->i_flags);
		fprintf(stderr, "file_acl	= %10u\n",
				inode->i_file_acl);
		for (int i = 0; i < EXT2_N_BLOCKS; i++)
		{
			fprintf(stderr, "block[%2u] 	= %10u\n", 
					i, inode->i_block[i]);
		}
		fprintf(stderr, "===========================================================\n");
	}
	return ;
}

void display_group_desc(struct ext2_group_desc *grp_desc)
{
	if (grp_desc)
	{
		fprintf(stderr, "===========================================================\n");
		fprintf(stderr, "block_bitmap 		= %10u\n", 
				grp_desc->bg_block_bitmap); 
		fprintf(stderr, "inode_bitmap 		= %10u\n", 
				grp_desc->bg_inode_bitmap); 
		fprintf(stderr, "inode_table 		= %10u\n", 
				grp_desc->bg_inode_table); 
		fprintf(stderr, "free_blocks_count 	= %10u\n", 
				grp_desc->bg_free_blocks_count);
		fprintf(stderr, "free_inodes_count 	= %10u\n",
				grp_desc->bg_free_inodes_count);
		fprintf(stderr, "used_dirs_count 	= %10u\n",
				grp_desc->bg_used_dirs_count);
		fprintf(stderr, "===========================================================\n");
	}
	return ;
}

void display_super_block(struct ext2_super_block *sup_blk)
{
	if(sup_blk)
	{
		fprintf(stderr, "===========================================================\n");
		fprintf(stderr, "magic_signature 	= %10X\n",
				sup_blk->s_magic);
		fprintf(stderr, "inodes_count 		= %10u\n",
				sup_blk->s_inodes_count);
		fprintf(stderr, "blocks_count 		= %10u\n",
				sup_blk->s_blocks_count);
		fprintf(stderr, "log_block_size 		= %10u\n", 
				sup_blk->s_log_block_size);
		fprintf(stderr, "block_size 		= %10u\n",
				1024 << sup_blk->s_log_block_size);
		fprintf(stderr, "inodes_per_group 	= %10u\n",
				sup_blk->s_inodes_per_group);
		fprintf(stderr, "blocks_per_group 	= %10u\n",
				sup_blk->s_blocks_per_group);
		fprintf(stderr, "first_inode		= %10u\n",
				sup_blk->s_first_ino);
		fprintf(stderr, "inode_size		= %10u\n",
				sup_blk->s_inode_size);
		fprintf(stderr, "block_group_number	= %10u\n",
				sup_blk->s_block_group_nr);
		fprintf(stderr, "volume_name		= %s\n",
				sup_blk->s_volume_name);
		fprintf(stderr, "===========================================================\n");

	}
	return ;
}

unsigned int read_file(int fd_dev, struct ext2_inode *inode, 
		unsigned int (*process)(int, void *, size_t))
{
	unsigned int size, 
		     blk_cnt, 
		     ret_val,
		     mod_size,
		     max_ind_idx,
		     dir_blk_idx, 
		     ind_blk_idx,
		     dind_blk_idx,
		     tind_blk_idx,
		     *block_arr;

	unsigned int ind_arr[block_size/4], 
		     dind_arr[block_size/4], 
		     tind_arr[block_size/4];
	char dat_buf[block_size];

	if (inode)
	{
		size = inode->i_size;
		block_arr = inode->i_block;
		mod_size = size % block_size;
		max_ind_idx = block_size / 4;
		blk_cnt = (unsigned int)(size / block_size) + ((mod_size > 0) ? 1 : 0);

		dir_blk_idx = 0;
		while (blk_cnt > 1 && dir_blk_idx < 12)
		{
			read_block(fd_dev, block_arr[dir_blk_idx++], dat_buf,
					sizeof dat_buf);
			if ((ret_val = process(fd_dev, dat_buf, sizeof dat_buf)))
				return ret_val;
			blk_cnt--;
		}

		if (blk_cnt > 1)
		{
			ind_blk_idx = 0;
			read_block(fd_dev, block_arr[dir_blk_idx++], ind_arr,
					sizeof ind_arr);

			while (blk_cnt > 1 && ind_blk_idx < max_ind_idx)
			{
				read_block(fd_dev, ind_arr[ind_blk_idx++], dat_buf,
						sizeof dat_buf);
				if ((ret_val = process(fd_dev, dat_buf, sizeof dat_buf)))
					return ret_val;
				blk_cnt--;

			}

			if (blk_cnt > 1)
			{
				dind_blk_idx = 0;
				read_block(fd_dev, block_arr[dir_blk_idx++], dind_arr,
						sizeof dind_arr);

				while (blk_cnt > 1 && dind_blk_idx < max_ind_idx)
				{
					ind_blk_idx = 0;
					read_block(fd_dev, dind_arr[dind_blk_idx++], ind_arr,
							sizeof ind_arr);

					while (blk_cnt > 1 && ind_blk_idx < max_ind_idx)
					{
						read_block(fd_dev, ind_arr[ind_blk_idx++], dat_buf,
								sizeof dat_buf);
						if ((ret_val = process(fd_dev, dat_buf, sizeof dat_buf)))
							return ret_val;
						blk_cnt--;
					}
				}

				if (blk_cnt > 1)
				{
					tind_blk_idx = 0;
					read_block(fd_dev, block_arr[dir_blk_idx++], tind_arr,
							sizeof tind_arr);

					while (blk_cnt > 1 && tind_blk_idx < max_ind_idx)
					{
						dind_blk_idx = 0;
						read_block(fd_dev, tind_arr[tind_blk_idx++], dind_arr,
								sizeof dind_arr);

						while (blk_cnt > 1 && dind_blk_idx < max_ind_idx)
						{
							ind_blk_idx = 0;
							read_block(fd_dev, dind_arr[dind_blk_idx++], ind_arr,
									sizeof ind_arr);

							while (blk_cnt > 1 && ind_blk_idx < max_ind_idx)
							{
								read_block(fd_dev, ind_arr[ind_blk_idx++], dat_buf,
										sizeof dat_buf);
								if ((ret_val = process(fd_dev, dat_buf, sizeof dat_buf)))
									return ret_val;
								blk_cnt--;
							}
						}
					}

					if (blk_cnt == 1)
					{
						if (ind_blk_idx < max_ind_idx)
						{
							read_block(fd_dev, ind_arr[ind_blk_idx++], dat_buf,
									sizeof dat_buf);
							ret_val = process(fd_dev, dat_buf, mod_size);
							blk_cnt--;
						}
						else if (dind_blk_idx < max_ind_idx)
						{
							ind_blk_idx = 0;
							read_block(fd_dev, dind_arr[dind_blk_idx++], ind_arr,
									sizeof ind_arr);

							read_block(fd_dev, ind_arr[ind_blk_idx++], dat_buf,
									sizeof dat_buf);
							ret_val = process(fd_dev, dat_buf, mod_size);
							blk_cnt--;
						}
						else if (tind_blk_idx < max_ind_idx)
						{
							ind_blk_idx = 0;
							dind_blk_idx = 0;
							read_block(fd_dev, tind_arr[tind_blk_idx++], dind_arr,
									sizeof dind_arr);
							read_block(fd_dev, dind_arr[dind_blk_idx++], ind_arr,
									sizeof ind_arr);

							read_block(fd_dev, ind_arr[ind_blk_idx++], dat_buf,
									sizeof dat_buf);
							ret_val = process(fd_dev, dat_buf, mod_size);
							blk_cnt--;
						}
						else
							fprintf(stderr, "failed to read file...\n");
					}
					else
						fprintf(stderr, "failed to read file...\n");
				}
				else
				{
					if (dind_blk_idx < max_ind_idx)
					{
						if (ind_blk_idx < max_ind_idx)
						{
							read_block(fd_dev, ind_arr[ind_blk_idx++], dat_buf,
									sizeof dat_buf);
							ret_val = process(fd_dev, dat_buf, mod_size);
							blk_cnt--;
						}
						else
						{
							ind_blk_idx = 0;
							read_block(fd_dev, dind_arr[dind_blk_idx++], ind_arr,
									sizeof ind_arr);

							read_block(fd_dev, ind_arr[ind_blk_idx++], dat_buf,
									sizeof dat_buf);
							ret_val = process(fd_dev, dat_buf, mod_size);
							blk_cnt--;
						}
					}
					else
					{
						ind_blk_idx = 0;
						dind_blk_idx = 0;
						tind_blk_idx = 0;

						read_block(fd_dev, block_arr[dir_blk_idx++], tind_arr,
								sizeof tind_arr);
						read_block(fd_dev, tind_arr[tind_blk_idx++], dind_arr,
								sizeof dind_arr);
						read_block(fd_dev, dind_arr[dind_blk_idx++], ind_arr,
								sizeof ind_arr);

						read_block(fd_dev, ind_arr[ind_blk_idx++], dat_buf,
								sizeof dat_buf);
						ret_val = process(fd_dev, dat_buf, mod_size);
						blk_cnt--;
					}
				}
			}
			else
			{
				if (ind_blk_idx < max_ind_idx)
				{
					read_block(fd_dev, ind_arr[ind_blk_idx++], dat_buf,
							sizeof dat_buf);
					ret_val = process(fd_dev, dat_buf, mod_size);
					blk_cnt--;
				}
				else
				{
					ind_blk_idx = 0;
					dind_blk_idx = 0;

					read_block(fd_dev, block_arr[dir_blk_idx++], dind_arr,
							sizeof dind_arr);
					read_block(fd_dev, dind_arr[dind_blk_idx++], ind_arr,
							sizeof ind_arr);

					read_block(fd_dev, ind_arr[ind_blk_idx++], dat_buf,
							sizeof dat_buf);
					ret_val = process(fd_dev, dat_buf, mod_size);
					blk_cnt--;

				}
			}
		}
		else
		{
			if (dir_blk_idx < 12)
			{
				read_block(fd_dev, block_arr[dir_blk_idx++], dat_buf,
						sizeof dat_buf);
				ret_val = process(fd_dev, dat_buf, mod_size);
				blk_cnt--;
			}
			else
			{
				ind_blk_idx = 0;
				read_block(fd_dev, block_arr[dir_blk_idx++], ind_arr,
						sizeof ind_arr);

				read_block(fd_dev, ind_arr[ind_blk_idx++], dat_buf,
						sizeof dat_buf);
				ret_val = process(fd_dev, dat_buf, mod_size);
				blk_cnt--;
			}
		}
	}
	return ret_val;
}

int main(const int argc, const char *argv[])
{
	char mode, *dev = NULL, *path = NULL;
	extern char entry[];
	ssize_t rd_cnt;	
	unsigned int fd_dev, n_inode, blk_grp_cnt;

	struct stat statbuf;

	struct ext2_super_block sup_blk;
	struct ext2_group_desc grp_desc;
	struct ext2_inode inode;

	memset(&sup_blk, 0, sz_sup_blk);
	memset(&grp_desc, 0, sz_grp_desc);
	memset(&inode, 0, sz_inode);

	dev = DEF_DEV;
	n_inode = atol(argv[1]);
	/*
	   if ((argc > 1 && argc < 5) 
	   && !stat((dev = argv[1]), &statbuf))
	   {
	   path = strdup(argv[2]);
	   mode = MOD_INODE;
	   if (!strcmp(argv[3], "data"))
	   mode = MOD_DATA;
	   }
	   else
	   {
	   fprintf(stderr, "usage :\n\tdisplay inode :\n"
	   "\t\t./ext2 device file inode\n"
	   "\tdisplay file content :\n"
	   "\t\t./ext2 device file data\n");
	   exit(0);
	   }
	   */

	/* opening the block device to read */
	if ((fd_dev = open(dev, O_RDONLY)) != -1)
	{
		/* reading super block from first block group */
		read_super_block(fd_dev, &sup_blk);
		display_super_block(&sup_blk);

		sz_inode = sup_blk.s_inode_size;
		if ((blk_grp_cnt = (total_inodes / inodes_per_group)))
		{
			/*
			   for (unsigned int idx = 0; idx < blk_grp_cnt; idx++)
			   {
			   read_group_desc(fd_dev, idx, &grp_desc);
			   fprintf(stderr, "group %d :\n", idx);
			   display_group_desc(&grp_desc);
			   }
			   */
			/*
			   for (unsigned int idx = sup_blk.s_first_ino; idx <= total_inodes; idx++)
			   {
			   read_inode(fd_dev, idx, &inode);
			   fprintf(stderr, "INODE (%u) :\n", idx);
			   display_inode(&inode);
			   }
			   */
			/*
			   read_inode(fd_dev, n_inode, &inode);
			   fprintf(stderr, "inode %u:\n", n_inode);
			   display_inode(&inode);
			   */
			/*
			   fprintf(stderr, "ret_val = %u\n", 
			   read_file(fd_dev, &inode, directory_entry));
			   */
			read_inode(fd_dev, n_inode, &inode);
			read_file(fd_dev, &inode, display_block);
		}
		else
			fprintf(stderr, "validation failed, file system broken\n");
	}
	else
		perror("open() : failed to open block device\n");

	return 0;
}
