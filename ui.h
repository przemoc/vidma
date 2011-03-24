/*
 * Copyright (C) 2011 Przemyslaw Pawelczyk <przemoc@gmail.com>
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

#ifndef UI_H
#define UI_H

#include <inttypes.h>

typedef struct ui_ops {
	int (*log)(const char *, ...);      /* log(format,...) */
	int (*yesno)(const char *, ...);    /* yesno(format,...) */
	int (*start_op)(const char *, int); /* start_op(title,steps_no) */
	int (*end_op)();                    /* end_op() */
	int (*next_step)(const char *);     /* next_step(name) */
	int (*set_step_prog_max)(uint64_t); /* set_step_max(max) */
	int (*set_step_prog_val)(uint64_t); /* set_step_val(val) */
} ui_ops_t;

extern ui_ops_t *ui;

extern ui_ops_t ui_cli;

#endif /* UI_H */
