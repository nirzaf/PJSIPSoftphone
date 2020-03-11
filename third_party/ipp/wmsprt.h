//*@@@+++@@@@******************************************************************
//
// Microsoft Windows Media
// Copyright (C) Microsoft Corporation. All rights reserved.
//
//*@@@---@@@@******************************************************************
/************************************************************************
 *                                                                      *
 *  WMS   ------   Windows Media Speech RT Codec                        *
 *                                                                      *
 *  Copyright (C) 2002 Microsoft Corporation. All rights reserved       *
 *                                                                      *
 *  Name:   wmsprt.h                                                    *
 *                                                                      *
 *  Note:   The header file for all the public functions of RT codec    *
 *                                                                      *
 *  Revision History:                                                   *
 *                                                                      *
 *    9/10/2002    Tian Wang       First version                        *
 *                                                                      *
 ************************************************************************/

#ifndef __WMSPRT_H__
#define __WMSPRT_H__

#include <stddef.h>

// =============== Global definitions ===============
#define MSRT_AUDIO_PAYLOAD_TYPE         114

#define RT_FIXED_RATE                   0
#define RT_VARIABLE_RATE                1

#define RT_PACKET_UNDEF                 0
#define RT_PACKET_DATA                  1
#define RT_PACKET_INDEPFEC              2

#define JITTER_MODE_NORMAL              0
#define JITTER_MODE_HIGHQUALITY         1
#define JITTER_MODE_LOWDELAY            2

#define PACKET_LOSS_MODE_NORMAL         0   // not significant 0% ~ 2%
#define PACKET_LOSS_MODE_LIGHT          1   // light 2% ~ 5%
#define PACKET_LOSS_MODE_MEDIUM         2   // medium 5% ~ 10%
#define PACKET_LOSS_MODE_HIGH           3   // high 10%+

#define SET_FEC_SHIFT                   0   //[0:3]
#define SET_FEC_MASK                    0x0f
#define SET_FEC_NONE                    0   // No FEC
#define SET_FEC_FRAME_RESET             1   // Use one stage per subframe for reseting
#define SET_FEC_ACB_CLOSELOOP           2   // Encode the previous excitation as the acb input for the current frame
#define SET_FEC_ACB_OPENLOOP            3   // Encode the previous excitation
#define SET_FEC_INTERNAL                4
#define SET_FEC_INDEPENDENT             5   // Encode the previous frame as normal mode

#define SET_COMPLEXITY_SHIFT            4   // [4:6]
#define SET_DEC_COMPLEXITY_SHIFT        0   // [0:2]
#define SET_COMPLEXITY_MASK             0x7
#define SET_COMPLEXITY_LEVEL_0          0
#define SET_COMPLEXITY_LEVEL_1          1
#define SET_COMPLEXITY_LEVEL_2          2
#define SET_COMPLEXITY_LEVEL_3          3

#define SET_DEC_COMFORTNOISE_SHIFT      3   // [3]
#define SET_DEC_COMFORTNOISE_MASK       0x1
#define SET_DEC_COMFORTNOISE_ON         1
#define SET_DEC_COMFORTNOISE_OFF        0

#ifndef U32
typedef unsigned long   U32;
#endif

#ifndef I32
typedef long   I32;
#endif

#ifndef U16
typedef unsigned short  U16;
#endif

#ifndef I16
typedef short   I16;
#endif

#ifndef U8
typedef unsigned char   U8;
#endif

#ifndef REFERENCE_TIME
typedef __int64 REFERENCE_TIME;
#endif

#ifndef LONG
typedef long   LONG;
#endif

#ifndef DWORD
typedef unsigned long   DWORD;
#endif

#ifndef Bool
typedef I32   Bool;
#endif

#ifndef BITS_PER_BYTE
#define BITS_PER_BYTE   8
#endif

typedef void* MSRTAEncHandle;
typedef void* MSRTADecHandle;
typedef void* MSRTADejitHandle;

typedef int MSRTARESULT ;

// SUCCESS codes
static const MSRTARESULT  MSRTA_OK                   = 0x00000000;
static const MSRTARESULT  MSRTA_S_FALSE              = 0x00000001;
static const MSRTARESULT  MSRTA_S_BUFUNDERFLOW       = 0x00000002;
static const MSRTARESULT  MSRTA_S_NEWPACKET          = 0x00000003;
static const MSRTARESULT  MSRTA_S_NO_MORE_FRAME      = 0x00000004;
static const MSRTARESULT  MSRTA_S_DEFAULT            = 0x00000005;
static const MSRTARESULT  MSRTA_S_SWITCHCHMODE       = 0x00000006;
static const MSRTARESULT  MSRTA_S_OUTPUT_BUFFER_FULL = 0x00000007;
static const MSRTARESULT  MSRTA_S_INPUTBUFFER_EMPTY  = 0x00000008;
static const MSRTARESULT  MSRTA_S_EOF = 0x00000009;

// ERROR codes
static const MSRTARESULT  MSRTA_E_FAIL           = 0x80004005;
static const MSRTARESULT  MSRTA_E_OUTOFMEMORY    = 0x8007000E;
static const MSRTARESULT  MSRTA_E_INVALIDARG     = 0x80070057;
static const MSRTARESULT  MSRTA_E_NOTSUPPORTED   = 0x80040000;
static const MSRTARESULT  MSRTA_E_LOSTPACKET     = 0x80040001;
static const MSRTARESULT  MSRTA_E_BROKEN_FRAME   = 0x80040002;
static const MSRTARESULT  MSRTA_E_BUFFEROVERFLOW = 0x80040003;
static const MSRTARESULT  MSRTA_E_ONHOLD         = 0x80040004;
static const MSRTARESULT  MSRTA_E_QUIT           = 0x80040005;

#define MSRTA_SUCCEEDED(Status) ((MSRTARESULT )(Status) >= 0)
#define MSRTA_FAILED(Status) ((MSRTARESULT )(Status)<0)

typedef enum _MSRTADecoderResetFlag{
    MSRTA_RESET_NO = 0,
    MSRTA_RESET_ALL,
    MSRTA_RESET_LSP,
} MSRTADecoderReset;

// =============== Structures ===============
typedef struct {

    // base WAVEFORMATEX
    U32     nSamplesPerSec;                 // [in] set sampling rate before initializing codec
    U32     nAvgBytesPerSec;                // [in] set byte rate before initializing codec
    U16     nChannels;                      // [in] must be set to 1 before intializing codec
    U16     nBlockAlign;                    // [out] max bytes per packet.
    U16     nSamplesPerFrame;               // [out] this is used in decoder dmo for testing variable rate
    U16     wFormatTag;

    U16     fVariableRate;                  // [in] Set true if variable rate is used
    Bool    fLocalDecoding;                 // [in] Set true for getting synthesized speech in encoder
    U16     fNPPMode;                       // [in] NPP Mode to turn on/off the npp(noise pre-process)& choose NPP profile level
} MSRTAFormatInfo;

typedef struct {

    DWORD   fUnmodifiedAudio;               // [out] true is frame has unmodified decoded audio
    DWORD   fStretchedAudio;                // [out] true is frame has some stretched decoded audio
    DWORD   fCompressedAudio;               // [out] true is frame has some shortened decoded audio
    DWORD   fConcealedAudio;                // [out] true is frame has some concealed audio
    DWORD   fComfortNoise;                  // [out] true is frame has some comfort noise generated during VAD
    U32     nAvgEnergy;                     // [out] Average energy in locality of pulled samples.
    REFERENCE_TIME  tmRTPSrcTimestamp;      // [out] RTP timestamp of source packet producing first sample in current audio block

} MSRTA_Healing_Info;

// =============== Prototypes ===============
#ifdef MSRTA_FS
#define DMOSTDCALL      __cdecl
#else
#define DMOSTDCALL
#endif

#ifdef  __cplusplus
extern "C" {
#endif


typedef void * (DMOSTDCALL * PFN_Malloc) (size_t lSize);
typedef void (DMOSTDCALL * PFN_Free) (void *pvMemory);

// ******* Encoder *******
MSRTARESULT  DMOSTDCALL MSRTAEncoderNew( MSRTAEncHandle *phStatus, PFN_Malloc pfMalloc );

// There are three parameters in MSRTAFormatInfo
// that should be set before calling MSRTAEncoderInit.
MSRTARESULT  DMOSTDCALL MSRTAEncoderInit( MSRTAEncHandle  hStatus,
                                          MSRTAFormatInfo *ptFormat,
                                          PFN_Malloc pfMalloc  );

MSRTARESULT  DMOSTDCALL MSRTAEncoderDelete( MSRTAEncHandle *phStatus, PFN_Free pfFree );

// The *piInputSize is the input buffer size that the encoder requires for each frame
MSRTARESULT  DMOSTDCALL MSRTAEncoderGetInputSize( MSRTAEncHandle hStatus, U32 *piInputSize );

// The *piOutputSize is the maximum output bitstream buffer length for each frame
MSRTARESULT  DMOSTDCALL MSRTAEncoderGetOutputSize( MSRTAEncHandle hStatus, U32 *piOutputSize );

// The *piMinOutputSize is the minmum output bitstream buffer length for each frame
MSRTARESULT  DMOSTDCALL MSRTAEncoderGetMinOutputSize( MSRTADecHandle hStatus, I32 *piMinOutputSize );

// The time duration for each frame
MSRTARESULT  DMOSTDCALL MSRTAEncoderGetDuration( MSRTAEncHandle hStatus, REFERENCE_TIME *ptmDurationInTimeStamp );

MSRTARESULT  DMOSTDCALL MSRTACheckFormat( MSRTAFormatInfo *ptFormat );

MSRTARESULT  DMOSTDCALL MSRTASetFormat( MSRTAEncHandle  *phStatus,
                                        MSRTAFormatInfo *ptFormat,
                                        PFN_Malloc pfMalloc,
                                        PFN_Free pfFree );

MSRTARESULT  DMOSTDCALL MSRTAGetCurrentFormat( MSRTAEncHandle hStatus, MSRTAFormatInfo *ptFormat );

MSRTARESULT  DMOSTDCALL MSRTAGetFormatFromBitrate( MSRTAFormatInfo *ptFormat );

MSRTARESULT  DMOSTDCALL MSRTAGetNumberofModes( U32 *pnModes );

MSRTARESULT  DMOSTDCALL MSRTAGetBitrate( U32 *piBitrate, U32 *piSamplingrate);

MSRTARESULT  DMOSTDCALL MSRTAGetTimestampDelay(MSRTAEncHandle hStatus, REFERENCE_TIME *ptmTimestampDelay );

MSRTARESULT  DMOSTDCALL MSRTAGetEncoderModeTreeInfo(U32 nSamplesPerSec, U32 *pdwRTVoiceEncodeOpt,
                                                    U32 cbBufferSize);
// Encode one frame
MSRTARESULT  DMOSTDCALL MSRTAEncode( MSRTAEncHandle hStatus,
                                     U8  *pSourceBuffer,
                                     U32 cbSourceBufferSize,
                                     U8  *pDestBuffer,
                                     U32 cbDestBufferSize,
                                     U32 *cbDestBufferUsed,
                                     U32 *cbDestBufOptionLength,
                                     U8  *pFECBuffer,
                                     U32 cbFECBufferSize,
                                     U32 *cbFECBufferUsed,
                                     REFERENCE_TIME tmInputTimeStamp,
                                     REFERENCE_TIME *tmOutputTimeStamp,
                                     U8  *pbReconBuf,
                                     U32 cbReconBufLength,
                                     U32 *cbReconBufUsed,
                                     I32 iBandwidth,
                                     I32 iPacketLossMode,
                                     I32 *piProperty,
                                     U32 iPackingLevel,
                                     U32 iFrameOrderInPacket );

// Reset the encoder
MSRTARESULT  DMOSTDCALL MSRTAResetEncoder( MSRTADecHandle hStatus);

MSRTARESULT  DMOSTDCALL MSRTAEncoderStat( MSRTAEncHandle hStatus );

// ******* Decoder *******
MSRTARESULT  DMOSTDCALL MSRTADecoderNew( MSRTADecHandle *phStatus, PFN_Malloc pfMalloc );

// There are three parameters in MSRTAFormatInfo
// that should be set before calling MSRTADecoderInit.
MSRTARESULT  DMOSTDCALL MSRTADecoderInit( MSRTADecHandle hStatus,
                                          MSRTAFormatInfo *ptFormat,
                                          PFN_Malloc pfMalloc );

MSRTARESULT  DMOSTDCALL MSRTADecoderDelete( MSRTADecHandle *phStatus, PFN_Free pfFree );

MSRTARESULT  DMOSTDCALL MSRTADecoderGetInputSize( MSRTADecHandle hStatus, U32 *piInputSize );

MSRTARESULT  DMOSTDCALL MSRTADecoderGetOutputSize( MSRTADecHandle hStatus, U32 *piOutputSize );

MSRTARESULT  DMOSTDCALL MSRTADecoderGetDuration( MSRTADecHandle hStatus, REFERENCE_TIME *ptmDurationInTimeStamp );

MSRTARESULT  DMOSTDCALL MSRTAPutEncoderModeTreeInfo(MSRTADecHandle phStatus, U32 *pdwRTVoiceEncodeOpt,
                                                    U32 cbBufferSize, PFN_Malloc pfMalloc);

MSRTARESULT  DMOSTDCALL MSRTADecode( MSRTADecHandle hStatus,
                                     U8  *pSourceBuffer,
                                     U32 cbSourceBufferSize,
                                     U32 *pcbInputBufUsed,
                                     U8  *pDestBuffer,
                                     U32 cbDestBufferSize,
                                     U8  *pPrevDestBuffer,
                                     U32 cbPrevDestBufferSize,
                                     U32 *pcbPrevDestBufUsed,
                                     MSRTADecoderReset iResetType,
                                     I32 *piProperty );

MSRTARESULT  DMOSTDCALL MSRTAResetDecoder( MSRTADecHandle hStatus);

// Lighter version of MSRTAResetDecoder(), if output buffer pointer is not null
// generate output assuming zero input
MSRTARESULT  DMOSTDCALL MSRTALiteResetDecoder( MSRTADecHandle hStatus,
                                               U8  *pDestBuffer,
                                               U32 cbDestBufferSize );

// ******* De-Jitter Module *******
typedef MSRTARESULT  (* PFN_RTInitDecoder ) (
    MSRTADecHandle hStatus         // Codec state
);

typedef MSRTARESULT  (* PFN_RTDecode )(
    MSRTADecHandle hStatus,         // Codec state
    U8  *pbSourceBuffer,            // Source data buffer pointer
    U32 cbSourceBufferSize,         // The length of the source data buffer
    U32 *pcbSourceBufferUsed,       // output number of bytes used in the source buffer
    U8  *pDestBuffer,               // The destination decoded data buffer pointer
    U32 cbDestBufferSize            // the Size of the decoded data buffer
);

typedef MSRTARESULT  (* PFN_RTLiteResetDecoder ) (
    MSRTADecHandle hStatus,         // Codec state
    U8  *pDestBuffer,               // The destination decoded data buffer pointer
    U32 cbDestBufferSize            // the Size of the decoded data buffer
);

// Allocate memory for jitter buffer control module
MSRTARESULT  DMOSTDCALL MSRTADejitterNew( MSRTADejitHandle *phStatus, PFN_Malloc pfMalloc, PFN_Free pfFree );

// Initialize the jitter buffer control module
MSRTARESULT  DMOSTDCALL MSRTADejitterInit( MSRTADejitHandle hStatus,
                                           MSRTADecHandle   *phMSRTAStatus,
                                           U32 nSamplesPerSec,
                                           LONG JitterMode );

MSRTARESULT  DMOSTDCALL MSRTASendToJitterBuffer( MSRTADejitHandle  hStatus,
                                                 U32 dwDataType,       // input type
                                                 U32 nSamplingRate,    // of the encoder generating this payload
                                                 U8 *pbDataBuf,        // the input data buffer
                                                 U32 cbInputSize,      // buffer length
                                                 REFERENCE_TIME tmSendTimestamp,   // sending time
                                                 REFERENCE_TIME tmRecvTimetamp,    // receiving time
                                                 DWORD dwSeqNum,                   // sequence number
                                                 U32 fNewTalkSpurt );              // 1 if new talk spurt


MSRTARESULT  DMOSTDCALL MSRTADecodePullData( MSRTADejitHandle  hStatus,
                                             U8  *pbDestBuffer,        // output buffer pointer
                                             U32 cbDestBufferSize,     // output buffer size
                                             REFERENCE_TIME *ptmSendTimestamp,
                                             U32 *pnDecSamplingRate,
                                             I32 *piProperty,
                                             MSRTA_Healing_Info *pAHinfo );

// Release jitter buffer control module handle
MSRTARESULT  DMOSTDCALL MSRTADejitterDelete( MSRTADejitHandle  *phStatus );

MSRTARESULT  DMOSTDCALL MSRTADecoderLossRate( MSRTADejitHandle  hStatus,
                                              I32 *PacketLossMode,      // Loss mode
                                              I32 *piLossRate );        // Actual loss rate


MSRTARESULT  DMOSTDCALL MSRTADejitterGetStat( MSRTADejitHandle  hStatus,
                                              I32 *pStat );             // pointer to stat info

MSRTARESULT  DMOSTDCALL MSRTADejitterSetLimits( MSRTADejitHandle  hStatus,
                                                I32 iMinDelay,          // min delay requested
                                                I32 iMaxDelay );        // max delay requested

MSRTARESULT DMOSTDCALL MSRTARestartDejitterBuffer( MSRTADejitHandle hMSRTAStatus );

MSRTARESULT DMOSTDCALL MSRTARTScanBitStream( MSRTADecHandle hStatus,
                                             U32 nSampRate,
                                             U8 *pbBitstream, U32 cbBufSize,
                                             U32 *pcbBufUsed, U32 *pfMoreFrames );

#ifdef  __cplusplus
}
#endif

#endif // defined(__WMSPRT_H__)
