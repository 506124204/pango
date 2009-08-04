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

#ifndef HB_OT_LAYOUT_COMMON_PRIVATE_HH
#define HB_OT_LAYOUT_COMMON_PRIVATE_HH

#include "hb-ot-layout-private.h"

#include "hb-open-types-private.hh"


/*
 *
 * OpenType Layout Common Table Formats
 *
 */


/*
 * Script, ScriptList, LangSys, Feature, FeatureList, Lookup, LookupList
 */

template <typename Type>
struct Record
{
  Tag		tag;		/* 4-byte Tag identifier */
  OffsetTo<Type>
		offset;		/* Offset from beginning of object holding
				 * the Record */
};

template <typename Type>
struct RecordListOf : ArrayOf<Record<Type> >
{
  inline const Type& operator [] (unsigned int i) const
  {
    if (HB_UNLIKELY (i >= this->len)) return Null(Type);
    return this+this->array[i].offset;
  }
  inline const Tag& get_tag (unsigned int i) const
  {
    if (HB_UNLIKELY (i >= this->len)) return Null(Tag);
    return this->array[i].tag;
  }
};


struct Script;
typedef Record<Script> ScriptRecord;
ASSERT_SIZE (ScriptRecord, 6);
struct LangSys;
typedef Record<LangSys> LangSysRecord;
ASSERT_SIZE (LangSysRecord, 6);
struct Feature;
typedef Record<Feature> FeatureRecord;
ASSERT_SIZE (FeatureRecord, 6);


struct LangSys
{
  inline const unsigned int get_feature_index (unsigned int i) const { return featureIndex[i]; }
  inline unsigned int get_feature_count (void) const { return featureIndex.len; }

  inline bool has_required_feature (void) const { return reqFeatureIndex != 0xffff; }
  inline int get_required_feature_index (void) const
  {
    if (reqFeatureIndex == 0xffff)
      return NO_INDEX;
   return reqFeatureIndex;;
  }

  Offset	lookupOrder;	/* = Null (reserved for an offset to a
				 * reordering table) */
  USHORT	reqFeatureIndex;/* Index of a feature required for this
				 * language system--if no required features
				 * = 0xFFFF */
  ArrayOf<USHORT>
		featureIndex;	/* Array of indices into the FeatureList */
};
ASSERT_SIZE_DATA (LangSys, 6, "\0\0\xFF\xFF");


struct Script
{
  inline const LangSys& get_lang_sys (unsigned int i) const
  {
    if (i == NO_INDEX) return get_default_lang_sys ();
    return this+langSys[i].offset;
  }
  inline unsigned int get_lang_sys_count (void) const { return langSys.len; }
  inline const Tag& get_lang_sys_tag (unsigned int i) const { return langSys[i].tag; }

  // LONGTERMTODO bsearch
  DEFINE_TAG_FIND_INTERFACE (LangSys, lang_sys);	/* find_lang_sys_index (), get_lang_sys_by_tag (tag) */

  inline bool has_default_lang_sys (void) const { return defaultLangSys != 0; }
  inline const LangSys& get_default_lang_sys (void) const { return this+defaultLangSys; }

  private:
  OffsetTo<LangSys>
		defaultLangSys;	/* Offset to DefaultLangSys table--from
	 			 * beginning of Script table--may be Null */
  ArrayOf<LangSysRecord>
		langSys;	/* Array of LangSysRecords--listed
				 * alphabetically by LangSysTag */
};
ASSERT_SIZE (Script, 4);

typedef RecordListOf<Script> ScriptList;
ASSERT_SIZE (ScriptList, 2);


struct Feature
{
  inline const unsigned int get_lookup_index (unsigned int i) const { return lookupIndex[i]; }
  inline unsigned int get_lookup_count (void) const { return lookupIndex.len; }

  /* TODO: implement get_feature_parameters() */
  /* TODO: implement FeatureSize and other special features? */
  Offset	featureParams;	/* Offset to Feature Parameters table (if one
				 * has been defined for the feature), relative
				 * to the beginning of the Feature Table; = Null
				 * if not required */
  ArrayOf<USHORT>
		lookupIndex;	/* Array of LookupList indices */
};
ASSERT_SIZE (Feature, 4);

typedef RecordListOf<Feature> FeatureList;
ASSERT_SIZE (FeatureList, 2);


struct LookupFlag : USHORT
{
  enum {
    RightToLeft		= 0x0001u,
    IgnoreBaseGlyphs	= 0x0002u,
    IgnoreLigatures	= 0x0004u,
    IgnoreMarks		= 0x0008u,
    UseMarkFilteringSet	= 0x0010u,
    Reserved		= 0x00E0u,
    MarkAttachmentType	= 0xFF00u,
  };
};
ASSERT_SIZE (LookupFlag, 2);

struct LookupSubTable
{
  private:
  USHORT	format;		/* Subtable format.  Different for GSUB and GPOS */
};
ASSERT_SIZE (LookupSubTable, 2);

struct Lookup
{
  inline const LookupSubTable& get_subtable (unsigned int i) const { return this+subTable[i]; }
  inline unsigned int get_subtable_count (void) const { return subTable.len; }

  inline unsigned int get_type (void) const { return lookupType; }
  inline unsigned int get_flag (void) const
  {
    unsigned int flag = lookupFlag;
    if (HB_UNLIKELY (flag & LookupFlag::UseMarkFilteringSet))
    {
      const USHORT &markFilteringSet = *(const USHORT*)
					((const char *) &subTable + subTable.get_size ());
      flag += (markFilteringSet << 16);
    }
    return flag;
  }

  USHORT	lookupType;		/* Different enumerations for GSUB and GPOS */
  USHORT	lookupFlag;		/* Lookup qualifiers */
  OffsetArrayOf<LookupSubTable>
		subTable;		/* Array of SubTables */
  USHORT	markFilteringSetX[0];	/* Index (base 0) into GDEF mark glyph sets
					 * structure. This field is only present if bit
					 * UseMarkFilteringSet of lookup flags is set. */
};
ASSERT_SIZE (Lookup, 6);

template <typename Type>
struct OffsetListOf : OffsetArrayOf<Type>
{
  inline const Type& operator [] (unsigned int i) const
  {
    if (HB_UNLIKELY (i >= this->len)) return Null(Type);
    return this+this->array[i];
  }
};

typedef OffsetListOf<Lookup> LookupList;
ASSERT_SIZE (LookupList, 2);


/*
 * Coverage Table
 */

struct CoverageFormat1
{
  friend struct Coverage;

  private:
  inline unsigned int get_coverage (hb_codepoint_t glyph_id) const
  {
    if (HB_UNLIKELY (glyph_id > 0xFFFF))
      return NOT_COVERED;
    GlyphID gid;
    gid = glyph_id;
    // TODO: bsearch
    unsigned int num_glyphs = glyphArray.len;
    for (unsigned int i = 0; i < num_glyphs; i++)
      if (gid == glyphArray[i])
        return i;
    return NOT_COVERED;
  }

  inline bool sanitize (SANITIZE_ARG_DEF) {
    return SANITIZE (glyphArray);
  }

  private:
  USHORT	coverageFormat;	/* Format identifier--format = 1 */
  ArrayOf<GlyphID>
		glyphArray;	/* Array of GlyphIDs--in numerical order */
};
ASSERT_SIZE (CoverageFormat1, 4);

struct CoverageRangeRecord
{
  friend struct CoverageFormat2;

  private:
  inline unsigned int get_coverage (hb_codepoint_t glyph_id) const
  {
    if (glyph_id >= start && glyph_id <= end)
      return (unsigned int) startCoverageIndex + (glyph_id - start);
    return NOT_COVERED;
  }

  inline bool sanitize (SANITIZE_ARG_DEF) {
    return SANITIZE_SELF ();
  }

  private:
  GlyphID	start;			/* First GlyphID in the range */
  GlyphID	end;			/* Last GlyphID in the range */
  USHORT	startCoverageIndex;	/* Coverage Index of first GlyphID in
					 * range */
};
ASSERT_SIZE_DATA (CoverageRangeRecord, 6, "\000\001");

struct CoverageFormat2
{
  friend struct Coverage;

  private:
  inline unsigned int get_coverage (hb_codepoint_t glyph_id) const
  {
    // TODO: bsearch
    unsigned int count = rangeRecord.len;
    for (unsigned int i = 0; i < count; i++)
    {
      unsigned int coverage = rangeRecord[i].get_coverage (glyph_id);
      if (coverage != NOT_COVERED)
        return coverage;
    }
    return NOT_COVERED;
  }

  inline bool sanitize (SANITIZE_ARG_DEF) {
    return SANITIZE (rangeRecord);
  }

  private:
  USHORT	coverageFormat;	/* Format identifier--format = 2 */
  ArrayOf<CoverageRangeRecord>
		rangeRecord;	/* Array of glyph ranges--ordered by
				 * Start GlyphID. rangeCount entries
				 * long */
};
ASSERT_SIZE (CoverageFormat2, 4);

struct Coverage
{
  inline unsigned int operator() (hb_codepoint_t glyph_id) const { return get_coverage (glyph_id); }

  unsigned int get_coverage (hb_codepoint_t glyph_id) const
  {
    switch (u.format) {
    case 1: return u.format1->get_coverage(glyph_id);
    case 2: return u.format2->get_coverage(glyph_id);
    default:return NOT_COVERED;
    }
  }

  inline bool sanitize (SANITIZE_ARG_DEF) {
    if (!SANITIZE (u.format)) return false;
    switch (u.format) {
    case 1: return u.format1->sanitize (SANITIZE_ARG);
    case 2: return u.format2->sanitize (SANITIZE_ARG);
    default:return true;
    }
  }

  private:
  union {
  USHORT		format;		/* Format identifier */
  CoverageFormat1	format1[];
  CoverageFormat2	format2[];
  } u;
};
ASSERT_SIZE (Coverage, 2);


/*
 * Class Definition Table
 */

struct ClassDefFormat1
{
  friend struct ClassDef;

  private:
  inline hb_ot_layout_class_t get_class (hb_codepoint_t glyph_id) const
  {
    if ((unsigned int) (glyph_id - startGlyph) < classValue.len)
      return classValue[glyph_id - startGlyph];
    return 0;
  }

  inline bool sanitize (SANITIZE_ARG_DEF) {
    return SANITIZE_SELF () && SANITIZE (classValue);
  }

  USHORT	classFormat;		/* Format identifier--format = 1 */
  GlyphID	startGlyph;		/* First GlyphID of the classValueArray */
  ArrayOf<USHORT>
		classValue;		/* Array of Class Values--one per GlyphID */
};
ASSERT_SIZE (ClassDefFormat1, 6);

struct ClassRangeRecord
{
  friend struct ClassDefFormat2;

  private:
  inline hb_ot_layout_class_t get_class (hb_codepoint_t glyph_id) const
  {
    if (glyph_id >= start && glyph_id <= end)
      return classValue;
    return 0;
  }

  inline bool sanitize (SANITIZE_ARG_DEF) {
    return SANITIZE_SELF ();
  }

  private:
  GlyphID	start;		/* First GlyphID in the range */
  GlyphID	end;		/* Last GlyphID in the range */
  USHORT	classValue;	/* Applied to all glyphs in the range */
};
ASSERT_SIZE_DATA (ClassRangeRecord, 6, "\000\001");

struct ClassDefFormat2
{
  friend struct ClassDef;

  private:
  inline hb_ot_layout_class_t get_class (hb_codepoint_t glyph_id) const
  {
    // TODO: bsearch
    unsigned int count = rangeRecord.len;
    for (unsigned int i = 0; i < count; i++)
    {
      int classValue = rangeRecord[i].get_class (glyph_id);
      if (classValue > 0)
        return classValue;
    }
    return 0;
  }

  inline bool sanitize (SANITIZE_ARG_DEF) {
    return SANITIZE (rangeRecord);
  }

  USHORT	classFormat;	/* Format identifier--format = 2 */
  ArrayOf<ClassRangeRecord>
		rangeRecord;	/* Array of glyph ranges--ordered by
				 * Start GlyphID */
};
ASSERT_SIZE (ClassDefFormat2, 4);

struct ClassDef
{
  inline unsigned int operator() (hb_codepoint_t glyph_id) const { return get_class (glyph_id); }

  hb_ot_layout_class_t get_class (hb_codepoint_t glyph_id) const
  {
    switch (u.format) {
    case 1: return u.format1->get_class(glyph_id);
    case 2: return u.format2->get_class(glyph_id);
    default:return 0;
    }
  }

  inline bool sanitize (SANITIZE_ARG_DEF) {
    if (!SANITIZE (u.format)) return false;
    switch (u.format) {
    case 1: return u.format1->sanitize (SANITIZE_ARG);
    case 2: return u.format2->sanitize (SANITIZE_ARG);
    default:return true;
    }
  }

  private:
  union {
  USHORT		format;		/* Format identifier */
  ClassDefFormat1	format1[];
  ClassDefFormat2	format2[];
  } u;
};
ASSERT_SIZE (ClassDef, 2);


/*
 * Device Tables
 */

struct Device
{
  int get_delta (unsigned int ppem_size) const
  {
    unsigned int f = deltaFormat;
    if (HB_UNLIKELY (f < 1 || f > 3))
      return 0;

    if (ppem_size < startSize || ppem_size > endSize)
      return 0;

    unsigned int s = ppem_size - startSize;

    unsigned int byte = deltaValue[s >> (4 - f)];
    unsigned int bits = (byte >> (16 - (((s & ((1 << (4 - f)) - 1)) + 1) << f)));
    unsigned int mask = (0xFFFF >> (16 - (1 << f)));

    int delta = bits & mask;

    if (delta >= ((mask + 1) >> 1))
      delta -= mask + 1;

    return delta;
  }

  inline int operator() (unsigned int ppem_size) const { return get_delta (ppem_size); }

  private:
  USHORT	startSize;	/* Smallest size to correct--in ppem */
  USHORT	endSize;	/* Largest size to correct--in ppem */
  USHORT	deltaFormat;	/* Format of DeltaValue array data: 1, 2, or 3 */
  USHORT	deltaValue[];	/* Array of compressed data */
};
ASSERT_SIZE (Device, 6);


#endif /* HB_OT_LAYOUT_COMMON_PRIVATE_HH */
