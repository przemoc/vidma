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

/** \file common.h
 * Common stuff.
 */

#ifndef COMMON_H
#define COMMON_H

#include <stddef.h>
#include <inttypes.h>
#include <sys/time.h>

/** Value denoting success. */
#define SUCCESS	0
/** Value denoting failure. */
#define FAILURE	1

/** Shorthand for 1 megabyte. */
#define _1MB (1 << 20)

/** Checks whether \p n is a power of 2. */
#define IS_POWER_OF_2(n) !(n & (n - 1))

/** Checks whether \p n is a positive power of 2. */
#define IS_POSITIVE_POWER_OF_2(n) (n > 1 && IS_POWER_OF_2(n))

/** Aligns \p n to next multiple of \p a (where \p a must be power of 2).
 *
 * \param n number requiring alignment
 * \param a alignment base (must be power of 2)
 */
#define ALIGN2(n, a) (((n) + (a) - 1) & ~((a) - 1))
/** Aligns \p n to next multiple of \p a.
 *
 * \param n number requiring alignment
 * \param a alignment base
 */
#define ALIGN(n, a)  ((((n) + (a) - 1) / (a)) * (a))

#define Q_(x) #x
/** Wraps \p x in quotes. */
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

/** Returns \a SUCCESS if file behind \p fd1 and \p fd2 is one and the same. */
int same_file_behind_fds_win(int fd1, int fd2);
int get_volume_free_space_win(int fd, uint64_t *bytes);

# define same_file_behind_fds same_file_behind_fds_win
# define get_volume_free_space get_volume_free_space_win

#else /* !__WIN32__ ~= POSIX */

# ifndef O_BINARY
# define O_BINARY 0
# endif

/** Returns \a SUCCESS if file behind \p fd1 and \p fd2 is one and the same. */
int same_file_behind_fds_posix(int fd1, int fd2);
int get_volume_free_space_posix(int fd, uint64_t *bytes);

# define same_file_behind_fds same_file_behind_fds_posix
# define get_volume_free_space get_volume_free_space_posix

#endif /* __WIN32 __ */

#endif /* COMMON_H */
