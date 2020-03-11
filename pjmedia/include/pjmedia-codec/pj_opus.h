
#ifndef __PJMEDIA_CODEC_OUR_OPUS_CODEC_H__
#define __PJMEDIA_CODEC_OUR_OPUS_CODEC_H__

/**
 * @file pj_opus.h
 * @brief OPUS codec.
 */

#include <pjmedia-codec/types.h>

#ifndef PJMEDIA_OUR_OPUS_CODEC_SAMPLE_RATE
#define PJMEDIA_OUR_OPUS_CODEC_SAMPLE_RATE 16000
#endif


PJ_BEGIN_DECL

PJ_DECL(pj_status_t) pjmedia_codec_opus_init( pjmedia_endpt *endpt);
PJ_DECL(pj_status_t) pjmedia_codec_opus_deinit();

PJ_END_DECL


/**
 * @}
 */

#endif	/* __PJMEDIA_CODEC_OUR_OPUS_CODEC_H__ */
