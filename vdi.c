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
#include <sys/stat.h>

#include "common.h"
#include "vdi.h"

/* ==== Exposed functions prototypes ======================================== */

static int vdi_check(int fd);
static void vdi_info(int fd);
static int vdi_resize(int fin, int fout, uint32_t new_msize);

vd_type_t vd_vdi = {
	.ext = "vdi",
	.name = "Virtual Disk Image",
	.ops = {
		.check      = vdi_check,
		.info       = vdi_info,
		.resize     = vdi_resize
	}
};

/* ==== Non-exposed functions prototypes ==================================== */

static void print_uuid(vdi_uuid_t *uuid);
static void print_info_from_struct(vdi_start_t *v, int full);
static void read_start(int fd, vdi_start_t *vdi);
static void write_start(int fd, vdi_start_t *vdi);
static int check_assumptions(vdi_start_t *vdi);
static int check_correctness(vdi_start_t *vdi);
static int resize_confirmation(vdi_start_t *vdi, int fin, int fout,
                               uint32_t new_blk_count);
static inline uint32_t data_offset(vdi_start_t *vdi, uint32_t blk_count);
static inline uint64_t disk_size(vdi_start_t *vdi, uint32_t blk_count);
static void rewrite_data(vdi_start_t *vdi, int fin, int fout,
                         uint32_t new_blk_count);
static void update_block_allocation_map(vdi_start_t *vdi, int fin, int fout);
static void update_header(vdi_start_t *vdi, int fd);
static int resize(vdi_start_t *vdi, int fin, int fout, uint32_t new_blk_count);

/* ==== Exposed functions definitions ======================================= */

static int vdi_check(int fd)
{
	vdi_start_t v;

	read_start(fd, &v);

	return v.pre.signature == VDI_SIGNATURE
	       ? SUCCESS : FAILURE;
}

static void vdi_info(int fd)
{
	vdi_start_t v;

	read_start(fd, &v);
	print_info_from_struct(&v, 1);
}

static int vdi_resize(int fin, int fout, uint32_t new_msize)
{
	vdi_start_t vdi;
	uint32_t new_blk_count;

	read_start(fin, &vdi);
	if (check_assumptions(&vdi) == FAILURE ||
	    check_correctness(&vdi) == FAILURE)
		return FAILURE;

	new_blk_count = ALIGN((uint64_t)new_msize * (uint64_t)_1MB,
	                      vdi.header.disk.blk_size) / vdi.header.disk.blk_size;
	if (resize_confirmation(&vdi, fin, fout, new_blk_count) != SUCCESS) {
		puts("Resize aborted.");
		return FAILURE;
	}

	return resize(&vdi, fin, fout, new_blk_count);
}

/* ==== Defines and Macros ================================================== */

#define PRINT(f,a...)  printf("%-*s = " f, 32, a)
#define PRINTU32(v,i)  PRINT("%08x %u\n", #i, (uint32_t)v->i, (uint32_t)v->i)
#define PRINTSTR(v,i)  PRINT("%s\n", #i, (char *)v->i)
#define PRINTUUID(v,i) \
	do { \
		PRINT("", #i); \
		print_uuid(&(v->i)); \
		puts(""); \
	} while (0)
#define PRINTU64(v,i) \
	PRINT("%016"PRIx64" %"PRIu64"\n", #i, (uint64_t)v->i, (uint64_t)v->i)

#define FILL_SHIFT 4
#define FILL_COUNT (1 << FILL_SHIFT)

/* ==== Non-exposed functions definitions =================================== */

static void print_uuid(vdi_uuid_t *uuid)
{
	printf("%08x-%04hx-%04hx-%02x%02x-%02x%02x%02x%02x%02x%02x",
	       uuid->part1, uuid->part2, uuid->part3,
	       uuid->part4[0], uuid->part4[1],
	       uuid->part5[0], uuid->part5[1], uuid->part5[2],
	       uuid->part5[3], uuid->part5[4], uuid->part5[5]
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
		PRINTU32(v, header.offset.bam);
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

static void read_start(int fd, vdi_start_t *vdi)
{
	lseek(fd, 0LL, SEEK_SET);
	read(fd, vdi, sizeof(vdi_start_t));
}

static void write_start(int fd, vdi_start_t *vdi)
{
	lseek(fd, 0LL, SEEK_SET);
	write(fd, vdi, sizeof(vdi_start_t));
}

static int check_assumptions(vdi_start_t *vdi)
{
	if (vdi->version != ((1 << 16) | 1)) {
		puts("ERROR   Not supported VDI format version.");
		return FAILURE;
	}
	if (vdi->header.disk.blk_extra_data != 0) {
		puts("ERROR   Blocks with extra data are not supported yet.");
		return FAILURE;
	}
	if (vdi->header.offset.bam > vdi->header.offset.data) {
		puts("ERROR   Block allocation map following data is not supported.");
		return FAILURE;
	}
	if (vdi->header.lchs.sector_size != VDI_SECTOR_SIZE) {
		puts("ERROR   Sector sizes other than " Q(VDI_SECTOR_SIZE)
		     " are not supported.");
		return FAILURE;
	}
	if (vdi->header.type != VDI_FIXED) {
		puts("ERROR   Non fixed-size VDI images are not supported yet.");
		return FAILURE;
	}

	return SUCCESS;
}

static int check_correctness(vdi_start_t *vdi)
{
	uint64_t start_end = sizeof(vdi_start_t);
	uint64_t bam_beg = vdi->header.offset.bam;
	uint64_t bam_end = vdi->header.offset.bam +
	                   VDI_BAM_SIZE(vdi->header.disk.blk_count);
	uint64_t data_beg = vdi->header.offset.data;
	uint64_t data_end = vdi->header.offset.data + vdi->header.disk.size;
	uint64_t disksize = disk_size(vdi, vdi->header.disk.blk_count);
	int errors = 0;

	if (bam_beg < start_end)
		errors += puts("ERROR   BAM overlaps file header.") >= 0;
	if (data_beg < start_end)
		errors += puts("ERROR   Data overlaps file header.") >= 0;
	if (!(data_beg >= bam_end && data_end >= bam_end) &&
	    !(bam_beg >= data_end && bam_end >= data_end))
		errors += puts("ERROR   BAM overlaps data.") >= 0;
	if (vdi->header.disk.blk_count_alloc > vdi->header.disk.blk_count)
		errors += puts("ERROR   Insane number of allocated blocks.") >= 0;
	if (disksize != vdi->header.disk.size)
		errors += printf("ERROR   block size (%u), block count (%u) "
		                 "and disk size (%"PRIu64") mismatch.\n",
		                 vdi->header.disk.blk_size, vdi->header.disk.blk_count,
		                 vdi->header.disk.size) >= 0;

	return !errors
	       ? SUCCESS : FAILURE;
}

static int resize_confirmation(vdi_start_t *vdi, int fin, int fout,
                               uint32_t new_blk_count)
{
	char buf[4];
	int32_t delta = data_offset(vdi, new_blk_count) - vdi->header.offset.data;
	int same_file = same_file_behind_fds(fin, fout) == SUCCESS;

	printf("Requested disk resize\n"
	       "from %21u block(s)\nto   %21u block(s)\n",
	       vdi->header.disk.blk_count, new_blk_count);
	printf("(each block has %10u bytes)\n",
	       vdi->header.disk.blk_size);
	printf("\nDisk size will change\n"
	       "from %21"PRIu64" bytes (%15"PRIu64" MB)\n"
	       "to   %21"PRIu64" bytes (%15"PRIu64" MB)\n",
	       vdi->header.disk.size, vdi->header.disk.size / _1MB,
	       disk_size(vdi, new_blk_count), disk_size(vdi, new_blk_count) / _1MB);
	puts("");
	if (same_file) {
		puts("Resize operation will be performed in-place.");
		if (delta)
			puts("WARNING All allocated blocks require moving.\n"
			     "        In case of fail DATA LOSS is highly POSSIBLE!\n"
			     "        Think twice before continuing!");
		else
			puts("CAUTION Only disk metadata will be modified.\n"
			     "        In case of fail data loss is highly unlikely,\n"
			     "        but image METADATA CAN BE CORRUPTED!");
		if (vdi->header.disk.size > disk_size(vdi, new_blk_count))
			puts("WARNING Shrinking disk in-place means\n"
			     "        IRRETRIEVABLY LOSING DATA KEPT BEYOND NEW SIZE!");
	} else {
		puts("Resize operation in fact will create resized copy of the image.");
		puts("NOTE    UUID of the new image will be the same as old one.");
		puts("NOTE    Input file is safe and won't be modified.");
	}

	printf("\nAre you sure you want to continue? (y/N) ");
	fgets(buf, sizeof(buf), stdin);

	return (buf[0] == 'y' || buf[0] == 'Y') && buf[1] == '\n'
	       ? SUCCESS : FAILURE;
}

static inline uint32_t min_data_offset(vdi_start_t *vdi, uint32_t blk_count)
{
	return ALIGN2(vdi->header.offset.bam + VDI_BAM_SIZE(blk_count),
	              VDI_SECTOR_SIZE);
}

static inline uint32_t data_offset(vdi_start_t *vdi, uint32_t blk_count)
{
	uint32_t min_offset_data = min_data_offset(vdi, blk_count);
	uint32_t min_offset_data_aligned = ALIGN2(min_offset_data,
	                                          VDI_DATA_OFFSET_ALIGNMENT);

	return   vdi->header.offset.data >= min_offset_data
	       ? vdi->header.offset.data  : min_offset_data_aligned;
}

static inline uint64_t disk_size(vdi_start_t *vdi, uint32_t blk_count)
{
	return ((uint64_t)blk_count) * ((uint64_t)vdi->header.disk.blk_size);
}

static void rewrite_data(vdi_start_t *vdi, int fin, int fout,
                         uint32_t new_blk_count)
{
	uint32_t i;
	char *buffer;
	uint64_t start, end;
	uint32_t bs = vdi->header.disk.blk_size;
	uint32_t b = vdi->header.disk.blk_count_alloc;
	uint32_t b2m = min_u32(b, new_blk_count);
	int32_t delta = data_offset(vdi, new_blk_count) - vdi->header.offset.data;
	int same_file = (same_file_behind_fds(fin, fout) == SUCCESS);

	lseek(fin, vdi->header.offset.data, SEEK_SET);
	lseek(fout, data_offset(vdi, new_blk_count), SEEK_SET);

	if (delta || !same_file) {
		if (same_file)
			puts(":: moving data");
		else
			puts(":: copying data");
		buffer = malloc(bs + delta * (delta > 0));
		start = gettimeofday_us();
		if (delta > 0) {
			read(fin, buffer + bs, delta);
			for (i = 1; i <= b2m; i++) {
				printf("\r:: block: %u / %u", i, b2m);
				memcpy(buffer, buffer + bs, delta);
				read(fin, buffer + delta, bs);
				write(fout, buffer, bs);
			}
		} else
			for (i = 1; i <= b2m; i++) {
				printf("\r:: block: %u / %u", i, b2m);
				read(fin, buffer, bs);
				write(fout, buffer, bs);
			}
		puts("\n:: syncing");
		fsync(fout);
		end = gettimeofday_us();
		free(buffer);
		if (same_file)
			printf(
			       ":: data moved (%u blocks by %u bytes "
			       "in %"PRIu64" ms = ~%"PRIu64" B/us)\n",
			       b2m,
			       abs(delta),
			       (end - start) / 1000,
			       ((uint64_t)b2m * (uint64_t)bs) / (end - start)
			      );
		else
			printf(
			       ":: data copied (%u blocks "
			       "in %"PRIu64" ms = ~%"PRIu64" B/us)\n",
			       b2m,
			       (end - start) / 1000,
			       ((uint64_t)b2m * (uint64_t)bs) / (end - start)
			      );
	}

	vdi->header.offset.data = data_offset(vdi, new_blk_count);
	vdi->header.disk.size = disk_size(vdi, new_blk_count);
	vdi->header.disk.blk_count = new_blk_count;
	vdi->header.disk.blk_count_alloc = b2m;
}

static void update_block_allocation_map(vdi_start_t *vdi, int fin, int fout)
{
	uint32_t i, j;
	vdi_bam_entry_t fill[FILL_COUNT];
	uint32_t blk_count = vdi->header.disk.blk_count;
	uint32_t max_bam_entry_count = (vdi->header.offset.data -
	                                vdi->header.offset.bam) /
	                               VDI_BAM_ENTRY_SIZE;

	puts(":: fixing block allocation map");
	lseek(fout, vdi->header.offset.bam, SEEK_SET);

	for (i = 0; i < (blk_count & -FILL_COUNT); i += FILL_COUNT) {
		for (j = 0; j < FILL_COUNT; j++)
			fill[j] = i + j;
		write(fout, fill, FILL_COUNT * VDI_BAM_ENTRY_SIZE);
	}
	for (j = i; i < blk_count; i++)
		fill[i - j] = i;
	write(fout, fill, (blk_count - j) * VDI_BAM_ENTRY_SIZE);

	memset(fill, 0, FILL_COUNT * VDI_BAM_ENTRY_SIZE);
	j = ALIGN2(i, FILL_COUNT);
	write(fout, fill, (j - i) * VDI_BAM_ENTRY_SIZE);
	for (i = j; i < max_bam_entry_count; i += FILL_COUNT)
		write(fout, fill, FILL_COUNT * VDI_BAM_ENTRY_SIZE);

	vdi->header.disk.blk_count_alloc = blk_count;

	puts(":: block allocation map fixed");
}

static void update_header(vdi_start_t *vdi, int fd)
{
	puts(":: updating header");
	/* VB will fix lchs section */
	vdi->header.lchs.cylinders = 0;
	vdi->header.lchs.heads = 0;
	vdi->header.lchs.sectors = 0;
	write_start(fd, vdi);
	puts(":: header updated");
}

static int resize(vdi_start_t *vdi, int fin, int fout, uint32_t new_blk_count)
{
	puts(":: mission started");
	rewrite_data(vdi, fin, fout, new_blk_count);
	update_block_allocation_map(vdi, fin, fout);
	ftruncate(fout,
	          vdi->header.offset.data +
	          disk_size(vdi, vdi->header.disk.blk_count_alloc));
	puts(":: disk resized");
	update_header(vdi, fout);
	print_info_from_struct(vdi, 0);
	puts(":: final syncing");
	fsync(fout);
	puts(":: mission accomplished");

	return SUCCESS;
}
