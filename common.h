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

#ifndef COMMON_H
#define COMMON_H

#include <inttypes.h>

#define SUCCESS	0
#define FAILURE	1

#define MB	(1 << 20)

#if __WIN32__

#include <io.h>
#include <windows.h>

# define S_IRGRP 0
# define S_IROTH 0

# define fsync _commit
# define lseek lseek64

int ftruncate64_win(int fd, int64_t length);
int same_file_behind_fds_win(int fd1, int fd2);

# define ftruncate ftruncate64_win
# define same_file_behind_fds same_file_behind_fds_win

#else /* !__WIN32__ ~= POSIX */

# define O_BINARY 0

int same_file_behind_fds_posix(int fd1, int fd2);

# define same_file_behind_fds same_file_behind_fds_posix

#endif /* __WIN32 __ */

#endif /* COMMON_H */
