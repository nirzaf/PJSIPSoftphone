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
//     USCI - Unified Speech Codec Interface
//
//  Purpose: USCI Echo Canceller header file.
//               Structures and functions declarations.
*****************************************************************************/
#ifndef __USC_EC_H__
#define __USC_EC_H__
#include "usc_base.h"

typedef enum {
   EC_SUBBAND     = 0,
   EC_FULLBAND    = 1,
   EC_FASTSUBBAND = 2
} USC_EC_Algs;

typedef enum {
    AD_OFF = 0,       /*            - no adaptation*/
    AD_FULLADAPT = 1, /*   -  full adaptation */
    AD_LITEADAPT = 2  /*     - lite adaptation*/
} USC_AdaptType;
/* USC echo canceller modes */
typedef struct {
   USC_AdaptType adapt;      /* 0 - disable adaptation, 1 - enable full adaptation, 2 - enable lite adaptation  */
   int           zeroCoeff;  /* 0 - no zero coeffs of filters, 1 - zero coeffs of filters  */
   int           cng;        /* 0 - disable CNG, 1 - enable CNG */
   int           nlp;        /* 0 - disable NLP, 1 or 2 - enable different NLP types */
   int           td;         /* 0 - disable ToneDisabler, 1 - enable ToneDisabler */
   int           ah;         /* 0 - disable anti-howling,  1-spectra-based HD, 2- energy-based HD */
   int           ap;         /* affine projection order,1...*/
   int           nr;         /* IPP NR noise mitigation level 0,...,5*/
   int           nr_smooth;  /* IPP NR smoothing mode 0,...,2 */
   int           dcFlag;     /* IPP NR DC compensation flag   */
}USC_EC_Modes;

/* USC echo canceller option */
typedef struct {
   USC_EC_Algs  algType;    /* EC algorithm type */
   USC_PCMType  pcmType;    /* PCM type to support */
   int          echotail;   /* echo tail length value, ms: in range [1, 200] for subband algorithm, [1, 16] for fullband algorithm */
   int          framesize;  /* EC frame size in bytes */
   int          nModes;     /* Number of modes ( actually, supposed to be equal to sizeof(USC_EC_Modes)/sizeof(int) ) */
   USC_EC_Modes modes;      /* EC modes*/
}USC_EC_Option;

/* USC echo canceller information */
typedef struct {
   const char     *name;     /* codec name */
   int             nPcmTypes;
   USC_PCMType    *pPcmTypesTbl; /* supported PCMs lookup table */
   USC_EC_Option   params;    /*options supported by the EC type or the current options of an instance*/
}USC_EC_Info;

typedef struct {
    USC_baseFxns std;
   /*   CancelEcho()
        pSin - pointer to send-in PCM data [input]
        pRin - pointer to receive-in PCM data [input]
        pSout - pointer to send-out PCM data [output]
    */
    USC_Status (*CancelEcho)(USC_Handle handle, short *pSin, short *pRin, short *pSout);
} USC_EC_Fxns;

#endif /* __USC_EC_H__ */
