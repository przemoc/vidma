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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "common.h"
#include "vdi.h"
#include "ui.h"

/* ==== Exposed functions prototypes ======================================== */

static int vdi_detect(int fd);
static void vdi_info(int fd);
static int vdi_resize(int fin, int fout, uint32_t new_msize);

vd_type_t vd_vdi = {
	.ext = "vdi",
	.name = "Virtual Disk Image",
	.ops = {
		.detect     = vdi_detect,
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
static void find_last_blocks(vdi_start_t *vdi, int fd,
                             uint32_t *block_no, uint32_t *block_pos);
static int resize_confirmation(vdi_start_t *vdi, int fin, int fout,
                               uint32_t new_blk_count);
static inline uint32_t data_offset(vdi_start_t *vdi, uint32_t blk_count);
static inline uint32_t ext_blk_size(vdi_start_t *vdi);
static inline uint64_t ext_blk_size64(vdi_start_t *vdi);
static inline uint64_t disk_size(vdi_start_t *vdi, uint32_t blk_count);
static inline uint64_t image_data_size(vdi_start_t *vdi,
                                       uint32_t blk_count_alloc);
static inline uint64_t image_size(vdi_start_t *vdi, uint32_t blk_count);
static void rewrite_data(vdi_start_t *vdi, int fin, int fout,
                         uint32_t new_blk_count);
static inline void fill_bam_with_unallocated_entries(vdi_bam_entry_t *bam,
                                                     uint32_t n);
static inline void fill_bam_with_consecutive_values(vdi_bam_entry_t *bam,
                                                    vdi_bam_entry_t start_val,
                                                    uint32_t n);
static void update_block_allocation_map(vdi_start_t *vdi, int fin, int fout,
                                        uint32_t new_blk_count);
static void update_file_size(vdi_start_t *vdi, int fd);
static void update_header(vdi_start_t *vdi, int fd);
static int resize(vdi_start_t *vdi, int fin, int fout, uint32_t new_blk_count);

/* ==== Exposed functions definitions ======================================= */

static int vdi_detect(int fd)
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
		ui->log("Resize aborted.\n");
		return FAILURE;
	}

	return resize(&vdi, fin, fout, new_blk_count);
}

/* ==== Defines and Macros ================================================== */

#define PRINT(f,a...)  ui->log("%-*s = " f, 32, a)
#define PRINTU32NN(v,i)  PRINT("%08x %u", #i, (uint32_t)v->i, (uint32_t)v->i)
#define PRINTU32(v,i)  PRINT("%08x %u\n", #i, (uint32_t)v->i, (uint32_t)v->i)
#define PRINTSTR(v,i)  PRINT("%s\n", #i, (char *)v->i)
#define PRINTUUID(v,i) \
	do { \
		PRINT("", #i); \
		print_uuid(&(v->i)); \
		ui->log("\n"); \
	} while (0)
#define PRINTU64(v,i) \
	PRINT("%016"PRIx64" %"PRIu64"\n", #i, (uint64_t)v->i, (uint64_t)v->i)
#define PRINTNOTE(s)   ui->log(" (%s)\n", s)

#define FILL_SHIFT 4
#define FILL_COUNT (1 << FILL_SHIFT)

char *types[5] = {
	"unknown",
	"dynamic",
	"fixed",
	"undo",
	"diff"
};

/* ==== Non-exposed functions definitions =================================== */

static void print_uuid(vdi_uuid_t *uuid)
{
	ui->log("%08x-%04hx-%04hx-%02x%02x-%02x%02x%02x%02x%02x%02x",
	        uuid->part1, uuid->part2, uuid->part3,
	        uuid->part4[0], uuid->part4[1],
	        uuid->part5[0], uuid->part5[1], uuid->part5[2],
	        uuid->part5[3], uuid->part5[4], uuid->part5[5]
	       );
}

static char *type(vdi_start_t *vdi)
{
	return types[vdi->header.type < VDI_TYPE_MAX_1 ?
	             vdi->header.type : 0];
}

static void print_info_from_struct(vdi_start_t *v, int full)
{
	if (full) {
		PRINTSTR(v, pre.file_info);
		PRINTU32(v, pre.signature);
		PRINTU32(v, version);
		PRINTU32(v, header.size);
		PRINTU32NN(v, header.type);
		PRINTNOTE(type(v));
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
		ui->log("ERROR   Not supported VDI format version.\n");
		return FAILURE;
	}
	if (ext_blk_size64(vdi) > UINT32_MAX) {
		ui->log("ERROR   Block size + extra data size > %"PRIu32".\n", UINT32_MAX);
		return FAILURE;
	}
	if (vdi->header.offset.bam > vdi->header.offset.data) {
		ui->log("ERROR   Block allocation map following data is not supported.\n");
		return FAILURE;
	}
	if (vdi->header.lchs.sector_size != VDI_SECTOR_SIZE) {
		ui->log("ERROR   Sector sizes other than " Q(VDI_SECTOR_SIZE)
		        " are not supported.\n");
		return FAILURE;
	}
	if (vdi->header.type != VDI_FIXED && vdi->header.type != VDI_DYNAMIC) {
		ui->log("ERROR   Non dynamic/fixed VDI images are not supported yet.\n");
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
		errors += ui->log("ERROR   BAM overlaps file header.\n") >= 0;
	if (data_beg < start_end)
		errors += ui->log("ERROR   Data overlaps file header.\n") >= 0;
	if (!(data_beg >= bam_end && data_end >= bam_end) &&
	    !(bam_beg >= data_end && bam_end >= data_end))
		errors += ui->log("ERROR   BAM overlaps data.\n") >= 0;
	if (vdi->header.disk.blk_count_alloc > vdi->header.disk.blk_count)
		errors += ui->log("ERROR   Insane number of allocated blocks.\n") >= 0;
	if (!IS_POSITIVE_POWER_OF_2(vdi->header.disk.blk_size))
		errors += ui->log("ERROR   Block size other than "
		                  "2^n (n >= 1).\n");
	if (vdi->header.disk.blk_extra_data &&
	    !IS_POSITIVE_POWER_OF_2(vdi->header.disk.blk_extra_data))
		errors += ui->log("ERROR   Block extra data size other than "
		                  "2^n (n >= 1).\n");
	if (disksize != vdi->header.disk.size)
		errors += ui->log("ERROR   block size (%u), block count (%u) "
		                  "and disk size (%"PRIu64") mismatch.\n",
		                  vdi->header.disk.blk_size, vdi->header.disk.blk_count,
		                  vdi->header.disk.size) >= 0;

	return !errors
	       ? SUCCESS : FAILURE;
}

static void find_last_blocks(vdi_start_t *vdi, int fd,
                             uint32_t *block_no, uint32_t *block_pos)
{
	vdi_bam_entry_t *bam;
	uint32_t i;
	uint32_t blocks = vdi->header.disk.blk_count;
	uint32_t n = 0;
	uint32_t no = 0;
	uint32_t pos = 0;

	lseek(fd, vdi->header.offset.bam, SEEK_SET);
	bam = malloc(_1MB);
	while (blocks -= n) {
		n = read(fd, bam,
		         min_u64(VDI_BAM_SIZE((uint64_t)blocks), _1MB)) /
		    VDI_BAM_ENTRY_SIZE;
		for (i = 0; i < n; i++)
			if (bam[i] != VDI_BLK_NONE) {
				no = i;
				if (pos < bam[i])
					pos = bam[i];
			}
	}
	free(bam);
	if (block_no)
		*block_no = no;
	if (block_pos)
		*block_pos = pos;
}

static int resize_confirmation(vdi_start_t *vdi, int fin, int fout,
                               uint32_t new_blk_count)
{
	uint32_t last_blk_no = 0;
	uint32_t last_blk_pos = 0;
	uint32_t min_blk_count = 1;
	int32_t delta = data_offset(vdi, new_blk_count) - vdi->header.offset.data;
	uint64_t new_disk_size = disk_size(vdi, new_blk_count);
	uint64_t old_image_size = image_size(vdi, vdi->header.disk.blk_count);
	uint64_t new_image_size = image_size(vdi, new_blk_count);
	int same_file = same_file_behind_fds(fin, fout) == SUCCESS;

	ui->log("Requested disk resize\n"
	        "from %21u block(s)\nto   %21u block(s)\n"
	        "(each block has %10u bytes)\n",
	        vdi->header.disk.blk_count, new_blk_count,
	        vdi->header.disk.blk_size);

	if (vdi->header.type == VDI_DYNAMIC) {
		find_last_blocks(vdi, fin, &last_blk_no, &last_blk_pos);
		min_blk_count = max_u32(last_blk_no, last_blk_pos) + 1;
		if (new_blk_count < min_blk_count) {
			ui->log("But minimal possible block count equals\n"
			        "     %21u block(s)\n",
			        min_blk_count);
			return FAILURE;
		}
	}

	ui->log("\nDisk size will change\n"
	        "from %21"PRIu64" bytes (%15"PRIu64" MB)\n"
	        "to   %21"PRIu64" bytes (%15"PRIu64" MB)\n"
	        "\n",
	        vdi->header.disk.size, vdi->header.disk.size / _1MB,
	        new_disk_size, new_disk_size / _1MB);
	ui->log("Image size will change\n"
	        "from %21"PRIu64" bytes (%15"PRIu64" MB)\n"
	        "to   %21"PRIu64" bytes (%15"PRIu64" MB)\n"
	        "\n",
	        old_image_size, old_image_size / _1MB,
	        new_image_size, new_image_size / _1MB);

	if (same_file) {
		ui->log("Resize operation will be performed in-place.\n");
		if (delta)
			ui->log("WARNING All allocated blocks require moving.\n"
			        "        In case of fail DATA LOSS is highly POSSIBLE!\n"
			        "        Think twice before continuing!\n");
		else
			ui->log("CAUTION Only disk metadata will be modified.\n"
			        "        In case of fail data loss is highly unlikely,\n"
			        "        but image METADATA CAN BE CORRUPTED!\n");
		if (vdi->header.disk.size > new_disk_size)
			ui->log("WARNING Shrinking disk in-place means\n"
			        "        IRRETRIEVABLY LOSING DATA KEPT BEYOND NEW SIZE!\n");
	} else {
		ui->log("Resize operation in fact will create resized copy of the image.\n");
		ui->log("NOTE    UUID of the new image will be the same as old one.\n");
		ui->log("NOTE    Input file is safe and won't be modified.\n");
	}

	return ui->yesno("Are you sure you want to continue?");
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

static inline uint32_t ext_blk_size(vdi_start_t *vdi)
{
	return vdi->header.disk.blk_extra_data + vdi->header.disk.blk_size;
}

static inline uint64_t ext_blk_size64(vdi_start_t *vdi)
{
	return (uint64_t)vdi->header.disk.blk_extra_data +
	       (uint64_t)vdi->header.disk.blk_size;
}

static inline uint64_t disk_size(vdi_start_t *vdi, uint32_t blk_count)
{
	return ((uint64_t)blk_count) * ((uint64_t)vdi->header.disk.blk_size);
}

static inline uint64_t image_data_size(vdi_start_t *vdi,
                                       uint32_t blk_count_alloc)
{
	return (uint64_t)ext_blk_size(vdi) * blk_count_alloc;
}

static inline uint64_t image_size(vdi_start_t *vdi, uint32_t blk_count)
{
	uint32_t blocks = blk_count;

	if (vdi->header.type == VDI_DYNAMIC)
		blocks = min_u32(vdi->header.disk.blk_count_alloc, blk_count);

	return data_offset(vdi, blk_count) + image_data_size(vdi, blocks);
}

static void rewrite_data(vdi_start_t *vdi, int fin, int fout,
                         uint32_t new_blk_count)
{
	uint32_t i;
	char *buffer;
	uint64_t start, end;
	uint32_t ebs = ext_blk_size(vdi);
	uint32_t blocks = min_u32(vdi->header.disk.blk_count_alloc, new_blk_count);
	int32_t delta = data_offset(vdi, new_blk_count) - vdi->header.offset.data;
	int same_file = (same_file_behind_fds(fin, fout) == SUCCESS);

	lseek(fin, vdi->header.offset.data, SEEK_SET);
	lseek(fout, data_offset(vdi, new_blk_count), SEEK_SET);

	if (delta || !same_file) {
		ui->next_step(same_file ? "Moving blocks" : "Copying blocks");
		ui->set_step_prog_max(blocks);
		buffer = malloc(ebs + delta * (delta > 0));
		start = gettimeofday_us();
		if (delta > 0) {
			read(fin, buffer + ebs, delta);
			for (i = 1; i <= blocks; i++) {
				ui->set_step_prog_val(i);
				memcpy(buffer, buffer + ebs, delta);
				read(fin, buffer + delta, ebs);
				write(fout, buffer, ebs);
			}
		} else
			for (i = 1; i <= blocks; i++) {
				ui->set_step_prog_val(i);
				read(fin, buffer, ebs);
				write(fout, buffer, ebs);
			}
		ui->log("Syncing\n");
		fsync(fout);
		end = gettimeofday_us();
		free(buffer);
		if (same_file)
			ui->log(
			        "Data moved (%u blocks by %u bytes "
			        "in %"PRIu64" ms = ~%"PRIu64" B/us)\n",
			        blocks,
			        abs(delta),
			        (end - start) / 1000,
			        ((uint64_t)blocks * (uint64_t)ebs) / (end - start)
			       );
		else
			ui->log(
			        "Data copied (%u blocks "
			        "in %"PRIu64" ms = ~%"PRIu64" B/us)\n",
			        blocks,
			        (end - start) / 1000,
			        ((uint64_t)blocks * (uint64_t)ebs) / (end - start)
			       );
	} else {
		ui->next_step("No need to move blocks");
		ui->set_step_prog_val(1);
	}

	vdi->header.offset.data = data_offset(vdi, new_blk_count);
	vdi->header.disk.size = disk_size(vdi, new_blk_count);
	vdi->header.disk.blk_count_alloc = blocks;
}

static inline void fill_bam_with_unallocated_entries(vdi_bam_entry_t *bam,
                                                     uint32_t n)
{
	for (uint32_t i = 0; i < n; i++)
		bam[i] = VDI_BLK_NONE;
}

static inline void fill_bam_with_consecutive_values(vdi_bam_entry_t *bam,
                                                    vdi_bam_entry_t start_val,
                                                    uint32_t n)
{
	for (uint32_t i = 0; i < n; i++)
		bam[i] = start_val + i;
}

static void update_block_allocation_map(vdi_start_t *vdi, int fin, int fout,
                                        uint32_t new_blk_count)
{
	uint32_t i, j;
	char *buffer;
	ssize_t size;
	vdi_bam_entry_t fill[FILL_COUNT];
	uint32_t blk_count = min_u32(vdi->header.disk.blk_count, new_blk_count);
	uint32_t total_end = (vdi->header.offset.data - vdi->header.offset.bam) /
	                     VDI_BAM_ENTRY_SIZE;
	int same_file = (same_file_behind_fds(fin, fout) == SUCCESS);

	ui->next_step("Updating block allocation map");

	/* Copy old BAM if needed. */
	if (!same_file) {
		lseek(fin, vdi->header.offset.bam, SEEK_SET);
		lseek(fout, vdi->header.offset.bam, SEEK_SET);
		buffer = malloc(_1MB);
		i = blk_count;
		while (i) {
			size = read(fin, buffer,
			            min_u64(VDI_BAM_SIZE((uint64_t)i), _1MB));
			write(fout, buffer, size);
			i -= size / VDI_BAM_ENTRY_SIZE;
		}
		free(buffer);
	}

	/* Fill new entries. */
	if (new_blk_count > blk_count) {
		i = blk_count;
		lseek(fout, vdi->header.offset.bam + VDI_BAM_SIZE(i), SEEK_SET);
		j = ALIGN2(i, FILL_COUNT);
		if (vdi->header.type == VDI_DYNAMIC) {
			fill_bam_with_unallocated_entries(fill, FILL_COUNT);
			i += write(fout, fill,
			           VDI_BAM_SIZE((uint64_t)min_u32(j - i,
			                                          new_blk_count - i))) /
			     VDI_BAM_ENTRY_SIZE;
			for (; i < (new_blk_count & -FILL_COUNT); i += FILL_COUNT)
				write(fout, fill, VDI_BAM_SIZE(FILL_COUNT));
			write(fout, fill, VDI_BAM_SIZE(new_blk_count - i));
		} else if (vdi->header.type == VDI_FIXED) {
			fill_bam_with_consecutive_values(fill, i, FILL_COUNT);
			i += write(fout, fill,
			           VDI_BAM_SIZE((uint64_t)min_u32(j - i,
			                                          new_blk_count - i))) /
			     VDI_BAM_ENTRY_SIZE;
			for (; i < (new_blk_count & -FILL_COUNT); i += FILL_COUNT) {
				fill_bam_with_consecutive_values(fill, i, FILL_COUNT);
				write(fout, fill, VDI_BAM_SIZE(FILL_COUNT));
			}
			fill_bam_with_consecutive_values(fill, i, new_blk_count - i);
			write(fout, fill, VDI_BAM_SIZE(new_blk_count - i));

			vdi->header.disk.blk_count_alloc = new_blk_count;
		}
	}

	vdi->header.disk.blk_count = new_blk_count;

	/* Fill with 0 area between BAM end and data beginning. */
	i = new_blk_count;
	lseek(fout, vdi->header.offset.bam + VDI_BAM_SIZE(i), SEEK_SET);
	j = ALIGN2(i, FILL_COUNT);
	memset(fill, 0, VDI_BAM_SIZE(FILL_COUNT));
	i += write(fout, fill,
	           VDI_BAM_SIZE((uint64_t)min_u32(j - i, total_end - i))) /
	     VDI_BAM_ENTRY_SIZE;
	for (; i < (total_end & -FILL_COUNT); i += FILL_COUNT)
		write(fout, fill, VDI_BAM_SIZE(FILL_COUNT));
	write(fout, fill, VDI_BAM_SIZE(total_end - i));

	ui->set_step_prog_val(1);
	ui->log("Syncing\n");
	fsync(fout);
}

static void update_file_size(vdi_start_t *vdi, int fd)
{
	ui->next_step("Updating file size");
	ftruncate(fd,
	          vdi->header.offset.data +
	          image_data_size(vdi, vdi->header.disk.blk_count_alloc));
	ui->set_step_prog_val(1);
	ui->log("Syncing\n");
	fsync(fd);
}

static void update_header(vdi_start_t *vdi, int fd)
{
	ui->next_step("Updating header");
	/* VB will fix lchs section */
	vdi->header.lchs.cylinders = 0;
	vdi->header.lchs.heads = 0;
	vdi->header.lchs.sectors = 0;
	write_start(fd, vdi);
	ui->set_step_prog_val(1);
	ui->log("Syncing\n");
	fsync(fd);
}

static int resize(vdi_start_t *vdi, int fin, int fout, uint32_t new_blk_count)
{
	ui->start_op("Resize", 4);
	rewrite_data(vdi, fin, fout, new_blk_count);
	update_block_allocation_map(vdi, fin, fout, new_blk_count);
	update_file_size(vdi, fout);
	update_header(vdi, fout);
	ui->end_op();
	ui->log("\n");
	print_info_from_struct(vdi, 0);

	return SUCCESS;
}
