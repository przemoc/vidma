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

/** \file ui.h
 * User interface common stuff.
 */

#ifndef UI_H
#define UI_H

#include <inttypes.h>

/** UI operations that must be supported. */
typedef struct ui_ops {

	/* log(char *format, ...) */
	int (*log)(const char *, ...);
	/**< Logs given message. */

	/* yesno(char *format, ...) */
	int (*yesno)(const char *, ...);
	/**< Asks yes/no question. */

	/* start_op(char *title, int steps_no) */
	int (*start_op)(const char *, int);
	/**< Informs about start of operation with given name and steps number. */

	/* end_op() */
	int (*end_op)();
	/**< Informs about end of previously started operation. */

	/* next_step(char *name) */
	int (*next_step)(const char *);
	/**< Informs about operation advancing to next step. */

	/* set_step_prog_max(uint64_t max) */
	int (*set_step_prog_max)(uint64_t);
	/**< Sets current step highest possible progress value (1 by default). */

	/* set_step_prog_val(uint64_t val) */
	int (*set_step_prog_val)(uint64_t);
	/**< Sets current step progress value. */

} ui_ops_t;

/** Chosen UI operations used by vidma. */
extern ui_ops_t *ui;

/** UI operations for CLI. */
extern ui_ops_t ui_cli;

#endif /* UI_H */
