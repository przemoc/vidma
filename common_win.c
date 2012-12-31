/*
 * Copyright (C) 2009-2012 Przemyslaw Pawelczyk <przemoc@gmail.com>
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
#include <winternl.h>

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

int get_volume_free_space_win(int fd, uint64_t *bytes)
{
	BOOL res;
	HANDLE handle = (HANDLE)_get_osfhandle(fd);
	IO_STATUS_BLOCK iostat;
	FILE_FS_SIZE_INFORMATION fsinfo;

	res = NtQueryVolumeInformationFile(handle, &iostat, &fsinfo,
	                                      sizeof(fsinfo), FileFsSizeInformation);
	if (!res)
		*bytes = (uint64_t)fsinfo.AvailableAllocationUnits.QuadPart *
		         (uint64_t)fsinfo.SectorsPerAllocationUnit *
		         (uint64_t)fsinfo.BytesPerSector;

	return !res ? SUCCESS : FAILURE;
}
