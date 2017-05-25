//    -----------------------------------------------------------------------------
//      Copyright (C) Siemens AG 1998  All Rights Reserved.
//    -----------------------------------------------------------------------------
//
//     Project: NUMARIS/4
//        File: \n4_servers1\pkg\MrServers\MrImaging\seq\common\Nav\SeqLoopNav.cpp
//     Version: \main\4d13c\5
//      Author: Clinical
//        Date: 2013-09-16 02:00:56 +02:00
//
//        Lang: C++
//
//     Descrip: MR::Measurement::CSequence::SeqLoop
//
//     Classes:
//
//    -----------------------------------------------------------------------------
#include "MrServers/MrImaging/seq/a_CV_nav_VE11/SeqLoopNav_sj.h"
#include "MrServers/MrImaging/seq/a_CV_nav_VE11/NavigatorShell_sj.h"
#include "MrServers/MrImaging/seq/a_CV_nav_VE11/SBBNavigator_sj.h"
#include "MrServers/MrImaging/seq/a_CV_nav_VE11/NavUI_sj.h"

#include "MrServers/MrImaging/libSeqUtil/libSeqUtil.h"
#include "MrServers/MrImaging/libSBB/libSBBmsg.h"
#include "MrServers/MrImaging/ut/libsequt.h"
#include "MrServers/MrImaging/Ice/IceProgramNavigator/NavigatorFBData.h"

#include "MrServers/MrMeasSrv/SeqIF/csequence.h"
#include "MrServers/MrMeasSrv/CoilIF/SelectedCoilElements.h"
#include "MrServers/MrMeasSrv/SeqIF/libRT/SEQSemaphore.h"
#ifndef WIN32
    #include "MrServers/MrMeasSrv/PMU/pmuSequence.h"
    #include "MrServers/MrMeasSrv/PMU/pmumsg.h"
#endif

#include "ProtBasic/Interfaces/MrPreparationPulses.h"
#include "ProtBasic/Interfaces/MrNavigator.h"

#include "MrServers/MrProtSrv/MrProt/MrProtArray.h"
#include "MrServers/MrProtSrv/MrProt/Physiology/MrPhysiology.h"
//#include "MrServers/MrProtSrv/MrProt/MrProtArray.h"
//#include "MrServers/MrProtSrv/MrProt/MrProtTypedefs.h"

#ifndef SEQ_NAMESPACE
    #error SEQ_NAMESPACE not defined
#endif

using namespace SEQ_NAMESPACE;

#define DEBUG_ORIGIN  DEBUG_SEQLOOP

//  --------------------------------------------------
//  Implementation of method SeqLoopNav::SeqLoopNav
//  --------------------------------------------------
SeqLoopNav::SeqLoopNav()
  : m_sNav0(NULL,"Nav1")
  , m_sSemaphore(NULL)

#if defined DEBUG
  , m_bDebug(true)
  , m_bPerturbNavFeedback(true)
  , m_bTraceNavFeedback(true)
  , m_bTraceAlways(true)
#else
  , m_bDebug(false)
  , m_bPerturbNavFeedback(false)
  , m_bTraceNavFeedback(false)
  , m_bTraceAlways(false)
#endif

  , m_lTotalNavDuration_us(0)
  , m_lNavCount(0)

  , m_lNavPercentComplete(0)
  , m_lNavPercentAccepted(0)
  , m_lNavAcceptCount(0)

  , m_eNavDecideCode0(DoNothing)
  , m_eNavDecideCode1(DoNothing)

  , m_lMaxNavInstances(MAX_NAV_INSTANCES)
  , m_lNumberOfNavInstances(0)

  , m_lNumberOfRelevantReadoutsDelivered(0)

  , m_lNavKernelSuccessCount(0)

  , m_bNavRepeatedScan(false)
  , m_bTerminateNavLoop(false)

  , m_lKernelRequestsPerMeasurementPreparingScans(0)

  , m_bIsPSIR(false) //new to VE11

  , m_bValidNavShiftVector(false) //new to VE11
  , m_dNavShiftAmountCorrected(0.0) //new to VE11
//, m_sNavShiftVector
//, m_sNavLocSlice
  , m_lRelevantReadoutsForMeasTime(0) //new to VE11
  , m_lNavStatusBufferIndex(0) //new to VE11
  , m_dRecentEfficiency(0.0) //new to VE11
{
    if ( SeqUT.isUnitTestActive() )
    {
        m_bDebug = true;
        m_bPerturbNavFeedback = true;
        m_bTraceNavFeedback = true;
        m_bTraceAlways =true;
    }

  m_apNav[0] = &m_sNav0;    // Must not exceed MAX_NAV_INSTANCES
  for (long lI=0; lI<MAX_NAV_INSTANCES; lI++) m_alNavRepeatCount[lI] = 0;
  for (long lI=0; lI<NAV_STATUS_BUFFER_SIZE; lI++) m_bNavStatusBuffer_WasAccepted[lI] = false; //new
}

//  --------------------------------------------------
//  Implementation of method SeqLoopNav::~SeqLoopNav
//  --------------------------------------------------
SeqLoopNav::~SeqLoopNav()
{
}

//  --------------------------------------------------
//  Implementation of method SeqLoopNav::prep
//  --------------------------------------------------
bool SeqLoopNav::prep (MrProt &rMrProt, SeqLim &rSeqLim, SeqExpo &rSeqExpo)
{
    const char *ptModule = {"SeqLoopNav::prep"};

    //  ----------------------------------------------------------------------------
    // Provide a pointer to the navigator SBB to the SBBDB and SBBIRns SBB functions
    // -----------------------------------------------------------------------------
    SBBDB.setpNavigator(&m_sNav0);
    SBBIRns.setpNavigator(&m_sNav0);

    //  ---------------------------------------------------------------------------------
    //  Prepare the basic navigator timing.
    //  ---------------------------------------------------------------------------------
    m_sNav0.setGSWDGradientPerformance(rMrProt,rSeqLim);
    m_sNav0.setNavNumber(0);

    //  -----------------------------------------
    //  Special initializations for the unit test
    //  -----------------------------------------
    if ( IS_UNIT_TEST_ACTIVE(rSeqLim)) //new implementation
    {
        // Play out the navs but in monitor-only mode, because slice following is
        // too confusing for the unit test.
        m_sNav0.setNavMode              (Value_NavModeMonitorOnly);
        m_sNav0.setNavPrepScansFlag     (false);
    }
    else
    {
        m_sNav0.setNavMode              ( rMrProt.NavigatorParam().getalFree()[Label_SelectionBox_NavMode]);
        m_sNav0.setNavPrepScansFlag     ((rMrProt.NavigatorParam().getalFree()[Label_CheckBox_PrepScan]==Value_CheckBox_On) ? true : false);
    }

    //  -------------------------------------------------
    //  Initialize navigator class with the UI selections
    //  -------------------------------------------------
    m_sNav0.setSEQDebugFlags            ( rMrProt.NavigatorParam().getalFree()[Label_Long_Arr1_SEQDebugFlags]);
    m_sNav0.setNavPulseType             ( rMrProt.NavigatorParam().getalFree()[Label_SelectionBox_NavPulseType]);
    m_sNav0.setNavMatrix                ( rMrProt.NavigatorParam().getalFree()[Label_Long_NavMatrix]);
    m_sNav0.setNavFov                   ( rMrProt.NavigatorParam().getalFree()[Label_Long_NavFov]);

	// new UI elements from ib
	m_sNav0.setNoOfNavs					( rMrProt.NavigatorParam().getalFree()[Label_Long_NoOfNavs]);			//JK2008
	m_sNav0.setNavTR_ms					( rMrProt.NavigatorParam().getalFree()[Label_Long_NavTR_ms]);			//ib
	m_sNav0.setScoutLength				( rMrProt.NavigatorParam().getalFree()[Label_Long_ScoutLength]);		//ib
	m_sNav0.setTimeStartAcq				( rMrProt.NavigatorParam().getalFree()[Label_Long_TimeStartAcq]);		//ib
	m_sNav0.setTimeEndAcq				( rMrProt.NavigatorParam().getalFree()[Label_Long_TimeEndAcq]);			//ib
	m_sNav0.setTimeEndCardiac			( rMrProt.NavigatorParam().getalFree()[Label_Long_TimeEndCardiac]);		//ib
	m_sNav0.setSliceSelection			( rMrProt.NavigatorParam().getalFree()[Label_Long_SliceSelection]);		//ib

    m_sNav0.setNavAcceptancePosition_mm ((double) rMrProt.NavigatorParam().getadFree()[Label_Double_NavAcceptancePosition]);
    m_sNav0.setNavAcceptanceWidth_mm    ((double) rMrProt.NavigatorParam().getadFree()[Label_Double_NavAcceptanceWidth]);
    m_sNav0.setNavSearchPosition_mm     ((double) rMrProt.NavigatorParam().getadFree()[Label_Double_NavSearchPosition]);
    m_sNav0.setNavSearchWidth_mm        ((double) rMrProt.NavigatorParam().getadFree()[Label_Double_NavSearchWidth]);
    m_sNav0.setNavCorrectionFactor      ((double) rMrProt.NavigatorParam().getadFree()[Label_Double_NavCorrectionFactor]);
    m_sNav0.setNavPrepTR_ms             ( rMrProt.NavigatorParam().getalFree()[Label_Long_NavPrepTR_ms]);
    m_sNav0.setNavPrepDuration_ms       ( rMrProt.NavigatorParam().getalFree()[Label_Long_NavPrepDuration_sec]*1000);
    m_sNav0.setNavSleepDuration_ms      ( rMrProt.NavigatorParam().getalFree()[Label_Long_NavSleepDuration_ms]);
    m_sNav0.setNavPollInterval_ms       ( rMrProt.NavigatorParam().getalFree()[Label_Long_Arr7_FeedbackPollInterval_ms]);
    m_sNav0.setNavFBMode                ( rMrProt.NavigatorParam().getalFree()[Label_SelectionBox_NavFBMode]);
	//add new ui elements from ian

    if ((rMrProt.NavigatorParam().getalFree()[Label_SelectionBox_NavPosition] == Value_NavBefEchoTrain) ||
        (rMrProt.NavigatorParam().getalFree()[Label_SelectionBox_NavPosition] == Value_NavBefAndAftEchoTrain))
    {
        m_sNav0.setNavExecuteBeforeKernel(true);
    }
    else
    {
        m_sNav0.setNavExecuteBeforeKernel(false);
    }

    if ((rMrProt.NavigatorParam().getalFree()[Label_SelectionBox_NavPosition] == Value_NavAftEchoTrain) ||
        (rMrProt.NavigatorParam().getalFree()[Label_SelectionBox_NavPosition] == Value_NavBefAndAftEchoTrain))
    {
        m_sNav0.setNavExecuteAfterKernel(true);
    }
    else
    {
        m_sNav0.setNavExecuteAfterKernel(false);
    }

    //  ----------------------------------------------
    //  Allow the NavIRSel pulse in certain cases only
    //  ----------------------------------------------
    m_sNav0.setNavUseIRSel(false);
    if (m_sNav0.getNavPulseType() == Value_NavPulseTypeCrossedPair)
    {
        if ((rMrProt.getsPrepPulses().getucInversion()   == SEQ::VOLUME_SELECTIVE) ||
            (rMrProt.getsPrepPulses().getucSatRecovery() == SEQ::SATREC_VOLUME_SELECTIVE) ||
            (rMrProt.getsPrepPulses().getucDarkBlood()   == true))
        {
            m_sNav0.setNavUseIRSel(true);
        }
    }

    //  ----------------------------
    //  Prepare the navigator timing
    //  ----------------------------
    if (!m_sNav0.prep(rMrProt,rSeqLim,rSeqExpo))
    {
        Trace (rSeqLim, ptModule, "m_sNav0.prep() FAILED");
        setNLSStatus(m_sNav0.getNLSStatus());
        return false;
    }
    setNumberOfNavInstances(1);

    // -------------------------------------
    // Run the prep method of the base class
    // -------------------------------------
    if (!BASE_TYPE::prep(rMrProt, rSeqLim, rSeqExpo))
    {
        if ( !rSeqLim.isContextPrepForBinarySearch() ) //slight change in VE11
        {
            Trace (rSeqLim, ptModule, "BASE_TYPE::prep() FAILED");
        }
        return (false);
    }

    //  ----------------------------------------------------
    //  Calculation of the the navigator duration and energy
    //  ----------------------------------------------------
    m_lTotalNavDuration_us = 0;
    m_totalNavRFInfo.clear(); //new way of calculating energy

    if (m_sNav0.isPrepared())
    {
        long lMultiplier = 1;
        if (m_sNav0.getNavExecuteBeforeKernel() &&
            m_sNav0.getNavExecuteAfterKernel())
        {
            lMultiplier = 2;
        }

        // Note that if DB or IRns preparation is used, their individual durations and
        // energies include the re-inversion pulse that preceeds the navigator.
        m_lTotalNavDuration_us = lMultiplier * m_sNav0.getTotalDuration_ExcludingIR_us();
        m_totalNavRFInfo   = lMultiplier * m_sNav0.getTotalRFInfo_ExcludingIR();
    }
    //  Debug flag for tracing feedback from the ICE program
    m_bTraceNavFeedback = ((m_sNav0.getSEQDebugFlags() & SEQDebug_ShowFeedbackData) != 0);
#if defined DEBUG
    //m_bTraceNavFeedback = true;
#endif


    return true;
}

//  --------------------------------------------------
//  Implementation of method SeqLoopNav::run
//  --------------------------------------------------
bool SeqLoopNav::run (pSEQRunKernel pf,MrProt &rMrProt,SeqLim &rSeqLim,SeqExpo &rSeqExpo,sSLICE_POS* pSlcPos,sREADOUT* psADC) //VE11 uses refs rather than pointers (essentially the same thing)
{
    static const char *ptModule  = {"SeqLoopNav::run"};

    // Just call BASE_TYPE::run_new
    if (!BASE_TYPE::run_new(pf,rMrProt,rSeqLim,rSeqExpo,pSlcPos,psADC))
    {
        Trace (rSeqLim, ptModule, "BASE_TYPE::run_new() FAILED");
        return false;
    }
    return true;
}


//  ----------------------------------------------------
//  Implementation of method SeqLoopNav::runOuterSliceLoopKernel
//  ----------------------------------------------------
bool SeqLoopNav::runOuterSliceLoopKernel (pSEQRunKernel pf, MrProt &rMrProt, SeqLim &rSeqLim, SeqExpo &rSeqExpo, sSLICE_POS* pSlcPos, sREADOUT* psADC)
{
    static const char *ptModule  = {"SeqLoopNav::runOuterSliceLoopKernel"};
    NLS_STATUS lStatus           = SBB_NORMAL;

    TR_RUN(rSeqLim) //different function



        // --------------------------------------------------------------------------------------------
        // Additional loop for navigator sequences:
        // Initiate an infinite loop, in which the inner loops are controlled by navigator feedback.
        // This loop terminates only when appropriate results have been received by the ICE program.
        // --------------------------------------------------------------------------------------------
        m_bNavRepeatedScan = false;
        m_bTerminateNavLoop = false;
        while (! m_bTerminateNavLoop ) //very different in VE11
        {

            if(!INHERITED_SEQLOOP_BASE_TYPE_NAV::runOuterSliceLoopKernel (pf, rMrProt, rSeqLim, rSeqExpo, pSlcPos, psADC))
        {
            if ( ! rSeqLim.isContextPrepForBinarySearch() )
            {
                TRACE_PUT1(TC_ALWAYS, TF_SEQ,"%s: Error encountered in SEQ_NAMESPACE::INHERITED_SEQLOOP_BASE_TYPE_NAV::runOuterSliceLoopKernel(...).", ptModule );
            }
            return ( false );
        }
        //  ------------------------------------------------------------------------------------
        //  Act on the decision from the first navigator (which occurred before the phases loop)
        //  ------------------------------------------------------------------------------------

    // we do not want to repeat the heartbeat in the second set for PSIR, the desired
    // behavior is that the navigator result for set 0 is also taken for set 1. (since this is used for PSIR single shot)
    // as a first try, lets simply ignore the resuts of the naviagtor scan here to make sure that only the first heartbeat makes the decision
#if defined __A_TRUFI_CV_NAV
//    bool bIsPSIRReconMode = (pMrProt->reconstructionMode() == SEQ::RECONMODE_PSIR);
    long lTriggerPulses   =  rMrProt.physiology().EKG().getlTriggerIntervals();
    if ( (m_lTriggerCount != (lTriggerPulses-1)) || (m_bIsPSIR == false)  ) // for non PSIR recon we are stepping in here - for PSIR recon we are only stepping in for the IR prepped image
    {
#endif
        // for non-PSIR recon we are stepping in here - for PSIR recon we are only stepping in for the IR prepped image
        m_bNavRepeatedScan = false;
        switch (m_eNavDecideCode0)
        {
        default:
        case DoNothing:
        case ShiftSlicePositionAndProceed:
        case RopeReorderAndProceed:
            break;

        case BreakAndRepeatNavigator:

            incrementNavRepeatCount(0);
            incrementNavAcceptCount(0);  // RK 2002.3.1: 0==recalculate percentage only

            if (m_bTraceNavFeedback)
            {
                TRACE_PUT1(TC_INFO,TF_SEQ,"%s: NavBefore: Repeating current iteration",ptModule);
                TRACE_PUT1(TC_INFO,TF_SEQ,"%s: --------------------------------------",ptModule);
            }

            m_bNavRepeatedScan = true;
            continue;  // Repeat the current iteration of the while loop.
            break;
        }

        //  ------------------------------------------------------------------------------------
        //  Act on the decision from the second navigator (which occurred after the phases loop)
        //  ------------------------------------------------------------------------------------
        switch (m_eNavDecideCode1)
        {
        default:
        case DoNothing:
        case ShiftSlicePositionAndProceed:
        case RopeReorderAndProceed:
            break;

        case BreakAndRepeatNavigator:

            incrementNavRepeatCount(0);
            incrementNavAcceptCount(0);  // RK 2002.3.1: 0==recalculate percentage only

            if (m_bTraceNavFeedback)
            {
                TRACE_PUT1(TC_INFO,TF_SEQ,"%s: NavAfter: Repeating current iteration",ptModule);
                TRACE_PUT1(TC_INFO,TF_SEQ,"%s: -------------------------------------",ptModule);
            }

            m_bNavRepeatedScan = true;
            continue;  // Repeat the current iteration of the while loop.
            break;
        }

        //  If this point is reached, the navigator loop will stop, and we proceed through k-space
        //  Otherwise, the "continue" statements above cause the current k-space samples to be repeated.
        m_bTerminateNavLoop = true;

        if (m_bTraceNavFeedback)
        {
            TRACE_PUT1(TC_INFO,TF_SEQ,"%s: OK-proceeding with next loop",ptModule);
            TRACE_PUT1(TC_INFO,TF_SEQ,"%s: ----------------------------",ptModule);
        }

        //  Some counters, for statistics
        incrementNavAcceptCount(1);  // RK 2002.3.1: 1==increment accept count and recalculate percentage

#if defined __A_TRUFI_CV_NAV
    } // end if trigger counter
    else
    {
        incrementNavRepeatCount(0);
        incrementNavAcceptCount(1);
        break;
    }
#endif

        } // while (! m_bTerminateNavLoop); end of navigator while loop

        m_eNavDecideCode0 = DoNothing;  // Reset navigator control codes
        m_eNavDecideCode1 = DoNothing;


    TRACE_PUT2(TC_ALWAYS, TF_SEQ,"m_lNavCount %d; m_dRecentEfficiency: %f;",m_lNavCount, m_dRecentEfficiency);


    TR_END(rSeqLim) //different function

    setNLSStatus (lStatus);

    return ( true );

}

//  -------------------------------------------------------
//  Implementation of method SeqLoopNav::runKernelCallsLoop
//  -------------------------------------------------------
bool SeqLoopNav::runKernelCallsLoop (pSEQRunKernel pf, MrProt &rMrProt, SeqLim &rSeqLim, SeqExpo &rSeqExpo, sSLICE_POS* pSlcPos, sREADOUT* psADC)
{
    static const char *ptModule  = {"SeqLoopNav::runKernelCallsLoop"};
    NLS_STATUS lStatus           = SBB_NORMAL;

    TR_RUN(rSeqLim)

    //TRACE_PUT1(TC_ALWAYS, TF_SEQ,"starting module %s", ptModule );

    // * -------------------------------------------------------------------------- *
    // * T2Prep                                                                     *
    // * -------------------------------------------------------------------------- *
#if defined __A_TRUFI_CV_NAV

        if ( performStandardECG(rMrProt, rSeqLim) )
        {
            // If the scan has SPAIR fat nulling, play SPAIR here and play the T2prep in LocalSeqLoop_CV, not here.
            // Note that in this case the T2prep is played AFTER the navigator block.
            if ( m_SBBOptfsPriorRO.isPrepared() ) 
            {
				/*
				#ifndef WIN32																				//ib
					m_myPMU.getStatisticData(myStatistics);														//ib
					m_MyControlIB.CalculateTdead(myStatistics->sECGOnlineStat.shPreviousPeriod);				//ib
					cout<<"Previous RR interval is "<<myStatistics->sECGOnlineStat.shPreviousPeriod<<endl;		//ib
				#endif
				*/

#ifdef SHOW_LOOP_STRUCTURE 
                if ( IS_TR_LAND(rSeqLim)  && m_SBBOptfsPriorRO.isPrepared() )   {   ShowEvent ("m_SBBOptfsPriorRO");   }
#endif
                if( !m_SBBOptfsPriorRO.run (rMrProt, rSeqLim, rSeqExpo,&(pSlcPos[m_lSliceIndex]) ) )
                {
                    setNLSStatus(m_SBBOptfsPriorRO.getNLSStatus());
                    TRACE_PUT2(TC_ALWAYS,TF_SEQ,"Error at %s(%d).",__FILE__,__LINE__);
                    return (false);
                }
            }
        #ifndef NAV_BEFORE_T2PREP
            else
            {
                if (m_bT2PrepEnabled && m_bExecuteT2Prep)
                {

#ifdef SHOW_LOOP_STRUCTURE
                    if ( IS_TR_LAND(rSeqLim)  && m_SBBT2Prep.isPrepared() )   {   ShowEvent ("SBBT2Prep");   }
#endif
                    TRACE_PUT1(TC_ALWAYS, TF_SEQ,"%s: Calling m_SBBT2Prep.run", ptModule );
                    if( !m_SBBT2Prep.run(rMrProt, rSeqLim, rSeqExpo, pSlcPos)) 
                    {
                        setNLSStatus(m_SBBT2Prep.getNLSStatus());
                        TRACE_PUT2(TC_ALWAYS,TF_SEQ,"Error at %s(%d).",__FILE__,__LINE__);
                        return (false);
                    }
                }
            }
        #endif
        }

#endif
    // ---------------------------------------------------------------
    // Prepare the navigator that is played out before the echo train.
    // ---------------------------------------------------------------
    // In any navigator measurement, there are ADCs for the navigator and other ADCs for the imaging sequence.
    // In order to keep track of measurement time in the SCT, all navigator ADCs before the echo train are marked as relevant ADCs.
    // Further, all of the ADCs of the imaging sequence are marked as NOT relevant, but this setting is performed only in the NavigatorShell
    // or the imaging sequece code itself.
    if (m_sNav0.getNavExecuteBeforeKernel()) m_sNav0.setRelevantADC(true);

#ifdef OLD_METHOD_FOR_RELEVANT_READOUTS
    // This section is NOT used.
    // Disable relevant ADC flags for repeated scans, so that the countdown
    // timer is displayed correctly
    if (m_bNavRepeatedScan)
    {
        m_sNav0.setRelevantADC(false);
        //TRACE_PUT1(TC_ALWAYS, TF_SEQ,"%s: REPEATED SCAN, REL ADC = false", ptModule );
    }
    else
    {
        updateNavPercentComplete(1,0);  // increment the number of delivered relevant readouts
    }
#else
    // If this is a repeated scan, increment the number of relevant ADCs and inform SCT
    // The strategy here is as follows:  We start the navigator measurement assuming 100% efficiency and no repeated scans.  This assumption
    // is the same as the standard (non-navigator) imaging case with N expected relevant ADCs.  The number of actually played-out relevant
    // ADCs is the counter i, and i runs from 0 to N.  However in a real navigator meausrement, there will be repeated scans due to movement
    // and detection of motion outside of the acceptance window.  Whenever we play out a repeated scan, we simply inform the SCT control task
    // that there is now one more relevant ADC than what was originally predicted.  I.e. we replace N with N+1 and N+1 then becomes the new N.
    // Meanwhile, the counter i continues to increment once per navigator.
    if (m_bNavRepeatedScan)
    {
        // Increment the number of delivered and the number of expected relevant readouts.  This has the effect of "freezing" the
        // countdown timer.
        updateNavPercentComplete(1,1);
        TRACE_PUT2(TC_ALWAYS, TF_SEQ,"%s: REPEATED SCAN, number of relevant ADCs incremented to value %d", ptModule, m_lRelevantReadoutsForMeasTime);
    }
    else
    {
        // Increment only the number of delivered relevant readouts. This is the normal case and causes the countdown timer to 
        // compute and display the decremented measurement time.
        updateNavPercentComplete(1,0);
    }
#endif

    // ---------------------------------------------------------------------------------
    // Play out the navigator before the echo train, including possibile slice-following.
    // Return with m_eNavDecideCode0 set appropriately.
    // ---------------------------------------------------------------------------------
    if (!runNavBefore(rMrProt, rSeqLim, rSeqExpo, pSlcPos))
    {
        Trace (rSeqLim, ptModule, "SeqLoopNav::runNavBefore() FAILED");
        return false;
    }

    // The ADCs of the imaging sequence are turned off when the navigator result is not favorable
    // This is only possible when a navigator is played out before the echo train
    const bool bWasReadoutEnabled = fRTIsReadoutEnabled();
    bool bReadoutsWereDisabled = false;
	if (m_sNav0.getNavExecuteBeforeKernel() && bWasReadoutEnabled && (m_eNavDecideCode0 == BreakAndRepeatNavigator))
    {
        bReadoutsWereDisabled = true;
		fRTSetReadoutEnable(0);
    }

    // -----------------------------------------------------------------------------------------------------------
    // Disable the playout of T2Prep in the base class, because we have played it out here before the navigator,
    // but only if the scan does not have SPAIR fat nulling dictating the order SPAIR-NAV-T2prep
    // and if Nav isn't always played before T2prep indicated by define flag:
    // -----------------------------------------------------------------------------------------------------------
#if defined __A_TRUFI_CV_NAV
    #ifndef NAV_BEFORE_T2PREP
    if ( !m_SBBOptfsPriorRO.isPrepared() )
        BASE_TYPE::setExecuteT2Prep(false);
    #endif
#endif

    // ------------------------------------------
    // Invoke runKernelCallsLoop of the base type
    // ------------------------------------------
    if (!BASE_TYPE::runKernelCallsLoop (pf, rMrProt, rSeqLim, rSeqExpo, pSlcPos, psADC))
    {
        Trace (rSeqLim, ptModule, "BASE_TYPE::runKernelCallsLoop() FAILED");
        return false;
    }

#if defined __A_TRUFI_CV_NAV
    BASE_TYPE::setExecuteT2Prep(m_bT2PrepEnabled);
#endif

	// The ADCs of the imaging sequence are re-enabled if they were previously disabled
	if (bReadoutsWereDisabled) fRTSetReadoutEnable(1);

    //  ---------------------------------------
    //  Run the navigator after the echo-train
    //  ---------------------------------------

    // All navigators after the echo train are relevant ADCs, but only if NO navigator is played out before the echo train.
    // This ensures only one relevant ADC per echo train.
    if (!m_sNav0.getNavExecuteBeforeKernel() && m_sNav0.getNavExecuteAfterKernel()) m_sNav0.setRelevantADC(true);
    else                                                                            m_sNav0.setRelevantADC(false);

    // Play out the navigator after the echo train
    if (!runNavAfter(rMrProt, rSeqLim, rSeqExpo, pSlcPos))
    {
        Trace (rSeqLim, ptModule, "SeqLoopNav::runNavAfter() FAILED");
        return false;
    }

    // Increment the number of times this loop is played out
    incrementNavCount();

    TR_END(rSeqLim)

    setNLSStatus (lStatus);

    return ( true );

}


//  -------------------------------------------------
//  Implementation of method SeqLoopNav::runNavBefore
//  -------------------------------------------------
//  --------------------------------------------------------
//  Method for executing the navigator before the echo train
//  --------------------------------------------------------
bool SeqLoopNav::runNavBefore (MrProt &rMrProt, SeqLim &rSeqLim, SeqExpo &rSeqExpo, sSLICE_POS* pSlcPos)
{
    static const char *ptModule  = {"SeqLoopNav::runNavBefore"};
	long lNavNumberEnd;		//sj

    if (!m_sNav0.isPrepared()) return true;
    if (!m_sNav0.getNavExecuteBeforeKernel()) return true;
    if (m_lPhaseCounter > 0) return true; //|| (m_lInnerSliceCounter > 0)) return true;

    //  Diagnostics
    if (m_bTraceNavFeedback)
    {
        // Percentage complete and percentage accepted
        TRACE_PUT3(TC_INFO,TF_SEQ,"%s: IMA %3d%%; ACCEPT %3d%%",ptModule,
            m_lNavPercentComplete,m_lNavPercentAccepted);
        TRACE_PUT1(TC_INFO,TF_SEQ,"%s: ---------------------",ptModule);
    }

    // --------------------------------------------------------------------------------------------
    // Navigator 0, before the kernel loop
    // --------------------------------------------------------------------------------------------
    //  Information for the ICE program
    m_sNav0.setNavFlagForBeforeEcho();

    //  Send info to the navigator SBB, so it can be transmitted to ICE for online display
    m_sNav0.setNavPercentComplete(getNavPercentComplete());
    m_sNav0.setNavPercentAccepted(getNavPercentAccepted());

	//ib -start ----------------------------------------------------------------------
    //		- this is for the scout
    //			set the number of navs to be run = 1024, (which is 2^10, fot the FFT)	
    //			hopefully this will repeat the nav 1024 times and i can collect the data for the model calculations
    //	------------------------------------------------------------------------------
    
	/*
    if (m_MyControlIB.getScoutOn_ib())
	{
		lNavNumberEnd = 2;		//=getsensitivity;
	}
    else
    {
		lNavNumberEnd = m_MyControlIB.getNoOfNavs();
    }
	*/
	lNavNumberEnd = m_sNav0.getNoOfNavs();
	//ib -end -----------------------------------------------------------------------------


	//cout << " Number of Navs is " << m_sNav0.getNoOfNavs( )<< endl;			//JK2008: This is correct
	//cout << " Nav Count " << m_sNav0.getNavCount() << endl;					//JK2008: This is correct
	for(int iNavNumber = 0; iNavNumber < lNavNumberEnd; iNavNumber++)			//JK2008
	{																			//JK2008
		/*
		//cout << "====in navlooper=== and nav =" << iNavNumber << endl;
 		if(iNavNumber == 0 && m_MyControlIB.getScoutOn_ib() == false)			//ib
 	    {	
	 	    #ifdef DEBUG
	 			m_MyControlIB.CalculateTdead(1080)	;							//ib
	 			cout << "====calcdeadtime=== and nav =" << iNavNumber << endl;
	 	    #endif
 	        m_sNav0.runPause(m_MyControlIB.getTwait(), false);				//ib - add pause to wait for next period to start for CS
		    //cout << "extratime===" << m_MyControlIB.getExtraTime() << endl;
 		}																		//ib
		*/

		//  Reset the semaphore for fSEQReceive
		if (!m_sNav0.semaphoreRelease(&m_sSemaphore))
		{
			Trace (rSeqLim, ptModule, "m_sNav0.semaphoreRelease() FAILED");
			setNLSStatus(m_sNav0.getNLSStatus());
			return false;
		}

		//  Play out the navigator
		if (!m_sNav0.run(rMrProt,rSeqLim,rSeqExpo,pSlcPos))
		{
			Trace (rSeqLim, ptModule, "m_sNav0.run() FAILED");
			setNLSStatus(m_sNav0.getNLSStatus());
			return false;
		}

		// Set a flag to indicate that we are ready to receive the next feedback.
		if (!m_sNav0.semaphoreAcquire(&m_sSemaphore))
		{
			Trace (rSeqLim, ptModule, "m_sNav0.semaphoreAcquire() FAILED");
			setNLSStatus(m_sNav0.getNLSStatus());
			return false;
		}

		//  Before checking for feedback, add some delay time to allow for data transfer, processing, etc.
		if (!m_sNav0.runPause(1000*m_sNav0.getNavSleepDuration_ms(),true))
		{
			Trace (rSeqLim, ptModule, "m_sNav0.runPause() FAILED");
			setNLSStatus(m_sNav0.getNLSStatus());
			return false;
		}

		//  Wait until the "WAKEUP" code is seen by the DSPs
		if (!m_sNav0.waitForWakeup())
		{
			Trace (rSeqLim, ptModule, "m_sNav0.waitForWakeup() FAILED");
			setNLSStatus(m_sNav0.getNLSStatus());
			return false;
		}

		#ifdef WIN32
		{
			//  When the sequence is run in simulation mode, execute fSEQReceive manually, so that the function
			//  can be tested.
			SEQData rSEQData;
			NavigatorFBData sNavigatorFBData;

			float fIceResult_1 = 128.;
			float fIceResult_2 = 125.;
			if ((m_sNav0.getNavCount()>2)&&(m_sNav0.getNavCount()%5==0)) fIceResult_1 = 32.;

			// if ((m_sNav0.getNavCount()>2)&&(m_sNav0.getNavCount()%5==0)) fIceResult_1 = 32.;	//ibnbnbnbnbn change back
	        
	        //#ifdef DEBUG
	        //fIceResult_1 = m_MyControlIB.XSampleVal_ib();	//sj
	        //cout<<fIceResult_1 <<endl;
	        //#endif

			// Prepare the feedback buffer
			sNavigatorFBData.lNumber = m_sNav0.getNavNumber();
			sNavigatorFBData.lCount  = m_sNav0.getNavCount();
			sNavigatorFBData.fResult = fIceResult_1;
			sNavigatorFBData.fMAGAcceptPos = fIceResult_2;

			rSEQData.setID("NAV");

			// "Deliver" the feedback
			if( !rSEQData.setData (&sNavigatorFBData, sizeof(sNavigatorFBData)) )
			{
				Trace (rSeqLim, ptModule, "rSEQData.setData() FAILED");
				setNLSStatus( SEQU_ERROR );
				return false;
			}

			// "Receive" the feedback
			NLS_STATUS lStatus = receive(rSeqLim,rSeqExpo,rSEQData);
			if ( (NLS_SEVERITY(lStatus)!=NLS_SUCCESS) )
			{
				Trace (rSeqLim, ptModule, "receive() FAILED");
				setNLSStatus( lStatus );
				return false;
			}
		}
		#endif

		//  ---------------------------------------------------------------------------------------------
		//  Unless this is a preparing scan, wait for the feedback from the ICE program (see fSEQReceive)
		//  ---------------------------------------------------------------------------------------------
		if( m_lKernelMode == KERNEL_PREPARE )
		{
			if (m_bTraceNavFeedback)
			{
				TRACE_PUT1(TC_INFO,TF_SEQ,"%s: Prepare scan - no feedback",ptModule);
			}
		}
		else
		{
			if (!m_sNav0.waitForFeedback(&m_sSemaphore,m_sNav0.getNavPollInterval_ms()*1000))
			{
				Trace (rSeqLim, ptModule, "m_sNav0.waitForFeedback() FAILED");
				setNLSStatus(m_sNav0.getNLSStatus());
				return false;
			}
		}

		// ------------------------------------------------------------------------
		//  Decide here what to do with the navigator feedback -> m_eNavDecideCode0
		// ------------------------------------------------------------------------
		if (!m_sNav0.decide(0,m_lKernelMode,rMrProt,rSeqLim,rSeqExpo,pSlcPos))
		{
			Trace (rSeqLim, ptModule, "m_sNav0.decide() FAILED");
			setNLSStatus(SEQU_ERROR);
			return false;
		}
		m_eNavDecideCode0 = m_sNav0.getNavDecideCode();

		if(iNavNumber < (m_sNav0.getNoOfNavs()-1)) m_eNavDecideCode0 = DoNothing;	//JK2008: get nav decide code and act on it for last nav ONLY

		setValidNavShiftVector(false);
		m_dNavShiftAmountCorrected = 0.0;

		// ycc CHARM 00382011
		// No repeat scan for this mode during unit test - fix energy and scan time estimation error
		if ( (rSeqLim.getSeqDebugMode().getDebugMask() & UNITTEST_ON) && (m_sNav0.getNavMode() == Value_NavModeMonitorOnly) )
		{
			m_eNavDecideCode0 = DoNothing;
		}

		//  ----------------------------------------------------
		//  Perform the slice-shift required for slice-following
		//  ----------------------------------------------------
		if (m_eNavDecideCode0 == ShiftSlicePositionAndProceed)
		{
			// -----------------------------------------------------------------------------
			// Calculate the shift vector, which is used later to modify the slice positions
			// -----------------------------------------------------------------------------
			double dZ = (double) m_sNav0.getNavShiftAmountCorrected_mm();

			if (m_bTraceNavFeedback)
			{
				TRACE_PUT3(TC_INFO,TF_SEQ,"%s: Shift amount  %8.3f (REFPOS: %8.3f)",
					ptModule,dZ,m_sNav0.getNavReferencePosition_mm());
			}

			switch (m_sNav0.getNavPulseType())
			{
				case Value_NavPulseType2DPencil:
					m_sNavShiftVector = rMrProt.navigatorArray()[0].normal();
					break;

				case Value_NavPulseTypeCrossedPair:
					NavigatorArray& rNavArray = rMrProt.navigatorArray();
					Navigator       sNav180   = rNavArray[1];
					sNav180.rotationAngle(0);
					VectorPat<double> e_phase, e_read;
					if( !sNav180.orientation(e_phase,e_read) )
					{
						Trace (rSeqLim, ptModule, "xxyy() FAILED");
						setNLSStatus(SEQU_ERROR);
						return false;
					}
					m_sNavShiftVector = e_read;
					break;
			}
			m_dNavShiftAmountCorrected = dZ;
			m_sNavShiftVector *= dZ;
			setValidNavShiftVector(true);

			#if defined __A_TRUFI_CV_NAV
				// For CV_nav, the shift is applied in the SBBTrufiCVKernel instead, and so the following part is skipped.
			#else
				//  ---------------------------------------------
				//  Modify all slice positions in the inner loops
				//  ---------------------------------------------
				SliceSeries& rSliceSeries = rMrProt.sliceSeries();
				long lInnerSliceCounter = m_lInnerSliceCounter;
				for (m_lInnerSliceCounter = m_lInnerSliceNumber-1; m_lInnerSliceCounter >=0; --m_lInnerSliceCounter )
				{
					if (!mapLoopCounterToSliceIndex(rMrProt) )
					{
						Trace (rSeqLim, ptModule, "mapLoopCounterToSliceIndex() FAILED");
						return false;
					}

					const Slice& rProtSlice = rSliceSeries.chronological(m_lSliceIndex);
					m_sNavLocSlice.copyFrom(rProtSlice);
					VectorPat<double> sPosVect = m_sNavLocSlice.position();

					if (m_bTraceNavFeedback)
					{
						TRACE_PUT4(TC_INFO,TF_SEQ,"%s: OLD sPosVect [%8.3f;%8.3f;%8.3f]\n",ptModule,sPosVect.dSag,sPosVect.dCor,sPosVect.dTra);
					}

					sPosVect += m_sNavShiftVector;  // Calculation to shift the slice position.

					if (m_bTraceNavFeedback)
					{
						TRACE_PUT4(TC_INFO,TF_SEQ,"%s: NEW sPosVect [%8.3f;%8.3f;%8.3f]\n",ptModule,sPosVect.dSag,sPosVect.dCor,sPosVect.dTra);
					}

					m_sNavLocSlice.position(sPosVect);
					if( !pSlcPos[m_lSliceIndex].prep(rMrProt,rSeqLim,m_sNavLocSlice,rSliceSeries.index(rProtSlice)) )
					{
						Trace (rSeqLim, ptModule, "pSlcPos[m_lSliceIndex].prep() FAILED");
						setNLSStatus( pSlcPos[m_lSliceIndex].getNLSStatus() );
						return false;
					}

				} //Loop over slices for slice pos modification

				// Restore m_lInnerSliceCounter
				m_lInnerSliceCounter = lInnerSliceCounter;
			#endif
		}
		else if (m_eNavDecideCode0 == BreakAndRepeatNavigator)
		{
			if (m_bTraceNavFeedback)
			{
				TRACE_PUT1(TC_INFO,TF_SEQ,"%s: REPEAT THIS SCAN",ptModule);
			}
		}
		else if (m_eNavDecideCode0 == RopeReorderAndProceed)
		{
			if (m_bTraceNavFeedback)
			{
				TRACE_PUT1(TC_INFO,TF_SEQ,"%s: ROPE REORDER (not implemented)",ptModule);
			}
		}
		else if (m_eNavDecideCode0 == DoNothing)
		{
			if (m_bTraceNavFeedback)
			{
				TRACE_PUT1(TC_INFO,TF_SEQ,"%s: DO NOTHING",ptModule);
			}
		}
		else
		{
			Trace (rSeqLim, ptModule, "Unrecognized m_eNavDecideCode0");
			setNLSStatus( SEQU_ERROR );
			return false;
		}

		if ( iNavNumber < lNavNumberEnd-1 )								//ib
	    {																	//ib
			m_sNav0.runPause(m_sNav0.getNavTR_ms()*1000-40000,false);		//ib - add pause to control nav TR
			//m_sNav0.runPause(10000,false);								//ib - add pause to control nav TR
		}

	}	//for(int iNavNumber = 0; iNavNumber < lNavNumberEnd; iNavNumber++)
    return true;
}

//  ------------------------------------------------
//  Implementation of method SeqLoopNav::runNavAfter
//  ------------------------------------------------
//  --------------------------------------------------------
//  Method for executing the navigator after the echo train
//  --------------------------------------------------------
bool SeqLoopNav::runNavAfter(MrProt &rMrProt, SeqLim &rSeqLim, SeqExpo &rSeqExpo, sSLICE_POS* pSlcPos)
{
    static const char *ptModule  = {"SeqLoopNav::runNavAfter"};

    if (!m_sNav0.isPrepared()) return true;
    if (!m_sNav0.getNavExecuteAfterKernel()) return true;
    if (m_lPhaseCounter != m_PhasesToMeasure - 1) return true; //|| (m_lInnerSliceCounter != m_lInnerSliceNumber-1)) return true;

    // --------------------------------------------------------------------------------------------
    // Navigator 1, after kernel loop
    // --------------------------------------------------------------------------------------------
    //  Information for the ICE program
    m_sNav0.setNavFlagForAfterEcho();

    //  Send info to the navigator SBB, so it can be transmitted to ICE for online display
    m_sNav0.setNavPercentComplete(getNavPercentComplete());
    m_sNav0.setNavPercentAccepted(getNavPercentAccepted());

    //  Reset the semaphore for fSEQReceive
    if (!m_sNav0.semaphoreRelease(&m_sSemaphore))
    {
        Trace (rSeqLim, ptModule, "m_sNav0.semaphoreRelease() FAILED");
        setNLSStatus(m_sNav0.getNLSStatus());
        return false;
    }

    //  Play out the navigator
    if (!m_sNav0.run(rMrProt,rSeqLim,rSeqExpo,pSlcPos))
    {
        Trace (rSeqLim, ptModule, "m_sNav0.run() FAILED");
        setNLSStatus(m_sNav0.getNLSStatus());
        return false;
    }

    // Set a flag to indicate that we are ready to receive the next feedback.
    if (!m_sNav0.semaphoreAcquire(&m_sSemaphore))
    {
        Trace (rSeqLim, ptModule, "m_sNav0.semaphoreAcquire() FAILED");
        setNLSStatus(m_sNav0.getNLSStatus());
        return false;
    }

    //  Before checking for feedback, add some delay time to allow for data transfer, processing, etc.
    if (!m_sNav0.runPause(1000*m_sNav0.getNavSleepDuration_ms(),true))
    {
        Trace (rSeqLim, ptModule, "m_sNav0.runPause() FAILED");
        setNLSStatus(m_sNav0.getNLSStatus());
        return false;
    }

    //  Wait until the "WAKEUP" code is seen by the DSPs
    if (!m_sNav0.waitForWakeup())
    {
        Trace (rSeqLim, ptModule, "m_sNav0.waitForWakeup() FAILED");
        setNLSStatus(m_sNav0.getNLSStatus());
        return false;
    }

#ifdef WIN32
    {
        //  When the sequence is run in simulation mode, execute fSEQReceive manually, so that the function
        //  can be tested.
        SEQData rSEQData;
        NavigatorFBData sNavigatorFBData;

        float fIceResult_1 = 129.;
        if ((m_sNav0.getNavCount()>3)&&(m_sNav0.getNavCount()%4==0)) fIceResult_1 = 39.;

        // Prepare the feedback buffer
        sNavigatorFBData.lNumber = m_sNav0.getNavNumber();
        sNavigatorFBData.lCount  = m_sNav0.getNavCount();
        sNavigatorFBData.fResult = fIceResult_1;

        rSEQData.setID("NAV");

        // "Deliver" the feedback
        if( !rSEQData.setData (&sNavigatorFBData, sizeof(sNavigatorFBData)) )
        {
            Trace (rSeqLim, ptModule, "rSEQData.setData() FAILED");
            setNLSStatus( SEQU_ERROR );
            return false;
        }

        // "Receive" the feedback
        NLS_STATUS lStatus = receive(rSeqLim,rSeqExpo,rSEQData);
        if ( (NLS_SEVERITY(lStatus)!=NLS_SUCCESS) )
        {
            Trace (rSeqLim, ptModule, "receive() FAILED");
            setNLSStatus( lStatus );
            return false;
        }
    }
#endif

    //  ---------------------------------------------------------------------------------------------
    //  Unless this is a preparing scan, wait for the feedback from the ICE program (see fSEQReceive)
    //  ---------------------------------------------------------------------------------------------
    if( m_lKernelMode == KERNEL_PREPARE )
    {
        if (m_bTraceNavFeedback)
        {
            TRACE_PUT1(TC_INFO,TF_SEQ,"%s: Prepare scan - no feedback",ptModule);
        }
    }
    else
    {
        if (!m_sNav0.waitForFeedback(&m_sSemaphore,m_sNav0.getNavPollInterval_ms()*1000))
        {
            Trace (rSeqLim, ptModule, "m_sNav0.waitForFeedback() FAILED");
            setNLSStatus(m_sNav0.getNLSStatus());
            return false;
        }
    }

    // --------------------------------------------------------------------------------------------
    //  Decide here what to do with the navigator feedback -> m_eNavDecideCode1
    // --------------------------------------------------------------------------------------------
    if (!m_sNav0.decide(1,m_lKernelMode,rMrProt,rSeqLim,rSeqExpo,pSlcPos))
    {
        Trace (rSeqLim, ptModule, "decide() FAILED");
        setNLSStatus(SEQU_ERROR);
        return false;
    }
    m_eNavDecideCode1 = m_sNav0.getNavDecideCode();


    if (m_eNavDecideCode1 == BreakAndRepeatNavigator)
    {
        if (m_bTraceNavFeedback)
        {
            TRACE_PUT1(TC_INFO,TF_SEQ,"%s: REPEAT THIS SCAN",ptModule);
        }
    }
    else if (m_eNavDecideCode1 == RopeReorderAndProceed)
    {
        if (m_bTraceNavFeedback)
        {
            TRACE_PUT1(TC_INFO,TF_SEQ,"%s: ROPE REORDER (not implemented)",ptModule);
        }
    }
    else if (m_eNavDecideCode1 == DoNothing)
    {
        if (m_bTraceNavFeedback)
        {
            TRACE_PUT1(TC_INFO,TF_SEQ,"%s: DO NOTHING",ptModule);
        }
    }
    else
    {
        Trace (rSeqLim, ptModule, "Unrecognized m_eNavDecideCode0");
        setNLSStatus( SEQU_ERROR );
        return false;
    }
    return true;
}



bool SeqLoopNav::receive(SeqLim&, SeqExpo&, SEQData& rSEQData)
{
    static const char* const ptModule = "SeqLoopNav::receive";

    const long expectedLength = 2*sizeof(int32_t) + 2*sizeof(float);
    long   lNavNumber      = 0;
    long   lNavCount       = 0;
    double dIceResult_1    = 0.0;
    double dIceResult_2    = 0.0;

    //TRACE_PUT1(TC_INFO,TF_SEQ,"%s: HERE I AM",ptModule);

    // If the data does not have the correct ID, just return.
    if ( strcmp(rSEQData.getID(), "NAV") )
    {
        TRACE_PUT2(TC_INFO, TF_SEQ, "%s: Was expecting feedback ID <NAV>, but got <%s>",ptModule, rSEQData.getID());
        return true;
    }
    if( rSEQData.getLength() != expectedLength )
    {
        TRACE_PUT3(TC_INFO,TF_SEQ,"%s: Incorrect size of feedback buffer (%ld bytes); was expecting %ld\n",
            ptModule, rSEQData.getLength(), expectedLength);
        setNLSStatus(SEQU_ERROR);
        return false;
    }

    //  -----------------------------------------------
    //  Retrieve the feedback data from the ICE program
    //  -----------------------------------------------
    NavigatorFBData* pNavigatorFBData = (NavigatorFBData*)rSEQData.getData();

    lNavNumber   = pNavigatorFBData->lNumber;
    lNavCount    = pNavigatorFBData->lCount;
    dIceResult_1 = pNavigatorFBData->fResult;
    dIceResult_2 = pNavigatorFBData->fMAGAcceptPos;

	/*
	//cout << " +++ Navigator valuelalala: " << dIceResult_1 << endl;			//JK2008 
	
    //m_MyControlIB.FileSaveAccess( dIceResult_1 );											//ib
               
    //cout << "getscouton ==  " << m_MyControlIB.getScoutOn_ib() << "and nav val ==   " << dIceResult_1 << endl;
    //cout << "getnavfeedback = " << dIceResult_1 << endl;
    m_MyControlIB.setNavVal_ib( dIceResult_1, m_MyControlIB.getScoutOn_ib() );	//ib
    
    //m_pRunLoopNav->m_MyControlIB.PlayOutCS_ib();								//ib
    //m_MyControlIB.CSIB_recorded( dIceResult_1, false, m_sNav0.getNavTR_ms() );	//ib
	*/

    //  ---------------------------------------------------------------------------------------
    //  For debugging purposes ONLY, we modify the results to test different paths of the logic
    //  ---------------------------------------------------------------------------------------
#ifdef DEBUG
	bool bWorkAsDebug = true;
#else
	bool bWorkAsDebug = SeqUT.isUnitTestActive();
#endif
    if (bWorkAsDebug && m_bPerturbNavFeedback)
    {
        if (lNavNumber == 0)
        {
            if      (m_apNav[lNavNumber]->getNavCount()<3    ) dIceResult_1 =  932.;
            else if (m_apNav[lNavNumber]->getNavCount()%2    ) dIceResult_1 =  126.;
            if (m_apNav[lNavNumber]->getNavCount()%23==0) dIceResult_1 = -123.;
            if (m_apNav[lNavNumber]->getNavCount()%24==0) dIceResult_1 = -369.;
        }
#if (MAX_NAV_INSTANCES > 1)
        if (lNavNumber == 1)
        {
            if (m_apNav[lNavNumber]->getNavCount()%51==0) dIceResult_1 = -132.;
        }
#endif
    }

    if (m_bTraceNavFeedback)
    {
        if (m_LoopLength3dE > 0)
        {
            TRACE_PUT6(TC_INFO,TF_SEQ,"%s: L%3ld/%3ld; FB: Count %3ld. Result1 %7.3f; Result2 %7.3f\n",
                ptModule,
                m_l3dECounter,m_LoopLength3dE-1,
                lNavCount,dIceResult_1,dIceResult_2);
        }
        else
        {
            TRACE_PUT6(TC_INFO,TF_SEQ,"%s: L%3ld/%3ld; FB: Count %3ld. Result1 %7.3f; Result2 %7.3f",
                ptModule,
                m_l3dECounter,m_LoopLength3dE-1,
                lNavCount,dIceResult_1,dIceResult_2);
            TRACE_PUT3(TC_INFO,TF_SEQ,"%s: P%3ld/%3ld;\n",
                ptModule,
                m_l3dECounter,m_LoopLength3dE-1);
        }
    }

    //  -----------------------------------------------------------
    //  Make the ice results known to the instance of the navigator
    //  -----------------------------------------------------------
    m_apNav[lNavNumber]->setNavIceResults(0,dIceResult_1);
    m_apNav[lNavNumber]->setNewAcceptPosition(dIceResult_2);    // set the new accept position

#ifndef WIN32
    if( m_sSemaphore.acquire(0) )
    {
        //  Fundamental timing error.
        TRACE_PUT0(TC_INFO,TF_SEQ,"The feedback has been received, but the sequence is not yet ready for it.");
        setNLSStatus (SEQU_ERROR);
        return false;
    }
#endif

    //  Unreserve the semaphore
    m_sSemaphore.release();

    return true;
}


//  -----------------------------------------------------------------
//  Implementation of SeqLoopNav::IsSevere
//  -----------------------------------------------------------------
bool SeqLoopNav::IsSevere (long lStatus) const
{
    return ( ( NLS_SEV & lStatus) == NLS_SEV );
}


//  -----------------------------------------------------------------
//  Implementation of SeqLoopNav::Trace
//  -----------------------------------------------------------------
void SeqLoopNav::Trace (SeqLim &rSeqLim, const char* ptModule, const char* ptErrorText) const
{
    if ( (! rSeqLim.isContextPrepForBinarySearch()) || (m_bTraceAlways) )  {
          TRACE_PUT2(TC_ALWAYS, TF_SEQ,"%s: %s", ptModule, ptErrorText);
    }
}

//  -----------------------------------------------------------------
//  Implementation of SeqLoopNav::getScanTimeAdditional
//  -----------------------------------------------------------------
//  This method overloads SeqLoop::getScanTimeAdditional().  It allows
//  the navigator duration to be included in the calculation of TRMin.
//  (see method TRTIFillTimes() of SeqLoop)
long SeqLoopNav::getScanTimeAdditional ()
{
    static const char *ptModule = {"SeqLoopNav::getScanTimeAdditional"};
    if (BASE_TYPE::getScanTimeRecovery() == 0)
    {
        long lOriginalValue = BASE_TYPE::getScanTimeAdditional();
        long lNewValue      = lOriginalValue + m_lTotalNavDuration_us;
#if defined DEBUG && defined DEBUG_NAV
        TRACE_PUT3(TC_ALWAYS, TF_SEQ,"%s: Original %d: New %d",
            ptModule,lOriginalValue,lNewValue);
#endif
        return (lNewValue);
    }
    else
    {
#if defined DEBUG && defined DEBUG_NAV
        TRACE_PUT2(TC_ALWAYS, TF_SEQ,"%s: BASE_TYPE::getScanTimeRecovery() already %ld; NOT Modified",
            ptModule,BASE_TYPE::getScanTimeRecovery());
#endif
        return (BASE_TYPE::getScanTimeAdditional());
    }
}

//  -----------------------------------------------------------------
//  Implementation of SeqLoopNav::getScanTimeAdditionalIR
//  -----------------------------------------------------------------
//  This method overloads SeqLoop::getScanTimeAdditionalIR().  It allows
//  the navigator duration to be included in the calculation of TIMin.
//  (see method TRTIFillTimes() of SeqLoop)
long SeqLoopNav::getScanTimeAdditionalIR ()
{
    static const char *ptModule = {"SeqLoopNav::getScanTimeAdditionalIR"};
    if (BASE_TYPE::getScanTimeRecovery() > 0)
    {
        long lOriginalValue = BASE_TYPE::getScanTimeAdditionalIR();
        long lNewValue      = lOriginalValue + m_lTotalNavDuration_us;
#if defined DEBUG && defined DEBUG_NAV
        TRACE_PUT3(TC_ALWAYS, TF_SEQ,"%s: Orig %d: New %d",
            ptModule,lOriginalValue,lNewValue);
#endif
        return (lNewValue);
    }
    else
    {
#if defined DEBUG && defined DEBUG_NAV
        TRACE_PUT2(TC_ALWAYS, TF_SEQ,"%s: BASE_TYPE::getScanTimeRecovery() already %ld; NOT Modified",
            ptModule,BASE_TYPE::getScanTimeRecovery());
#endif
        return(BASE_TYPE::getScanTimeAdditionalIR());
    }
}

#if defined __A_TRUFI_CV_NAV
bool SeqLoopNav::prepRCMSats (MrProt &rMrProt, SeqLim &rSeqLim, MrProtocolData::SeqExpo &rSeqExpo)
{
    static const char *ptModule  = {"SeqLoopNav::prepRCMSats"};
    TRACE_PUT1(TC_ALWAYS, TF_SEQ,"starting module %s", ptModule);
    TR_RUN(rSeqLim)

    if (IS_CV_FAT_SUPPRESSION_SPAIR)
    {
        // Increase minimum TI to let the navigator duration be included in the calculation of dDwellTime_us for the SPAIR (m_SBBOptfsPriorRO) pulse.   
        setminTINoSats_us(getminTINoSats_us() + (m_sNav0.getNavExecuteBeforeKernel()?m_lTotalNavDuration_us:0) ); // only consider NAV duration if NAV played before readout
    }

    if (!BASE_TYPE::prepRCMSats (rMrProt, rSeqLim, rSeqExpo))
        return (false);

    TR_END(rSeqLim)
    return (true);
} 
#endif

//  -----------------------------------------------------------------
//  Implementation of SeqLoopNav::getRFEnergyAdditional
//  -----------------------------------------------------------------
//  This method overloads SeqLoop::getRFEnergyAdditional().  It allows
//  the navigator energy to be included in the calculation of total
//  energy.
//double SeqLoopNav::getRfEnergyAdditional ()
//{
//    TRACE_PUT1(TC_ALWAYS, TF_SEQ,"SeqLoopNav::getRFEnergyAdditional: m_dTotalNavEnergy_Ws %f",
//        m_dTotalNavEnergy_Ws);
//    return m_dTotalNavEnergy_Ws;
//}

/*
//  -----------------------------------------------------------------
//  Implementation of SeqLoopNav::getScanTimeDB
//  -----------------------------------------------------------------
//  This method overloads SeqLoop::getScanTimeDB().  It allows
//  the navigator duration to be included in the calculation of TRMin.
//  (see method TRTIFillTimes() of SeqLoop)
long SeqLoopNav::getScanTimeDB ()
{
    long lScanTimeDB = BASE_TYPE::getScanTimeDB();

    long lScanTimeDBPlusNav = 0;

    if ((m_lDBIRFlag < 0) || (m_lDBIRFlag == 1))
    {
        lScanTimeDBPlusNav = lScanTimeDB + m_lTotalNavDuration_us;
        m_lDBIRFlag = 1;
    }
    else
    {
        lScanTimeDBPlusNav = lScanTimeDB;
    }

    TRACE_PUT1(TC_ALWAYS, TF_SEQ,"SeqLoopNav::getScanTimeDB: lScanTimeDBPlusNav %d",
        lScanTimeDBPlusNav);

    return (lScanTimeDBPlusNav);
}

//  -----------------------------------------------------------------
//  Implementation of SeqLoopNav::getScanTimeRecovery
//  -----------------------------------------------------------------
//  This method overloads SeqLoop::getScanTimeRecovery().  It allows
//  the navigator duration to be included in the calculation of TRMin.
//  (see method TRTIFillTimes() of SeqLoop)
long SeqLoopNav::getScanTimeRecovery ()
{
    long lScanTimeIR = BASE_TYPE::getScanTimeRecovery();

    long lScanTimeIRPlusNav = 0;

    if ((m_lDBIRFlag < 0) || (m_lDBIRFlag == 2))
    {
        lScanTimeIRPlusNav = lScanTimeIR + m_lTotalNavDuration_us;
        m_lDBIRFlag = 2;
    }
    else
    {
        lScanTimeIRPlusNav = lScanTimeIR;
    }

    TRACE_PUT1(TC_ALWAYS, TF_SEQ,"SeqLoopNav::getScanTimeIR: lScanTimeIRPlusNav %d",
        lScanTimeIRPlusNav);

    return (lScanTimeIRPlusNav);
}

//  -----------------------------------------------------------------
//  Implementation of SeqLoopNav::getRfEnergyDB
//  -----------------------------------------------------------------
double SeqLoopNav::getRfEnergyDB ()
{
    double dEnergyDB = BASE_TYPE::getRfEnergyDB();

    double dEnergyDBPlusNav = 0.;

    if ((m_lDBIRFlag < 0) || (m_lDBIRFlag == 1))
    {
        dEnergyDBPlusNav = dEnergyDB + m_dTotalNavEnergy_Ws;
        m_lDBIRFlag = 1;
    }
    else
    {
        dEnergyDBPlusNav = dEnergyDB;
    }

    //dEnergyDBPlusNav = dEnergyDB + m_dTotalNavEnergy_Ws;

    TRACE_PUT1(TC_ALWAYS, TF_SEQ,"SeqLoopNav::getRfEnergyDB:       dEnergyDBPlusNav %f",
        dEnergyDBPlusNav);

    return (dEnergyDBPlusNav);
}

//  -----------------------------------------------------------------
//  Implementation of SeqLoopNav::getRfEnergyRecovery
//  -----------------------------------------------------------------
double SeqLoopNav::getRfEnergyRecovery ()
{
    double dEnergyIR = BASE_TYPE::getRfEnergyRecovery();

    double dEnergyIRPlusNav = 0.0;

    if ((m_lDBIRFlag < 0) || (m_lDBIRFlag == 2))
    {
        dEnergyIRPlusNav = dEnergyIR + m_dTotalNavEnergy_Ws;
        m_lDBIRFlag = 2;
    }
    else
    {
        dEnergyIRPlusNav = dEnergyIR;
    }

    //dEnergyIRPlusNav = dEnergyIR + m_dTotalNavEnergy_Ws;

    TRACE_PUT1(TC_ALWAYS, TF_SEQ,"SeqLoopNav::getRfEnergyRecovery: dEnergyIRPlusNav %f",
        dEnergyIRPlusNav);

    return (dEnergyIRPlusNav);
}


//  -----------------------------------------------------------------
//  Implementation of SeqLoopNav::getEnergy
//  -----------------------------------------------------------------
double SeqLoopNav::getEnergy (MrProt &rMrProt)
{
    static const char *ptModule = {"SeqLoopNav::getEnergy"};

    // Run the getEnergy method of the base class
    double dEnergy = BASE_TYPE::getEnergy(rMrProt);

    double dAdditionalEnergy = m_dTotalNavEnergy_Ws;

    TRACE_PUT3(TC_ALWAYS, TF_SEQ,"%s: dEnergy(orig) %f; dAdditionalEnergy(1nav) %f",
        ptModule,dEnergy,dAdditionalEnergy);

    // for all kernel requests and measurement repeats
    switch ( m_ePerformPreparingScans )
    {
    case Always:
        dAdditionalEnergy *= m_lKernelRequestsInFirstMeas * (m_RepetitionsToMeasure + 1);
        break;

    case OnlyFirstRepetition:
        dAdditionalEnergy *= m_lKernelRequestsInFirstMeas + m_lKernelRequestsInSecondMeas * m_RepetitionsToMeasure;
        break;

    case Never:
        dAdditionalEnergy *= m_lKernelRequestsInFirstMeas * (m_RepetitionsToMeasure + 1);
        break;

    }

    double dTotalEnergy = dEnergy + dAdditionalEnergy;

    TRACE_PUT3(TC_ALWAYS, TF_SEQ,"%s: dTotalEnergy %f (m_lKernelRequestsInFirstMeas %d)",
        ptModule,dTotalEnergy,m_lKernelRequestsInFirstMeas);

    return (dTotalEnergy);
}
*/

//  -----------------------------------------------------------------
//  Implementation of SeqLoopNav::getMaxNavInstances
//  -----------------------------------------------------------------
long SeqLoopNav::getMaxNavInstances()
{
    return m_lMaxNavInstances;
}

//  -----------------------------------------------------------------
//  Implementation of SeqLoopNav::getNumberOfNavInstances
//  -----------------------------------------------------------------
long SeqLoopNav::getNumberOfNavInstances()
{
    return m_lNumberOfNavInstances;
}

//  -----------------------------------------------------------------
//  Implementation of SeqLoopNav::getNavRepeatCount
//  -----------------------------------------------------------------
long SeqLoopNav::getNavRepeatCount(long lIndex)
{
    return m_alNavRepeatCount[lIndex];
}

//  -----------------------------------------------------------------
//  Implementation of SeqLoopNav::getNavPercentComplete
//  -----------------------------------------------------------------
long SeqLoopNav::getNavPercentComplete()
{
    return m_lNavPercentComplete;
}

//  -----------------------------------------------------------------
//  Implementation of SeqLoopNav::getNavPercentAccepted
//  -----------------------------------------------------------------
long SeqLoopNav::getNavPercentAccepted()
{
    return m_lNavPercentAccepted;
}

//  -----------------------------------------------------------------
//  Implementation of SeqLoopNav::setNumberOfNavInstances
//  -----------------------------------------------------------------
void SeqLoopNav::setNumberOfNavInstances(long lI)
{
    m_lNumberOfNavInstances = lI;
}

//  -----------------------------------------------------------------
//  Implementation of SeqLoopNav::setNumberOfRelevantReadoutsDelivered
//  -----------------------------------------------------------------
void SeqLoopNav::setNumberOfRelevantReadoutsDelivered(long lI)
{
    m_lNumberOfRelevantReadoutsDelivered = lI;
}


//  -----------------------------------------------------------------
//  Implementation of SeqLoopNav::setNavPercentComplete
//  -----------------------------------------------------------------
void SeqLoopNav::setNavPercentComplete(long lI)
{
    m_lNavPercentComplete = ((lI>100)?100:((lI<0)?0:lI));
}

//  -----------------------------------------------------------------
//  Implementation of SeqLoopNav::setNavPercentAccepted
//  -----------------------------------------------------------------
void SeqLoopNav::setNavPercentAccepted(long lI)
{
    m_lNavPercentAccepted = ((lI>100)?100:((lI<0)?0:lI));
}

//  -----------------------------------------------------------------
//  Implementation of SeqLoopNav::updateNavPercentComplete
//  -----------------------------------------------------------------
void SeqLoopNav::updateNavPercentComplete(long lNumberOfRelevantReadoutsDeliveredIncrement, long lRelevantReadoutsForMeasTimeIncrement)
{
    if (lNumberOfRelevantReadoutsDeliveredIncrement>0) m_lNumberOfRelevantReadoutsDelivered += lNumberOfRelevantReadoutsDeliveredIncrement;
    if (lRelevantReadoutsForMeasTimeIncrement>0)       m_lRelevantReadoutsForMeasTime       += lRelevantReadoutsForMeasTimeIncrement;
    setNavPercentComplete((long) (100.0 * (double)(m_lNumberOfRelevantReadoutsDelivered-1) / (double)m_lRelevantReadoutsForMeasTime));

    // inform SCT, that there is a larger number of relevant readouts than previously reported
    if (lRelevantReadoutsForMeasTimeIncrement>0) fRTSetNoOfExpRelRxForMeasTime(m_lRelevantReadoutsForMeasTime);

    TRACE_PUT3(TC_ALWAYS,TF_SEQ,"Readouts delivered %d; expected %d; percentcomplete %d",
        m_lNumberOfRelevantReadoutsDelivered,m_lRelevantReadoutsForMeasTime,getNavPercentComplete());
}

//  -----------------------------------------------------------------
//  Implementation of SeqLoopNav::incrementNavCount
//  -----------------------------------------------------------------
void SeqLoopNav::incrementNavCount()
{
    m_lNavCount++;
}

//  -----------------------------------------------------------------
//  Implementation of SeqLoopNav::incrementNavAcceptCount
//  -----------------------------------------------------------------
void SeqLoopNav::incrementNavAcceptCount(long lIncrementFlag)
{
    if (lIncrementFlag != 0)
    {
        m_lNavAcceptCount++;
        updateNavStatusBuffers(true);  // Keep track of recent efficiency
    }
    else
    {
        updateNavStatusBuffers(false);  // Keep track of recent efficiency
    }

    if (m_lNavCount > 0)
    {
        setNavPercentAccepted((long) (100.0 * m_lNavAcceptCount / m_lNavCount + 0.5));
        
    }
    else
    {
        setNavPercentAccepted(0);
    }
}

//  -----------------------------------------------------------------
//  Implementation of SeqLoopNav::incrementNavRepeatCount
//  -----------------------------------------------------------------
void SeqLoopNav::incrementNavRepeatCount(long lIndex)
{
    if ((lIndex >= 0) && (lIndex < m_lNumberOfNavInstances)) m_alNavRepeatCount[lIndex] += 1;
}


void SeqLoopNav::calcKernelRequestsPerMeasurement (MrProt &rMrProt, SeqLim &rSeqLim)
{
  //## begin SeqLoop::calcKernelRequestsPerMeasurement%37207479032D.body preserve=yes
    static const char *ptModule = {"SeqLoopNav::calcKernelRequestsPerMeasurement"};

    TR_RUN(rSeqLim)

    //TRACE_PUT1(TC_INFO,TF_SEQ,"%s: HERE I AM",ptModule);

    m_lKernelRequestsInFirstMeas = m_lKernelRequestsInSecondMeas = 0;


    // * -------------------------------------------------------------------------- *
    // * Phase correction scans                                                     *
    // * -------------------------------------------------------------------------- *
    if (m_PhaseCorScans || rMrProt.phaseStabilize())
    {
        // PhaseCor Scans: IN PRINCIPLE NUMBER OF SLICES

        // m_lPhaseCorScansTotal = m_SlicesToMeasure; MN 160101
        m_lPhaseCorScansTotal = m_SlicesToMeasure * m_KernelCallsLoopLength;
    }
    else
    {
        m_lPhaseCorScansTotal = 0;
    }

    m_lKernelRequestsInFirstMeas += m_lPhaseCorScansTotal;




    // * -------------------------------------------------------------------------- *
    // * Imaging  Scans                                                             *
    // * -------------------------------------------------------------------------- *
    if (m_LoopLength3dE)
    {
        m_lKernelRequestsInFirstMeas += m_LoopLength3dE * m_SlicesToMeasure * m_PhasesToMeasure * m_FreeLoopLength *
                                        m_AcquisitionsOuter * m_AcquisitionsInner * m_KernelCallsLoopLength;
    }
    else
    {
        m_lKernelRequestsInFirstMeas += m_SlicesToMeasure * m_PartitionsToMeasure * m_FreeLoopLength * m_LinesToMeasure *
                                        m_PhasesToMeasure * m_AcquisitionsOuter * m_AcquisitionsInner * m_KernelCallsLoopLength;
    }


    // * -------------------------------------------------------------------------- *
    // * Preparation Scans                                                          *
    // * -------------------------------------------------------------------------- *
    m_lKernelRequestsPerMeasurementPreparingScans = 0;
    switch ( m_ePerformPreparingScans ) {

        case Always:
            m_lKernelRequestsInFirstMeas    += m_PreparingScansTotal * m_KernelCallsLoopLength;
            m_lKernelRequestsPerMeasurementPreparingScans = m_PreparingScansTotal * m_KernelCallsLoopLength;
            if (m_RepetitionsToMeasure) { m_lKernelRequestsInSecondMeas = m_lKernelRequestsInFirstMeas; }
        break;

        case OnlyFirstRepetition:
            if (m_RepetitionsToMeasure) { m_lKernelRequestsInSecondMeas = m_lKernelRequestsInFirstMeas; }
            m_lKernelRequestsInFirstMeas    += m_PreparingScansTotal * m_KernelCallsLoopLength;
            m_lKernelRequestsPerMeasurementPreparingScans = m_PreparingScansTotal * m_KernelCallsLoopLength;
        break;

        case Never:
            if (m_RepetitionsToMeasure) { m_lKernelRequestsInSecondMeas = m_lKernelRequestsInFirstMeas; }
        break;

    }


    if (IS_TR_INP(rSeqLim))
    {
        TRACE_PUT2(TC_STAFI, TF_SEQ,"%s:PreparingScans                   = %d\n",   ptModule, (int) m_PreparingScansTotal);
        TRACE_PUT2(TC_STAFI, TF_SEQ,"%s:PhaseCorScans                    = %d\n",   ptModule, (int) m_lPhaseCorScansTotal);
        TRACE_PUT2(TC_STAFI, TF_SEQ,"%s:ImagingScans                     = %d\n\n", ptModule, (int) (m_lKernelRequestsInFirstMeas-m_PreparingScansTotal-m_lPhaseCorScansTotal));
        TRACE_PUT2(TC_STAFI, TF_SEQ,"%s:m_lKernelRequestsInFirstMeas     = %d\n\n", ptModule, (int) (m_lKernelRequestsInFirstMeas));
        TRACE_PUT2(TC_STAFI, TF_SEQ,"%s:m_lKernelRequestsInSecondMeas    = %d\n\n", ptModule, (int) (m_lKernelRequestsInSecondMeas));
    }

    TR_END(rSeqLim)

  //## end SeqLoop::calcKernelRequestsPerMeasurement%37207479032D.body
}

long SeqLoopNav::getKernelRequestsPerMeasurementPreparingScans(void)
{
    return m_lKernelRequestsPerMeasurementPreparingScans;
}

bool SeqLoopNav::IsPSIR()
{
    return m_bIsPSIR;
}

void SeqLoopNav::setIsPSIR(const bool bValue)
{
    m_bIsPSIR = bValue;
}

bool SeqLoopNav::ValidNavShiftVector(void)
{
    return m_bValidNavShiftVector;
}

bool SeqLoopNav::TraceNavFeedback(void)
{
    return m_bTraceNavFeedback;
}

void SeqLoopNav::setValidNavShiftVector(const bool bValue)
{
    m_bValidNavShiftVector = bValue;
}

double SeqLoopNav::getNavShiftAmountCorrected(void)
{
    return m_dNavShiftAmountCorrected;
}

VectorPat<double> SeqLoopNav::getNavShiftVector()
{
    return m_sNavShiftVector;
}

Slice SeqLoopNav::getNavLocSlice()
{
    return m_sNavLocSlice;
}

double SeqLoopNav::updateNavStatusBuffers(bool WasAccepted)
{
    m_bNavStatusBuffer_WasAccepted[m_lNavStatusBufferIndex] = WasAccepted;
    m_lNavStatusBufferIndex++;
    if (m_lNavStatusBufferIndex == NAV_STATUS_BUFFER_SIZE) m_lNavStatusBufferIndex = 0;
    return (computeRecentEfficiency());
}

double SeqLoopNav::computeRecentEfficiency()
{
    long lI, n;
    n = 0;
    for (lI=0; lI<NAV_STATUS_BUFFER_SIZE; lI++) { if (m_bNavStatusBuffer_WasAccepted[lI]) n++; }
    m_dRecentEfficiency = ((double)n / (double) NAV_STATUS_BUFFER_SIZE);
    return m_dRecentEfficiency;
}