//	-----------------------------------------------------------------------------
//	  Copyright (C) Siemens AG 1998  All Rights Reserved.
//	-----------------------------------------------------------------------------
//
//	 Project: NUMARIS/4
//	    File: \n4\pkg\MrServers\MrImaging\libSBB\SBBNavigator.cpp
//	 Version:
//	  Author: OESINI5C
//	    Date: n.a.
//
//	    Lang: C++
//
//	 Descrip: MR::Measurement::CSequence::libSeqUtil
//
//	 Classes:
//
//	-----------------------------------------------------------------------------

/*
#define SIDELOBE_DISTANCE_mm     (200)
#define RESOLUTION_FREQUENCY_mm  (5)
#define RESOLUTION_PHASE_mm      (10)
#define MAG_HEARTBEATS           (20)
*/

#include "MrServers/MrImaging/seq/a_CV_nav_VE11/SBBNavigator_sj.h"

#include "MrServers/MrImaging/libSBB/libSBBmsg.h"
#include "MrServers/MrImaging/seq/SeqDebug.h"                   // for trace macros
#include "MrServers/MrImaging/ut/libSeqUT.h"

#include "MrServers/MrMeasSrv/MeasNuclei/Base/MeasNucleus.h"	// for MeasNucleus
#include "MrServers/MrMeasSrv/MeasPatient/MeasPatient.h"
#include "MrServers/MrMeasSrv/SeqIF/csequence.h"
#include "MrServers/MrMeasSrv/SeqIF/libRT/RTController.h"       // for getAbsTimeOfEventBlockMSec()
#include "MrServers/MrMeasSrv/SeqFW/libSSL/libSSL.h"

#include "MrCommon/MrNFramework/MrTrace/MPCUTrace/MPCUTrace.h"	// for TRACE_PUT macros

#define DEBUG_ORIGIN DEBUG_SBBKernel							// needed by mTRrun;

//  -----------------------------------------------------------------
//  Implementation of SeqBuildBlockNavigator member functions
//  -----------------------------------------------------------------
SeqBuildBlockNavigator_sj::SeqBuildBlockNavigator_sj (SBBList* pSBBList, char* pIdent) //sj
/*
: m_bUseSpoiler(false)
, m_bGradSpoiling(true)
, m_bUseIRSel(false)
, m_bWakeupDelivered(false)
, m_dRFPhaseInc(0.0)
, m_iPatDirection(0)
, m_iPatPosition(0)

, m_eNavDecideCode(DoNothing)

, m_ulSEQDebugFlags(0)
, m_lNumberOfNavigatorInstances(0)
, m_lNavNumber(-1)
, m_lNavCount(0)
, m_lNavCountTotal(0)
, m_lNavSuccessCount(0)
, m_lNavAcceptCount(0)
, m_lNavPercentComplete(0)
, m_lNavPercentAccepted(0)

, m_lNavIndexExc(-1)
, m_lNavIndexRef(-1)

, m_lNavMode(0)
, m_lNavFBMode(0)
, m_lNavPulseType(0)

, m_lNavMatrix(0)
, m_lNavFov(0)
, m_dNavAcceptancePosition_mm(0.0)
, m_dNavAcceptanceWidth_mm(0.0)
, m_dNavSearchPosition_mm(0.0)
, m_dNavSearchWidth_mm(0.0)
, m_dNavCorrectionFactor(0.0)

, m_bRelevantADC(false)
, m_bNavPrepScans(false)
, m_bNavIsLastHPScan(false)
, m_bNavIsLastRTScan(false)
, m_bNavIsAfterEcho(false)
, m_bNoFeedback(false)
, m_lNavPrepTR_ms(0)
, m_lNavPrepDuration_ms(0)
, m_lNavSleepDuration_ms(0)
, m_lNavPollInterval_ms(0)

, m_dTimestampAfterNav_ms(0)
, m_dMinNavFeedbackTime_ms(+1.0E6)
, m_dMaxNavFeedbackTime_ms(-1.0E6)

, m_bNavExecuteBeforeKernel(false)
, m_bNavExecuteAfterKernel(false)

, m_dNavReferencePosition_mm(0.0)
, m_dNavShiftAmount_mm(0.0)
, m_bNavReferencePositionOK(false)
, m_adNewAcceptPosition(0.0)

, m_RF090("RF090")
, m_RF180("RF180")

, m_GPSS090("SS090")
, m_GPSSR("SSR")
, m_GPSS180("SS180")
, m_GPRORW("RORW")
, m_GPRO("RO")
, m_GPS01("GPS01")
, m_GPS02("GPS02")
, m_GPSp1("Spoil1")
, m_GPSp2("Spoil2")
, m_GPSp3("Spoil3")
, m_GPRFX("RefX")
, m_GPRFY("RefY")

, m_RotMatrix090("RM090")
, m_RotMatrix180("RM180")

, m_Txz090Set("Set090")
, m_Txz090Neg("Neg090")
, m_Txz180Set("Set180")
, m_Txz180Neg("Neg180")
, m_RxzSet("RxSet")
, m_RxzNeg("RxNeg")

, m_TxNCOSet("TxSet")
, m_TxNCONeg("TxNeg")
, m_RxNCOSet("RxSet")
, m_RxNCONeg("RxNeg")

, m_ADC("NavADC")

, m_WAKEUP("WAKEUP")

#if defined INCLUDE_SUPPORT_FOR_2DRF_PULSES
, m_2DExcitation(NULL,pIdent)
, m_Trajectory(NULL)
#endif

// For Selective IR pulse
, m_RFSelIR("RFSelIR")
, m_RFSelIRzSet("RFSelIRzSet")
, m_RFSelIRzNeg("RFSelIRzNeg")
, m_GPSelIR1("GPSelIR1")
, m_GPSelIR2("GPSelIR2")
, m_IRSelRFDuration(10240)
, m_dIRSelFlipAngle_deg(240)
, m_dScaleSSThickness(2.0)
, m_dSelIREnergyPerRequest(0.0)
, m_SelIRDurationPerRequest(0)
*/
/*,*/: SeqBuildBlockNavigator(pSBBList, pIdent) //JK was seqbuildblock
{
	/*
    for (long lI=0; lI<__NAV_ICE_BUFFER_LENGTH; lI++)
    {
        m_adNavCurrIceResults[lI] = m_adNavPrevIceResults[lI] = -999.;
    }
    setIdent(pIdent);
    
#if defined INCLUDE_SUPPORT_FOR_2DRF_PULSES
    m_2DExcitation.setTrajectory(&m_Trajectory);
    m_2DExcitation.setProfile(&m_RectProfile);
#endif
	*/
}


SeqBuildBlockNavigator_sj::~SeqBuildBlockNavigator_sj()
{
}

//-----------------------------------------------------------------------------------

//JK2008
long SeqBuildBlockNavigator_sj::getNoOfNavs()
{
  return m_lNoOfNavs;
}

void SeqBuildBlockNavigator_sj::setNoOfNavs (long lValue)
{
  m_lNoOfNavs = lValue;
}

//JK2008
//ib-start
long SeqBuildBlockNavigator_sj::getNavTR_ms()
{
  return m_lNavTR;
}

void SeqBuildBlockNavigator_sj::setNavTR_ms (long lValue)
{
  m_lNavTR = lValue;
}

long SeqBuildBlockNavigator_sj::getCont1()
{
  return m_lCont1;
}

void SeqBuildBlockNavigator_sj::setCont1 (long lValue)
{
  m_lCont1 = lValue;
}

long SeqBuildBlockNavigator_sj::getTimeStartAcq()
{
  return m_lTimeStartAcq;
}

void SeqBuildBlockNavigator_sj::setTimeStartAcq(long lValue)
{
  m_lTimeStartAcq = lValue;
}

long SeqBuildBlockNavigator_sj::getTimeEndAcq()
{
  return m_lTimeEndAcq;
}

void SeqBuildBlockNavigator_sj::setTimeEndAcq(long lValue)
{
  m_lTimeEndAcq = lValue;
}

long SeqBuildBlockNavigator_sj::getTimeEndCardiac()
{
  return m_lTimeEndCardiac;
}

void SeqBuildBlockNavigator_sj::setTimeEndCardiac(long lValue)
{
  m_lTimeEndCardiac = lValue;
}

long SeqBuildBlockNavigator_sj::getScoutLength()
{
  return m_lScoutLength;
}

void SeqBuildBlockNavigator_sj::setScoutLength (long lValue)
{
  m_lScoutLength = lValue;
}

long SeqBuildBlockNavigator_sj::getSliceSelection();
{
	return m_lSliceSelection
}

void SeqBuildBlockNavigator_sj::setSliceSelection(long lValue);
{
	m_lSliceSelection = lValue;
}

//ib-end