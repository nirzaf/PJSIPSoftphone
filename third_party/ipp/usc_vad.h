/*****************************************************************************
//
// INTEL CORPORATION PROPRIETARY INFORMATION
// This software is supplied under the terms of a license agreement or
// nondisclosure agreement with Intel Corporation and may not be copied
// or disclosed except in accordance with the terms of that agreement.
// Copyright (c) 2005-2011 Intel Corporation. All Rights Reserved.
//
// Intel(R) Integrated Performance Primitives
//
//     USCI - Unified Speech Codec Interface
//
//  Purpose: USCI Voice Activity Detector header file.
//               Structures and functions declarations.
*****************************************************************************/
#ifndef __USCVAD_H__
#define __USCVAD_H__
#include "usc_base.h"

/* Used for only VAD_SR. VAD decision states. */
typedef enum
{
    NODECISION    = -1,//0
    ACTIVE        = 0,//3
    INACTIVE      = 1,//1
    END_OF_STREAM = 2
}
VADDecisionState;

/* USC VAD modes, (may be alternated) */
typedef struct {
   int truncate; /* 0 - no truncate */
   int vad; /* 0 - disabled, otherwize vad type (1,2, more if any) */
   int hpf; /* high pass filter: 1- on, 0- off */
}USC_VAD_Modes;

/* USC VAD option */
typedef struct {
   int framesize; /* a VAD frame size +++++*/
   USC_PCMType pcmType; /* PCM type to support */
   int nModes;
   USC_VAD_Modes modes;
}USC_VAD_Option;

/* USC VAD information */
typedef struct {
   const char *name; /* VAD name */
   int nPcmTypes;
   USC_PCMType *pPcmTypesTbl;  /* supported PCMs lookup table */
   USC_VAD_Option params; /* what is supported */
}USC_VAD_Info;

/* USC functions table.
   VAD for codec should supply a function table structure, which is derived from this base table.
   VAD table is to be specified for each codec by name as follows:
       USC_<codec-name>_VAD_Fxns, for example USC_G729_VAD_Fxns.
   The typical usage model:
    - Questing a VAD about memory requirement using  MemAlloc() function
      which returns a memory banks description table with required bank sizes.
    - Use Init() function to create VAD instance according to modes requested.
      VAD handle is returned. Thus different instances of particular codec may be created
      and used in parallel.
    - detectVAD() - Voice Activity Detector functions.
    - GetInfo() - inquire VAD state or VAD requirement.
*/

typedef struct {

    USC_baseFxns std;

   /*   VADetect()
        in - input audio stream (pcm) pointer,
        out - output audio stream (pcm) pointer ,
        pVADDecision - 0 (No voice detected), 1 (voice detected)
    */

    USC_Status (*VADetect)(USC_Handle handle, USC_PCMStream *in, USC_PCMStream *out, VADDecisionState *pVADDecision);

} USC_VAD_Fxns;

#endif /* __USCVAD_H__ */
