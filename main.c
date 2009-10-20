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

/*
 * vidma - Virtual Disks Manipulator
 * version 0.1
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include "common.h"
#include "vdi.h"

int main(int argc, char *argv[])
{
	vd_type_t *types[] = {
		&vd_vdi,
		NULL
	};
	vd_type_t **type = types;
	uint32_t new_msize = 0, endianness_test = 0x00000001;
	int fin, fout, result;
	char *tmp;

	if (!((char*)&endianness_test)[0]) {
		fprintf(stderr, "This program requires little-endian machine. Sorry!");
		exit(FAILURE);
	}

	if (argc == 1) {
		printf("vidma - Virtual Disks Manipulator\n(C) 2009 Przemyslaw Pawelczyk\n\nUsage: %s <input file> <new size in MB> [output file]\n\nUSE AT YOUR OWN RISK! NO WARRANTY!\n", argv[0]);
		exit(SUCCESS);
	} else if (argc == 3 || argc == 4) {
		new_msize = strtoll(argv[2], &tmp, 10);
		if (*argv[2] == '\0' || new_msize <= 0 || *tmp != '\0') {
			fprintf(stderr, "Incorrect second argument!\n");
			exit(FAILURE);
		}
	} else if (argc > 4) {
		fprintf(stderr, "Too many arguments!\n");
		exit(FAILURE);
	}

	fin = open(argv[1], O_RDONLY | O_BINARY);

	for (; *type != NULL; type++) {
		if ((*type)->ops.check(fin) == SUCCESS) {
			fprintf(stderr, "Recognized file format = %s (%s)\n", (*type)->name, (*type)->ext);
			break;
		}
	}

	if (*type == NULL) {
		fprintf(stderr, "Unrecognized file format!\n");
		exit(FAILURE);
	}

	fout = open(argc == 4 ? argv[3] : argv[1], O_CREAT | O_WRONLY | O_BINARY, S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);

	if (argc == 2) {
		(*type)->ops.print_info(fin);
		return 0;
	}

	result = (*type)->ops.resize(fin, fout, new_msize);

	close(fout);
	close(fin);

	return result;
}
