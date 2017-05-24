//    -----------------------------------------------------------------------------
//      Copyright (C) Siemens AG 1998  All Rights Reserved.
//    -----------------------------------------------------------------------------
//
//     Project: NUMARIS/4
//        File: \n4\pkg\MrServers\MrImaging\libSBB\SBBNavigator.h
//     Version: \main\4d13c\2
//      Author: OESINI5C
//        Date: 2012-03-14 17:10:37 +01:00
//
//        Lang: C++
//
//     Descrip: MR::Measurement::CSequence::libSeqUtil
//
//     Classes:
//
//    -----------------------------------------------------------------------------
//		Inherits SeqBuildBlockNavigator to add extra ui functionality
//	  -----------------------------------------------------------------------------

#ifndef SBBNAVIGATOR_SJ_H
#define SBBNAVIGATOR_SJ_H 1

#define __NAV_ICE_BUFFER_LENGTH 3  // The maximum number of ice results returned.

#include "MrServers/MrMeasSrv/SeqIF/libRT/libRT.h"
#include "MrServers/MrMeasSrv/SeqIF/libRT/sGRAD_PULSE.h"
#include "MrServers/MrMeasSrv/SeqIF/libRT/sFREQ_PHASE.h"
#include "MrServers/MrMeasSrv/SeqIF/libRT/sROT_MATRIX.h"
#include "MrServers/MrMeasSrv/SeqIF/libRT/sRF_PULSE.h"
#include "MrServers/MrMeasSrv/SeqIF/libRT/sREADOUT.h"
#include "MrServers/MrMeasSrv/SeqIF/libRT/sSYNC.h"

#if defined INCLUDE_SUPPORT_FOR_2DRF_PULSES
#include "MrServers/MrImaging/libSBB/SBB2DExcitation.h"
#include "MrServers/MrImaging/libSBB/SBB2DExcitationProfiles.h"
#include "MrServers/MrImaging/libSBB/SBB2DExcitationTrajectories.h"
#endif

#include "MrServers/MrImaging/libSBB/SeqBuildBlock.h"
#include "MrServers/MrMeasSrv/SeqIF/libRT/include/SEQSemaphore.h"
#include "MrServers/MrProtSrv/MrProt/SeqIF/SeqExpoRFBlockInfo.h"

#include "MrServers/MrImaging/seq/common/Nav/SBBNavigator.h"

/*
#ifdef LOCAL_SeqLoop
    #ifdef __IMP_EXP
    #undef __IMP_EXP
    #endif
    #define __IMP_EXP
    #pragma message( "Local SeqLoop build" )
#else  //  LOCAL_SeqLoop not defined
    #ifdef BUILD_libSBB
      #define __OWNER
    #endif

    #include "MrCommon/MrGlobalDefinitions/ImpExpCtrl.h" // import/export control

#endif

#define __SBBNAVIGATOR_FEEDBACK_BUFFER_LENGTH   (16)  // The maximum number of ice results returned.
#define SEQDebug_ShowFeedbackData (1)
*/

#include "MrServers/MrImaging/libSBB/SBBNavigatorMdhDef.h"

/*
// Defining the different actions following evaluation of feedback
enum eNavDecideCode { DoNothing=1,
                      BreakAndRepeatNavigator,
                      ShiftSlicePositionAndProceed,
                      RopeReorderAndProceed };
*/

/// \ingroup grp_libsbb
/// \brief  SBB that implements a navigator
/// \todo add documentation
class /*__IMP_EXP*/ SeqBuildBlockNavigator_sj : public SeqBuildBlockNavigator
{
public:

  SeqBuildBlockNavigator_sj (SBBList* /*pSBBList*/, char* pIdent);

  virtual ~SeqBuildBlockNavigator_sj();

/*
  bool   init     (SeqLim &rSeqLim);
  bool   prep     (MrProt &rMrProt, SeqLim &rSeqLim, SeqExpo &rSeqExpo);
  bool   run      (MrProt &rMrProt, SeqLim &rSeqLim, SeqExpo &rSeqExpo, sSLICE_POS* pSLC);
  bool   runIR    (MrProt &rMrProt, SeqLim &rSeqLim, SeqExpo &rSeqExpo, sSLICE_POS* pSLC);
*/
  //bool   decide   (long lPrePost, long lKernelMode, MrProt&  /*rMrProt*/, SeqLim &rSeqLim, SeqExpo& /*rSeqExpo*/, sSLICE_POS* /*pSLC*/);
/*
  eNavDecideCode getNavDecideCode();
  bool   runPause (unsigned long lPauseDuration_us, bool bUseWakeup);
  bool   waitForWakeup ();
  bool   semaphoreRelease (SEQSemaphore* pSemaphore);
  bool   semaphoreAcquire (SEQSemaphore* pSemaphore);
  bool   waitForFeedback  (SEQSemaphore* pSemaphore, long lPollInterval_us);

  void   setSEQDebugFlags(unsigned long ulValue);
  void   setGradSpoiling(bool getalBValue);
  void   setNavNumber(long lValue);
  void   setNavMode (long lValue);
  void   setNavPulseType (long lValue);
  void   setNavMatrix (long lValue);
  void   setNavFov (long lValue);
  void   setNavAcceptancePosition_mm (double dValue);
  void   setNavAcceptanceWidth_mm (double dValue);
  void   setNavSearchPosition_mm (double dValue);
  void   setNavSearchWidth_mm (double dValue);
  void   setNavCorrectionFactor (double dValue);
  void   setNavPrepScansFlag (bool getalBValue);
  void   setNavPrepTR_ms (long lValue);
  void   setNavPrepDuration_ms (long lValue);
  void   setNavSleepDuration_ms (long lValue);
  void   setNavPollInterval_ms (long lValue);
  void   setNavFBMode (long lValue);
  void   setNavIceResults(long lIndex, double dValue);
  void   setNavPhaseInc(double dValue);
  void   setNavUseIRSel(bool getalBValue);

  void   setNavFlagForLastRTScan();
  void   setNavFlagForLastHPScan();
  void   setNavFlagForNoFeedback();
  void   setNavPercentComplete(long lValue);
  void   setNavPercentAccepted(long lValue);

  void   setNavExecuteAfterKernel(bool getalBValue);
  void   setNavExecuteBeforeKernel(bool getalBValue);

  void   setNavFlagForBeforeEcho();
  void   setNavFlagForAfterEcho();

  void   setNavReferencePosition_mm(double dValue);
  void   setNavShiftAmount_mm(double dValue);
  void   setNewAcceptPosition (double dValue);

  double getNewAcceptPosition ();
  double getNavShiftAmountCorrected_mm();

  unsigned long getSEQDebugFlags();
  double getNavCurrIceResults(long lIndex);
  double getNavPrevIceResults(long lIndex);
  long   getNavCount();
  long   getNavCountTotal();
  long   getNavNumber();
  double getNavAcceptancePosition_mm();
  double getNavAcceptanceWidth_mm();
  double getNavSearchPosition_mm();
  double getNavSearchWidth_mm();
  double getNavCorrectionFactor();
  long   getNavPulseType();
  long   getNavMatrix();
  long   getNavMode();
  long   getNavFBMode();
  long   getNavPrepTR_ms();
  long   getNavPrepDuration_ms();
  long   getNavPercentComplete();
  long   getNavPercentAccepted();

  double getTimestampAfterNav_ms();
  double getTimeSinceLastNav_ms();
  double getMaxNavFeedbackTime_ms();
  double getMinNavFeedbackTime_ms();

  long   getNavSleepDuration_ms();
  long   getNavPollInterval_ms();

  long   getTotalDuration_us();
  long   getTotalDuration_ExcludingIR_us();
  MrProtocolData::SeqExpoRFInfo getTotalRFInfo();
  MrProtocolData::SeqExpoRFInfo getTotalRFInfo_ExcludingIR();

  bool   getNavExecuteAfterKernel();
  bool   getNavExecuteBeforeKernel();
  bool   getNavReferencePositionOK();
  double getNavReferencePosition_mm();

  bool   isNavPrepScan();
  void   setRelevantADC(bool getalBValue);
  MrProtocolData::SeqExpoRFInfo getSelIRRFInfoPerRequest();
  long   getSelIRDurationPerRequest();
*/
  long getNoOfNavs(); 					//JK2008
  void setNoOfNavs(long lValue); 		//JK2008
  long getTimeStartAcq(); 				//ib
  void setTimeStartAcq(long lValue); 	//ib
  long getTimeEndAcq(); 				//ib
  void setTimeEndAcq(long lValue); 		//ib
  long getTimeEndCardiac(); 			//ib
  void setTimeEndCardiac(long lValue); 	//ib
  long getNavTR_ms(); 					//ib
  void setNavTR_ms(long lValue); 		//ib
  long getCont1(); 						//ib
  void setCont1(long lValue); 			//ib
  long getScoutLength(); 				//ib
  void setScoutLength(long lValue); 	//ib
  long getSliceSelection(); 			//ib
  void setSliceSelection(long lValue); 	//ib

protected:

/*
  bool   m_bUseSpoiler;
  bool   m_bGradSpoiling;
  bool   m_bUseIRSel;
  bool   m_bWakeupDelivered;
  double m_dRFPhaseInc;
  int    m_iPatDirection;
  int    m_iPatPosition;

  eNavDecideCode m_eNavDecideCode;

  unsigned long m_ulSEQDebugFlags;
  long   m_lNumberOfNavigatorInstances;
  long   m_lNavNumber;
  long   m_lNavCount;
  long   m_lNavCountTotal;
  long   m_lNavSuccessCount;
  long   m_lNavAcceptCount;
  long   m_lNavPercentComplete;
  long   m_lNavPercentAccepted;
  double m_adNavCurrIceResults[__NAV_ICE_BUFFER_LENGTH];
  double m_adNavPrevIceResults[__NAV_ICE_BUFFER_LENGTH];

  long   m_lNavIndexExc;
  long   m_lNavIndexRef;

  long   m_lNavMode;
  long   m_lNavFBMode;
  long   m_lNavPulseType;

  long   m_lNavMatrix;
  long   m_lNavFov;
  double m_dNavAcceptancePosition_mm;
  double m_dNavAcceptanceWidth_mm;
  double m_dNavSearchPosition_mm;
  double m_dNavSearchWidth_mm;
  double m_dNavCorrectionFactor;

  bool   m_bRelevantADC;
  bool   m_bNavPrepScans;
  bool   m_bNavIsLastHPScan;
  bool   m_bNavIsLastRTScan;
  bool   m_bNavIsAfterEcho;
  bool   m_bNoFeedback;
  long   m_lNavPrepTR_ms;
  long   m_lNavPrepDuration_ms;
  long   m_lNavSleepDuration_ms;
  long   m_lNavPollInterval_ms;

  double m_dTimestampAfterNav_ms;
  double m_dMinNavFeedbackTime_ms;
  double m_dMaxNavFeedbackTime_ms;

  bool   m_bNavExecuteBeforeKernel;
  bool   m_bNavExecuteAfterKernel;

  double m_dNavReferencePosition_mm;
  double m_dNavShiftAmount_mm;
  bool   m_bNavReferencePositionOK;
  double m_adNewAcceptPosition;
*/

private:

/*
  sRF_PULSE_EXT m_RF090;
  sRF_PULSE_EXT m_RF180;

  sGRAD_PULSE m_GPSS090;
  sGRAD_PULSE m_GPSSR;
  sGRAD_PULSE m_GPSS180;
  sGRAD_PULSE m_GPRORW;
  sGRAD_PULSE m_GPRO;
  sGRAD_PULSE m_GPS01;
  sGRAD_PULSE m_GPS02;
  sGRAD_PULSE m_GPSp1;
  sGRAD_PULSE m_GPSp2;
  sGRAD_PULSE m_GPSp3;
  sGRAD_PULSE m_GPRFX;
  sGRAD_PULSE m_GPRFY;

  sROT_MATRIX m_RotMatrix090;
  sROT_MATRIX m_RotMatrix180;

  sFREQ_PHASE m_Txz090Set;
  sFREQ_PHASE m_Txz090Neg;
  sFREQ_PHASE m_Txz180Set;
  sFREQ_PHASE m_Txz180Neg;
  sFREQ_PHASE m_RxzSet;
  sFREQ_PHASE m_RxzNeg;

  sFREQ_PHASE m_TxNCOSet;
  sFREQ_PHASE m_TxNCONeg;
  sFREQ_PHASE m_RxNCOSet;
  sFREQ_PHASE m_RxNCONeg;

  sREADOUT m_ADC;

  sSYNC_WAKEUP m_WAKEUP;

#if defined INCLUDE_SUPPORT_FOR_2DRF_PULSES
  // Members for 2DRF excitation
  C2DRectProfile m_RectProfile;
  SeqBuildBlock2DTrajBlippedEPI m_Trajectory;
  SeqBuildBlock2DExcitation m_2DExcitation;
#endif

  // For Selective IR Pulse
  sRF_PULSE_EXT m_RFSelIR;
  sFREQ_PHASE   m_RFSelIRzSet;
  sFREQ_PHASE   m_RFSelIRzNeg;
  sGRAD_PULSE   m_GPSelIR1;
  sGRAD_PULSE   m_GPSelIR2;
  long   m_IRSelRFDuration;
  double m_dIRSelFlipAngle_deg;
  double m_dScaleSSThickness;
  MrProtocolData::SeqExpoRFInfo m_SelIRRFInfoPerRequest;
  long   m_SelIRDurationPerRequest;
*/

  long m_lNoOfNavs;				//ib
  long m_lTimeStartAcq;			//ib
  long m_lTimeEndAcq;			//ib
  long m_lTimeEndCardiac;		//ib
  long m_lNavTR_ms;				//ib	
  long m_lCont1;				//ib	
  long m_lScoutLength;			//ib	
  long m_lSliceSelection;		//ib

  SeqBuildBlockNavigator_sj(const SeqBuildBlockNavigator_sj &right);

  const SeqBuildBlockNavigator_sj &operator=(const SeqBuildBlockNavigator_sj &right);
};


#endif
