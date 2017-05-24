//  -----------------------------------------------------------------------------
//    Copyright (C) Siemens AG 1998  All Rights Reserved.
//  -----------------------------------------------------------------------------
//
//   Project: NUMARIS/4
//      File: \n4\pkg\MrServers\MrImaging\seq\common\Nav\NavUI.h
//   Version: \main\10
//    Author: Clinical
//      Date: 2011-07-12 20:28:09 +02:00
//
//      Lang: C++
//
//   Descrip: MR::Measurement::CSequence::libSeqUtil
//
//   Classes:
//
//  -----------------------------------------------------------------------------

#ifndef __NavUI_H
#define __NavUI_H 1


// Constants which identify the special menu variables and their menu format.
// Enums must not begin with a value zero!
enum eSeqSpecialParLabels  { Label_Dummy=1,                      // dummy
                             Label_SelectionBox_NavMode,         // prospective, follow slice, rope, etc.
                             Label_SelectionBox_NavPulseType,    // 2dRF, crossed pair, etc.
                             Label_SelectionBox_NavFBMode,       // Either HP or RT Feedback
                             Label_SelectionBox_NavPosition,     // Pre, Pre&Post, or Post
                             Label_Long_NavMatrix,               // matrix size (base2)
                             Label_Long_NavFov,                  // FOV

							 Label_Long_NoOfNavs,				//JK2008 
							 Label_Long_TimeStartAcq,			//ib	
							 Label_Long_TimeEndAcq,				//ib	
							 Label_Long_TimeEndCardiac,			//ib	
							 Label_Long_NavTR_ms,	 			//ib
							 Label_Long_PoleSensitivity,		//ib
							 Label_Long_ScoutLength,			//ib
							 Label_Long_SliceSelection,			//ib

                             Label_CheckBox_PrepScan,            // Prep on/off
                             Label_Long_NavPrepTR_ms,      // TR for prep scans
                             Label_Long_NavPrepDuration_sec,     // Duration of prep scans
                             Label_Long_NavSleepDuration_ms,     // Time to wait after a nav scan
                             Label_Long_Arr0_RotatePlot,         // Array paramter 0
                             Label_Long_Arr1_SEQDebugFlags,      // Array paramter 1
                             Label_Long_Arr2_ICEDebugFlags,      // Array paramter 2
                             Label_Long_Arr3_ICESmoothLength,
                             Label_Long_Arr4_ICELSQFitLength,
                             Label_Long_Arr5_ICEImageSendInterval_Msec,
                             Label_Long_Arr6_ICETimeScaleInterval_Sec,
                             Label_Long_Arr7_FeedbackPollInterval_ms,
                             Label_Long_Arr8_NoOfPrepPulses,     // For tfiseg
                             Label_Long_Arr9_NoOfPrepBeats };    // For tfiseg

enum eSeqSpecialParLabelsD { Label_DummyD=1,                     // dummy
                             Label_Double_NavAcceptancePosition, // Position of acceptance window
                             Label_Double_NavAcceptanceWidth,    // Width of acceptance window
                             Label_Double_NavSearchPosition,     // Position of search window
                             Label_Double_NavSearchWidth,        // Width of search window
                             Label_Double_NavCorrectionFactor }; // Correction factor expressed as percentage

// Constants which identify the different selection box and check box states
// Enums must not begin with a value zero!
enum eSeqValueNavPulseType { Value_NavPulseType2DPencil=1,   // 2dRF
                             Value_NavPulseTypeCrossedPair,  // crossed pair
                             Value_NavPulseTypeCount__ };    // THIS MUST BE THE LAST ONE

enum eSeqValueNavMode      { Value_NavModeProspectiveGating=1, // prospective
                             Value_NavModeFollowSlice,         // follow slice
                             Value_NavModeMonitorOnly,         // monitor only
                             Value_NavModeOff,                 // OFF
                             Value_NavModeCount__ };           // THIS MUST BE THE LAST ONE

enum eSeqValueFBMode       { Value_NavFBMode_HP=1,           // HP Feedback
                             Value_NavFBMode_RT,             // RT Feedback
                             Value_NavFBModeCount__ };       // THIS MUST BE THE LAST ONE

enum eSeqValueNavPosition  { Value_NavBefEchoTrain=1,        // Before echo train
                             Value_NavAftEchoTrain,          // After echo train
                             Value_NavBefAndAftEchoTrain,    // Before and after echo train
                             Value_NavPositionCount__ };     // THIS MUST BE THE LAST ONE

enum eSeqValueCheckBox     { Value_CheckBox_On=1,            // check box enabled
                             Value_CheckBox_Off,             // check box disabled
                             Value_CheckBoxCount__ };        // THIS MUST BE THE LAST ONE


//  Debug masks for diagnostic output from specific parts of ice program
#define _ICEDEBUG_SIMULATE_FULL               (0x00000001) //1
#define _ICEDEBUG_SIMULATE_FB_ONLY            (0x00000002) //2
#define _ICEDEBUG_onlineFeedback              (0x00000004) //4
#define _ICEDEBUG_prepareForFeedback          (0x00000008) //8
#define _ICEDEBUG_deliverProfileImage         (0x00000010) //16

#define _ICEDEBUG_setProfileImageHeader       (0x00000040) //64
#define _ICEDEBUG_deliverGlobalImages         (0x00000080) //128
#define _ICEDEBUG_copyImageToGlobalObject     (0x00000100) //256
#define _ICEDEBUG_decorateProfileImage        (0x00000200) //512

#define _ICEDEBUG_createHistogramImage        (0x00000400) //1024
#define _ICEDEBUG_setHistogramImageHeader     (0x00000800) //2048

#define _ICEDEBUG_prepare                     (0x00004000) //16384
#define _ICEDEBUG_prepareHpFeedback           (0x00008000) //32768
#define _ICEDEBUG_prepareRtFeedback           (0x00010000) //65536

#define _ICEDEBUG_hpFeedback                  (0x00020000) //131072
#define _ICEDEBUG_rtFeedback                  (0x00040000) //262144
#define _ICEDEBUG_online                      (0x00080000) //524288

#define _ICEDEBUG_offline                     (0x00100000) //1048576


#if defined (_MSC_VER) 

#include "ProtBasic/Interfaces/SeqBuff.h"                      // SeqLim and SeqExpo

#include "MrServers/MrProtSrv/MrProtocol/libUILink/UILinkLimited.h"
#include "MrServers/MrProtSrv/MrProtocol/libUILink/UILinkSelection.h"
#include "MrServers/MrProtSrv/MrProtocol/libUILink/UILinkArray.h"
#include "MrServers/MrProtSrv/MrProtocol/libUILink/UILinkNumeric.h"
#include "MrServers/MrProtSrv/MrProtocol/UILink/StdProtRes/StdProtRes.h"
#include "MrServers/MrProtSrv/MrProtocol/UILink/MrStdNameTags.h"

#include "MrServers/MrMeasSrv/MeasUtils/nlsmac.h"
#include "MrServers/MrMeasSrv/SeqIF/Sequence/Sequence.h"
#include "MrServers/MrMeasSrv/SeqFW/libGSL/libGSL.h"

class MrProt;
class SeqLim;

//----------------------------------------
// define functions for "special" card
//----------------------------------------
class NavUI
{

public:
    NavUI ();

    virtual ~NavUI();


    NLS_STATUS init (SeqLim &rSeqLim);

    NLS_STATUS prep (MrProt &rMrProt, SeqLim &rSeqLim, SeqExpo &rSeqExpo);

    long getDefaultNavPulseType();

#ifdef SEQUENCE_CLASS
    MrUILinkArray::PFctErase          m_pOrigNavEraseHandler;
    MrUILinkArray::PFctInsert         m_pOrigNavInsertHandler;
    MrUILinkArray::PFctIsAvailable    m_pOrigNavIsAvailableHandler;
    LINK_VECTOR_TYPE::PFctSetValue    m_pFctNavPosVecSetVal_orig;
    LINK_SELECTION_TYPE::PFctSetValue m_pFctNavPosSelSetVal_orig;
    LINK_DOUBLE_TYPE::PFctSetValue    m_pFctNavPosTraSetVal_orig;
    LINK_DOUBLE_TYPE::PFctSetValue    m_pFctNavPosCorSetVal_orig;
    LINK_DOUBLE_TYPE::PFctSetValue    m_pFctNavPosSagSetVal_orig;

    long m_lOriginalNavMode;
#endif

protected:

private:

    // Default values for each special parameter.
    long   m_Default_SelectionBox_NavMode;
    long   m_Default_SelectionBox_NavPulseType;
    long   m_Default_SelectionBox_NavFBMode;
    long   m_Default_SelectionBox_NavPosition;
    long   m_Default_Long_NavMatrix;
    long   m_Default_Long_NavFov;

	long   m_Default_Long_NoOfNavs;				//JK2008
    long   m_Default_Long_TimeStartAcq;			//ib	
    long   m_Default_Long_TimeEndAcq;			//ib	
    long   m_Default_Long_TimeEndCardiac;		//ib	
	long   m_Default_Long_NavTR_ms;				//ib
	long   m_Default_Long_PoleSensitivity;		//ib
	long   m_Default_Long_ScoutLength;			//ib
	long   m_Default_Long_SliceSelection;		//ib 

    double m_Default_Double_NavAcceptancePosition;
    double m_Default_Double_NavAcceptanceWidth;
    double m_Default_Double_NavSearchPosition;
    double m_Default_Double_NavSearchWidth;
    double m_Default_Double_NavCorrectionFactor;
    long   m_Default_CheckBox_PrepScan;
    long   m_Default_Long_NavPrepTR_ms;
    long   m_Default_Long_NavPrepDuration_sec;
    long   m_Default_Long_NavSleepDuration_ms;
    long   m_Default_Label_Long_Arr0_RotatePlot;
    long   m_Default_Label_Long_Arr1_SEQDebugFlags;
    long   m_Default_Label_Long_Arr2_ICEDebugFlags;
    long   m_Default_Label_Long_Arr3_ICESmoothLength;
    long   m_Default_Label_Long_Arr4_ICELSQFitLength;
    long   m_Default_Label_Long_Arr5_ICEImageSendInterval_Msec;
    long   m_Default_Label_Long_Arr6_ICETimeScaleInterval_Sec;
    long   m_Default_Label_Long_Arr7_FeedbackPollInterval_ms;
    long   m_Default_Label_Long_Arr8_NoOfPrepPulses;
    long   m_Default_Label_Long_Arr9_NoOfPrepBeats;

};

inline long NavUI::getDefaultNavPulseType()
{
    return m_Default_SelectionBox_NavPulseType;
}

#endif  // if !defined VXWORKS && !defined BUILD_PLATFORM_LINUX
#endif  // ifndef NavUI_H
