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

int ftruncate64_win(int fd, int64_t length)
{
	BOOL result;
	HANDLE handle = (HANDLE)_get_osfhandle(fd);
	LARGE_INTEGER offset;

	offset.QuadPart = length;
	SetFilePointerEx(handle, offset, NULL, FILE_BEGIN);
	SetFileValidData(handle, offset.QuadPart);
	result = SetEndOfFile(handle);
	/*
	 * POSIX ftruncate() does not change file offset, so it should be
	 * restored for compatibility, but we can ignore it for now.
	 */

	return result ? 0 : -1;
}

int same_file_behind_fds_win(int fd1, int fd2)
{
	BY_HANDLE_FILE_INFORMATION fd1_info, fd2_info;

	GetFileInformationByHandle((HANDLE)_get_osfhandle(fd1), &fd1_info);
	GetFileInformationByHandle((HANDLE)_get_osfhandle(fd2), &fd2_info);

	return (fd1_info.dwVolumeSerialNumber == fd2_info.dwVolumeSerialNumber &&
	        fd1_info.nFileIndexHigh == fd2_info.nFileIndexHigh &&
	        fd1_info.nFileIndexLow == fd2_info.nFileIndexLow)
	       ? SUCCESS : FAILURE;
}