#include <stdio.h>
#include <stdlib.h>
#include "type.h"
#include "list.h"

/* The NTFS oem_id "NTFS    " */
#define NTFS_SB_MAGIC	const_cpu_to_u64(0x202020205346544eULL)

/**
 * struct BIOS_PARAMETER_BLOCK - BIOS parameter block (BPB) structure.
 */
typedef struct {
	u16 bytes_per_sector;		/* Size of a sector in bytes. */
	u8  sectors_per_cluster;	/* Size of a cluster in sectors. */
	u16 reserved_sectors;		/* zero */
	u8  fats;			/* zero */
	u16 root_entries;		/* zero */
	u16 sectors;			/* zero */
	u8  media_type;			/* 0xf8 = hard disk */
	u16 sectors_per_fat;		/* zero */
/*0x0d*/u16 sectors_per_track;		/* Required to boot Windows. */
/*0x0f*/u16 heads;			/* Required to boot Windows. */
/*0x11*/u32 hidden_sectors;		/* Offset to the start of the partition
					   relative to the disk in sectors.
					   Required to boot Windows. */
/*0x15*/u32 large_sectors;		/* zero */
/* sizeof() = 25 (0x19) bytes */
} __attribute__((__packed__)) BIOS_PARAMETER_BLOCK;

/**
 * struct NTFS_BOOT_SECTOR - NTFS boot sector structure.
 */
typedef struct {
	u8  jump[3];			/* Irrelevant (jump to boot up code).*/
	u64 oem_id;			/* Magic "NTFS    ". */
/*0x0b*/BIOS_PARAMETER_BLOCK bpb;	/* See BIOS_PARAMETER_BLOCK. */
	u8 physical_drive;		/* 0x00 floppy, 0x80 hard disk */
	u8 current_head;		/* zero */
	u8 extended_boot_signature; 	/* 0x80 */
	u8 reserved2;			/* zero */
/*0x28*/s64 number_of_sectors;	/* Number of sectors in volume. Gives
					   maximum volume size of 2^63 sectors.
					   Assuming standard sector size of 512
					   bytes, the maximum byte size is
					   approx. 4.7x10^21 bytes. (-; */
	s64 mft_lcn;			/* Cluster location of mft data. */
	s64 mftmirr_lcn;		/* Cluster location of copy of mft. */
	s8  clusters_per_mft_record;	/* Mft record size in clusters. */
	u8  reserved0[3];		/* zero */
	s8  clusters_per_index_record;	/* Index block size in clusters. */
	u8  reserved1[3];		/* zero */
	u64 volume_serial_number;	/* Irrelevant (serial number). */
	u32 checksum;			/* Boot sector checksum. */
/*0x54*/u8  bootstrap[426];		/* Irrelevant (boot up code). */
	u16 end_of_sector_marker;	/* End of boot sector magic. Always is
					   0xaa55 in little endian. */
/* sizeof() = 512 (0x200) bytes */
} __attribute__((__packed__)) NTFS_BOOT_SECTOR;


/**
 * enum NTFS_RECORD_TYPES -
 *
 * Magic identifiers present at the beginning of all ntfs record containing
 * records (like mft records for example).
 */
typedef enum {
	/* Found in $MFT/$DATA. */
	magic_FILE = (u32)(0x454c4946), /* Mft entry. */
	magic_INDX = (u32)(0x58444e49), /* Index buffer. */
	magic_HOLE = (u32)(0x454c4f48), /* ? (NTFS 3.0+?) */

	/* Found in $LogFile/$DATA. */
	magic_RSTR = (u32)(0x52545352), /* Restart page. */
	magic_RCRD = (u32)(0x44524352), /* Log record page. */

	/* Found in $LogFile/$DATA.  (May be found in $MFT/$DATA, also?) */
	magic_CHKD = (u32)(0x444b4843), /* Modified by chkdsk. */

	/* Found in all ntfs record containing records. */
	magic_BAAD = (u32)(0x44414142), /* Failed multi sector
						       transfer was detected. */

	/*
	 * Found in $LogFile/$DATA when a page is full or 0xff bytes and is
	 * thus not initialized.  User has to initialize the page before using
	 * it.
	 */
	magic_empty = (u32)(0xffffffff),/* Record is empty and has
						       to be initialized before
						       it can be used. */
} NTFS_RECORD_TYPES;


/**
 * enum MFT_RECORD_FLAGS -
 *
 * These are the so far known MFT_RECORD_* flags (16-bit) which contain
 * information about the mft record in which they are present.
 *
 * MFT_RECORD_IS_4 exists on all $Extend sub-files.
 * It seems that it marks it is a metadata file with MFT record >24, however,
 * it is unknown if it is limited to metadata files only.
 *
 * MFT_RECORD_IS_VIEW_INDEX exists on every metafile with a non directory
 * index, that means an INDEX_ROOT and an INDEX_ALLOCATION with a name other
 * than "$I30". It is unknown if it is limited to metadata files only.
 */
typedef enum {
	MFT_RECORD_IN_USE		= (u16)(0x0001),
	MFT_RECORD_IS_DIRECTORY		= (u16)(0x0002),
	MFT_RECORD_IS_4			= (u16)(0x0004),
	MFT_RECORD_IS_VIEW_INDEX	= (u16)(0x0008),
	MFT_REC_SPACE_FILLER		= (u16)(0xffff),
					/* Just to make flags 16-bit. */
} __attribute__((__packed__)) MFT_RECORD_FLAGS;




/**
 * enum FILE_ATTR_FLAGS - File attribute flags (32-bit).
 */
typedef enum {
	/*
	 * These flags are only present in the STANDARD_INFORMATION attribute
	 * (in the field file_attributes).
	 */
	FILE_ATTR_READONLY		= (u32)(0x00000001),
	FILE_ATTR_HIDDEN		= (u32)(0x00000002),
	FILE_ATTR_SYSTEM		= (u32)(0x00000004),
	/* Old DOS valid. Unused in NT.	= cpu_to_le32(0x00000008), */

	FILE_ATTR_DIRECTORY		= (u32)(0x00000010),
	/* FILE_ATTR_DIRECTORY is not considered valid in NT. It is reserved
	   for the DOS SUBDIRECTORY flag. */
	FILE_ATTR_ARCHIVE		= (u32)(0x00000020),
	FILE_ATTR_DEVICE		= (u32)(0x00000040),
	FILE_ATTR_NORMAL		= (u32)(0x00000080),

	FILE_ATTR_TEMPORARY		= (u32)(0x00000100),
	FILE_ATTR_SPARSE_FILE		= (u32)(0x00000200),
	FILE_ATTR_REPARSE_POINT		= (u32)(0x00000400),
	FILE_ATTR_COMPRESSED		= (u32)(0x00000800),

	FILE_ATTR_OFFLINE		= (u32)(0x00001000),
	FILE_ATTR_NOT_CONTENT_INDEXED	= (u32)(0x00002000),
	FILE_ATTR_ENCRYPTED		= (u32)(0x00004000),

	FILE_ATTR_VALID_FLAGS		= (u32)(0x00007fb7),
	/* FILE_ATTR_VALID_FLAGS masks out the old DOS VolId and the
	   FILE_ATTR_DEVICE and preserves everything else. This mask
	   is used to obtain all flags that are valid for reading. */
	FILE_ATTR_VALID_SET_FLAGS	= (u32)(0x000031a7),
	/* FILE_ATTR_VALID_SET_FLAGS masks out the old DOS VolId, the
	   FILE_ATTR_DEVICE, FILE_ATTR_DIRECTORY, FILE_ATTR_SPARSE_FILE,
	   FILE_ATTR_REPARSE_POINT, FILE_ATRE_COMPRESSED and FILE_ATTR_ENCRYPTED
	   and preserves the rest. This mask is used to to obtain all flags that
	   are valid for setting. */

	/**
	 * FILE_ATTR_I30_INDEX_PRESENT - Is it a directory?
	 *
	 * This is a copy of the MFT_RECORD_IS_DIRECTORY bit from the mft
	 * record, telling us whether this is a directory or not, i.e. whether
	 * it has an index root attribute named "$I30" or not.
	 *
	 * This flag is only present in the FILE_NAME attribute (in the
	 * file_attributes field).
	 */
	FILE_ATTR_I30_INDEX_PRESENT	= (u32)(0x10000000),

	/**
	 * FILE_ATTR_VIEW_INDEX_PRESENT - Does have a non-directory index?
	 *
	 * This is a copy of the MFT_RECORD_IS_VIEW_INDEX bit from the mft
	 * record, telling us whether this file has a view index present (eg.
	 * object id index, quota index, one of the security indexes and the
	 * reparse points index).
	 *
	 * This flag is only present in the $STANDARD_INFORMATION and
	 * $FILE_NAME attributes.
	 */
	FILE_ATTR_VIEW_INDEX_PRESENT	= (u32)(0x20000000),
} __attribute__((__packed__)) FILE_ATTR_FLAGS;



/**
 * enum FILE_NAME_TYPE_FLAGS - Possible namespaces for filenames in ntfs.
 * (8-bit).
 */
typedef enum {
	FILE_NAME_POSIX			= 0x00,
		/* This is the largest namespace. It is case sensitive and
		   allows all Unicode characters except for: '\0' and '/'.
		   Beware that in WinNT/2k files which eg have the same name
		   except for their case will not be distinguished by the
		   standard utilities and thus a "del filename" will delete
		   both "filename" and "fileName" without warning. */
	FILE_NAME_WIN32			= 0x01,
		/* The standard WinNT/2k NTFS long filenames. Case insensitive.
		   All Unicode chars except: '\0', '"', '*', '/', ':', '<',
		   '>', '?', '\' and '|'. Further, names cannot end with a '.'
		   or a space. */
	FILE_NAME_DOS			= 0x02,
		/* The standard DOS filenames (8.3 format). Uppercase only.
		   All 8-bit characters greater space, except: '"', '*', '+',
		   ',', '/', ':', ';', '<', '=', '>', '?' and '\'. */
	FILE_NAME_WIN32_AND_DOS		= 0x03,
		/* 3 means that both the Win32 and the DOS filenames are
		   identical and hence have been saved in this single filename
		   record. */
} __attribute__((__packed__)) FILE_NAME_TYPE_FLAGS;


/**
 * struct MFT_RECORD - An MFT record layout (NTFS 3.1+)
 *
 * The mft record header present at the beginning of every record in the mft.
 * This is followed by a sequence of variable length attribute records which
 * is terminated by an attribute of type AT_END which is a truncated attribute
 * in that it only consists of the attribute type code AT_END and none of the
 * other members of the attribute structure are present.
 */
typedef struct {
/*Ofs*/
/*  0	NTFS_RECORD; -- Unfolded here as gcc doesn't like unnamed structs. */
	NTFS_RECORD_TYPES magic;/* Usually the magic is "FILE". */
	u16 usa_ofs;		/* See NTFS_RECORD definition above. */
	u16 usa_count;		/* See NTFS_RECORD definition above. */

/*  8*/	leLSN lsn;		/* $LogFile sequence number for this record.
				   Changed every time the record is modified. */
/* 16*/	u16 sequence_number;	/* Number of times this mft record has been
				   reused. (See description for MFT_REF
				   above.) NOTE: The increment (skipping zero)
				   is done when the file is deleted. NOTE: If
				   this is zero it is left zero. */
/* 18*/	u16 link_count;	/* Number of hard links, i.e. the number of
				   directory entries referencing this record.
				   NOTE: Only used in mft base records.
				   NOTE: When deleting a directory entry we
				   check the link_count and if it is 1 we
				   delete the file. Otherwise we delete the
				   FILE_NAME_ATTR being referenced by the
				   directory entry from the mft record and
				   decrement the link_count.
				   FIXME: Careful with Win32 + DOS names! */
/* 20*/	u16 attrs_offset;	/* Byte offset to the first attribute in this
				   mft record from the start of the mft record.
				   NOTE: Must be aligned to 8-byte boundary. */
/* 22*/	MFT_RECORD_FLAGS flags;	/* Bit array of MFT_RECORD_FLAGS. When a file
				   is deleted, the MFT_RECORD_IN_USE flag is
				   set to zero. */
/* 24*/	u32 bytes_in_use;	/* Number of bytes used in this mft record.
				   NOTE: Must be aligned to 8-byte boundary. */
/* 28*/	u32 bytes_allocated;	/* Number of bytes allocated for this mft
				   record. This should be equal to the mft
				   record size. */
/* 32*/	leMFT_REF base_mft_record;/* This is zero for base mft records.
				   When it is not zero it is a mft reference
				   pointing to the base mft record to which
				   this record belongs (this is then used to
				   locate the attribute list attribute present
				   in the base record which describes this
				   extension record and hence might need
				   modification when the extension record
				   itself is modified, also locating the
				   attribute list also means finding the other
				   potential extents, belonging to the non-base
				   mft record). */
/* 40*/	u16 next_attr_instance; /* The instance number that will be
				   assigned to the next attribute added to this
				   mft record. NOTE: Incremented each time
				   after it is used. NOTE: Every time the mft
				   record is reused this number is set to zero.
				   NOTE: The first instance number is always 0.
				 */
/* The below fields are specific to NTFS 3.1+ (Windows XP and above): */
/* 42*/ u16 reserved;		/* Reserved/alignment. */
/* 44*/ u32 mft_record_number;	/* Number of this mft record. */
/* sizeof() = 48 bytes */
/*
 * When (re)using the mft record, we place the update sequence array at this
 * offset, i.e. before we start with the attributes. This also makes sense,
 * otherwise we could run into problems with the update sequence array
 * containing in itself the last two bytes of a sector which would mean that
 * multi sector transfer protection wouldn't work. As you can't protect data
 * by overwriting it since you then can't get it back...
 * When reading we obviously use the data from the ntfs record header.
 */
} __attribute__((__packed__)) MFT_RECORD;




/* Forward declaration */
typedef struct _ntfs_volume ntfs_volume;
typedef struct _ntfs_inode ntfs_inode;
typedef struct _ntfs_attr ntfs_attr;
typedef struct _runlist_element runlist_element;
typedef runlist_element runlist;



/**
 * struct _runlist_element - in memory vcn to lcn mapping array element.
 * @vcn:	starting vcn of the current array element
 * @lcn:	starting lcn of the current array element
 * @length:	length in clusters of the current array element
 *
 * The last vcn (in fact the last vcn + 1) is reached when length == 0.
 *
 * When lcn == -1 this means that the count vcns starting at vcn are not
 * physically allocated (i.e. this is a hole / data is sparse).
 */
struct _runlist_element {/* In memory vcn to lcn mapping structure element. */
	VCN vcn;	/* vcn = Starting virtual cluster number. */
	LCN lcn;	/* lcn = Starting logical cluster number. */
	s64 length;	/* Run length in clusters. */
};



/**
 * enum ATTR_TYPES - System defined attributes (32-bit).
 *
 * Each attribute type has a corresponding attribute name (Unicode string of
 * maximum 64 character length) as described by the attribute definitions
 * present in the data attribute of the $AttrDef system file.
 *
 * On NTFS 3.0 volumes the names are just as the types are named in the below
 * enum exchanging AT_ for the dollar sign ($). If that isn't a revealing
 * choice of symbol... (-;
 */
typedef enum {
	AT_UNUSED			= (u32)(         0),
	AT_STANDARD_INFORMATION		= (u32)(      0x10),
	AT_ATTRIBUTE_LIST		= (u32)(      0x20),
	AT_FILE_NAME			= (u32)(      0x30),
	AT_OBJECT_ID			= (u32)(      0x40),
	AT_SECURITY_DESCRIPTOR		= (u32)(      0x50),
	AT_VOLUME_NAME			= (u32)(      0x60),
	AT_VOLUME_INFORMATION		= (u32)(      0x70),
	AT_DATA				= (u32)(      0x80),
	AT_INDEX_ROOT			= (u32)(      0x90),
	AT_INDEX_ALLOCATION		= (u32)(      0xa0),
	AT_BITMAP			= (u32)(      0xb0),
	AT_REPARSE_POINT		= (u32)(      0xc0),
	AT_EA_INFORMATION		= (u32)(      0xd0),
	AT_EA				= (u32)(      0xe0),
	AT_PROPERTY_SET			= (u32)(      0xf0),
	AT_LOGGED_UTILITY_STREAM	= (u32)(     0x100),
	AT_FIRST_USER_DEFINED_ATTRIBUTE	= (u32)(    0x1000),
	AT_END				= (u32)(0xffffffff),
} ATTR_TYPES;


/**
 * struct ntfs_attr - ntfs in memory non-resident attribute structure
 * @rl:			if not NULL, the decompressed runlist
 * @ni:			base ntfs inode to which this attribute belongs
 * @type:		attribute type
 * @name:		Unicode name of the attribute
 * @name_len:		length of @name in Unicode characters
 * @state:		NTFS attribute specific flags describing this attribute
 * @allocated_size:	copy from the attribute record
 * @data_size:		copy from the attribute record
 * @initialized_size:	copy from the attribute record
 * @compressed_size:	copy from the attribute record
 * @compression_block_size:		size of a compression block (cb)
 * @compression_block_size_bits:	log2 of the size of a cb
 * @compression_block_clusters:		number of clusters per cb
 * @crypto:		(valid only for encrypted) see description below
 *
 * This structure exists purely to provide a mechanism of caching the runlist
 * of an attribute. If you want to operate on a particular attribute extent,
 * you should not be using this structure at all. If you want to work with a
 * resident attribute, you should not be using this structure at all. As a
 * fail-safe check make sure to test NAttrNonResident() and if it is false, you
 * know you shouldn't be using this structure.
 *
 * If you want to work on a resident attribute or on a specific attribute
 * extent, you should use ntfs_lookup_attr() to retrieve the attribute (extent)
 * record, edit that, and then write back the mft record (or set the
 * corresponding ntfs inode dirty for delayed write back).
 *
 * @rl is the decompressed runlist of the attribute described by this
 * structure. Obviously this only makes sense if the attribute is not resident,
 * i.e. NAttrNonResident() is true. If the runlist hasn't been decompressed yet
 * @rl is NULL, so be prepared to cope with @rl == NULL.
 *
 * @ni is the base ntfs inode of the attribute described by this structure.
 *
 * @type is the attribute type (see layout.h for the definition of ATTR_TYPES),
 * @name and @name_len are the little endian Unicode name and the name length
 * in Unicode characters of the attribute, respectively.
 *
 * @state contains NTFS attribute specific flags describing this attribute
 * structure. See ntfs_attr_state_bits above.
 *
 * @crypto points to private structure of crypto code. You should not access
 * fields of this structure, but you can check whether it is NULL or not. If it
 * is not NULL, then we successfully obtained FEK (File Encryption Key) and
 * ntfs_attr_p{read,write} calls probably would succeed. If it is NULL, then we
 * failed to obtain FEK (do not have corresponding PFX file, wrong password,
 * etc..) or library was compiled without crypto support. Attribute size can be
 * changed without knowledge of FEK, so you can use ntfs_attr_truncate in any
 * case.
 * NOTE: This field valid only if attribute encrypted (eg., NAttrEncrypted
 * returns non-zero).
 */
struct _ntfs_attr {
	runlist_element *rl;
	ntfs_inode *ni;
	ATTR_TYPES type;
	ntfschar *name;
	u32 name_len;
	unsigned long state;
	s64 allocated_size;
	s64 data_size;
	s64 initialized_size;
	s64 compressed_size;
	u32 compression_block_size;
	u8 compression_block_size_bits;
	u8 compression_block_clusters;
	u32 crypto; // don't care
	struct list_head list_entry;
	int nr_references;
};


/**
 * struct _ntfs_inode - The NTFS in-memory inode structure.
 *
 * It is just used as an extension to the fields already provided in the VFS
 * inode.
 */
struct _ntfs_inode {
	u64 mft_no;		/* Inode / mft record number. */
	MFT_RECORD *mrec;	/* The actual mft record of the inode. */
	ntfs_volume *vol;	/* Pointer to the ntfs volume of this inode. */
	unsigned long state;	/* NTFS specific flags describing this inode.
				   See ntfs_inode_state_bits above. */
	FILE_ATTR_FLAGS flags;	/* Flags describing the file.
				   (Copy from STANDARD_INFORMATION) */
	/*
	 * Attribute list support (for use by the attribute lookup functions).
	 * Setup during ntfs_open_inode() for all inodes with attribute lists.
	 * Only valid if NI_AttrList is set in state.
	 */
	u32 attr_list_size;	/* Length of attribute list value in bytes. */
	u8 *attr_list;		/* Attribute list value itself. */
	/* Below fields are always valid. */
	s32 nr_extents;		/* For a base mft record, the number of
				   attached extent inodes (0 if none), for
				   extent records this is -1. */
	union {		/* This union is only used if nr_extents != 0. */
		ntfs_inode **extent_nis;/* For nr_extents > 0, array of the
					   ntfs inodes of the extent mft
					   records belonging to this base
					   inode which have been loaded. */
		ntfs_inode *base_ni;	/* For nr_extents == -1, the ntfs
					   inode of the base mft record. */
	};

	/* Below fields are valid only for base inode. */

	/*
	 * These two fields are used to sync filename index and guaranteed to be
	 * correct, however value in index itself maybe wrong (windows itself
	 * do not update them properly).
	 */
	s64 data_size;		/* Data size of unnamed DATA attribute. */
	s64 allocated_size;	/* Allocated size stored in the filename
				   index. (NOTE: Equal to allocated size of
				   the unnamed data attribute for normal or
				   encrypted files and to compressed size
				   of the unnamed data attribute for sparse or
				   compressed files.) */

	/*
	 * These four fields are copy of relevant fields from
	 * STANDARD_INFORMATION attribute and used to sync it and FILE_NAME
	 * attribute in the index.
	 */
	time_t creation_time;
	time_t last_data_change_time;
	time_t last_mft_change_time;
	time_t last_access_time;

	/* These 2 fields are used to keep track of opened inodes. */
	struct list_head list_entry;	/* Keep pointers to the next/prev list
					   entry. */
	int nr_references;		/* How many times this inode was
					   opened.  We really close inode only
					   when this reaches zero. */

	struct list_head attr_cache;	/* List of opened attributes. */
};

/**
 * struct _ntfs_volume - structure describing an open volume in memory.
 */
struct _ntfs_volume {
	union {
		struct ntfs_device *dev;	/* NTFS device associated with
						   the volume. */
		void *sb;	/* For kernel porting compatibility. */
	};
	char *vol_name;		/* Name of the volume. */
	unsigned long state;	/* NTFS specific flags describing this volume.
				   See ntfs_volume_state_bits above. */

	ntfs_inode *vol_ni;	/* ntfs_inode structure for FILE_Volume. */
	u8 major_ver;		/* Ntfs major version of volume. */
	u8 minor_ver;		/* Ntfs minor version of volume. */
	u16 flags;		/* Bit array of VOLUME_* flags. */
	u16 guid;		/* The volume guid if present (otherwise it is
				   a NULL guid). */

	u16 sector_size;	/* Byte size of a sector. */
	u8 sector_size_bits;	/* Log(2) of the byte size of a sector. */
	u32 cluster_size;	/* Byte size of a cluster. */
	u32 mft_record_size;	/* Byte size of a mft record. */
	u32 indx_record_size;	/* Byte size of a INDX record. */
	u8 cluster_size_bits;	/* Log(2) of the byte size of a cluster. */
	u8 mft_record_size_bits;/* Log(2) of the byte size of a mft record. */
	u8 indx_record_size_bits;/* Log(2) of the byte size of a INDX record. */

	/* Variables used by the cluster and mft allocators. */
	u8 mft_zone_multiplier;	/* Initial mft zone multiplier. */
	s64 mft_data_pos;	/* Mft record number at which to allocate the
				   next mft record. */
	LCN mft_zone_start;	/* First cluster of the mft zone. */
	LCN mft_zone_end;	/* First cluster beyond the mft zone. */
	LCN mft_zone_pos;	/* Current position in the mft zone. */
	LCN data1_zone_pos;	/* Current position in the first data zone. */
	LCN data2_zone_pos;	/* Current position in the second data zone. */

	s64 nr_clusters;	/* Volume size in clusters, hence also the
				   number of bits in lcn_bitmap. */
	ntfs_inode *lcnbmp_ni;	/* ntfs_inode structure for FILE_Bitmap. */
	ntfs_attr *lcnbmp_na;	/* ntfs_attr structure for the data attribute
				   of FILE_Bitmap. Each bit represents a
				   cluster on the volume, bit 0 representing
				   lcn 0 and so on. A set bit means that the
				   cluster and vice versa. */

	LCN mft_lcn;		/* Logical cluster number of the data attribute
				   for FILE_MFT. */
	ntfs_inode *mft_ni;	/* ntfs_inode structure for FILE_MFT. */
	ntfs_attr *mft_na;	/* ntfs_attr structure for the data attribute
				   of FILE_MFT. */
	ntfs_attr *mftbmp_na;	/* ntfs_attr structure for the bitmap attribute
				   of FILE_MFT. Each bit represents an mft
				   record in the $DATA attribute, bit 0
				   representing mft record 0 and so on. A set
				   bit means that the mft record is in use and
				   vice versa. */

	int mftmirr_size;	/* Size of the FILE_MFTMirr in mft records. */
	LCN mftmirr_lcn;	/* Logical cluster number of the data attribute
				   for FILE_MFTMirr. */
	ntfs_inode *mftmirr_ni;	/* ntfs_inode structure for FILE_MFTMirr. */
	ntfs_attr *mftmirr_na;	/* ntfs_attr structure for the data attribute
				   of FILE_MFTMirr. */

	ntfschar *upcase;	/* Upper case equivalents of all 65536 2-byte
				   Unicode characters. Obtained from
				   FILE_UpCase. */
	u32 upcase_len;		/* Length in Unicode characters of the upcase
				   table. */

	u32 *attrdef;	      /* Attribute definitions. Obtained from
				   FILE_AttrDef. */
	s32 attrdef_len;	/* Size of the attribute definition table in
				   bytes. */

	long nr_free_clusters;	/* This two are self explaining. */
	long nr_free_mft_records;

	struct list_head inode_cache[512]; /* List of opened
								inodes. */
};



struct filename {
   struct list_head list;                    /* Previous/Next links */
   ntfschar         *uname;                  /* Filename in unicode */
   int              uname_len;               /* and its length */
   long long        size_alloc;              /* Allocated size (multiple of cluster size) */
   long long        size_data;               /* Actual size of data */
   FILE_ATTR_FLAGS  flags;
   time_t           date_c;                  /* Time created */
   time_t           date_a;                  /*	     altered */
   time_t           date_m;                  /*	     mft record changed */
   time_t           date_r;	             /*	     read */
   char             *name;		     /* Filename in current locale */
   FILE_NAME_TYPE_FLAGS name_space;
   leMFT_REF	    parent_mref;
   char		    *parent_name;
};


struct data {
	struct list_head list;		/* Previous/Next links */
	char		*name;		/* Stream name in current locale */
	ntfschar	*uname;		/* Unicode stream name */
	int		 uname_len;	/* and its length */
	int		 resident;	/* Stream is resident */
	int		 compressed;	/* Stream is compressed */
	int		 encrypted;	/* Stream is encrypted */
	long long	 size_alloc;	/* Allocated size (multiple of cluster size) */
	long long	 size_data;	/* Actual size of data */
	long long	 size_init;	/* Initialised size, may be less than data size */
	long long	 size_vcn;	/* Highest VCN in the data runs */
	runlist_element *runlist;	/* Decoded data runs */
	int		 percent;	/* Amount potentially recoverable */
	void		*data;		/* If resident, a pointer to the data */
};

struct ufile {
	long long	 inode;		/* MFT record number */
	time_t		 date;		/* Last modification date/time */
	struct list_head name;		/* A list of filenames */
	struct list_head data;		/* A list of data streams */
	char		*pref_name;	/* Preferred filename */
	char		*pref_pname;	/*	     parent filename */
	long long	 max_size;	/* Largest size we find */
	int		 attr_list;	/* MFT record may be one of many */
	int		 directory;	/* MFT record represents a directory */
	MFT_RECORD	*mft;		/* Raw MFT record */
};


/* Function Interfaces */
void load_ntfs_mft(ntfs_volume *);
void fill_ntfs_info(ntfs_volume*, NTFS_BOOT_SECTOR);
