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
//     USCI base header
//            Basic structures and functions declarations.
//
*****************************************************************************/

#ifndef __USC_BASE_H__
#define __USC_BASE_H__

#ifndef NULL
#ifdef  __cplusplus
#define NULL    0
#else
#define NULL    ((void *)0)
#endif
#endif

#if defined( USC_W32DLL )
  #if defined( _MSC_VER ) || defined( __ICL ) || defined ( __ECL )
    #define USCFUN  __declspec(dllexport)
  #else
    #define USCFUN  extern
  #endif
#else
  #define USCFUN  extern
#endif

/* USC error code */
typedef enum {
   /* errors: negative response */
   USC_UnsupportedFrameSize = -11,
   USC_BadArgument          = -10,
   USC_UnsupportedEchoTail  = -9,
   USC_NotInitialized       = -8,
   USC_InvalidHandler       = -7,
   USC_NoOperation          = -6,
   USC_UnsupportedPCMType   = -5,
   USC_UnsupportedBitRate   = -4,
   USC_UnsupportedFrameType = -3,
   USC_UnsupportedVADType   = -2,
   USC_BadDataPointer       = -1,
   USC_NoError              =  0,
   /* warnings: positive response */
   USC_StateNotChanged      =  1
}USC_Status;

/* USC algorithm type */
typedef enum {
   USC_Codec     = 0,        /* IPP */
   USC_EC        = 1,        /*  a  */
   USC_Filter    = 2,        /*  l  */
   USC_TD        = 3,        /*  g  */
   USC_FilterVAD = 4,        /*  os */
   USC_T38       = 5,        /*     */
   USC_MAX_ALG   = 0xffff    /*     */
}USC_AlgType;

/* USC PCM stream type */
typedef struct {
   int  sample_frequency; /* sample rate in Hz */
   int  bitPerSample;     /* bit per sample */
   int  nChannels;        /* file characteristic */
}USC_PCMType;

/* USC memory types */
typedef enum {
   USC_OBJECT       = 0, /* persistent memory */
   USC_BUFFER       = 1, /* scratch memory */
   USC_CONST        = 2, /* memory for tables, constants  */
   USC_MAX_MEM_TYPES     /* Number of memory types */
} USC_MemType;


/* USC memory space types */
typedef enum {
   USC_NORMAL    = 0,   /* normal memory space */
   USC_MEM_CHIP  = 1,   /* high speed memory*/
   USC_MAX_SPACE        /* Number of memory space types */
}USC_MemSpaceType;

/* USC memory banks */
typedef struct {
   char            *pMem;         /* memory buffer pointer */
   int              nbytes;       /* memory buffer length in bytes */
   int              align;        /* memory buffer alignment type */
   USC_MemType      memType;      /* memory usage type */
   USC_MemSpaceType memSpaceType; /* memory type */
}USC_MemBank;

/* USC PCM stream */
typedef struct {
   char        *pBuffer;   /* PCM data buffer pointer */
   int          nbytes;    /* data buffer size in byte */
   USC_PCMType  pcmType;   /* PCM info */
   int          bitrate;   /* in bps */
}USC_PCMStream;

typedef void* USC_Handle;

/*/////////////////////////////////////////////////////////////////////////
  USC base functions definition
*/

/*  GetInfoSize() - Returns a size of memory buffer an application
   to allocate which can be used as the GetInfo function pInfo parameter
*/
typedef USC_Status (*GetInfoSize_func)(int *pSize);
/*  Get_Info() - quest an algorithm specific information
       General inquiry is possible without initialization when handle==NULL.
       pInfo - pointer to the structure to be filled by USC algorithm
*/
typedef USC_Status (*GetInfo_func)(USC_Handle handle, void *pInfo);
/*  NumAlloc() - inquiring number of memory buffers
       memOptions - poiter to the algorithm specific memory options structure
       nbanks     - number of table entries (size of pBanks table).
*/
typedef USC_Status (*NumAlloc_func)(const void *memOptions, int *nbanks);
/*  MemAlloc() - inquiring information about memory requirement
                  (buffers to be allocated)
       memOptions - pointer to the algorithm specific memory options structure
       pBanks     - pointer to the input/output of memory banks table
                    to be filled with memory requirement

*/
typedef USC_Status (*MemAlloc_func)(const void *memOptions, USC_MemBank *pBanks);
/*  Init() - create an USC algorithm handle and set it to initial state
       initOptions - pointer to an algorithm specific options structure
       pBanks      - allocated memory banks of number as after NumAlloc
       handle      - pointer to the output algorithm instance pointer
*/
typedef USC_Status (*Init_func)(const void *initOptions, const USC_MemBank *pBanks, USC_Handle *handle );
/*  Reinit() - set an algorithm to initial state
       reinitParams - pointer to an USC algorithm specific options structure
       handle       - pointer to the input algorithm instance pointer
*/
typedef USC_Status (*Reinit_func)(const void *reinitParams, USC_Handle handle );
/*  Control() - alternate an algorithm modes
               The only modes were set on Init() may be alternated.
       controlParams - pointer to the algorithm specific control options structure
       handle        - pointer to the input algorithm instance pointer
*/
typedef USC_Status (*Control_func)(const void *controlParams, USC_Handle handle );

/*/////////////////////////////////////////////////////////////////////////
// USC base functions table definition.
    - Query an USC algorithm memory requirement by MemAlloc function
    - Use Init function to instantiate an USC algorithm object and get its handle.
      Different instances of an USC algorithm may be created and used in parallel.
*/

typedef struct {
   USC_AlgType       algType;

   GetInfoSize_func  GetInfoSize;
   GetInfo_func      GetInfo;
   NumAlloc_func     NumAlloc;
   MemAlloc_func     MemAlloc;
   Init_func         Init;
   Reinit_func       Reinit;
   Control_func      Control;

} USC_baseFxns;

/*USC bad arguments check helpers
   _USC_NO_BADARG_CHECKS to switch bad agrument check off */
#ifdef _USC_NO_BADARG_CHECKS
   #define USC_BADARG( expr, ErrCode )
#else
   #define USC_BADARG( expr, ErrCode )\
                {if (expr) { return ErrCode; }}
#endif

   #define USC_CHECK_PTR( ptr )\
                USC_BADARG( NULL==(ptr), USC_BadDataPointer )

   #define USC_CHECK_HANDLE( ptr )\
                USC_BADARG( NULL==(ptr), USC_InvalidHandler )

#endif /* __USC_BASE_H__ */
