/*
 * Copyright (C) 2009-2011 Przemyslaw Pawelczyk <przemoc@gmail.com>
 *
 * This software is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2.
 * See <http://www.gnu.org/licenses/gpl-2.0.txt>.
 *
 * This software is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 */

#include "common.h"

#include <sys/stat.h>

int same_file_behind_fds_posix(int fd1, int fd2)
{
	struct stat fd1_stat, fd2_stat;

	fstat(fd1, &fd1_stat);
	fstat(fd2, &fd2_stat);

	return (fd1_stat.st_dev == fd2_stat.st_dev &&
	        fd1_stat.st_ino == fd2_stat.st_ino)
	       ? SUCCESS : FAILURE;
}
