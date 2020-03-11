/* $Id: types.h 5791 2018-05-14 01:24:43Z ming $ */
/* 
 * Copyright (C) 2008-2011 Teluu Inc. (http://www.teluu.com)
 * Copyright (C) 2003-2008 Benny Prijono <benny@prijono.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */
#ifndef __PJMEDIA_CODEC_TYPES_H__
#define __PJMEDIA_CODEC_TYPES_H__

/**
 * @file types.h
 * @brief PJMEDIA-CODEC types and constants
 */

#include <pjmedia-codec/config.h>
#include <pjmedia/codec.h>

/**
 * @defgroup pjmedia_codec_types PJMEDIA-CODEC Types and Constants
 * @ingroup PJMEDIA_CODEC
 * @brief Constants used by PJMEDIA-CODEC
 * @{
 */



/**
 * These are the dynamic payload types that are used by audio codecs in
 * this library. Also see the header file <pjmedia/codec.h> for list
 * of static payload types.
 */
#if defined(PJMEDIA_RTP_USE_OUR_PT) && (PJMEDIA_RTP_USE_OUR_PT!=0)

// for buggy Asterisk 11 with directmedia problems
// https://issues.asterisk.org/jira/browse/ASTERISK-17410

#define PJMEDIA_RTP_PT_START = (PJMEDIA_RTP_PT_DYNAMIC-1)
#define PJMEDIA_RTP_PT_SPEEX_NB 110
#define PJMEDIA_RTP_PT_SPEEX_WB 117
#define PJMEDIA_RTP_PT_SPEEX_UWB 119
#define PJMEDIA_RTP_PT_SILK_NB 96
#define PJMEDIA_RTP_PT_SILK_MB 100
#define PJMEDIA_RTP_PT_SILK_WB 107
#define PJMEDIA_RTP_PT_SILK_SWB 108
#define PJMEDIA_RTP_PT_ILBC 97
#define PJMEDIA_RTP_PT_L16_16KHZ_MONO 118
/*
a=rtpmap:112 AAL2-G726-32/8000
a=rtpmap:111 G726-32/8000
a=rtpmap:102 G7221/16000
a=rtpmap:115 G7221/32000
a=rtpmap:116 G719/48000
a=rtpmap:10 L16/8000
a=rtpmap:118 L16/16000
a=rtpmap:101 telephone-event/8000
*/
//not used in Asterisk and can be any not used <= 127
#define PJMEDIA_RTP_PT_GSMEFR 113
#define PJMEDIA_RTP_PT_AMR 120
#define PJMEDIA_RTP_PT_AMRWB 121
#define PJMEDIA_RTP_PT_AMRWBE 122
#define PJMEDIA_RTP_PT_OPUS 123
#define PJMEDIA_RTP_PT_L16_8KHZ_MONO 124
#define PJMEDIA_RTP_PT_L16_8KHZ_STEREO 125
#define PJMEDIA_RTP_PT_L16_16KHZ_STEREO 126

#else

enum pjmedia_audio_pt
{
    /* According to IANA specifications, dynamic payload types are to be in
     * the range 96-127 (inclusive). This enum is structured to place the
     * values of the payload types specified below into that range.
     *
     * PJMEDIA_RTP_PT_DYNAMIC is defined in <pjmedia/codec.h>. It is defined
     * to be 96.
     *
     * PJMEDIA_RTP_PT_TELEPHONE_EVENTS is defined in <pjmedia/config.h>.
     * The default value is 96.
     */
#if PJMEDIA_RTP_PT_TELEPHONE_EVENTS
    PJMEDIA_RTP_PT_START = PJMEDIA_RTP_PT_TELEPHONE_EVENTS,
#else
    PJMEDIA_RTP_PT_START = (PJMEDIA_RTP_PT_DYNAMIC-1),
#endif

    PJMEDIA_RTP_PT_SPEEX_NB,			/**< Speex narrowband/8KHz  */
    PJMEDIA_RTP_PT_SPEEX_WB,			/**< Speex wideband/16KHz   */
    PJMEDIA_RTP_PT_SPEEX_UWB,			/**< Speex 32KHz	    */
    PJMEDIA_RTP_PT_SILK_NB,			/**< SILK narrowband/8KHz   */
    PJMEDIA_RTP_PT_SILK_MB,			/**< SILK mediumband/12KHz  */
    PJMEDIA_RTP_PT_SILK_WB,			/**< SILK wideband/16KHz    */
    PJMEDIA_RTP_PT_SILK_SWB,			/**< SILK 24KHz		    */
    PJMEDIA_RTP_PT_ILBC,			/**< iLBC (13.3/15.2Kbps)   */
    PJMEDIA_RTP_PT_AMR,				/**< AMR (4.75 - 12.2Kbps)  */
    PJMEDIA_RTP_PT_AMRWB,			/**< AMRWB (6.6 - 23.85Kbps)*/
    PJMEDIA_RTP_PT_AMRWBE,			/**< AMRWBE		    */
    PJMEDIA_RTP_PT_G726_16,			/**< G726 @ 16Kbps	    */
    PJMEDIA_RTP_PT_G726_24,			/**< G726 @ 24Kbps	    */
    PJMEDIA_RTP_PT_G726_32,			/**< G726 @ 32Kbps	    */
    PJMEDIA_RTP_PT_G726_40,			/**< G726 @ 40Kbps	    */
    PJMEDIA_RTP_PT_G722_1_16,			/**< G722.1 (16Kbps)	    */
    PJMEDIA_RTP_PT_G722_1_24,			/**< G722.1 (24Kbps)	    */
    PJMEDIA_RTP_PT_G722_1_32,			/**< G722.1 (32Kbps)	    */
    PJMEDIA_RTP_PT_G7221C_24,			/**< G722.1 Annex C (24Kbps)*/
    PJMEDIA_RTP_PT_G7221C_32,			/**< G722.1 Annex C (32Kbps)*/
    PJMEDIA_RTP_PT_G7221C_48,			/**< G722.1 Annex C (48Kbps)*/
    PJMEDIA_RTP_PT_G7221_RSV1,			/**< G722.1 reserve	    */
    PJMEDIA_RTP_PT_G7221_RSV2,			/**< G722.1 reserve	    */
    PJMEDIA_RTP_PT_OPUS,			/**< OPUS                   */
#if PJMEDIA_CODEC_L16_HAS_8KHZ_MONO
    PJMEDIA_RTP_PT_L16_8KHZ_MONO,		/**< L16 @ 8KHz, mono	    */
#endif
#if PJMEDIA_CODEC_L16_HAS_8KHZ_STEREO
    PJMEDIA_RTP_PT_L16_8KHZ_STEREO,		/**< L16 @ 8KHz, stereo     */
#endif
    //PJMEDIA_RTP_PT_L16_11KHZ_MONO,		/**< L16 @ 11KHz, mono	    */
    //PJMEDIA_RTP_PT_L16_11KHZ_STEREO,		/**< L16 @ 11KHz, stereo    */
#if PJMEDIA_CODEC_L16_HAS_16KHZ_MONO
    PJMEDIA_RTP_PT_L16_16KHZ_MONO,		/**< L16 @ 16KHz, mono	    */
#endif
#if PJMEDIA_CODEC_L16_HAS_16KHZ_STEREO
    PJMEDIA_RTP_PT_L16_16KHZ_STEREO,		/**< L16 @ 16KHz, stereo    */
#endif
    //PJMEDIA_RTP_PT_L16_22KHZ_MONO,		/**< L16 @ 22KHz, mono	    */
    //PJMEDIA_RTP_PT_L16_22KHZ_STEREO,		/**< L16 @ 22KHz, stereo    */
    //PJMEDIA_RTP_PT_L16_32KHZ_MONO,		/**< L16 @ 32KHz, mono	    */
    //PJMEDIA_RTP_PT_L16_32KHZ_STEREO,		/**< L16 @ 32KHz, stereo    */
#if PJMEDIA_CODEC_L16_HAS_48KHZ_MONO
    PJMEDIA_RTP_PT_L16_48KHZ_MONO,		/**< L16 @ 48KHz, mono	    */
#endif
#if PJMEDIA_CODEC_L16_HAS_48KHZ_STEREO
    PJMEDIA_RTP_PT_L16_48KHZ_STEREO,		/**< L16 @ 48KHz, stereo    */
#endif

    /* Caution!
     * Ensure the value of the last pt above is <= 127.
     */
};

#endif

/**
 * These are the dynamic payload types that are used by video codecs in
 * this library.
 */
#if defined(PJMEDIA_RTP_USE_OUR_PT) && (PJMEDIA_RTP_USE_OUR_PT!=0)

// for buggy Asterisk 11 with directmedia problems
// https://issues.asterisk.org/jira/browse/ASTERISK-17410

#define PJMEDIA_RTP_PT_H263P 98
#define PJMEDIA_RTP_PT_H264 99
#define PJMEDIA_RTP_PT_VP8 103
/*
Asterisk PBX 11.5.1 codecs:
a=rtpmap:31 H261/90000

a=rtpmap:34 H263/90000
a=fmtp:34 F=0;I=0;J=0;T=0;K=0;N=0;BPP=0;HRD=0

a=rtpmap:98 h263-1998/90000
a=fmtp:98 QCIF=1;CIF=1;F=0;I=0;J=0;T=0;K=0;N=0;BPP=0;HRD=0

a=rtpmap:99 H264/90000
a=fmtp:99 profile-level-id=42E01E

a=rtpmap:104 MP4V-ES/90000
*/

#else

enum pjmedia_video_pt
{
     /* Video payload types */
     PJMEDIA_RTP_PT_VID_START = (PJMEDIA_RTP_PT_DYNAMIC-1),
     PJMEDIA_RTP_PT_H263P,
     PJMEDIA_RTP_PT_H264,
     PJMEDIA_RTP_PT_H264_RSV1,
     PJMEDIA_RTP_PT_H264_RSV2,
     PJMEDIA_RTP_PT_H264_RSV3,
     PJMEDIA_RTP_PT_H264_RSV4,

     /* Caution!
      * Ensure the value of the last pt above is <= 127.
      */
};

#endif

/**
 * @}
 */


#endif	/* __PJMEDIA_CODEC_TYPES_H__ */
