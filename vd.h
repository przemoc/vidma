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

/** \file vd.h
 * Virtual Disk common stuff.
 */

#ifndef VD_H
#define VD_H

#include <inttypes.h>

/** VD operations that can be supported.
 *
 * All functions take file descriptor as first argument.
 * Following arguments depend on the operation.
 */
typedef struct vd_ops {

	/* detect(int fd) */
	int (*detect)(int);
	/**< Detects whether image seems to conform VD type. */

	/* info(int fd) */
	void (*info)(int);
	/**< Logs information about the image. */

	/* resize(int fd_in, int fd_out, uint32_t new_size_in_mb) */
	int (*resize)(int, int, uint32_t);
	/**< Resizes the image. */

} vd_ops_t;

/** VD type definition. */
typedef struct vd_type {
	char *ext;          /**< Common file extension. */
	char *name;         /**< Name. */
	vd_ops_t ops;       /**< Operations. */
} vd_type_t;

#endif /* VD_H */
