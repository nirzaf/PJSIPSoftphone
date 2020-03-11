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
//  Purpose: USCI Filter header file.
//               Structures and functions declarations.
*****************************************************************************/

#ifndef __USC_FILTER_H__
#define __USC_FILTER_H__

#include "usc_base.h"
/* VAD decision results. */
typedef enum
{
    NODECISION    = -1,
    ACTIVE        = 0,
    INACTIVE      = 1,
    END_OF_STREAM = 2
} USC_FrameType;

/* USC filter modes */
typedef struct {
   int vad; /* 0 - disabled, otherwize vad type (1,2, more if any) */
   int reserved2;
   int reserved3;
   int reserved4;
}USC_FilterModes;

/* USC filter option */
typedef struct {
   int           minframesize;          /* Minimum filter frame size in bytes */
   int           framesize;             /* Optimal filter frame size in bytes */
   int           maxframesize;          /* Maximim filter frame size in bytes */
   USC_PCMType      pcmType;            /* PCM type supported by the filter*/
   USC_FilterModes  modes;              /* Filter modes*/
}USC_FilterOption;

/* USC filter information */
typedef struct {
   const char       *name;         /* filter name */
   int            nOptions;      /*Number of supported options*/
   USC_FilterOption  params[1];     /*options supported by the filter type or the current options of an instance*/
}USC_FilterInfo;

typedef struct {
    USC_baseFxns std;
    /*   SetDlyLine()
        The function copies the pointed vector to internal delay line
        handle - pointer to the input algorithm instance pointer
        pDlyLine - pointer to delay line for lowpass filtering in the lenght of framesize [input]
    */
    USC_Status (*SetDlyLine)(USC_Handle handle, char *pDlyLine);
   /*   Filter()
        handle - pointer to the input algorithm instance pointer
        pIn - pointer to input PCM data [input]
        pOut - pointer to output PCM data [output]
    */
    USC_Status (*Filter)(USC_Handle handle, USC_PCMStream *pIn, USC_PCMStream *pOut, USC_FrameType *pDecision);
} USC_Filter_Fxns;

#endif /* __USC_FILTER_H__ */
