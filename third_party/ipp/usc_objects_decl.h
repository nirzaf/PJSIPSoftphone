/*****************************************************************************
//
// INTEL CORPORATION PROPRIETARY INFORMATION
// This software is supplied under the terms of a license agreement or
// nondisclosure agreement with Intel Corporation and may not be copied
// or disclosed except in accordance with the terms of that agreement.
// Copyright (c) 2007-2011 Intel Corporation. All Rights Reserved.
//
// Intel(R) Integrated Performance Primitives
//
//     USCI - Unified Speech Codec Interface
//
//     External USC function tables declaration.
//
*****************************************************************************/
#ifndef _USC_OBJECTS_DECL_H_
#define _USC_OBJECTS_DECL_H_

#ifdef __cplusplus
extern "C" {
#endif

   /* USC codecs function tables declaration*/
   #if (defined _USC_G729 || defined _USC_CODECS || defined _USC_ALL)
      USCFUN USC_Fxns USC_G729I_Fxns;
      USCFUN USC_Fxns USC_G729A_Fxns;
   #endif
   #if (defined _USC_G729FP || defined _USC_CODECS || defined _USC_ALL)
      USCFUN USC_Fxns USC_G729IFP_Fxns;
      USCFUN USC_Fxns USC_G729AFP_Fxns;
   #endif
   #if (defined _USC_AMRWB || defined _USC_CODECS || defined _USC_ALL)
      USCFUN USC_Fxns USC_AMRWB_Fxns;
   #endif
   #if (defined _USC_G728 || defined _USC_CODECS || defined _USC_ALL)
      USCFUN USC_Fxns USC_G728_Fxns;
   #endif
   #if (defined _USC_G722 || defined _USC_CODECS || defined _USC_ALL)
      USCFUN USC_Fxns USC_G722_Fxns;
   #endif
   #if (defined _USC_G726 || defined _USC_CODECS || defined _USC_ALL)
      USCFUN USC_Fxns USC_G726_Fxns;
   #endif
   #if (defined _USC_G723 || defined _USC_CODECS || defined _USC_ALL)
      USCFUN USC_Fxns USC_G723_Fxns;
   #endif
   #if (defined _USC_GSMAMR || defined _USC_CODECS || defined _USC_ALL)
      USCFUN USC_Fxns USC_GSMAMR_Fxns;
   #endif
   #if (defined _USC_GSMFR || defined _USC_CODECS || defined _USC_ALL)
      USCFUN USC_Fxns USC_GSMFR_Fxns;
   #endif
   #if (defined _USC_G722SB || defined _USC_CODECS || defined _USC_ALL)
      USCFUN USC_Fxns USC_G722SB_Fxns;
   #endif
   #if (defined _USC_G711 || defined _USC_CODECS || defined _USC_ALL)
      USCFUN USC_Fxns USC_G711A_Fxns;
      USCFUN USC_Fxns USC_G711U_Fxns;
   #endif
   #if (defined _USC_AMRWBE || defined _USC_CODECS || defined _USC_ALL)
      USCFUN USC_Fxns USC_AMRWBE_Fxns;
   #endif
   #if (defined _USC_G7291 || defined _USC_CODECS || defined _USC_ALL)
      USCFUN USC_Fxns USC_G7291_Fxns;
   #endif
   #if (defined _USC_ILBC)
      USCFUN USC_Fxns USC_ILBC_Fxns;
   #endif
   #if (defined _USC_MSRTA)
      USCFUN USC_Fxns USC_MSRTAFP_Fxns;
   #endif

   /* USC filters function tables declaration*/

   #if (defined _USC_EPHM_NR || defined _USC_FILTERS || defined _USC_ALL)
      USCFUN USC_Filter_Fxns USC_NR_Fxns;
   #endif
   #if (defined _USC_EC_NR || defined _USC_FILTERS || defined _USC_ALL)
      USCFUN USC_Filter_Fxns USC_ECNR_Fxns;
   #endif

   #if (defined _USC_RTA_NR || defined _USC_FILTERS || defined _USC_ALL)
      USCFUN USC_Filter_Fxns USC_RTANR_FP_Fxns;
   #endif

   #if (defined _USC_ALC || defined _USC_FILTERS || defined _USC_ALL)
      USCFUN USC_Filter_Fxns USC_ALC_Fxns;
   #endif

   /* USC tone detectors function tables declaration*/

   #if (defined _USC_ANS_TD || defined _USC_TONES || defined _USC_ALL)
      USCFUN USC_TD_Fxns USC_ANSs_Fxns;
   #endif

   #if (defined _USC_ANSAM_TD || defined _USC_TONES || defined _USC_ALL)
      USCFUN USC_TD_Fxns USC_ANSam_Fxns;
   #endif

   #if (defined _USC_DTMF_TD || defined _USC_TONES || defined _USC_ALL)
      USCFUN USC_TD_Fxns USC_DTMF_Fxns;
   #endif

   /* USC VAD algorithms function tables declaration*/

   #if (defined _USC_G729_VAD || defined _USC_FILTERS || defined _USC_ALL)
      USCFUN USC_Filter_Fxns USC_G729_VAD_Fxns;
   #endif
   #if (defined _USC_G729FP_VAD || defined _USC_FILTERS || defined _USC_ALL)
      USCFUN USC_Filter_Fxns USC_G729FP_VAD_Fxns;
   #endif
   #if (defined _USC_G723_VAD || defined _USC_FILTERS || defined _USC_ALL)
      USCFUN USC_Filter_Fxns USC_G723_VAD_Fxns;
   #endif

   //#if (defined _USC_INTEL_VAD || defined _USC_FILTERS || defined _USC_ALL)
   //   USCFUN USC_Filter_Fxns USC_SR_VAD_Fxns;
   //#endif
   #if (defined _USC_AMRWBE_VAD || defined _USC_FILTERS || defined _USC_ALL)
      USCFUN USC_Filter_Fxns USC_AMRWBE_VAD_Fxns;
   #endif

   /* USC EC function tables declaration*/
   #if (defined _USC_EC_FP || defined _USC_EC_ALL || defined _USC_ALL)
      USCFUN USC_EC_Fxns USC_ECFP_Fxns;
   #endif

#ifdef __cplusplus
}
#endif

#endif /*_USC_OBJECTS_DECL_H_*/
