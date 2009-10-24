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

#ifndef VDI_H
#define VDI_H

#include "common.h"
#include "vd.h"

enum vdi_type {
	VDI_NORMAL = 1,
	VDI_FIXED,
	VDI_UNDO,
	VDI_DIFF
};

#define VDI_COMMENT_SIZE	(256)
#define VDI_SIGNATURE	(0xbeda107f)

/**
 * Calculate offset of the data.
 *
 * Block mapping area starts after 512 bytes (1 sector) and it's also aligned
 * to sectors (2^9 bytes each), therefore each full sector has only 128 (2^7)
 * entries.
 */
#define VDI_OFFSET_DATA(msize)	((1 + ((msize + 127) >> 7)) << 9)

#define VDI_DISK_SIZE(msize)	(((uint64_t)msize) << 20)

/* 16 bytes */
typedef struct vdi_uuid {
	uint32_t   part1;
	uint16_t   part2;
	uint16_t   part3;
	uint8_t    part4[2];
	uint8_t    part5[6];
}  vdi_uuid_t;

/* 68 bytes */
typedef struct vdi_preheader {
	char       file_info[64];
	uint32_t   signature;
} vdi_preheader_t;

/* 16 bytes */
typedef struct vdi_chs {
	uint32_t   cylinders;       /* = 0 */
	uint32_t   heads;           /* = 0 */
	uint32_t   sectors;         /* = 0 */
	uint32_t   sector_size;     /* = 512 */
} vdi_chs_t;

/* 8 bytes */
typedef struct vdi_offset {
	uint32_t   blocks;          /* = 512 */
	uint32_t   data;            /* = (1 + ((blk_count + 127) >> 7)) << 9 */
} vdi_off_t;

/* 64 bytes */
typedef struct vdi_uuid_set {
	vdi_uuid_t create;
	vdi_uuid_t modify;
	vdi_uuid_t linkage;
	vdi_uuid_t parent_modify;
} vdi_uset_t;

/* 24 bytes */
typedef struct vdi_disk_info {
	uint64_t   size;            /* in bytes */
	uint32_t   blk_size;        /* = 1MB */
	uint32_t   blk_extra_data;  /* = 0 */
	uint32_t   blk_count;
	uint32_t   blk_count_alloc;
} vdi_di_t;

/* 400 bytes */
#pragma pack(1)
typedef struct vdi_header {
	uint32_t   size;
	uint32_t   type;
	uint32_t   flags;
	char       comment[VDI_COMMENT_SIZE];
	vdi_off_t  offset;
	vdi_chs_t  pchs;
	uint32_t   dummy;           /* = 0 */
	vdi_di_t   disk;
	vdi_uset_t uuid;
	vdi_chs_t  lchs;
} vdi_header_t;
#pragma pack()

/* 512 bytes */
typedef struct vdi_start {
	vdi_preheader_t pre;
	uint32_t        version;
	vdi_header_t    header;
	uint32_t        garbage[10];
} vdi_start_t;

int vdi_check(int fd);
void vdi_print_info(int fd);
int vdi_resize(int fin, int fout, uint32_t new_msize);

extern vd_type_t vd_vdi;

#endif /* VDI_H */
