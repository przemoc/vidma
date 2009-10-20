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

#ifndef VD_H
#define VD_H

#include <inttypes.h>

typedef struct vd_ops {
	int (*check)(int);
	void (*print_info)(int);
	int (*resize)(int, int, uint32_t);
} vd_ops_t;

typedef struct vd_type {
	char *ext;
	char *name;
	vd_ops_t ops;
} vd_type_t;

#endif /* VD_H */
