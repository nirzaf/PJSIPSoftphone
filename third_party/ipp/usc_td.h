/*****************************************************************************
//
// INTEL CORPORATION PROPRIETARY INFORMATION
// This software is supplied under the terms of a license agreement or
// nondisclosure agreement with Intel Corporation and may not be copied
// or disclosed except in accordance with the terms of that agreement.
// Copyright (c) 2006-2011 Intel Corporation. All Rights Reserved.
//
// Intel(R) Integrated Performance Primitives
//
//     USC - Unified Speech Codec interface library
//
//  Purpose: USCI Tones Detector header file.
//               Structures and functions declarations.
*****************************************************************************/
#ifndef __USC_TD_H__
#define __USC_TD_H__
#include "usc_base.h"

/* USC supported tones */
typedef enum {
   USC_NoTone          =  -1,
   USC_Tone_0          =  0,  /*  941 Hz + 1336 Hz */
   USC_Tone_1          =  1,  /*  697 Hz + 1209 Hz */
   USC_Tone_2          =  2,  /*  697 Hz + 1336 Hz */
   USC_Tone_3          =  3,  /*  697 Hz + 1477 Hz */
   USC_Tone_4          =  4,  /*  770 Hz + 1209 Hz */
   USC_Tone_5          =  5,  /*  770 Hz + 1336 Hz */
   USC_Tone_6          =  6,  /*  770 Hz + 1477 Hz */
   USC_Tone_7          =  7,  /*  852 Hz + 1209 Hz */
   USC_Tone_8          =  8,  /*  852 Hz + 1336 Hz */
   USC_Tone_9          =  9,  /*  852 Hz + 1477 Hz */
   USC_Tone_ASTERISK   = 10,  /*  941 Hz + 1209 Hz */
   USC_Tone_HASH       = 11,  /*  941 Hz + 1477 Hz */
   USC_Tone_A          = 12,  /*  697 Hz + 1633 Hz */
   USC_Tone_B          = 13,  /*  770 Hz + 1633 Hz */
   USC_Tone_C          = 14,  /*  852 Hz + 1633 Hz */
   USC_Tone_D          = 15,  /*  941 Hz + 1633 Hz */
   USC_ANS             = 32,  /*  2100+/-15 Hz   */
   USC_slashANS        = 33,  /*  2100+/-15 Hz, Phase rev 450+/-25 ms */
   USC_ANSam           = 34,  /*  2100+/-1 Hz AM at 15+/-0.1 Hz */
   USC_slashANSam      = 35   /*  2100+/-1 Hz, Phase rev 450+/-25 ms, AM at 15+/-0.1 Hz */
}USC_ToneID;

/* USC tone detector modes */
typedef struct {
   int reserved1;
   int reserved2;
   int reserved3;
   int reserved4;
}USC_TD_Modes;

/* USC TD option */
typedef struct {
   int           minframesize;          /* Minimum frame size in bytes */
   int           framesize;             /* Optimal frame size in bytes */
   int           maxframesize;          /* Maximim frame size in bytes */
   USC_PCMType       pcmType;            /* PCM type supported by the filter*/
   USC_TD_Modes     modes;              /* Filter modes*/
}USC_TD_Option;

/* USC TD information */
typedef struct {
   const char        *name;         /* filter name */
   int               nToneIDs;      /*Number of supported tone IDs*/
   const USC_ToneID  *pToneIDsTbl;  /* Table of tones IDs*/
   int               nOptions;      /*Number of supported options*/
   USC_TD_Option    params[1];     /*options supported by the filter type or the current options of an instance*/
}USC_TD_Info;

typedef struct {
    USC_baseFxns std;
   /*   DetectTone()
        handle        - pointer to the input algorithm instance pointer
        pIn           - pointer to input PCM data [input]
        pDetectedTone - pointer to output detected tone [output]
    */
    USC_Status (*DetectTone)(USC_Handle handle, USC_PCMStream *pIn, USC_ToneID *pDetectedToneID);
    /*
        volume - The power level of the tone, expressed in dBm0 after dropping the sign, with range from 0 to -63 dBm0. [input]
    */
    USC_Status (*GenerateTone)(USC_Handle handle, USC_ToneID ToneID, int volume, int durationMS, USC_PCMStream *pOut);

} USC_TD_Fxns;

#endif /* __USC_TD_H__ */
