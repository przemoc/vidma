/*
 * Copyright (C) 2012 Przemyslaw Pawelczyk <przemoc@gmail.com>
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

#include <stdarg.h>
#include <stdio.h>

#include "common.h"
#include "ui.h"

static int steps = 0;
static int step = 0;
static uint64_t pmax = 0;
static uint64_t pval = 0;

/* ==== Exposed functions prototypes ======================================== */

static int cli_log(const char *format, ...);
static int cli_yesno(const char *format, ...);
static int cli_start_op(const char *title, int steps_no);
static int cli_end_op();
static int cli_next_step(const char *name);
static int cli_set_step_prog_max(uint64_t max);
static int cli_set_step_prog_val(uint64_t val);

ui_ops_t ui_cli = {
	.log               = cli_log,
	.yesno             = cli_yesno,
	.start_op          = cli_start_op,
	.end_op            = cli_end_op,
	.next_step         = cli_next_step,
	.set_step_prog_max = cli_set_step_prog_max,
	.set_step_prog_val = cli_set_step_prog_val,
};

/* ==== Exposed functions definitions ======================================= */

static int cli_log(const char *format, ...)
{
	va_list ap;
	int ret;

	if (step)
		printf("[%d/%d] ", step, steps);
	va_start(ap, format);
	ret = vprintf(format, ap);
	va_end(ap);

	return ret;
}

static int cli_yesno(const char *format, ...)
{
	va_list ap;
	char buf[4];

	puts("");
	va_start(ap, format);
	vprintf(format, ap);
	va_end(ap);
	printf(" (y/N) ");
	fflush(stdout);
	fgets(buf, sizeof(buf), stdin);

	return (buf[0] == 'y' || buf[0] == 'Y') && buf[1] == '\n'
	       ? SUCCESS : FAILURE;
}

static int cli_start_op(const char *title, int steps_no)
{
	steps = steps_no;
	step = 0;

	return printf("\nOperation: %s\n", title);
}

static int cli_end_op()
{
	step = 0;
	steps = 0;
	puts("Operation finished");

	return 0;
}

static int cli_next_step(const char *name)
{
	step++;
	printf("[%d/%d] %s: ", step, steps, name);
	cli_set_step_prog_max(1);

	return 0;
}

static int cli_set_step_prog_max(uint64_t max)
{
	pmax = max;
	pval = 0;

	return 0;
}

static int cli_set_step_prog_val(uint64_t val)
{
	static int len = 0;

	pval = val;
	if (pmax > 1) {
		while (len) {
			putchar('\b');
			len--;
		}
		len = printf("%"PRIu64"/%"PRIu64" ", pval, pmax);
	}
	if (pval == pmax) {
		puts("Done");
		len = 0;
	}

	return len;
}
