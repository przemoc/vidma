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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>

#include "common.h"
#include "vdi.h"

vd_type_t vd_vdi = {
	.ext = "vdi",
	.name = "Virtual Disk Image",
	.ops = {
		.check      = vdi_check,
		.print_info = vdi_print_info,
		.resize     = vdi_resize
	}
};

/* Forward declarations */
static void print_uuid(vdi_uuid_t *uuid);
static void print_info_from_struct(vdi_start_t *v, int full);
static int check_assumptions(vdi_start_t *vdi);
static void move_data(vdi_start_t *vdi, int fin, int fout, uint32_t new_msize);
static void fix_block_mapping(int fd, uint32_t new_msize);
static void update_header(vdi_start_t *vdi, int fd, uint32_t new_msize);

/*** PUBLIC ***/

int vdi_check(int fd)
{
	vdi_start_t v;
	lseek(fd, 0LL, SEEK_SET);
	read(fd, &v, sizeof(vdi_start_t));
	return v.pre.signature == VDI_SIGNATURE ? SUCCESS : FAILURE;
}

void vdi_print_info(int fd)
{
	vdi_start_t v;
	lseek(fd, 0LL, SEEK_SET);
	read(fd, &v, sizeof(vdi_start_t));
	print_info_from_struct(&v, 1);
}

int vdi_resize(int fin, int fout, uint32_t new_msize)
{
	vdi_start_t vdi;

	lseek(fin, 0LL, SEEK_SET);
	read(fin, &vdi, sizeof(vdi_start_t));

	if (check_assumptions(&vdi) == FAILURE)
		return FAILURE;

	puts(":: mission started");
	move_data(&vdi, fin, fout, new_msize);
	ftruncate(fout, VDI_OFFSET_DATA(new_msize) + VDI_DISK_SIZE(new_msize));
	puts(":: disk resized");
	fix_block_mapping(fout, new_msize);
	update_header(&vdi, fout, new_msize);
	print_info_from_struct(&vdi, 0);
	puts(":: final syncing");
	fsync(fout);
	puts(":: mission accomplished");
	return SUCCESS;
}

/*** PRIVATE ***/

#define FIELD_LEN	32

#define PRINTU32(v,i)	printf("%-*s = %08x %u\n", FIELD_LEN, #i, v->i, v->i)
#define PRINTSTR(v,i)	printf("%-*s = %s\n", FIELD_LEN, #i, v->i)
#define PRINTUUID(v,i)	printf("%-*s = ", FIELD_LEN, #i); print_uuid(&(v->i)); puts("")
#define PRINTU64(v,i)	printf("%-*s = %016"PRIx64" %"PRIu64"\n", FIELD_LEN, #i, (uint64_t)v->i, v->i)

static void print_uuid(vdi_uuid_t *uuid)
{
	printf("%08x-%04hx-%04hx-%02x%02x-%02x%02x%02x%02x%02x%02x",
	       uuid->part1, uuid->part2, uuid->part3, uuid->part4[0], uuid->part4[1],
	       uuid->part5[0], uuid->part5[1], uuid->part5[2], uuid->part5[3], uuid->part5[4], uuid->part5[5]
	      );
}

static void print_info_from_struct(vdi_start_t *v, int full)
{
	if (full) {
		PRINTSTR(v, pre.file_info);
		PRINTU32(v, pre.signature);
		PRINTU32(v, version);
		PRINTU32(v, header.size);
		PRINTU32(v, header.type);
		PRINTU32(v, header.flags);
		PRINTU32(v, header.offset.blocks);
	}
	PRINTU32(v, header.offset.data);
	if (full) {
		PRINTU32(v, header.pchs.cylinders);
		PRINTU32(v, header.pchs.heads);
		PRINTU32(v, header.pchs.sectors);
		PRINTU32(v, header.pchs.sector_size);
		PRINTU32(v, header.dummy);
	}
	PRINTU64(v, header.disk.size);
	if (full) {
		PRINTU32(v, header.disk.blk_size);
		PRINTU32(v, header.disk.blk_extra_data);
	}
	PRINTU32(v, header.disk.blk_count);
	PRINTU32(v, header.disk.blk_count_alloc);
	if (full) {
		PRINTUUID(v, header.uuid.create);
		PRINTUUID(v, header.uuid.modify);
		PRINTUUID(v, header.uuid.linkage);
		PRINTUUID(v, header.uuid.parent_modify);
	}
	PRINTU32(v, header.lchs.cylinders);
	PRINTU32(v, header.lchs.heads);
	PRINTU32(v, header.lchs.sectors);
	if (full)
		PRINTU32(v, header.lchs.sector_size);
}

static int check_assumptions(vdi_start_t *vdi)
{
	if (vdi->header.offset.blocks != 512 ||
	    vdi->header.pchs.cylinders != 0 ||
	    vdi->header.pchs.heads != 0 ||
	    vdi->header.pchs.sectors != 0 ||
	    vdi->header.pchs.sector_size != 512 ||
	    vdi->header.disk.blk_size != MB ||
	    vdi->header.disk.blk_extra_data != 0 ||
	    vdi->header.lchs.sector_size != 512) {
		puts("\nProvided file doesn't fit into author's assumptions.");
		return FAILURE;
	}
	if (vdi->header.type != VDI_FIXED) {
		puts("\nYou can only operate on fixed VDI images!");
		return FAILURE;
	}
	return SUCCESS;
}

static void move_data(vdi_start_t *vdi, int fin, int fout, uint32_t new_msize)
{
	int32_t delta = VDI_OFFSET_DATA(new_msize) - vdi->header.offset.data;
	uint32_t i;
	char *buffer;
	struct timeval start, end;

	lseek(fin, vdi->header.offset.data, SEEK_SET);
	lseek(fout, VDI_OFFSET_DATA(new_msize), SEEK_SET);

	if (delta || same_file_behind_fds(fin, fout) == FAILURE) {
		puts(":: moving data");
		buffer = malloc(MB + delta * (delta > 0));
		gettimeofday(&start, NULL);
		if (delta > 0) {
			read(fin, buffer + MB, delta);
			for (i = 1; i <= vdi->header.disk.blk_count_alloc; i++) {
				printf("\r:: block: %u / %u", i, vdi->header.disk.blk_count_alloc);
				memcpy(buffer, buffer + MB, delta);
				read(fin, buffer + delta, MB);
				write(fout, buffer, MB);
			}
		} else
			for (i = 1; i <= new_msize; i++) {
				printf("\r:: block: %u / %u", i, new_msize);
				read(fin, buffer, MB);
				write(fout, buffer, MB);
			}
		puts("\n:: syncing");
		fsync(fout);
		gettimeofday(&end, NULL);
		free(buffer);
		printf(
		       ":: data moved (%u MB by %u bytes in %"PRIu64" ms = ~%.2f MB/s)\n", 
		       delta > 0 ? vdi->header.disk.blk_count_alloc : new_msize, abs(delta),
		       end.tv_sec * UINT64_C(1000) + end.tv_usec / 1000 - start.tv_sec * UINT64_C(1000) - start.tv_usec / 1000,
		       (double)(delta > 0 ? vdi->header.disk.blk_count_alloc : new_msize) / (end.tv_sec + end.tv_usec * 0.000001 - start.tv_sec - start.tv_usec * 0.000001)
		      );
	}
}

#define FILL_POWER	3
#define FILL_COUNT	(1 << FILL_POWER)
#define ZEROS_POWER	4
#define ZEROS_COUNT	(1 << ZEROS_POWER)

static void fix_block_mapping(int fd, uint32_t new_msize)
{
	uint32_t zeros[ZEROS_COUNT];
	uint32_t i, j, n[8];

	puts(":: fixing block mapping area");
	lseek(fd, sizeof(vdi_start_t), SEEK_SET);
	for (i = 0; i < (new_msize & -FILL_COUNT); i += FILL_COUNT) {
		n[0] = i;
		n[1] = i + 1;
		n[2] = i + 2;
		n[3] = i + 3;
		n[4] = i + 4;
		n[5] = i + 5;
		n[6] = i + 6;
		n[7] = i + 7;
		write(fd, n, FILL_COUNT * sizeof(uint32_t));
	}
	for (; i < new_msize; i++)
		write(fd, &i, sizeof(uint32_t));

	memset(zeros, 0, ZEROS_COUNT * sizeof(uint32_t));

	j = (i + ZEROS_COUNT - 1) & -ZEROS_COUNT;
	write(fd, zeros, (j - i) * sizeof(uint32_t));
	for (i = j; i < (VDI_OFFSET_DATA(new_msize) - sizeof(vdi_start_t)) >> 2; i += ZEROS_COUNT)
		write(fd, zeros, ZEROS_COUNT * sizeof(uint32_t));
	puts(":: block mapping area fixed");
}

static void update_header(vdi_start_t *vdi, int fd, uint32_t new_msize)
{
	puts(":: updating header");
	vdi->header.offset.data = VDI_OFFSET_DATA(new_msize);
	vdi->header.disk.size = VDI_DISK_SIZE(new_msize);
	vdi->header.disk.blk_count = new_msize;
	vdi->header.disk.blk_count_alloc = new_msize;
	/* VB will fix lchs section */
	vdi->header.lchs.cylinders = 0;
	vdi->header.lchs.heads = 0;
	vdi->header.lchs.sectors = 0;
	lseek(fd, 0LL, SEEK_SET);
	write(fd, vdi, sizeof(vdi_start_t));
	puts(":: header updated");
}
