/*
 * Copyright (C) 2009 Przemyslaw Pawelczyk <przemoc@gmail.com>
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

#ifndef COMMON_H
#define COMMON_H

#define SUCCESS	0
#define FAILURE	1

#define MB	(1 << 20)

#if !__WIN32__
# define O_BINARY	0
# define FSYNC(fd)	fsync(fd)
# define FTRUNC(fd,siz)	ftruncate(fd, siz)
# define ARE_FILES_BEHIND_FDS_EQUAL(fd1, fd2) \
	({ \
		struct stat fd1_stat, fd2_stat; \
		fstat(fd1, &fd1_stat); \
		fstat(fd2, &fd2_stat); \
		fd1_stat.st_dev == fd2_stat.st_dev && fd1_stat.st_ino == fd2_stat.st_ino; \
	})
#else
# define lseek	lseek64
# define S_IRGRP	0
# define S_IROTH	0
# define FSYNC(fd)	FlushFileBuffers((HANDLE)_get_osfhandle(fd))
# define FTRUNC(fd,siz) \
	{ \
		HANDLE handle_ = (HANDLE)_get_osfhandle(fd); \
		LARGE_INTEGER offset_; \
		offset_.QuadPart = siz; \
		SetFilePointerEx(handle_, offset_, NULL, FILE_BEGIN); \
		SetFileValidData(handle_, offset_.QuadPart); \
		SetEndOfFile(handle_); \
	}
# define ARE_FILES_BEHIND_FDS_EQUAL(fd1, fd2) \
	({ \
		BY_HANDLE_FILE_INFORMATION fd1_info, fd2_info; \
		GetFileInformationByHandle((HANDLE)_get_osfhandle(fd1), &fd1_info); \
		GetFileInformationByHandle((HANDLE)_get_osfhandle(fd2), &fd2_info); \
		fd1_info.dwVolumeSerialNumber == fd2_info.dwVolumeSerialNumber && fd1_info.nFileIndexHigh == fd2_info.nFileIndexHigh && fd1_info.nFileIndexLow == fd2_info.nFileIndexLow; \
	})
#endif

#endif /* COMMON_H */
