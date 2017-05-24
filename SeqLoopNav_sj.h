//  -----------------------------------------------------------------------------
//    Copyright (C) Siemens AG 1998  All Rights Reserved.
//  -----------------------------------------------------------------------------
//
//   Project: NUMARIS/4
//      File: \n4_servers1\pkg\MrServers\MrImaging\seq\common\Nav\SeqLoopNav.h
//   Version: \main\4d13c\3
//    Author: Clinical
//      Date: 2013-09-16 02:00:55 +02:00
//      Date: 2013-09-16 02:00:55 +02:00
//
//      Lang: C++
//
//   Descrip: MR::Measurement::CSequence::libSeqUtil
//
//   Classes:
//
//  -----------------------------------------------------------------------------

#ifndef __INCL_SeqLoopNav
#define __INCL_SeqLoopNav

#define MAX_NAV_INSTANCES      (1)
#define NAV_STATUS_BUFFER_SIZE (30)

//  -----------------------------------------------------------------
//  Definition of local SeqLoop version
//  -----------------------------------------------------------------

#include "MrServers/MrImaging/Seq/a_CV_nav_VE11/FileSave_sj.h"				//ib-c
#include "MrServers/MrImaging/Seq/a_CV_nav_VE11/ControlSystem_sj.h"			//ib
/*
#ifndef WIN32												//ib
	#include "MrServers/MrMeasSrv/PMU/pmuSequence.h"		//ib
    #include "MrServers/MrMeasSrv/PMU/pmumsg.h"				//ib
#endif
*/

#include "MrServers/MrMeasSrv/SeqIF/libRT/sREADOUT.h"

#if defined __A_TRUFI_CV_NAV
    // control order of T2prep and navigator module: conventional T2prep was before nav, but for 3T and/or adiabatic T2prep
    // Nav signal is degraded so that Nav must be played before T2prep 
    #define NAV_BEFORE_T2PREP

    #include "MrServers/MrImaging/seq/a_CV/LocalSeqLoopCV.h"
    typedef SEQ_NAMESPACE::LocalSeqLoopCV INHERITED_SEQLOOP_BASE_TYPE_NAV;

#elif defined __A_FL3D_CE_NAV

    #include "MrServers/MrImaging/seq/common/AdvMRA/SeqLoopAdvMRA.h"
    typedef SEQ_NAMESPACE::SeqLoopAdvMRA INHERITED_SEQLOOP_BASE_TYPE_NAV;

#elif defined (__A_TSE_NAV) || defined (__A_TSE_VFL_NAV)

    #include "MrServers/MrImaging/libSBB/SEQLoop.h"
    typedef SeqLoop INHERITED_SEQLOOP_BASE_TYPE_NAV;

#else

    #include "MrServers/MrImaging/libSBB/SEQLoop.h"
    typedef SEQ_NAMESPACE::SeqLoop INHERITED_SEQLOOP_BASE_TYPE_NAV;

#endif

#include "MrServers/MrImaging/seq/a_CV_nav_VE11/SBBNavigator_sj.h"
#include "MrServers/MrMeasSrv/SeqIF/libMES/SEQData.h"

typedef NLS_STATUS (*pSEQReceive) (SeqLim&,SeqExpo&,SEQData&);

class SeqLoopNav : public INHERITED_SEQLOOP_BASE_TYPE_NAV
{

public:

    typedef INHERITED_SEQLOOP_BASE_TYPE_NAV BASE_TYPE;
    typedef SeqLoopNav MY_TYPE;

    //  Default constructor
    SeqLoopNav();

    //  Destructor
    virtual ~SeqLoopNav();

    //  Overloaded methods of inherited base class
    bool prep (MrProt &rMrProt, SeqLim &rSeqLim, SeqExpo &rSeqExpo);

    bool run                (pSEQRunKernel pf,MrProt &rMrProt,SeqLim &rSeqLim,SeqExpo &rSeqExpo,sSLICE_POS* pSlcPos,sREADOUT* psADC);
    bool runOuterSliceLoopKernel (pSEQRunKernel pf,MrProt &rMrProt,SeqLim &rSeqLim,SeqExpo &rSeqExpo,sSLICE_POS* pSlcPos,sREADOUT* psADC);
    bool runKernelCallsLoop (pSEQRunKernel pf,MrProt &rMrProt,SeqLim &rSeqLim,SeqExpo &rSeqExpo,sSLICE_POS* pSlcPos,sREADOUT* psADC);

    //  Overloaded function allows nav time to be included in TR/TI calculations
    virtual long getScanTimeAdditional   ();
    virtual long getScanTimeAdditionalIR ();

#if defined __A_TRUFI_CV_NAV
    //  ----------------------------------------------------------------------
    //
    //  Name        :  SeqLoopNav::prepRCMSats
    //
    //  Description :
    ///   \brief     Adds the navigator duration the the minimum required TI time
    ///              and then calls the base class method BASE_TYPE::prepRCMSats  
    ///              to prepare the SPAIR pulse and time delay included in
    ///              the SPAIR (SBBOptfs) module.
    //
    //  Return      :  void
    //
    //  ----------------------------------------------------------------------
    virtual bool prepRCMSats (MrProt &rMrProt, SeqLim &rSeqLim, MrProtocolData::SeqExpo &rSeqExpo);
#endif

    //  Overloaded function
    virtual void calcKernelRequestsPerMeasurement (MrProt &rMrProt, SeqLim &rSeqLim);

    //  Methods for executing the navigator before and after the echo train
    bool runNavBefore (MrProt &rMrProt, SeqLim &rSeqLim, SeqExpo &rSeqExpo, sSLICE_POS* pSlcPos);
    bool runNavAfter  (MrProt &rMrProt, SeqLim &rSeqLim, SeqExpo &rSeqExpo, sSLICE_POS* pSlcPos);

    //  Feedback receive function
    virtual bool receive(SeqLim&, SeqExpo&, SEQData& rSEQData);

    //  Diagnostic functions
    bool IsSevere (long) const;
    void Trace (SeqLim&, const char*, const char*) const;

    //  Set/Get methods for member variables
    long getMaxNavInstances();
    long getNumberOfNavInstances();
    void setNumberOfNavInstances(long lI);

    long getNavPercentComplete();
    void setNavPercentComplete(long lI);

    long getNavPercentAccepted();
    void setNavPercentAccepted(long lI);

    long getNavRepeatCount(long lIndex);
    void incrementNavRepeatCount(long lIndex);

    void incrementNavAcceptCount(long lIncrementFlag);
    void incrementNavCount();

    void setNumberOfRelevantReadoutsDelivered(long lI);
    void updateNavPercentComplete(long lNumberOfRelevantReadoutsDeliveredIncrement, long lRelevantReadoutsForMeasTimeIncrement);

    long getKernelRequestsPerMeasurementPreparingScans(void);

    bool IsPSIR();
    void setIsPSIR(const bool bValue);

    bool ValidNavShiftVector();
    void setValidNavShiftVector(const bool bValue);

    bool TraceNavFeedback();

    double getNavShiftAmountCorrected(void);
    VectorPat<double> getNavShiftVector();
    Slice getNavLocSlice();

    //  The navigator SBB function
    SeqBuildBlockNavigator_sj  m_sNav0;						//sj
    SeqBuildBlockNavigator_sj* m_apNav[MAX_NAV_INSTANCES];	//sj

    //  Semaphore used to synchronize run and receive.
    SEQSemaphore m_sSemaphore;
    
    // number of relevant readouts for measurement time
    void setRelevantReadoutsForMeasTime(long lValue) {m_lRelevantReadoutsForMeasTime = lValue;}
    long getRelevantReadoutsForMeasTime(void)        {return m_lRelevantReadoutsForMeasTime;}

    // update status buffers, return efficiency
    double updateNavStatusBuffers(bool WasAccepted);

    // compute the efficiency of the most recent scans, sets m_dRecentEfficiency
    double computeRecentEfficiency(void);

	/*
	FileSaveIB m_MyFileIB;				//ib-s
    ControlSystemIB2 m_MyControlIB;		//ib-c
	#ifndef WIN32						//ib
		CPmuSequence m_myPMU;			//ib
	#endif
	*/

protected:

    //  Diagnostics
    bool m_bDebug;
    bool m_bPerturbNavFeedback;
    bool m_bTraceNavFeedback;
    bool m_bTraceAlways;

    long   m_lTotalNavDuration_us;
    MrProtocolData::SeqExpoRFInfo m_totalNavRFInfo;

    //  A value that counts the number of navs, once per heartbeat
    long m_lNavCount;

    //  A value which indicates the percentage of completed successful scans
    long m_lNavPercentComplete;

    //  A value which indicates the percentage of completed accepted scans
    long m_lNavPercentAccepted;
    long m_lNavAcceptCount;

    //  A member variable set by the decide method.
    eNavDecideCode m_eNavDecideCode0;
    eNavDecideCode m_eNavDecideCode1;

    //  The maximum number of navigator instances
    long m_lMaxNavInstances;

    //  Number of navigator SBB instances actually used.  Note: For crossed-pair navigator
    //  objects, each PAIR of GSP objects is counted as one.  For 2DRF navigators, each
    //  individual GSP object is counted as one.
    long m_lNumberOfNavInstances;

    //  The number of relevant readouts that have been delivered.
    long m_lNumberOfRelevantReadoutsDelivered;

    //  The total number of times a navigator is repeated
    long m_alNavRepeatCount[MAX_NAV_INSTANCES];

    //  The number of times the kernel is executed and the feedback gives acceptable results.
    long m_lNavKernelSuccessCount;

    //  Flag that indicates that the current scan is a repeated scan
    bool m_bNavRepeatedScan;

    //  Flag that is set true when no repeated scan is needed
    bool m_bTerminateNavLoop;

    //  Number of kernel calls for preparing scans
    long m_lKernelRequestsPerMeasurementPreparingScans;

    // flag to handle PSIR case - nav result of PD maps to be ignored
    bool m_bIsPSIR;

    // Initialized as false, set to true the first time m_sShiftVector is filled with feedback data
    bool m_bValidNavShiftVector;

    // the shift vector that specifies how much to move the slice for slice-following
    double m_dNavShiftAmountCorrected;
    VectorPat<double> m_sNavShiftVector;

    // modified slice object to be calculated during ::run
    // this must be allocated before run to avoid xenomai mode switches
    Slice m_sNavLocSlice;
    
    // number of relevant readouts for measurement time calculation
    long m_lRelevantReadoutsForMeasTime;

    // A buffer that contains the status for the last NAV_STATUS_BUFFER_SIZE navigators
    bool m_bNavStatusBuffer_WasAccepted[NAV_STATUS_BUFFER_SIZE];

    // The index of the next element of the nav status buffer to be processed
    long m_lNavStatusBufferIndex;

    // A measure of the recent navigator efficiency
    double m_dRecentEfficiency;
};

#endif
