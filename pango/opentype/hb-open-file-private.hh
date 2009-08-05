/*
 * Copyright (C) 2007,2008,2009  Red Hat, Inc.
 *
 *  This is part of HarfBuzz, an OpenType Layout engine library.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and its documentation for any purpose, provided that the
 * above copyright notice and the following two paragraphs appear in
 * all copies of this software.
 *
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
 * ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN
 * IF THE COPYRIGHT HOLDER HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * THE COPYRIGHT HOLDER SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE COPYRIGHT HOLDER HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 * Red Hat Author(s): Behdad Esfahbod
 */

#ifndef HB_OPEN_FILE_PRIVATE_HH
#define HB_OPEN_FILE_PRIVATE_HH

#include "hb-open-type-private.hh"


/*
 *
 * The OpenType Font File
 *
 */


/*
 * Organization of an OpenType Font
 */

struct OpenTypeFontFile;
struct OffsetTable;
struct TTCHeader;

typedef struct TableDirectory
{
  inline bool sanitize (SANITIZE_ARG_DEF, const void *base) {
    return SANITIZE_SELF () && SANITIZE (tag) &&
	   SANITIZE_MEM (CONST_CHARP(base) + (unsigned long) offset, length);
  }

  Tag		tag;		/* 4-byte identifier. */
  CheckSum	checkSum;	/* CheckSum for this table. */
  ULONG		offset;		/* Offset from beginning of TrueType font
				 * file. */
  ULONG		length;		/* Length of this table. */
} OpenTypeTable;
ASSERT_SIZE (TableDirectory, 16);

typedef struct OffsetTable
{
  friend struct OpenTypeFontFile;
  friend struct TTCHeader;

  STATIC_DEFINE_GET_FOR_DATA (OffsetTable);
  DEFINE_TAG_ARRAY_INTERFACE (OpenTypeTable, table);	/* get_table_count(), get_table(i), get_table_tag(i) */
  DEFINE_TAG_FIND_INTERFACE  (OpenTypeTable, table);	/* find_table_index(tag), get_table_by_tag(tag) */

  unsigned int get_face_count (void) const { return 1; }

  private:
  /* OpenTypeTables, in no particular order */
  DEFINE_ARRAY_TYPE (TableDirectory, tableDir, numTables);

  public:
  inline bool sanitize (SANITIZE_ARG_DEF, const void *base) {
    if (!(SANITIZE_SELF () && SANITIZE_MEM (tableDir, sizeof (tableDir[0]) * numTables))) return false;
    unsigned int count = numTables;
    for (unsigned int i = 0; i < count; i++)
      if (!SANITIZE_BASE (tableDir[i], base))
        return false;
    return true;
  }

  private:
  Tag		sfnt_version;	/* '\0\001\0\00' if TrueType / 'OTTO' if CFF */
  USHORT	numTables;	/* Number of tables. */
  USHORT	searchRange;	/* (Maximum power of 2 <= numTables) x 16 */
  USHORT	entrySelector;	/* Log2(maximum power of 2 <= numTables). */
  USHORT	rangeShift;	/* NumTables x 16-searchRange. */
  TableDirectory tableDir[];	/* TableDirectory entries. numTables items */
} OpenTypeFontFace;
ASSERT_SIZE (OffsetTable, 12);

/*
 * TrueType Collections
 */

struct TTCHeader
{
  friend struct OpenTypeFontFile;

  STATIC_DEFINE_GET_FOR_DATA_CHECK_MAJOR_VERSION (TTCHeader, 1, 2);

  unsigned int get_face_count (void) const { return table.len; }

  const OpenTypeFontFace& get_face (unsigned int i) const
  {
    return this+table[i];
  }

  bool sanitize (SANITIZE_ARG_DEF) {
    if (!SANITIZE (version)) return false;
    if (version.major < 1 || version.major > 2) return true;
    /* XXX Maybe we shouldn't NEUTER these offsets, they may cause a full copy of
     * the whole font right now. */
    //if (!table.sanitize (SANITIZE_ARG)) return false;
    return table.sanitize (SANITIZE_ARG, CONST_CHARP(this), CONST_CHARP(this));
  }

  private:
  Tag		ttcTag;		/* TrueType Collection ID string: 'ttcf' */
  FixedVersion	version;	/* Version of the TTC Header (1.0 or 2.0),
				 * 0x00010000 or 0x00020000 */
  LongOffsetLongArrayOf<OffsetTable>
		table;		/* Array of offsets to the OffsetTable for each font
				 * from the beginning of the file */
};
ASSERT_SIZE (TTCHeader, 12);


/*
 * OpenType Font File
 */

struct OpenTypeFontFile
{
  static const hb_tag_t TrueTypeTag	= HB_TAG ( 0 , 1 , 0 , 0 );
  static const hb_tag_t CFFTag		= HB_TAG ('O','T','T','O');
  static const hb_tag_t TTCTag		= HB_TAG ('t','t','c','f');

  STATIC_DEFINE_GET_FOR_DATA (OpenTypeFontFile);

  unsigned int get_face_count (void) const
  {
    switch (tag) {
    default: return 0;
    case TrueTypeTag: case CFFTag: return OffsetTable::get_for_data (CONST_CHARP(this)).get_face_count ();
    case TTCTag: return TTCHeader::get_for_data (CONST_CHARP(this)).get_face_count ();
    }
  }
  const OpenTypeFontFace& get_face (unsigned int i) const
  {
    switch (tag) {
    default: return Null(OpenTypeFontFace);
    /* Note: for non-collection SFNT data we ignore index.  This is because
     * Apple dfont container is a container of SFNT's.  So each SFNT is a
     * non-TTC, but the index is more than zero. */
    case TrueTypeTag: case CFFTag: return OffsetTable::get_for_data (CONST_CHARP(this));
    case TTCTag: return TTCHeader::get_for_data (CONST_CHARP(this)).get_face (i);
    }
  }

  /* This is how you get a table */
  inline const char* get_table_data (const OpenTypeTable& table) const
  {
    if (HB_UNLIKELY (table.offset == 0)) return NULL;
    return ((const char*) this) + table.offset;
  }

  bool sanitize (SANITIZE_ARG_DEF) {
    switch (tag) {
    default: return true;
    case TrueTypeTag: case CFFTag: return SANITIZE_THIS (CAST (OffsetTable, *this, 0));
    case TTCTag: return SANITIZE (CAST (TTCHeader, *this, 0));
    }
  }

  Tag		tag;		/* 4-byte identifier. */
};
ASSERT_SIZE (OpenTypeFontFile, 4);


#endif /* HB_OPEN_FILE_PRIVATE_HH */