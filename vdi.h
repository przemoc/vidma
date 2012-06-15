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

/** \file vdi.h
 * VDI - Virtual Disk Image.
 *
 * Format introduced by VirtualBox and mostly used by it.
 *
 * VDI consists of 3 areas:
 * - preheader + header (start),
 * - block allocation map (BAM),
 * - allocated blocks (data).
 *
 * BAM offset and data offset used to be aligned to 512 bytes in the old days,
 * but since VirtualBox v4.0 (released 2010-12-22) they changed it to 4096.
 * It's a good change considering block/cluster sizes of filesystems and
 * sector sizes of drives today.
 *
 * If BAM offset is not 4K-aligned, there is no real gain in fixing it though.
 * And there is definitely no point in decreasing once increased data offset.
 * Even if it's only 1MB, then BAM can map up to (almost) 256K blocks.
 * Take into an account standard block size = 1MB and you get (almost) 256GB.
 *
 * That's why vidma preserves BAM offset and enforces 1 megabyte alignment for
 * data offset, but only if movement of allocated blocks is unavoidable.
 * Forgive my waste.
 */

#ifndef VDI_H
#define VDI_H

#include "common.h"
#include "vd.h"

/** VDI comment size. */
#define VDI_COMMENT_SIZE (256)

/** VDI signature. */
#define VDI_SIGNATURE    (0xbeda107f)

/** VDI type identifier. */
enum vdi_type {
	/** Dynamic image. */
	VDI_DYNAMIC = 1,
	/** Fixed image. */
	VDI_FIXED,
	/** Undo image. */
	VDI_UNDO,
	/** Differencing image. */
	VDI_DIFF,
	/** Known VDI types count + 1. */
	VDI_TYPE_MAX_1
};

/** Type used in entries of BAM. */
typedef uint32_t vdi_bam_entry_t;

/** Returns size of single BAM entry in VDI. */
#define VDI_BAM_ENTRY_SIZE        sizeof(vdi_bam_entry_t)
/** Calculates BAM size for \p blk_count in VDI. */
#define VDI_BAM_SIZE(blk_count)   ((blk_count) * VDI_BAM_ENTRY_SIZE)
/** Default data offset alignment in VDI enforced by vidma. */
#define VDI_DATA_OFFSET_ALIGNMENT _1MB
/** Sector size in VDI. */
#define VDI_SECTOR_SIZE           512

/** Unallocated block. */
#define VDI_BLK_NONE              ((uint32_t)-1)
/** Unallocated block meant to be filled with zeros. */
#define VDI_BLK_ZERO              ((uint32_t)-2)

/** UUID type used in VDI. */
typedef struct vdi_uuid {
	uint32_t   part1;
	uint16_t   part2;
	uint16_t   part3;
	uint8_t    part4[2];
	uint8_t    part5[6];
}  vdi_uuid_t;
/* 16 bytes */

/** VDI preheader: file info and signature. */
typedef struct vdi_preheader {
	char       file_info[64];
	uint32_t   signature;
} vdi_preheader_t;
/* 68 bytes */

/** VDI cylinders/head/sectors/sector size information. */
typedef struct vdi_chs {
	uint32_t   cylinders;       /* = 0 */
	uint32_t   heads;           /* = 0 */
	uint32_t   sectors;         /* = 0 */
	uint32_t   sector_size;     /* = 512 */
} vdi_chs_t;
/* 16 bytes */

/** VDI offset information: BAM and data. */
typedef struct vdi_offset {
	uint32_t   bam;
	uint32_t   data;
} vdi_off_t;
/* 8 bytes */

/** VDI UUIDs of linked images. */
typedef struct vdi_uuid_set {
	vdi_uuid_t create;
	vdi_uuid_t modify;
	vdi_uuid_t linkage;
	vdi_uuid_t parent_modify;
} vdi_uset_t;
/* 64 bytes */

/** VDI disk image information. */
typedef struct vdi_disk_info {
	uint64_t   size;
	uint32_t   blk_size;
	uint32_t   blk_extra_data;
	uint32_t   blk_count;
	uint32_t   blk_count_alloc;
} vdi_di_t;
/* 24 bytes */

#pragma pack(1)
/** VDI header */
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
/* 400 bytes */
#pragma pack()

/** VDI start: preheader, version, header and garbage. */
typedef struct vdi_start {
	vdi_preheader_t pre;
	uint32_t        version;
	vdi_header_t    header;
	uint32_t        garbage[10];
} vdi_start_t;
/* 512 bytes */

extern vd_type_t vd_vdi;

#endif /* VDI_H */
