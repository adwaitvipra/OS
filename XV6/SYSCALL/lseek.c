int sys_lseek(void)
{
	int offset, whence;
	struct file *fp;

	argint(1, &offset);
	if (argfd(0, 0, &fp) < 0 || argint(2, &whence) < 0)
		return -1;
	return fileseek(fp, offset, whence);
}

int fileseek(struct file *fp, int offset, int whence)
{
	uint fsize;
	if (!fp->readable || fp->type == FD_PIPE
			|| whence > 2)
		return -1;
	if (fp->type == FD_INODE)
	{
		ilock(fp->ip);
		fsize = fp->ip->size;
		iunlock(fp->ip);

		switch (whence)
		{
			case SEEK_SET :
				break;

			case SEEK_CURR :
				{
					offset += fp->off;
					break;
				}

			case SEEK_END :
				{
					offset += (fsize - 1);
					break;
				}
			default :
				return -1;
		};

		if (offset < fsize)
		{
			fp->off = offset;
			return offset;
		}
		return -1;
	}
	panic("fileseek");
}
~                                                                         

