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

#include <stddef.h>
#include <inttypes.h>
#include <sys/time.h>

#define SUCCESS	0
#define FAILURE	1

#define _1MB (1 << 20)

#define ALIGN2(n, a) (((n) + (a) - 1) & ~((a) - 1))
#define ALIGN(n, a)  ((((n) + (a) - 1) / (a)) * (a))

#define Q_(x) #x
#define Q(x) Q_(x)

static inline uint64_t gettimeofday_us()
{
	struct timeval t;
	gettimeofday(&t, NULL);
	return (uint64_t)t.tv_sec * UINT64_C(1000000) + (uint64_t)t.tv_usec;
}

static inline uint32_t min_u32(uint32_t a, uint32_t b)
{
	return a < b ? a : b;
}

static inline uint64_t min_u64(uint64_t a, uint64_t b)
{
	return a < b ? a : b;
}

static inline uint32_t max_u32(uint32_t a, uint32_t b)
{
	return a > b ? a : b;
}

static inline uint64_t max_u64(uint64_t a, uint64_t b)
{
	return a > b ? a : b;
}

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

# ifndef O_BINARY
# define O_BINARY 0
# endif

int same_file_behind_fds_posix(int fd1, int fd2);

# define same_file_behind_fds same_file_behind_fds_posix

#endif /* __WIN32 __ */

#endif /* COMMON_H */
