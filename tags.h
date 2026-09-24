/*
 *   tags is a support module which provides access to metadata tags (and xing/vbri/lame tags as well)
 *   Copyright (C) 2013-2021 Elio Blanca <eblanca76@users.sourceforge.net>
 *
 *   This library is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU Lesser General Public
 *   License as published by the Free Software Foundation; either
 *   version 2.1 of the License, or (at your option) any later version.
 *
 *   This library is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public
 *   License along with this library; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef TAGS_H
#define TAGS_H

#define LAMETAGSIZE        36
#define LAME_STRING_LENGTH 48

#define TAG_NOTAG           0
#define TAG_VBRITAG         1
#define TAG_XINGTAG         2
#define TAG_LAMECBRTAG      3

#define TAG_ID3TAG_11_12_LENGTH                             128
#define TAG_ID3TAG_ENH_LENGTH                               227

/*
 * this value MUST be carefully choosen!
 * (or some calculation will be needed, so boring)
 * Current identification routines work for id3v2, apetag and wave riff,
 * (the metadata tags we can usually find at the very beginning)
 * and they require 10 bytes, 32 bytes and 12 bytes (just look at the code)
 * at least.
 * Should a new tag appear, this value will have to be evaluated again.
 */
#define HEAD_METADATA_MIN_IDENTIFICATION_LENGTH             32

/*
 * checkid3v2(), checkapetagx_head() and checkwaveriff() first locate a short
 * signature ("ID3", "APETAGEX", "RIFF") inside the buffer they are given,
 * then read a larger, fixed-size record starting at that signature. The
 * caller must therefore never report a signature match unless at least this
 * many further REAL bytes are known to follow it in the buffer, or the
 * record read will run past the data the caller actually has:
 *   ID3V2_HEAD_SEARCH_SLACK    = ID3V2_HEADER_LENGTH(10)   - strlen("ID3")(3)
 *   APETAGX_HEAD_SEARCH_SLACK  = sizeof(apefooter_t)(32)   - strlen("APETAGEX")(8)
 *   WAVERIFF_SEARCH_SLACK      = WAVE_RIFF_HEADER_LENGTH(12) - strlen("RIFF")(4)
 * The caller does this by subtracting the slack from the searchable length
 * it passes in (see the head-of-file tag scan in main()), rather than by
 * capping how close to end-of-buffer a refill is allowed to happen (which is
 * what HEAD_METADATA_MIN_IDENTIFICATION_LENGTH above actually guarantees,
 * and is not sufficient by itself: it bounds the buffer's minimum size right
 * after a refill, not how close to the tail of a much larger, still mostly
 * full buffer a signature match may legally sit).
 */
#define ID3V2_HEAD_SEARCH_SLACK                              7
#define APETAGX_HEAD_SEARCH_SLACK                            24
#define WAVERIFF_SEARCH_SLACK                                8

typedef struct id3tag_t {
    unsigned char tag_name[8+1];        /* "v1.0", "v1.1", "v1.2", "Enhanced" */
    unsigned char title[90+1];          /* max length achieved from v1(30) + enh(60) */
    unsigned char artist[90+1];         /* max length achieved from v1(30) + enh(60) */
    unsigned char album[90+1];          /* max length achieved from v1(30) + enh(60) */
    unsigned char year[4+1];            /* only id3v1 provide this */
    unsigned char comment[45+1];        /* max length achieved from v1.0(30, NOT v1.1's
                                            28 - the original 43+1 sizing missed that the
                                            v1.2 extension can follow a v1.0 base tag too)
                                            + v1.2(15). See metadata_cpy() in tags.c: with
                                            the old, undersized array, a v1.0 tag combined
                                            with a v1.2 extension overflowed comment[] by 2
                                            bytes into the adjacent track_n/genre[0] fields. */
    unsigned char track_n;
    unsigned char genre[1+30+1];        /* genre field from enh(30) supersedes similar ones from both v1 and v1.2 */
    unsigned char speed;
    unsigned char start_time[6+1];
    unsigned char end_time[6+1];
} id3tag_t;

/* structure for MusicMatch tag */
typedef struct mmtag_t {
    unsigned int  tag_size;
    unsigned int  image_size;
    unsigned int  metadata_size;
    off_t         image_offset;
    char          mm_ver[5];   /* this field is actually four bytes long, this trick ensures there will always be a null string terminator */
    char          tag_ver[5];  /* this field is actually four bytes long, this trick ensures there will always be a null string terminator */
    char          enc_ver[5];  /* this field is actually four bytes long, this trick ensures there will always be a null string terminator */
    char          image_ext[5];/* this field is actually four bytes long, this trick ensures there will always be a null string terminator */
    char          header_present;
} mmtag_t;

/* structure to receive extracted info tag */
typedef struct vbrtagdata_t
{
    char    *tagId;
    off_t   tagStartsAt;
    unsigned int     header;           /* mpeg header of the frame containing the tag */
    int     frameSize;                 /* I want to keep the size of the frame containing the tag */
    char    infoTag;
    short   version;
    /* The following two fields come from lame vbr tag - they differ from similar info
       written by fhg encoders (stored into streamInfo structure) */
    short   encDelay;                  /* encoder delay (start) */
    short   encPadding;                /* encoder padding (samples added at the end of the wave) */
    short   tocEntries;
    short   sizePerTocEntry;
    short   framesPerTocEntry;
    int     tocSize;
    unsigned int reported_frames;      /* total bit stream frames from Vbr header data */
    unsigned int bytes;                /* total bit stream bytes from Vbr header data*/
    int     vbr_scale;                 /* encoded vbr scale from Vbr header data*/
    char    lame_buggy_vbrheader;
    unsigned char lametag[LAMETAGSIZE];
    char    lametagVerified;
    int     lameMusicCRC;                  /* this is actually 16 bit long
                                              but I need to set -1 for signaling an empty value */
} vbrtagdata_t;


int  extract_enc_string(char *, unsigned char *, int);
char checkvbrinfotag(vbrtagdata_t *, unsigned char *, off_t, char *);
void show_info_tag (vbrtagdata_t *);
void show_id3v1(id3tag_t *);
int  checkid3v1(FILE *, off_t, id3tag_t *);
int  checkid3v2_footer(FILE *, off_t, unsigned char *, unsigned char *);
int  checkapetagx_tail(FILE *, off_t, int *, int *, char *);
int  checklyrics3v1(FILE *, off_t);
int  checklyrics3v2(FILE *, off_t);
void checkmmtag(FILE *, off_t, mmtag_t *);
char checkmm_partial_tag(FILE *, off_t, mmtag_t *);
int  checkid3v2(unsigned char *, int, int *, unsigned char *, unsigned char *);
int  checkapetagx_head(unsigned char *, int, int *, int *, int *, char *);
int  checkwaveriff(unsigned char *, int, int *);
int  checkwaveriff_datachunk(unsigned char *, int *, int *);

#endif
