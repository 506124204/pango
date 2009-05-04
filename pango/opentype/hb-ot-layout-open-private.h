/*
 * Copyright (C) 2007,2008  Red Hat, Inc.
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

#ifndef HB_OT_LAYOUT_OPEN_PRIVATE_H
#define HB_OT_LAYOUT_OPEN_PRIVATE_H

#ifndef HB_OT_LAYOUT_CC
#error "This file should only be included from hb-ot-layout.c"
#endif

#include "hb-ot-layout-private.h"


#define NO_INDEX		((unsigned int) 0xFFFF)
#define NO_CONTEXT		((unsigned int) -1)

/*
 * Int types
 */

/* XXX define these as structs of chars on machines that do not allow
 * unaligned access */
#define DEFINE_INT_TYPE1(NAME, TYPE, BIG_ENDIAN) \
  inline NAME& operator = (TYPE i) { v = BIG_ENDIAN(i); return *this; } \
  inline operator TYPE(void) const { return BIG_ENDIAN(v); } \
  inline bool operator== (NAME o) const { return v == o.v; } \
  private: TYPE v; \
  public:
#define DEFINE_INT_TYPE0(NAME, type) DEFINE_INT_TYPE1 (NAME, type, hb_be_##type)
#define DEFINE_INT_TYPE(NAME, u, w)  DEFINE_INT_TYPE0 (NAME, u##int##w##_t)
#define DEFINE_INT_TYPE_STRUCT(NAME, u, w) \
  struct NAME { \
    DEFINE_INT_TYPE(NAME, u, w) \
  }

/*
 * Array types
 */

/* get_len() is a method returning the number of items in an array-like object */
#define DEFINE_LEN(Type, array, num) \
  inline unsigned int get_len(void) const { return num; } \

/* get_size() is a method returning the size in bytes of an array-like object */
#define DEFINE_SIZE(Type, array, num) \
  inline unsigned int get_size(void) const { return sizeof (*this) + sizeof (Type) * num; }

#define DEFINE_LEN_AND_SIZE(Type, array, num) \
  DEFINE_LEN(Type, array, num) \
  DEFINE_SIZE(Type, array, num)

/* An array type is one that contains a variable number of objects
 * as its last item.  An array object is extended with len() and size()
 * methods, as well as overloaded [] operator. */
#define DEFINE_ARRAY_TYPE(Type, array, num) \
  DEFINE_INDEX_OPERATOR(Type, array, num) \
  DEFINE_LEN_AND_SIZE(Type, array, num)
#define DEFINE_INDEX_OPERATOR(Type, array, num) \
  inline const Type& operator[] (unsigned int i) const { \
    if (HB_UNLIKELY (i >= num)) return Null##Type; \
    return array[i]; \
  }

/* An offset array type is like an array type, but it contains a table
 * of offsets to the objects, relative to the beginning of the current
 * object. */
#define DEFINE_OFFSET_ARRAY_TYPE(Type, array, num) \
  DEFINE_OFFSET_INDEX_OPERATOR(Type, array, num) \
  DEFINE_LEN_AND_SIZE(Offset, array, num)
#define DEFINE_OFFSET_INDEX_OPERATOR(Type, array, num) \
  inline const Type& operator[] (unsigned int i) const { \
    if (HB_UNLIKELY (i >= num)) return Null##Type; \
    if (HB_UNLIKELY (!array[i])) return Null##Type; \
    return *(const Type *)((const char*)this + array[i]); \
  }

/* A record array type is like an array type, but it contains a table
 * of records to the objects.  Each record has a tag, and an offset
 * relative to the beginning of the current object. */
#define DEFINE_RECORD_ARRAY_TYPE(Type, array, num) \
  DEFINE_RECORD_ACCESSOR(Type, array, num) \
  DEFINE_LEN_AND_SIZE(Record, array, num)
#define DEFINE_RECORD_ACCESSOR(Type, array, num) \
  inline const Type& operator[] (unsigned int i) const { \
    if (HB_UNLIKELY (i >= num)) return Null##Type; \
    if (HB_UNLIKELY (!array[i].offset)) return Null##Type; \
    return *(const Type *)((const char*)this + array[i].offset); \
  } \
  inline const Tag& get_tag (unsigned int i) const { \
    if (HB_UNLIKELY (i >= num)) return NullTag; \
    return array[i].tag; \
  }


#define DEFINE_ARRAY_INTERFACE(Type, name) \
  inline const Type& get_##name (unsigned int i) const { \
    return (*this)[i]; \
  } \
  inline unsigned int get_##name##_count (void) const { \
    return this->get_len (); \
  }
#define DEFINE_INDEX_ARRAY_INTERFACE(name) \
  inline unsigned int get_##name##_index (unsigned int i) const { \
    if (HB_UNLIKELY (i >= get_len ())) return NO_INDEX; \
    return (*this)[i]; \
  } \
  inline unsigned int get_##name##_count (void) const { \
    return get_len (); \
  }


/*
 * List types
 */

#define DEFINE_LIST_ARRAY(Type, name) \
  inline const Type##List& get_##name##_list (void) const { \
    if (HB_UNLIKELY (!name##List)) return Null##Type##List; \
    return *(const Type##List *)((const char*)this + name##List); \
  }

#define DEFINE_LIST_INTERFACE(Type, name) \
  inline const Type& get_##name (unsigned int i) const { \
    return get_##name##_list ()[i]; \
  } \
  inline unsigned int get_##name##_count (void) const { \
    return get_##name##_list ().get_len (); \
  }

/*
 * Tag types
 */

#define DEFINE_TAG_ARRAY_INTERFACE(Type, name) \
  DEFINE_ARRAY_INTERFACE (Type, name); \
  inline const Tag& get_##name##_tag (unsigned int i) const { \
    return (*this)[i].tag; \
  }
#define DEFINE_TAG_LIST_INTERFACE(Type, name) \
  DEFINE_LIST_INTERFACE (Type, name); \
  inline const Tag& get_##name##_tag (unsigned int i) const { \
    return get_##name##_list ().get_tag (i); \
  }

#define DEFINE_TAG_FIND_INTERFACE(Type, name) \
  inline bool find_##name##_index (hb_tag_t tag, unsigned int *name##_index) const { \
    const Tag t = tag; \
    for (unsigned int i = 0; i < get_##name##_count (); i++) { \
      if (t == get_##name##_tag (i)) { \
        if (name##_index) *name##_index = i; \
        return true; \
      } \
    } \
    if (name##_index) *name##_index = NO_INDEX; \
    return false; \
  } \
  inline const Type& get_##name##_by_tag (hb_tag_t tag) const { \
    unsigned int i; \
    if (find_##name##_index (tag, &i)) \
      return get_##name (i); \
    else \
      return Null##Type; \
  }

/*
 * Class features
 */

/* makes class uninstantiable.  should be used for union classes that don't
 * contain any complete type */
#define DEFINE_NON_INSTANTIABLE(Type) \
  protected: inline Type() {} /* cannot be instantiated */ \
  public:

// TODO use a global nul-array for most Null's
/* defines Null##Type as a safe nil instance of Type */
#define DEFINE_NULL_DATA(Type, size, data) \
  static const unsigned char Null##Type##Data[size] = data; \
  DEFINE_NULL_ALIAS (Type, Type)
#define DEFINE_NULL(Type, size) \
	DEFINE_NULL_DATA(Type, size, "")
#define DEFINE_NULL_ASSERT_SIZE(Type, size) \
	DEFINE_NULL_ASSERT_SIZE_DATA(Type, size, "")
#define DEFINE_NULL_ASSERT_SIZE_DATA(Type, size, data) \
  ASSERT_SIZE (Type, size); \
  DEFINE_NULL_DATA (Type, size, data)
#define DEFINE_NULL_ALIAS(NewType, OldType) \
  /* XXX static */ const NewType &Null##NewType = *(NewType *)Null##OldType##Data

/* get_for_data() is a static class method returning a reference to an
 * instance of Type located at the input data location.  It's just a
 * fancy, NULL-safe, cast! */
#define STATIC_DEFINE_GET_FOR_DATA(Type) \
  static inline const Type& get_for_data (const char *data) { \
    extern const Type &Null##Type; \
    if (HB_UNLIKELY (data == NULL)) return Null##Type; \
    return *(const Type*)data; \
  } \
  static inline Type& get_for_data (char *data) { \
    return *(Type*)data; \
  }


#define DEFINE_GET_ACCESSOR(Type, name, Name) \
  inline const Type& get_##name (void) const { \
    if (HB_UNLIKELY (!Name)) return Null##Type; \
    return *(const Type*)((const char*)this + Name); \
  }
#define DEFINE_GET_HAS_ACCESSOR(Type, name, Name) \
  DEFINE_GET_ACCESSOR (Type, name, Name); \
  inline bool has_##name (void) const { \
    return Name != 0; \
  }




/*
 *
 * The OpenType Font File
 *
 */



/*
 * Data Types
 */


/* "The following data types are used in the OpenType font file.
 *  All OpenType fonts use Motorola-style byte ordering (Big Endian):" */


DEFINE_INT_TYPE_STRUCT (BYTE,	 u,  8);	/*  8-bit unsigned integer. */
DEFINE_NULL_ASSERT_SIZE (BYTE, 1);
DEFINE_INT_TYPE_STRUCT (CHAR,	  ,  8);	/*  8-bit signed integer. */
DEFINE_NULL_ASSERT_SIZE (CHAR, 1);
DEFINE_INT_TYPE_STRUCT (USHORT,  u, 16);	/* 16-bit unsigned integer. */
DEFINE_NULL_ASSERT_SIZE (USHORT, 2);
DEFINE_INT_TYPE_STRUCT (SHORT,	  , 16);	/* 16-bit signed integer. */
DEFINE_NULL_ASSERT_SIZE (SHORT, 2);
DEFINE_INT_TYPE_STRUCT (ULONG,	 u, 32);	/* 32-bit unsigned integer. */
DEFINE_NULL_ASSERT_SIZE (ULONG, 4);
DEFINE_INT_TYPE_STRUCT (LONG,	  , 32);	/* 32-bit signed integer. */
DEFINE_NULL_ASSERT_SIZE (LONG, 4);

/* Date represented in number of seconds since 12:00 midnight, January 1,
 * 1904. The value is represented as a signed 64-bit integer. */
DEFINE_INT_TYPE_STRUCT (LONGDATETIME, , 64);

/* 32-bit signed fixed-point number (16.16) */
struct Fixed {
  inline Fixed& operator = (int32_t v) { i = (int16_t) (v >> 16); f = (uint16_t) v; return *this; } \
  inline operator int32_t(void) const { return (((int32_t) i) << 16) + (uint16_t) f; } \
  inline bool operator== (Fixed o) const { return i == o.i && f == o.f; } \

  inline operator double(void) const { return (uint32_t) this / 65536.; }
  inline int16_t int_part (void) const { return i; }
  inline uint16_t frac_part (void) const { return f; }

  private:
  SHORT i;
  USHORT f;
};
DEFINE_NULL_ASSERT_SIZE (Fixed, 4);

/* Smallest measurable distance in the em space. */
struct FUNIT;

/* 16-bit signed integer (SHORT) that describes a quantity in FUnits. */
struct FWORD : SHORT {
};
DEFINE_NULL_ASSERT_SIZE (FWORD, 2);

/* 16-bit unsigned integer (USHORT) that describes a quantity in FUnits. */
struct UFWORD : USHORT {
};
DEFINE_NULL_ASSERT_SIZE (UFWORD, 2);

/* 16-bit signed fixed number with the low 14 bits of fraction (2.14). */
struct F2DOT14 : SHORT {
  inline operator double() const { return (uint32_t) this / 16384.; }
};
DEFINE_NULL_ASSERT_SIZE (F2DOT14, 2);

/* Array of four uint8s (length = 32 bits) used to identify a script, language
 * system, feature, or baseline */
struct Tag {
  inline Tag (void) { v[0] = v[1] = v[2] = v[3] = 0; }
  inline Tag (uint32_t v) { (ULONG&)(*this) = v; }
  inline Tag (const char *c) { v[0] = c[0]; v[1] = c[1]; v[2] = c[2]; v[3] = c[3]; }
  inline bool operator== (Tag o) const { return v[0]==o.v[0]&&v[1]==o.v[1]&&v[2]==o.v[2]&&v[3]==o.v[3]; }
  inline bool operator== (const char *c) const { return v[0]==c[0]&&v[1]==c[1]&&v[2]==c[2]&&v[3]==c[3]; }
  inline bool operator== (uint32_t i) const { return i == (uint32_t) *this; }
  inline operator uint32_t(void) const { return (v[0]<<24)+(v[1]<<16) +(v[2]<<8)+v[3]; }
  /* What the char* converters return is NOT nul-terminated.  Print using "%.4s" */
  inline operator const char* (void) const { return (const char *)this; }
  inline operator char* (void) { return (char *)this; }

  private:
  char v[4];
};
ASSERT_SIZE (Tag, 4);
DEFINE_NULL_DATA (Tag, 5, "    ");

/* Glyph index number, same as uint16 (length = 16 bits) */
DEFINE_INT_TYPE_STRUCT (GlyphID, u, 16);
DEFINE_NULL_ASSERT_SIZE (GlyphID, 2);

/* Offset to a table, same as uint16 (length = 16 bits), Null offset = 0x0000 */
DEFINE_INT_TYPE_STRUCT (Offset, u, 16);
DEFINE_NULL_ASSERT_SIZE (Offset, 2);

/* CheckSum */
struct CheckSum : ULONG {
  static uint32_t CalcTableChecksum (ULONG *Table, uint32_t Length) {
    uint32_t Sum = 0L;
    ULONG *EndPtr = Table+((Length+3) & ~3) / sizeof(ULONG);

    while (Table < EndPtr)
      Sum += *Table++;
    return Sum;
  }
};
DEFINE_NULL_ASSERT_SIZE (CheckSum, 4);


/*
 * Version Numbers
 */

struct USHORT_Version : USHORT {
};
DEFINE_NULL_ASSERT_SIZE (USHORT_Version, 2);

struct Fixed_Version : Fixed {
  inline int16_t major (void) const { return this->int_part(); }
  inline int16_t minor (void) const { return this->frac_part(); }
};
DEFINE_NULL_ASSERT_SIZE (Fixed_Version, 4);


/*
 * Organization of an OpenType Font
 */

struct OpenTypeFontFile;
struct OffsetTable;
struct TTCHeader;

typedef struct TableDirectory {

  friend struct OpenTypeFontFile;
  friend struct OffsetTable;

  inline bool is_null (void) const { return length == 0; }
  inline const Tag& get_tag (void) const { return tag; }
  inline unsigned long get_checksum (void) const { return checkSum; }
  inline unsigned long get_offset (void) const { return offset; }
  inline unsigned long get_length (void) const { return length; }

  private:
  Tag		tag;		/* 4-byte identifier. */
  CheckSum	checkSum;	/* CheckSum for this table. */
  ULONG		offset;		/* Offset from beginning of TrueType font
				 * file. */
  ULONG		length;		/* Length of this table. */
} OpenTypeTable;
DEFINE_NULL_ASSERT_SIZE (TableDirectory, 16);
DEFINE_NULL_ALIAS (OpenTypeTable, TableDirectory);

typedef struct OffsetTable {

  friend struct OpenTypeFontFile;
  friend struct TTCHeader;

  DEFINE_TAG_ARRAY_INTERFACE (OpenTypeTable, table);	/* get_table_count(), get_table(i), get_table_tag(i) */
  DEFINE_TAG_FIND_INTERFACE  (OpenTypeTable, table);	/* find_table_index(tag), get_table_by_tag(tag) */

  private:
  /* OpenTypeTables, in no particular order */
  DEFINE_ARRAY_TYPE (TableDirectory, tableDir, numTables);

  private:
  Tag		sfnt_version;	/* '\0\001\0\00' if TrueType / 'OTTO' if CFF */
  USHORT	numTables;	/* Number of tables. */
  USHORT	searchRange;	/* (Maximum power of 2 <= numTables) x 16 */
  USHORT	entrySelector;	/* Log2(maximum power of 2 <= numTables). */
  USHORT	rangeShift;	/* NumTables x 16-searchRange. */
  TableDirectory tableDir[];	/* TableDirectory entries. numTables items */
} OpenTypeFontFace;
DEFINE_NULL_ASSERT_SIZE (OffsetTable, 12);
DEFINE_NULL_ALIAS (OpenTypeFontFace, OffsetTable);

/*
 * TrueType Collections
 */

struct TTCHeader {

  friend struct OpenTypeFontFile;

  private:
  /* OpenTypeFontFaces, in no particular order */
  DEFINE_OFFSET_ARRAY_TYPE (OffsetTable, offsetTable, numFonts);
  /* XXX check version here? */

  private:
  Tag	ttcTag;		/* TrueType Collection ID string: 'ttcf' */
  ULONG	version;	/* Version of the TTC Header (1.0 or 2.0),
			 * 0x00010000 or 0x00020000 */
  ULONG	numFonts;	/* Number of fonts in TTC */
  ULONG	offsetTable[];	/* Array of offsets to the OffsetTable for each font
			 * from the beginning of the file */
};
DEFINE_NULL_ASSERT_SIZE (TTCHeader, 12);


/*
 * OpenType Font File
 */

struct OpenTypeFontFile {
  DEFINE_NON_INSTANTIABLE(OpenTypeFontFile);
  static const hb_tag_t TrueTypeTag	= HB_TAG ( 0 , 1 , 0 , 0 );
  static const hb_tag_t CFFTag		= HB_TAG ('O','T','T','O');
  static const hb_tag_t TTCTag		= HB_TAG ('t','t','c','f');

  STATIC_DEFINE_GET_FOR_DATA (OpenTypeFontFile);

  DEFINE_ARRAY_INTERFACE (OpenTypeFontFace, face);	/* get_face_count(), get_face(i) */

  inline const Tag& get_tag (void) const { return tag; }

  /* This is how you get a table */
  inline const char* get_table_data (const OpenTypeTable& table) const {
    return (*this)[table];
  }
  inline char* get_table_data (const OpenTypeTable& table) {
    return (*this)[table];
  }

  private:
  inline const char* operator[] (const OpenTypeTable& table) const {
    if (G_UNLIKELY (table.offset == 0)) return NULL;
    return ((const char*)this) + table.offset;
  }
  inline char* operator[] (const OpenTypeTable& table) {
    if (G_UNLIKELY (table.offset == 0)) return NULL;
    return ((char*)this) + table.offset;
  }

  /* Array interface sans get_size() */
  unsigned int get_len (void) const {
    switch (tag) {
    default: return 0;
    case TrueTypeTag: case CFFTag: return 1;
    case TTCTag: return ((const TTCHeader&)*this).get_len();
    }
  }
  const OpenTypeFontFace& operator[] (unsigned int i) const {
    if (HB_UNLIKELY (i >= get_len ())) return NullOpenTypeFontFace;
    switch (tag) {
    default: case TrueTypeTag: case CFFTag: return (const OffsetTable&)*this;
    case TTCTag: return ((const TTCHeader&)*this)[i];
    }
  }

  private:
  Tag		tag;		/* 4-byte identifier. */
};
DEFINE_NULL_ASSERT_SIZE (OpenTypeFontFile, 4);



/*
 *
 * OpenType Layout Common Table Formats
 *
 */

/*
 * Script, ScriptList, LangSys, Feature, FeatureList, Lookup, LookupList
 */

typedef struct Record {
  Tag		tag;		/* 4-byte Tag identifier */
  Offset	offset;		/* Offset from beginning of object holding
				 * the Record */
} ScriptRecord, LangSysRecord, FeatureRecord;
DEFINE_NULL_ASSERT_SIZE (Record, 6);

struct LangSys {

  DEFINE_INDEX_ARRAY_INTERFACE (feature);

  inline const bool has_required_feature (void) const {
    return reqFeatureIndex != 0xffff;
  }
  /* Returns NO_INDEX if none */
  inline int get_required_feature_index (void) const {
    if (reqFeatureIndex == 0xffff)
      return NO_INDEX;
    return reqFeatureIndex;;
  }

  private:
  /* Feature indices, in no particular order */
  DEFINE_ARRAY_TYPE (USHORT, featureIndex, featureCount);

  private:
  Offset	lookupOrder;	/* = Null (reserved for an offset to a
				 * reordering table) */
  USHORT	reqFeatureIndex;/* Index of a feature required for this
				 * language system--if no required features
				 * = 0xFFFF */
  USHORT	featureCount;	/* Number of FeatureIndex values for this
				 * language system--excludes the required
				 * feature */
  USHORT	featureIndex[];	/* Array of indices into the FeatureList--in
				 * arbitrary order. featureCount entires long */
};
DEFINE_NULL_ASSERT_SIZE_DATA (LangSys, 6, "\0\0\xFF\xFF");

struct Script {

  /* DEFINE_ARRAY_INTERFACE (LangSys, lang_sys) but handling defaultLangSys */

  inline const LangSys& get_lang_sys (unsigned int i) const {
    if (i == NO_INDEX) return get_default_lang_sys ();
    return (*this)[i];
  }
  inline unsigned int get_lang_sys_count (void) const {
    return this->get_len ();
  }

  inline const Tag& get_lang_sys_tag (unsigned int i) const {
    return get_tag (i);
  }

  // LONGTERMTODO bsearch
  DEFINE_TAG_FIND_INTERFACE (LangSys, lang_sys);	/* find_lang_sys_index (), get_lang_sys_by_tag (tag) */

  inline const bool has_default_lang_sys (void) const {
    return defaultLangSys != 0;
  }
  inline const LangSys& get_default_lang_sys (void) const {
    if (HB_UNLIKELY (!defaultLangSys))
      return NullLangSys;
    return *(LangSys*)((const char*)this + defaultLangSys);
  }

  private:
  /* LangSys', in sorted alphabetical tag order */
  DEFINE_RECORD_ARRAY_TYPE (LangSys, langSysRecord, langSysCount);

  private:
  Offset	defaultLangSys;	/* Offset to DefaultLangSys table--from
				 * beginning of Script table--may be Null */
  USHORT	langSysCount;	/* Number of LangSysRecords for this script--
				 * excluding the DefaultLangSys */
  LangSysRecord	langSysRecord[];/* Array of LangSysRecords--listed
				 * alphabetically by LangSysTag */
};
DEFINE_NULL_ASSERT_SIZE (Script, 4);

struct ScriptList {

  friend struct GSUBGPOS;

private:
  /* Scripts, in sorted alphabetical tag order */
  DEFINE_RECORD_ARRAY_TYPE (Script, scriptRecord, scriptCount);

private:
  USHORT	scriptCount;	/* Number of ScriptRecords */
  ScriptRecord	scriptRecord[]; /* Array of ScriptRecords--listed alphabetically
				 * by ScriptTag */
};
DEFINE_NULL_ASSERT_SIZE (ScriptList, 2);

struct Feature {

  DEFINE_INDEX_ARRAY_INTERFACE (lookup);	/* get_lookup_count(), get_lookup_index(i) */

  private:
  /* LookupList indices, in no particular order */
  DEFINE_ARRAY_TYPE (USHORT, lookupIndex, lookupCount);

  /* TODO: implement get_feature_parameters() */
  /* TODO: implement FeatureSize and other special features? */

  private:
  Offset	featureParams;	/* Offset to Feature Parameters table (if one
				 * has been defined for the feature), relative
				 * to the beginning of the Feature Table; = Null
				 * if not required */
  USHORT	lookupCount;	/* Number of LookupList indices for this
				 * feature */
  USHORT	lookupIndex[];	/* Array of LookupList indices for this
				 * feature--zero-based (first lookup is
				 * LookupListIndex = 0) */
};
DEFINE_NULL_ASSERT_SIZE (Feature, 4);

struct FeatureList {

  friend struct GSUBGPOS;

  private:
  /* Feature indices, in sorted alphabetical tag order */
  DEFINE_RECORD_ARRAY_TYPE (Feature, featureRecord, featureCount);

  private:
  USHORT	featureCount;	/* Number of FeatureRecords in this table */
  FeatureRecord	featureRecord[];/* Array of FeatureRecords--zero-based (first
				 * feature has FeatureIndex = 0)--listed
				 * alphabetically by FeatureTag */
};
DEFINE_NULL_ASSERT_SIZE (FeatureList, 2);

struct LookupFlag : USHORT {
  static const unsigned int RightToLeft		= 0x0001u;
  static const unsigned int IgnoreBaseGlyphs	= 0x0002u;
  static const unsigned int IgnoreLigatures	= 0x0004u;
  static const unsigned int IgnoreMarks		= 0x0008u;
  static const unsigned int Reserved		= 0x00F0u;
  static const unsigned int MarkAttachmentType	= 0xFF00u;
};
DEFINE_NULL_ASSERT_SIZE (LookupFlag, 2);

struct LookupSubTable {
  DEFINE_NON_INSTANTIABLE(LookupSubTable);

  private:
  USHORT	format;		/* Subtable format.  Different for GSUB and GPOS */
};
DEFINE_NULL_ASSERT_SIZE (LookupSubTable, 2);


struct Lookup {
  DEFINE_NON_INSTANTIABLE(Lookup);

  DEFINE_ARRAY_INTERFACE (LookupSubTable, subtable);	/* get_subtable_count(), get_subtable(i) */

  inline bool is_right_to_left	(void) const { return lookupFlag & LookupFlag::RightToLeft; }
  inline bool ignore_base_glyphs(void) const { return lookupFlag & LookupFlag::IgnoreBaseGlyphs; }
  inline bool ignore_ligatures	(void) const { return lookupFlag & LookupFlag::IgnoreLigatures; }
  inline bool ignore_marks	(void) const { return lookupFlag & LookupFlag::IgnoreMarks; }
  inline unsigned int get_mark_attachment_type (void) const { return lookupFlag & LookupFlag::MarkAttachmentType; }

  inline unsigned int get_type (void) const { return lookupType; }
  inline unsigned int get_flag (void) const { return lookupFlag; }

  private:
  /* SubTables, in the desired order */
  DEFINE_OFFSET_ARRAY_TYPE (LookupSubTable, subTableOffset, subTableCount);

  protected:
  USHORT	lookupType;	/* Different enumerations for GSUB and GPOS */
  USHORT	lookupFlag;	/* Lookup qualifiers */
  USHORT	subTableCount;	/* Number of SubTables for this lookup */
  Offset	subTableOffset[];/* Array of offsets to SubTables-from
				  * beginning of Lookup table */
};
DEFINE_NULL_ASSERT_SIZE (Lookup, 6);

struct LookupList {

  friend struct GSUBGPOS;

  private:
  /* Lookup indices, in sorted alphabetical tag order */
  DEFINE_OFFSET_ARRAY_TYPE (Lookup, lookupOffset, lookupCount);

  private:
  USHORT	lookupCount;	/* Number of lookups in this table */
  Offset	lookupOffset[];	/* Array of offsets to Lookup tables--from
				 * beginning of LookupList--zero based (first
				 * lookup is Lookup index = 0) */
};
DEFINE_NULL_ASSERT_SIZE (LookupList, 2);

/*
 * Coverage Table
 */

struct CoverageFormat1 {

  friend struct Coverage;

  private:
  /* GlyphIDs, in sorted numerical order */
  DEFINE_ARRAY_TYPE (GlyphID, glyphArray, glyphCount);

  inline hb_ot_layout_coverage_t get_coverage (hb_codepoint_t glyph_id) const {
    GlyphID gid;
    if (HB_UNLIKELY (glyph_id > 65535))
      return -1;
    gid = glyph_id;
    // TODO: bsearch
    for (unsigned int i = 0; i < glyphCount; i++)
      if (gid == glyphArray[i])
        return i;
    return -1;
  }

  private:
  USHORT	coverageFormat;	/* Format identifier--format = 1 */
  USHORT	glyphCount;	/* Number of glyphs in the GlyphArray */
  GlyphID	glyphArray[];	/* Array of GlyphIDs--in numerical order */
};
ASSERT_SIZE (CoverageFormat1, 4);

struct CoverageRangeRecord {

  friend struct CoverageFormat2;

  private:
  inline hb_ot_layout_coverage_t get_coverage (hb_codepoint_t glyph_id) const {
    if (glyph_id >= start && glyph_id <= end)
      return startCoverageIndex + (glyph_id - start);
    return -1;
  }

  private:
  GlyphID	start;			/* First GlyphID in the range */
  GlyphID	end;			/* Last GlyphID in the range */
  USHORT	startCoverageIndex;	/* Coverage Index of first GlyphID in
					 * range */
};
DEFINE_NULL_ASSERT_SIZE_DATA (CoverageRangeRecord, 6, "\001");

struct CoverageFormat2 {

  friend struct Coverage;

  private:
  /* CoverageRangeRecords, in sorted numerical start order */
  DEFINE_ARRAY_TYPE (CoverageRangeRecord, rangeRecord, rangeCount);

  inline hb_ot_layout_coverage_t get_coverage (hb_codepoint_t glyph_id) const {
    // TODO: bsearch
    for (unsigned int i = 0; i < rangeCount; i++) {
      int coverage = rangeRecord[i].get_coverage (glyph_id);
      if (coverage >= 0)
        return coverage;
    }
    return -1;
  }

  private:
  USHORT		coverageFormat;	/* Format identifier--format = 2 */
  USHORT		rangeCount;	/* Number of CoverageRangeRecords */
  CoverageRangeRecord	rangeRecord[];	/* Array of glyph ranges--ordered by
					 * Start GlyphID. rangeCount entries
					 * long */
};
ASSERT_SIZE (CoverageFormat2, 4);

struct Coverage {
  DEFINE_NON_INSTANTIABLE(Coverage);

  unsigned int get_size (void) const {
    switch (u.coverageFormat) {
    case 1: return u.format1.get_size ();
    case 2: return u.format2.get_size ();
    default:return sizeof (u.coverageFormat);
    }
  }

  hb_ot_layout_coverage_t get_coverage (hb_codepoint_t glyph_id) const {
    switch (u.coverageFormat) {
    case 1: return u.format1.get_coverage(glyph_id);
    case 2: return u.format2.get_coverage(glyph_id);
    default:return -1;
    }
  }

  private:
  union {
  USHORT		coverageFormat;	/* Format identifier */
  CoverageFormat1	format1;
  CoverageFormat2	format2;
  } u;
};
DEFINE_NULL (Coverage, 2);

/*
 * Class Definition Table
 */

struct ClassDefFormat1 {

  friend struct ClassDef;

  private:
  /* GlyphIDs, in sorted numerical order */
  DEFINE_ARRAY_TYPE (USHORT, classValueArray, glyphCount);

  inline hb_ot_layout_class_t get_class (hb_codepoint_t glyph_id) const {
    if (glyph_id >= startGlyph && glyph_id - startGlyph < glyphCount)
      return classValueArray[glyph_id - startGlyph];
    return 0;
  }

  private:
  USHORT	classFormat;		/* Format identifier--format = 1 */
  GlyphID	startGlyph;		/* First GlyphID of the classValueArray */
  USHORT	glyphCount;		/* Size of the classValueArray */
  USHORT	classValueArray[];	/* Array of Class Values--one per GlyphID */
};
ASSERT_SIZE (ClassDefFormat1, 6);

struct ClassRangeRecord {

  friend struct ClassDefFormat2;

  private:
  inline hb_ot_layout_class_t get_class (hb_codepoint_t glyph_id) const {
    if (glyph_id >= start && glyph_id <= end)
      return classValue;
    return 0;
  }

  private:
  GlyphID	start;		/* First GlyphID in the range */
  GlyphID	end;		/* Last GlyphID in the range */
  USHORT	classValue;	/* Applied to all glyphs in the range */
};
DEFINE_NULL_ASSERT_SIZE_DATA (ClassRangeRecord, 6, "\001");

struct ClassDefFormat2 {

  friend struct ClassDef;

  private:
  /* ClassRangeRecords, in sorted numerical start order */
  DEFINE_ARRAY_TYPE (ClassRangeRecord, rangeRecord, rangeCount);

  inline hb_ot_layout_class_t get_class (hb_codepoint_t glyph_id) const {
    // TODO: bsearch
    for (unsigned int i = 0; i < rangeCount; i++) {
      int classValue = rangeRecord[i].get_class (glyph_id);
      if (classValue > 0)
        return classValue;
    }
    return 0;
  }

  private:
  USHORT		classFormat;	/* Format identifier--format = 2 */
  USHORT		rangeCount;	/* Number of Number of ClassRangeRecords */
  ClassRangeRecord	rangeRecord[];	/* Array of glyph ranges--ordered by
					 * Start GlyphID */
};
ASSERT_SIZE (ClassDefFormat2, 4);

struct ClassDef {
  DEFINE_NON_INSTANTIABLE(ClassDef);

  unsigned int get_size (void) const {
    switch (u.classFormat) {
    case 1: return u.format1.get_size ();
    case 2: return u.format2.get_size ();
    default:return sizeof (u.classFormat);
    }
  }

  hb_ot_layout_class_t get_class (hb_codepoint_t glyph_id) const {
    switch (u.classFormat) {
    case 1: return u.format1.get_class(glyph_id);
    case 2: return u.format2.get_class(glyph_id);
    default:return 0;
    }
  }

  private:
  union {
  USHORT		classFormat;	/* Format identifier */
  ClassDefFormat1	format1;
  ClassDefFormat2	format2;
  } u;
};
DEFINE_NULL (ClassDef, 2);

/*
 * Device Tables
 */

struct Device {
  DEFINE_NON_INSTANTIABLE(Device);

   unsigned int get_size (void) const {
    int count = endSize - startSize + 1;
    if (count < 0) count = 0;
    switch (deltaFormat) {
    case 1: return sizeof (Device) + sizeof (USHORT) * ((count+7)/8);
    case 2: return sizeof (Device) + sizeof (USHORT) * ((count+3)/4);
    case 3: return sizeof (Device) + sizeof (USHORT) * ((count+1)/2);
    default:return sizeof (Device);
    }
  }

  int get_delta (int ppem_size) const {
    if (ppem_size >= startSize && ppem_size <= endSize &&
        deltaFormat >= 1 && deltaFormat <= 3) {
      int s = ppem_size - startSize;
      int f = deltaFormat;

      uint16_t byte = deltaValue[s >> (4 - f)];
      uint16_t bits = byte >> (16 - (((s & ((1 << (4 - f)) - 1)) + 1) << f));
      uint16_t mask = 0xFFFF >> (16 - (1 << f));

      int delta = bits & mask;

      if (delta >= ((mask + 1) >> 1))
        delta -= mask + 1;

      return delta;
    }
    return 0;
  }

  private:
  USHORT	startSize;	/* Smallest size to correct--in ppem */
  USHORT	endSize;	/* Largest size to correct--in ppem */
  USHORT	deltaFormat;	/* Format of DeltaValue array data: 1, 2, or 3 */
  USHORT	deltaValue[];	/* Array of compressed data */
};
DEFINE_NULL_ASSERT_SIZE (Device, 6);

/*
 * GSUB/GPOS Common
 */

struct GSUBGPOS {
  static const hb_tag_t GSUBTag		= HB_TAG ('G','S','U','B');
  static const hb_tag_t GPOSTag		= HB_TAG ('G','P','O','S');

  STATIC_DEFINE_GET_FOR_DATA (GSUBGPOS);
  /* XXX check version here? */

  DEFINE_TAG_LIST_INTERFACE (Script,  script );	/* get_script_count (), get_script (i), get_script_tag (i) */
  DEFINE_TAG_LIST_INTERFACE (Feature, feature);	/* get_feature_count(), get_feature(i), get_feature_tag(i) */
  DEFINE_LIST_INTERFACE     (Lookup,  lookup );	/* get_lookup_count (), get_lookup (i) */

  // LONGTERMTODO bsearch
  DEFINE_TAG_FIND_INTERFACE (Script,  script );	/* find_script_index (), get_script_by_tag (tag) */
  DEFINE_TAG_FIND_INTERFACE (Feature, feature);	/* find_feature_index(), get_feature_by_tag(tag) */

  private:
  DEFINE_LIST_ARRAY(Script,  script);
  DEFINE_LIST_ARRAY(Feature, feature);
  DEFINE_LIST_ARRAY(Lookup,  lookup);

  private:
  Fixed_Version	version;	/* Version of the GSUB/GPOS table--initially set
				 * to 0x00010000 */
  Offset	scriptList;  	/* Offset to ScriptList table--from beginning of
				 * GSUB/GPOS table */
  Offset	featureList; 	/* Offset to FeatureList table--from beginning of
				 * GSUB/GPOS table */
  Offset	lookupList; 	/* Offset to LookupList table--from beginning of
				 * GSUB/GPOS table */
};
DEFINE_NULL_ASSERT_SIZE (GSUBGPOS, 10);

#endif /* HB_OT_LAYOUT_OPEN_PRIVATE_H */
