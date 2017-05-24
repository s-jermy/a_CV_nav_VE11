//	-----------------------------------------------------------------------------
//	  Copyright (C) Siemens AG 1998  All Rights Reserved.
//	-----------------------------------------------------------------------------
//
//	 Project: NUMARIS/4
//	    File: \n4\pkg\MrServers\MrImaging\seq\common\Nav\NavigatorShell.cpp
//	 Version: \main\44
//	  Author: Randall Kroeker
//	    Date: 2014-07-31 17:22:01 +02:00
//
//	    Lang: C++
//
//	 Descrip: Navigator shell
//
//	 Classes:
//
//	-----------------------------------------------------------------------------
// Interface and definitions
#include "MrServers/MrImaging/seq/common/Nav/NavigatorShell.h"
#include "MrServers/MrImaging/seq/common/Nav/NavigatorICEProgDef.h"
#include "MrServers/MrImaging/Ice/IceProgramNavigator/NavigatorFBData.h"
#include "MrServers/MrImaging/libSeqSysProp/SysProperties.h"

#include "ProtBasic/Interfaces/MrNavigator.h"

#include "MrServers/MrProtSrv/MrProt/Physiology/MrPhysiology.h"
#include "MrServers/MrProtSrv/MrProt/Navigator/CNavigator.h"

// namespace check
#ifndef SEQ_NAMESPACE
    #error SEQ_NAMESPACE not defined
#endif


#ifdef DEBUG_ORIGIN
#undef DEBUG_ORIGIN
#endif
#define DEBUG_ORIGIN DEBUG_SBBKernel

// use sequence name space
using namespace SEQ_NAMESPACE;
                      
#if defined BASE_SEQUENCE_HAS_OOD

    //  -----------------------------------------------------------------
    //  Instansiation of sequence class
    //  -----------------------------------------------------------------
    #if defined __A_TRUFI_CV_NAV

        #ifdef SEQUENCE_CLASS_TRUFI_CV_NAV
            //#pragma message (__FILE__": Instantiating class NavigatorShell");
            SEQIF_DEFINE (SEQ_NAMESPACE::NavigatorShell);
        #endif

    #elif defined __A_FL3D_CE_NAV
        
        #ifdef SEQUENCE_CLASS_FL3D_CE_NAV
            //#pragma message (__FILE__": Instantiating class NavigatorShell");
            SEQIF_DEFINE (SEQ_NAMESPACE::NavigatorShell);
        #endif

    #elif defined __A_TSE_NAV

        #ifdef SEQUENCE_CLASS_TSE_NAV
            //#pragma message (__FILE__": Instantiating class NavigatorShell");
            SEQIF_DEFINE (SEQ_NAMESPACE::NavigatorShell);
        #endif

	#elif defined __A_TSE_VFL_NAV

        #ifdef SEQUENCE_CLASS_TSE_VFL_NAV
            //#pragma message (__FILE__": Instantiating class NavigatorShell");
            SEQIF_DEFINE (SEQ_NAMESPACE::NavigatorShell);
        #endif

    #endif

#else

    //  -----------------------------------------------------------------
    //  Instansiation of sequence class
    //  -----------------------------------------------------------------
    static NavigatorShell sNavigatorShell;

    //  -----------------------------------------------------------------
    //  Exported Interface
    //  -----------------------------------------------------------------
    NLS_STATUS fSEQInit(SeqLim& rSeqLim)
    {
        return sNavigatorShell.initialize(rSeqLim);
    }

    NLS_STATUS fSEQPrep(MrProt& rMrProt, SeqLim& rSeqLim, SeqExpo& rSeqExpo)
    {
        return sNavigatorShell.prepare(rMrProt,rSeqLim,rSeqExpo);
    }

    NLS_STATUS fSEQCheck(MrProt& rMrProt, SeqLim& rSeqLim, SeqExpo& rSeqExpo, SEQCheckMode* pSEQCheckMode)
    {
        return sNavigatorShell.check(rMrProt,rSeqLim,rSeqExpo,pSEQCheckMode);
    }

    NLS_STATUS fSEQRun(MrProt& rMrProt, SeqLim& rSeqLim, SeqExpo& rSeqExpo)
    {
        return sNavigatorShell.run(rMrProt,rSeqLim,rSeqExpo);
    }

    NLS_STATUS fSEQReceive(SeqLim& rSeqLim, SeqExpo& rSeqExpo, SEQData& rSEQData)
    {
         return sNavigatorShell.receive(rSeqLim,rSeqExpo,rSEQData);
    }

    NLS_STATUS fSEQConvProt (const MrProt &rMrProtSrc, MrProt &rMrProtDst)  
    {
         return sNavigatorShell.convProt(rMrProtSrc, rMrProtDst);
    }

    //  -----------------------------------------------------------------
    //  Redefine entry points and include original source code
    //  -----------------------------------------------------------------
    #define fSEQInit    fSEQInit_orig
    #define fSEQPrep    fSEQPrep_orig
    #define fSEQCheck   fSEQCheck_orig
    #define fSEQRun     fSEQRun_orig
    #define fSEQReceive fSEQReceive_orig
    #define fSEQConvProt fSEQConvProt_orig
    #define SeqLoop     SeqLoopNav        //  Redefine typedef during include of source code

    #if defined __A_TSE_NAV_____NONONO

        #pragma message (__FILE__": Base sequence is MrServers/MrImaging/seq/a_tse/a_tse.cpp")
        #include "MrServers/MrImaging/seq/a_tse.cpp"
        #define   m_RunLoop  mySeqLoop

    #else

        #pragma message (__FILE__": Base sequence is not defined - ABORT compilation")
        UNDEFINED_SEQUENCE_SOURCE_FILE;

    #endif

    #undef fSEQInit
    #undef fSEQPrep
    #undef fSEQCheck
    #undef fSEQRun
    #undef fSEQConvProt
    #undef fSEQReceive
    #undef SeqLoop

#endif

//  -----------------------------------------------------------------
//  Implementation of NavigatorShell::NavigatorShell
//  -----------------------------------------------------------------
NavigatorShell::NavigatorShell()
  : m_pRunLoopNav(NULL)
#if !defined BASE_SEQUENCE_HAS_OOD
  , m_pFctSeqInit(NULL)
  , m_pFctSeqPrep(NULL)
  , m_pFctSeqCheck(NULL)
  , m_pFctSeqRun(NULL)
  , m_pFctSeqConvProt(NULL)
#endif
  , m_lNumberOfPrepLoops(0)
  , m_lNumberOfPrepPrepLoops(0)
  , m_lNumberOfTriggerHalts(0)
  , m_lNavDurationPerRequest_us(0)
  , m_bTriggering(false)
  , m_lNumberOfRelevantADCsForPrep(0)
  , m_dTimeBetweenRelevantADCs_ms(1000.)
{
#ifdef WIN32
    m_pNavUI = &m_sNavUI;
#endif
    #if defined __A_TRUFI_CV_NAV
        m_bSupportPACE = false;
    #endif
    m_bDebug = false;	//SeqUT.isUnitTestActive();
#if defined DEBUG && defined DEBUG_NAV   //DEBUG_NAV activates deep traces visible within POET.
                                         //It should not be active in the product sequence!
    m_bDebug = true;
#endif

}

//  -----------------------------------------------------------------
//  Implementation of NavigatorShell::~NavigatorShell
//  -----------------------------------------------------------------
NavigatorShell::~NavigatorShell()
{
#if defined __A_TRUFI_CV_NAV
    delete m_pRunLoopNav; 
    m_pRunLoopNav = NULL;
#endif
}


//  -----------------------------------------------------------------
//  Implementation of NavigatorShell::initialize
//  -----------------------------------------------------------------
NLSStatus NavigatorShell::initialize(SeqLim& rSeqLim)
{
    static const char* const ptModule = "NavigatorShell::initialize";
    NLS_STATUS lStatus = SEQU_NORMAL;
    
    mTRrun;

    m_bTraceAlways = true;


#if !defined BASE_SEQUENCE_HAS_OOD
    
    //  ---------------------------------------------------------------
    //  Initialize pointers to the entry points of the imaging sequence
    //  ---------------------------------------------------------------
    m_pFctSeqInit     = fSEQInit_orig;
    m_pFctSeqPrep     = fSEQPrep_orig;
    m_pFctSeqCheck    = fSEQCheck_orig;
    m_pFctSeqRun      = fSEQRun_orig;
    #ifndef __A_TSE_NAV    // tse.cpp currently does not provide a fSEQConvProt method
        m_pFctSeqConvProt = fSEQConvProt_orig;
    #endif
    
#endif
    
#if defined (__A_TSE_NAV) || defined (__A_TSE_VFL_NAV)
    
	m_pRunLoopNav = BASE_TYPE::getSeqLoop();
    
#elif !defined __A_TRUFI_CV_NAV
    
    m_pRunLoopNav = &m_RunLoop;
    
#endif
    
    //  -------------------------------------------
    //  Call the init function of the base sequence
    //  -------------------------------------------
#if defined BASE_SEQUENCE_HAS_OOD
    if (m_bDebug) TRACE_PUT1(TC_INFO, TF_SEQ,"%s: Calling BASE_TYPE::initialize()",ptModule);
    lStatus = BASE_TYPE::initialize(rSeqLim);
#else
    if (m_bDebug) TRACE_PUT1(TC_INFO, TF_SEQ,"%s: Calling (*m_pFctSeqInit)()",ptModule);
    lStatus = (*m_pFctSeqInit)(rSeqLim);
#endif
    if ( IsSevere(lStatus) )
    {
        Trace (rSeqLim, ptModule, "INIT method of base sequence FAILED");
        return ( lStatus );
    }
    
    //TRACE_PUT2(TC_ALWAYS, TF_SEQ,"%s: Address of SeqLoopNav %8.8p",ptModule,m_pRunLoopNav);

    //  ----------------------------------------
    //  Only 3D imaging (disabled 2005.09.08)
    //  ----------------------------------------
    //lStatus = rSeqLim.setDimension( SEQ::DIM_3 );
    //if ( IsSevere(lStatus) )
    //{
    //    Trace (rSeqLim, ptModule, "rSeqLim.setDimension() FAILED");
    //    return ( lStatus );
    //}

    //  ----------------------------------------
    //  Disable the option of respiratory gating
    //  ----------------------------------------
#if !defined __A_TRUFI_CV_NAV
    lStatus = rSeqLim.getPhysioModes().unset(SEQ::SIGNAL_RESPIRATION, SEQ::METHOD_TRIGGERING);
    if ( IsSevere(lStatus) )
    {
        Trace (rSeqLim, ptModule, "rSeqLim.getPhysioModes().unset FAILED");
        return ( lStatus );
    }
#endif
    
    //  ----------------------------------------
    //  Enable "Capture Cycle"
    //  ----------------------------------------
    lStatus = rSeqLim.enableCaptureCycle();  
    if ( IsSevere(lStatus) )
    {
        Trace (rSeqLim, ptModule, "rSeqLim.enableCaptureCycle() FAILED");
        return ( lStatus );
    }
    
    //  ---------------------------------
    //  Disable the option of retrogating
    //  ---------------------------------
    lStatus = rSeqLim.getPhysioModes().unset(SEQ::SIGNAL_CARDIAC, SEQ::METHOD_RETROGATING);
    if ( IsSevere(lStatus) )
    {
        Trace (rSeqLim, ptModule, "rSeqLim.getPhysioModes().unset() FAILED");
        return ( lStatus );
    }

    lStatus = rSeqLim.setRetroGatedImages(0,0,1,0) ;
    if ( IsSevere(lStatus) )
    {
        Trace (rSeqLim, ptModule, "rSeqLim.setRetroGatedImages() FAILED");
        return ( lStatus );
    }
    
    //  -----------------------------------------------
    //  Disable the option of slice-selective inversion
    //  -----------------------------------------------
    lStatus = rSeqLim.getInversion().unset(SEQ::SLICE_SELECTIVE);
    if ( IsSevere(lStatus) )
    {
        Trace (rSeqLim, ptModule, "rSeqLim.getInversion().unset() FAILED");
        return ( lStatus );
    }
    
    //  ----------------------------------------------------------------------------
    //  Disable the options for adaptive triggering and inline evaluation for CV_nav  
    //  ----------------------------------------------------------------------------
#if defined __A_TRUFI_CV_NAV

    // no support of adaptive triggering
    lStatus = rSeqLim.setAdaptiveTriggering(SEQ::OFF);
    if ( IsSevere(lStatus) )
    {
        Trace (rSeqLim, ptModule, "rSeqLim.setAdaptiveTriggering() FAILED");
        return ( lStatus );
    }

    // no support of trigger lock time
    lStatus = rSeqLim.setTriggerLockTime(0,0,0,0);
    if ( IsSevere(lStatus) )
    {
        Trace (rSeqLim, ptModule, "rSeqLim.setTriggerLockTime() FAILED");
        return ( lStatus );
    }

    // no support of Inline evaluation
    lStatus = rSeqLim.setInlineEva(SEQ::INLINEEVA_OFF);
    if ( IsSevere(lStatus) )
    {
        Trace (rSeqLim, ptModule, "rSeqLim.setInlineEva() FAILED");
        return ( lStatus );
    }

    // no support of Motion correction
    lStatus = rSeqLim.setMotionCorr(SEQ::MOTION_COR_NONE);
    if ( IsSevere(lStatus) )
    {
        Trace (rSeqLim, ptModule, "rSeqLim.setMotionCorr() FAILED");
        return ( lStatus );
    }

    // no support of Proton density scans
    lStatus = rSeqLim.setProtonDensMaps(0,0,0,0);
    if ( IsSevere(lStatus) )
    {
        Trace (rSeqLim, ptModule, "rSeqLim.setProtonDensMaps() FAILED");
        return ( lStatus );
    }

    // set the display of these boxes off 
    rSeqLim.getAdaptiveTriggering().setDisplayMode  (SEQ::DM_OFF);
    rSeqLim.getTriggerLockTime().setDisplayMode     (SEQ::DM_OFF);
    rSeqLim.getInlineEva().setDisplayMode           (SEQ::DM_OFF);
    rSeqLim.getMotionCorr().setDisplayMode          (SEQ::DM_OFF);
    rSeqLim.getProtonDensMaps().setDisplayMode      (SEQ::DM_OFF);

#endif

    //  -----------------------------------------------------------------------------
    //  Disable multiple acquisitions and repetitions, since this is not supported by
    //  the ICE program
    //  -----------------------------------------------------------------------------
#ifndef __A_TSE_VFL_NAV
    lStatus = rSeqLim.setAverages(1, 1, 1, 1);
    if ( IsSevere(lStatus) )
    {
        Trace (rSeqLim, ptModule, "rSeqLim.setAverages() FAILED");
        return ( lStatus );
    }
#endif
    
    lStatus = rSeqLim.setRepetitions(0,0,1,0);
    if ( IsSevere(lStatus) )
    {
        Trace (rSeqLim, ptModule, "rSeqLim.setRepetitions() FAILED");
        return ( lStatus );
    }
    
    //  ------------------------------------------------------------------------------
    //  Disable some other UI options
    //  ------------------------------------------------------------------------------
    //lStatus = rSeqLim.setConcatenations(1, 1, 1, 1);
    //if ( IsSevere(lStatus) )
    //{
    //    Trace (rSeqLim, ptModule, "rSeqLim.setConcatenations() FAILED");
    //    return ( lStatus );
    //}
    
    // Only sequential slices with more than one slice (Navigator is similar to DB behaviour)
    lStatus = rSeqLim.setMultiSliceMode(SEQ::MSM_SEQUENTIAL);
    if ( IsSevere(lStatus) )
    {
        Trace (rSeqLim, ptModule, "rSeqLim.setMultiSliceMode() FAILED");
        return ( lStatus );
    }

    lStatus = rSeqLim.setContrasts(1, 1, 1, 1);
    if ( IsSevere(lStatus) )
    {
        Trace (rSeqLim, ptModule, "rSeqLim.setContrasts() FAILED");
        return ( lStatus );
    }
    
    lStatus = rSeqLim.setPhases(1, 1, 1, 1);
    if ( IsSevere(lStatus) )
    {
        Trace (rSeqLim, ptModule, "rSeqLim.setPhases() FAILED");
        return ( lStatus );
    }
    
    lStatus = rSeqLim.setTrajectory(SEQ::TRAJECTORY_CARTESIAN);
    if ( IsSevere(lStatus) )
    {
        Trace (rSeqLim, ptModule, "rSeqLim.setTrajectory() FAILED");
        return ( lStatus );
    }    

    //  --------------------------------------------------------------------------
    //  For the fl3d_ce sequence, just use SEQ::TOM_MINIMIZE_TE as the default
    //  --------------------------------------------------------------------------
#if defined __A_FL3D_CE_NAV
#pragma message (__FILE__": Using SEQ::TOM_MINIMIZE_TE as the default")
    lStatus = rSeqLim.setTOM(SEQ::TOM_MINIMIZE_TE,SEQ::TOM_MINIMIZE_TE_TR,SEQ::TOM_OFF);
    if ( IsSevere(lStatus) )
    {
        Trace (rSeqLim, ptModule, "rSeqLim.setPhasePartialFourierFactor() FAILED");
        return ( lStatus );
    }
#endif

    //  --------------------------------------------------------------------------
    //  For the fl3d_ce sequence, allow more options for PhasePartialFourierFactor
    //  --------------------------------------------------------------------------
#if defined __A_FL3D_CE_NAV
#pragma message (__FILE__": Allowing PF_OFF, PF_7_8, and PF_6_8 for PhasePartialFourierFactor")
    lStatus = rSeqLim.setPhasePartialFourierFactor(SEQ::PF_OFF, SEQ::PF_7_8, SEQ::PF_6_8);
    if ( IsSevere(lStatus) )
    {
        Trace (rSeqLim, ptModule, "rSeqLim.setPhasePartialFourierFactor() FAILED");
        return ( lStatus );
    }
#endif
    
    //  --------------------------------------------------------------------------
    //  For the fl3d_ce sequence, adjust the default TI and TR values
    //  --------------------------------------------------------------------------
#if defined __A_FL3D_CE_NAV
#pragma message (__FILE__": Default TI changed to 170 ms")
    lStatus = rSeqLim.setTI    (       0,        1000,     1500000,        1000,        170000);
    if ( IsSevere(lStatus) )
    {
        Trace (rSeqLim, ptModule, "rSeqLim.setTI() FAILED");
        return ( lStatus );
    }

#pragma message (__FILE__": Default TR changed to 500 ms")
    lStatus = rSeqLim.setTR    (       0,        1000,     2000000,          10,        500000);
    if ( IsSevere(lStatus) )
    {
        Trace (rSeqLim, ptModule, "rSeqLim.setTR() FAILED");
        return ( lStatus );
    }

#if defined TURN_OFF_RF_SPOILING
	#ifdef DEBUG
		bool bWorkAsDebug = true;
    #pragma message (__FILE__": RF Spoiling turned off!!!!")
	#else
		bool bWorkAsDebug = SeqUT.isUnitTestActive();
	#endif
    if ( bWorkAsDebug )
    {
    rSeqLim.setRFSpoiling(SEQ::OFF);
    }
#endif

#endif
    
#if defined (__A_TSE_NAV) || defined (__A_TSE_VFL_NAV)
    //lStatus = rSeqLim.setTR    (0,     100,    10000000,         100,     3000000);
    //if ( IsSevere(lStatus) )
    //{
    //    Trace (rSeqLim, ptModule, "rSeqLim.setTR() FAILED");
    //    return ( lStatus );
    //}

    // only permit the classical fatsat options
    lStatus = rSeqLim.setFatSuppression(SEQ::FAT_SUPPRESSION_OFF  , SEQ::FAT_SATURATION);
    if ( IsSevere(lStatus) )
    {
        Trace (rSeqLim, ptModule, "rSeqLim.setFatSuppression() FAILED");
        return ( lStatus );
    }

    // CHARM 382011 in default protocol, avoid improper initialization of RFBlockInfos
    lStatus = rSeqLim.setIntro(SEQ::OFF, SEQ::ON);
    if ( IsSevere(lStatus) )
    {
        Trace (rSeqLim, ptModule, "rSeqLim.setIntro() FAILED");
        return ( lStatus );
    }

#endif

    //  ------------------------------------
    //  Initialize special UI for navigators
    //  ------------------------------------
#ifdef WIN32
    lStatus = m_sNavUI.init(rSeqLim);
    if ( IsSevere(lStatus) )
    {
        Trace (rSeqLim, ptModule, "m_sNavUI.init() FAILED");
        return ( lStatus );
    }
#endif
    
    //  -------------------------------------------------------------------------------
    //  Define the maximum number of possible navigator objects.  For each crossed pair
    //  of slices, we need 2 objects.  For each 2D pencil, we need only one.  When the
    //  minimum value is set to zero, it is possible to turn off the display of navs in
    //  the GSP.
    //  -------------------------------------------------------------------------------
    long lMaxGSPNavs = 2;
    long lDefGSPNavs = 0;
    long lMinGSPNavs = 0;
#if defined __A_FL3D_CE_NAV
    lMinGSPNavs = 0;
    lDefGSPNavs = 2;
#elif defined (__A_TSE_NAV) || defined (__A_TSE_VFL_NAV)
    lMinGSPNavs = 0;
    lDefGSPNavs = 2;
#else
    lMinGSPNavs = 2;
    lDefGSPNavs = 2;
#endif
    lStatus = rSeqLim.setNavigators(lMinGSPNavs, lMaxGSPNavs, lMaxGSPNavs, lDefGSPNavs);
    if ( IsSevere(lStatus) )
    {
        Trace (rSeqLim, ptModule, "rSeqLim.setNavigators() FAILED");
        return ( lStatus );
    }
    
    //  -----------------------------------------------
    //  Define the FOV limits for the navigator objects
    //  -----------------------------------------------
    const double dFoVMax_mm  = SysProperties::getFoVMax();
    double dFoVMin_mm        =  1.0;
    double dFoVDef_mm        = 10.0;
    const double dFoVIncr_mm =  1.0;
    
    //  ----------------------------------------------------------
    //  Set the FOV and thickness limits for the navigator objects
    //  ----------------------------------------------------------
    long lI=0;
    for (lI=0; lI<lMaxGSPNavs; lI++)
    {
        lStatus = rSeqLim.setNavigatorPhaseFOV (lI, dFoVMin_mm, dFoVMax_mm, dFoVIncr_mm, dFoVDef_mm);
        if ( IsSevere(lStatus) )
        {
            Trace (rSeqLim, ptModule, "rSeqLim.setNavigatorPhaseFOV() FAILED");
            return ( lStatus );
        }
        
        lStatus = rSeqLim.setNavigatorReadFOV (lI, dFoVMin_mm, dFoVMax_mm, dFoVIncr_mm, dFoVDef_mm);
        if ( IsSevere(lStatus) )
        {
            Trace (rSeqLim, ptModule, "rSeqLim.setNavigatorReadFOV() FAILED");
            return ( lStatus );
        }
        
        if (lI<2)
        {
            lStatus = rSeqLim.setNavigatorThickness(lI, 3.0, 20.0, 0.5, 10.0);  //  unit is [mm]
        }
        else
        {
            lStatus = rSeqLim.setNavigatorThickness(lI, 3.0, 20.0, 0.5,  5.0);  //  unit is [mm]
        }
        if ( IsSevere(lStatus) )
        {
            Trace (rSeqLim, ptModule, "rSeqLim.setNavigatorThickness() FAILED");
            return ( lStatus );
        }
    }
    
    //  ----------------------------
    //  Initialize the navigator SBB
    //  ----------------------------
    if (!m_pRunLoopNav->m_sNav0.init(rSeqLim))
    {
        Trace (rSeqLim, ptModule, "m_pRunLoopNav->m_sNav0.init() FAILED");
        return (m_pRunLoopNav->m_sNav0.getNLSStatus());
    }

    //  ----------------------------
    //  Allow motion adaptive gating
    //  ----------------------------
    lStatus = rSeqLim.setRespMotionAdaptiveGating(SEQ::ON,SEQ::OFF);
    if ( IsSevere(lStatus) )
    {
        Trace (rSeqLim, ptModule, "rSeqLim.setRespMotionAdaptiveGating() FAILED");
        return ( lStatus );
    }

    
    //  ---------------------------------------------------------------------------
    //  Disable this sequence for certain system sytem types (Charm 310010, 309859)
    //  ---------------------------------------------------------------------------
    //  Re-Enable this sequence for Trio 
    lStatus = rSeqLim.setNotSupportedSystemTypes("007");  //Concerto 
    if ( IsSevere(lStatus) )
    {
        Trace (rSeqLim, ptModule, "rSeqLim.setNotSupportedSystemTypes() FAILED");
        return ( lStatus );
    }

    mTRend;
    return lStatus;
}


//  -----------------------------------------------------------------
//  Implementation of NavigatorShell::prepare
//  -----------------------------------------------------------------
NLSStatus NavigatorShell::prepare(MrProt& rMrProt, SeqLim& rSeqLim, SeqExpo& rSeqExpo)
{
    static const char* const ptModule = "NavigatorShell::prepare";
    NLS_STATUS lStatus = SEQU_NORMAL;

    SEQ::PhysioSignal myTrigSignalHigh = SEQ::SIGNAL_NONE;
    SEQ::PhysioSignal myTrigSignalLow  = SEQ::SIGNAL_NONE;
    SEQ::PhysioMethod myTrigMethodHigh = SEQ::METHOD_NONE;
    SEQ::PhysioMethod myTrigMethodLow  = SEQ::METHOD_NONE;
    
    m_bTraceAlways = false;
    
    mTRrun;
    
    if (m_bDebug)
    {
        TRACE_PUT1(TC_ALWAYS, TF_SEQ,"%s: Starting",ptModule);
        TRACE_PUT2(TC_ALWAYS, TF_SEQ,"%s: Address of SeqLoopNav %8.8p",ptModule,m_pRunLoopNav);
        #if defined __A_TRUFI_CV_NAV
        TRACE_PUT2(TC_ALWAYS, TF_SEQ,"%s: m_bSupportPACE = %d",ptModule, m_bSupportPACE);       
        #endif
                                                            TRACE_PUT1(TC_INFO, TF_SEQ,"%s: -----------------------------------",ptModule);
        if (rSeqLim.isContextPrepForBinarySearch())        TRACE_PUT1(TC_INFO, TF_SEQ,"%s: isContextPrepForBinarySearch",ptModule);
        if (rSeqLim.isContextPrepForMrProtUpdate())        TRACE_PUT1(TC_INFO, TF_SEQ,"%s: isContextPrepForMrProtUpdate",ptModule);
        if (rSeqLim.isContextPrepForScanTimeCalculation()) TRACE_PUT1(TC_INFO, TF_SEQ,"%s: isContextPrepForScanTimeCalculation",ptModule);  
        if (rSeqLim.isContextNormal())                     TRACE_PUT1(TC_INFO, TF_SEQ,"%s: isContextNormal",ptModule);
                                                            TRACE_PUT1(TC_INFO, TF_SEQ,"%s: -----------------------------------",ptModule);
    }
    


    //  ---------------------------------
    //  Physiological imaging?
    //  ---------------------------------
    rMrProt.physiology().getPhysioMode (myTrigSignalHigh, myTrigMethodHigh, myTrigSignalLow, myTrigMethodLow);
    m_bTriggering = ((myTrigSignalHigh & SEQ::SIGNAL_CARDIAC) != 0);
    
#ifdef WIN32
    //  ---------------------------------
    //  Prepare special UI for navigators
    //  ---------------------------------
    lStatus = m_sNavUI.prep(rMrProt, rSeqLim, rSeqExpo);
    if ( IsSevere(lStatus) )
    {
        Trace (rSeqLim, ptModule, "m_sNavUI.prep() FAILED");
        return ( lStatus );
    }
#endif


//  ---------------------------------
//  Switch off RT processing in CV sequence
//  ---------------------------------

#ifdef __A_TRUFI_CV_NAV
    INHERITED_SEQUENCE_BASE_TYPE::m_bRealTimeProcessing = false;
#endif

    //  -------------------------------------------
    //  Call the prep function of the base sequence
    //  -------------------------------------------
#if defined BASE_SEQUENCE_HAS_OOD
    if (m_bDebug) TRACE_PUT1(TC_INFO, TF_SEQ,"%s: Calling BASE_TYPE::prepare()",ptModule);
    lStatus = BASE_TYPE::prepare(rMrProt, rSeqLim, rSeqExpo);
#else
    if (m_bDebug) TRACE_PUT1(TC_INFO, TF_SEQ,"%s: Calling (*m_pFctSeqPrep)()",ptModule);
    lStatus = (*m_pFctSeqPrep)(rMrProt, rSeqLim, rSeqExpo);
#endif
    if ( IsSevere(lStatus) )
    {
        Trace (rSeqLim, ptModule, "PREP method of base sequence FAILED");
        return ( lStatus );
    }
    //else
    //{
    //    Trace (rSeqLim, ptModule, "PREP method of base sequence OK");
    //}

#if defined __A_TSE_VFL_NAV
    if ((rMrProt.NavigatorParam().getalFree()[Label_SelectionBox_NavMode] != Value_NavModeOff) &&
        (rMrProt.NavigatorParam().getalFree()[Label_SelectionBox_NavPosition] != Value_NavBefEchoTrain))
    {
        lStatus = SEQU_ERROR;
        return lStatus;
    }
#endif

    //  ---------------------------------------------------------
    //  Retrieve measurement time and RF energy of the navigators
    //  ---------------------------------------------------------
    m_lNavDurationPerRequest_us = m_pRunLoopNav->m_sNav0.getTotalDuration_ExcludingIR_us();
    m_NavRFInfoPerRequest   = m_pRunLoopNav->m_sNav0.getTotalRFInfo_ExcludingIR();

    if(rMrProt.reconstructionMode() == SEQ::RECONMODE_PSIR)
    {
        m_NavRFInfoPerRequest += m_pRunLoopNav->m_sNav0.getTotalRFInfo_ExcludingIR(); // PSIR is using a trigger loop
    }

    //  ----------------------------------------------------------------
    //  Retrieve measurement time and RF energy of the original sequence
    //  ----------------------------------------------------------------
    MrProtocolData::SeqExpoRFInfo originalRFInfoInSequence = rSeqExpo.getRFInfo();
    double dOriginalMeasureTimeUsec       = rSeqExpo.getMeasureTimeUsec();
    long   lNumberOfTriggerHaltsTotal          = m_pRunLoopNav->getKernelRequestsPerMeasurement(rSeqLim, FirstMeas, 1);
    long   lNumberOfTriggerHaltsPreparingScans = m_pRunLoopNav->getKernelRequestsPerMeasurementPreparingScans();
    
    //  ----------------------------------------
    //  Special UI restriction for prep TR times
    //  ----------------------------------------
    if (m_pRunLoopNav->m_sNav0.isPrepared())
    {
        if (1000*m_pRunLoopNav->m_sNav0.getNavPrepTR_ms() < m_lNavDurationPerRequest_us)
        {
            Trace (rSeqLim, ptModule, "Invalid TR for nav scout");
            return (SEQU_ERROR);
        }
    }
    
    //  -----------
    //  Diagnostics
    //  -----------
    if (m_bDebug)
    {
        TRACE_PUT2(TC_INFO,TF_SEQ,"%s: originalRFInfoInSequence.getPulseEnergyWs()      %11.6f Ws",ptModule,originalRFInfoInSequence.getPulseEnergyWs());
        TRACE_PUT2(TC_INFO,TF_SEQ,"%s: dOriginalMeasureTimeUsec            %11.6f",ptModule,dOriginalMeasureTimeUsec);
        TRACE_PUT2(TC_INFO,TF_SEQ,"%s: lNumberOfTriggerHaltsTotal          %d"    ,ptModule,lNumberOfTriggerHaltsTotal);
        TRACE_PUT2(TC_INFO,TF_SEQ,"%s: lNumberOfTriggerHaltsPreparingScans %d"    ,ptModule,lNumberOfTriggerHaltsPreparingScans);
        TRACE_PUT3(TC_INFO,TF_SEQ,"%s: Duration SBBDB %d; SBBIRns %d",ptModule,m_pRunLoopNav->getScanTimeDB(),m_pRunLoopNav->getScanTimeRecovery());
    }

    //  ----------------------------------------------------------------
    //  Store the name of the original ICE program in the protocol, then
    //  overwrite it with the new ICE program name.
    //  ----------------------------------------------------------------
    if ( rSeqLim.isContextNormal() )
    {
        //  Get the name of the original ICE program
        if (m_bDebug) TRACE_PUT2(TC_INFO, TF_SEQ,"%s: Original ICE program name %s",ptModule,rSeqLim.getICEProgramFilename().get());
        
        //  Set the name of the new ICE program
        #pragma message (__FILE__": Using IceProgramNavigator as ICE program")
        rSeqExpo.setICEProgramFilename( "%SiemensIceProgs%\\IceProgramNavigator");
        
        // IceProgram3DnonCartesian in case of 3D Radial recon
        if(rMrProt.kSpace().trajectory() == SEQ::TRAJECTORY_RADIAL && rMrProt.kSpace().dimension() == SEQ::DIM_3)
        {
            rSeqExpo.setICEProgramFilename( "%CustomerIceProgs%\\IceProgram3DnonCartesian" );
        }

        // use IceProgramNavigator_IcePAT for 2D accelerated protocols
        if ((rMrProt.PAT().getlAccelFact3D() > 1) && (rMrProt.PAT().getlAccelFactPE() > 1) && (rMrProt.kSpace().dimension() == SEQ::DIM_3))    
        {
            rSeqExpo.setICEProgramFilename ("%SiemensIceProgs%\\IceProgramNavigator_IcePAT");
        }
        
        if (m_bDebug) TRACE_PUT2(TC_INFO, TF_SEQ,"%s:      New ICE program name %s",ptModule,rSeqLim.getICEProgramFilename().get());
        
        //  Disable multiple offline calls in OnIce (Charm 310024)
        rSeqExpo.setOnlineFFT ( SEQ::ONLINE_FFT_ACQ_END );
            
        //  Disable TREff calculation
        rSeqExpo.setICEProgramParam(ICE_PROGRAM_PARA_CTRL_MASK,
            rSeqExpo.getICEProgramParam(ICE_PROGRAM_PARA_CTRL_MASK) | ICE_PROGRAM_PARA_NO_TREFF_CALCULATION);

        // switch off counter mode for PAT ref lines ((Charm 383508)
        long lIceProgramMOC = rSeqExpo.getICEProgramParam(ICE_PROGRAM_PARA_MULTI_OFFLINE_CALL);
        lIceProgramMOC &= ~ICE_PROGRAM_MSK_IPAT_TRIGGER_PERCOUNTER;
        rSeqExpo.setICEProgramParam(ICE_PROGRAM_PARA_MULTI_OFFLINE_CALL, lIceProgramMOC);
    }
    
    //  ----------------------------------------------------------
    //  Update the exported values of energy and measurement times
    //  NOTE: ONLY the first navigator instance is used.
    //  ----------------------------------------------------------
    if (m_pRunLoopNav->m_sNav0.isPrepared())
    {
        if (m_pRunLoopNav->m_sNav0.isNavPrepScan())
        {
            //  -----------------------
            //  PREP scans are selected
            //  -----------------------

            //  We need to export the measurement time and RF energy, since the prep scans
            //  are played out outside of SeqLoop.

            long   lDuration_ms = m_pRunLoopNav->m_sNav0.getNavPrepDuration_ms();  // Requested duration of prep scans
            long   lTR_ms       = m_pRunLoopNav->m_sNav0.getNavPrepTR_ms();        // Requested TR period for prep scans
            
            m_lNumberOfPrepPrepLoops = 3;  //  Preps for the prep measurement
            m_lNumberOfPrepLoops     = 1;

            if (lTR_ms > 0) m_lNumberOfPrepLoops = (lDuration_ms + lTR_ms - 1) / lTR_ms;
            if (m_lNumberOfPrepLoops<1) m_lNumberOfPrepLoops=1;
            
            m_lNumberOfPrepLoops += m_lNumberOfPrepPrepLoops;  //  Number of times the navigator is executed
            
            //  ----------------------------------------------
            //  Calculate exported measurement time and energy
            //  ----------------------------------------------
            double dTotalTime_us   = 1000.0 * m_lNumberOfPrepLoops * lTR_ms;
            MrProtocolData::SeqExpoRFInfo totalRFInfo = m_lNumberOfPrepLoops * m_NavRFInfoPerRequest;
            
            //  ---------------------------------------------------
            //  Calculate the number of relevant ADCs for the scout
            //  ---------------------------------------------------
            m_lNumberOfRelevantADCsForPrep = (long) (dTotalTime_us / (m_dTimeBetweenRelevantADCs_ms*1000.) + 0.5);
            
            // CHARM 382011
            // reset rSeqExpo buffer (might still contain multiple energy blocks from previous call of fSeqPrep() in binary search !!!)
#if defined __A_TSE_VFL_NAV
            if (rMrProt.NavigatorParam().getalFree()[Label_CheckBox_PrepScan] == Value_CheckBox_On)
                rSeqExpo.resetAllRFBlockInfos();     // note: return status MUST NOT be checked in this case: will return false, if no energy blocks available (that's the general case at this point!)
#endif

            //  -------------------
            //  Set exported values
            //  -------------------
            rSeqExpo.setEstimatedMeasureTimeUsec(dTotalTime_us);
            rSeqExpo.setMeasureTimeUsec         (dTotalTime_us);
            rSeqExpo.setTotalMeasureTimeUsec    (dTotalTime_us);
            rSeqExpo.setRFInfo   (totalRFInfo);
            
            rSeqExpo.setMeasTimePerBreathholdUSec   ( 0 );
            rSeqExpo.setNoOfBreathholdsPerMeas      ( 0 );

            if (!m_bTriggering)
            {
                rSeqExpo.setRelevantReadoutsForMeasTime ( m_lNumberOfRelevantADCsForPrep );
            }
            
            // Provide the number of relevant readouts to the SeqLoop class
            m_pRunLoopNav->setRelevantReadoutsForMeasTime(rSeqExpo.getRelevantReadoutsForMeasTime());
            
            if (m_bDebug)
            {
                TRACE_PUT2(TC_ALWAYS, TF_SEQ,"%s: MeasTimePerBreathholdUSec   %12.3f;",ptModule,rSeqExpo.getMeasTimePerBreathholdUSec());
                TRACE_PUT2(TC_ALWAYS, TF_SEQ,"%s: NoOfBreathholdsPerMeas      %3ld;",  ptModule,rSeqExpo.getNoOfBreathholdsPerMeas());
                TRACE_PUT2(TC_ALWAYS, TF_SEQ,"%s: RelevantReadoutsForMeasTime %3ld;",  ptModule,rSeqExpo.getRelevantReadoutsForMeasTime());
            }
            
        }
        else
        {
            //  --------------------------
            //  IMAGING scans are selected
            //  --------------------------
            //m_bDebug = (!rSeqLim.isContextPrepForBinarySearch());

            if (m_bDebug) TRACE_PUT1(TC_INFO,TF_SEQ,"m_pRunLoopNav->getdScanTimeTrigHalt (FirstMeas)  %f",m_pRunLoopNav->getdScanTimeTrigHalt (FirstMeas));
            if (m_bDebug) TRACE_PUT1(TC_INFO,TF_SEQ,"m_pRunLoopNav->getdScanTimeTrigHalt (SecondMeas) %f",m_pRunLoopNav->getdScanTimeTrigHalt (SecondMeas));

            //  --------------------------------------------------------
            //  Estimated measurement time according to PMU (RR capture)
            //  --------------------------------------------------------
#ifdef WIN32                                                                                           
            if ( ( myTrigMethodHigh != SEQ::METHOD_NONE ) &&
                 ( rSeqLim.isContextPrepForScanTimeCalculation() || rSeqLim.isContextNormal() ) &&
                 ( m_pRunLoopNav->getTrigHaltDuration()) ) 
            {        
                long lPhysioHalts = long (m_pRunLoopNav->getdScanTimeTrigHalt (FirstMeas) + 
                                          m_pRunLoopNav->getdScanTimeTrigHalt (SecondMeas) * rMrProt.repetitions()) / m_pRunLoopNav->getTrigHaltDuration();

                SeqUT.setExpectedPhysioHalts (lPhysioHalts);                                                                            
                long lThreshold_us = rMrProt.tr()[0] * maximum (1L, rMrProt.physiology().phases()) * maximum (1L, rSeqExpo.getAcqWindowFactor())
                    + rMrProt.physiology().triggerDelay(rMrProt.physiology().signal(1));

                rSeqExpo.setEstimatedMeasureTimeUsec (fSUGetEstimatedMeasTimeUsec(rMrProt, rSeqExpo, lPhysioHalts, lThreshold_us / 1000));
                if (m_bDebug) TRACE_PUT1(TC_INFO,TF_SEQ,"lPhysioHalts (WIN32)                         %d;",lPhysioHalts);
            }
            else
            {
                //if (m_bDebug) TRACE_PUT0(TC_INFO,TF_SEQ,"ESTIMATED MEASUREMENT TIME NOT PREPARED");
                rSeqExpo.setEstimatedMeasureTimeUsec (0);        
            }
#endif        
            if (m_bDebug) TRACE_PUT1(TC_INFO,TF_SEQ,"Estimated measurement time %f msec;",rSeqExpo.getEstimatedMeasureTimeUsec() / 1000.);

            //  -----------------------
            //  Number of relevant ADCs
            //  -----------------------
            //  This forces the first ADC following each trigger to be set as relevant
            //  for measurement time calculations.  However, SeqLoopNav will subsequently
            //  disable this setting for repeated scans.
            m_pRunLoopNav->setTimeBetweenRelevantADCUsec (0);

            //  This informs the measurement system how many relevant ADCs there will be.
            rSeqExpo.setRelevantReadoutsForMeasTime(lNumberOfTriggerHaltsTotal-lNumberOfTriggerHaltsPreparingScans);
            if (m_bDebug) TRACE_PUT1(TC_INFO,TF_SEQ,"lNumberOfTriggerHaltsTotal          %d;",lNumberOfTriggerHaltsTotal);
            if (m_bDebug) TRACE_PUT1(TC_INFO,TF_SEQ,"lNumberOfTriggerHaltsPreparingScans %d;",lNumberOfTriggerHaltsPreparingScans);

            if(rMrProt.reconstructionMode() == SEQ::RECONMODE_PSIR)
            {
                // Nav is playing out more relevant readouts in case of PSIR
                rSeqExpo.setRelevantReadoutsForMeasTime(2* rSeqExpo.getRelevantReadoutsForMeasTime());
#if !defined VXWORKS                                                                                           
                SeqUT.setExpectedPhysioHalts(2*SeqUT.getExpectedPhysioHalts());
#endif
            }
            
            // Provide the number of relevant readouts to the SeqLoop class
            m_pRunLoopNav->setRelevantReadoutsForMeasTime(rSeqExpo.getRelevantReadoutsForMeasTime());
            if (m_bDebug) TRACE_PUT1(TC_INFO,TF_SEQ,"RelevantReadoutsForMeasTime %d;",rSeqExpo.getRelevantReadoutsForMeasTime());

            //  ------------------------------------------------------
            //  Update the RF energy exports to include the NAV pulses
            //  ------------------------------------------------------
            MrProtocolData::SeqExpoRFInfo totalNavRFInfo = lNumberOfTriggerHaltsTotal * m_NavRFInfoPerRequest;

            if (m_bDebug)
            {
                TRACE_PUT2(TC_INFO,TF_SEQ,"%s: RFEnergyInSequence_Ws (orig) %12.6f Ws",ptModule,originalRFInfoInSequence.getPulseEnergyWs());
                TRACE_PUT2(TC_INFO,TF_SEQ,"%s: m_NavRFInfoPerRequest.getPulseEnergyWs()    %12.6f Ws",ptModule,m_NavRFInfoPerRequest.getPulseEnergyWs());
                TRACE_PUT2(TC_INFO,TF_SEQ,"%s: lNumberOfTriggerHaltsTotal  %12.6f   ",ptModule,(double)lNumberOfTriggerHaltsTotal);
                TRACE_PUT2(TC_INFO,TF_SEQ,"%s: totalNavRFInfo.getPulseEnergyWs()             %12.6f Ws",ptModule,totalNavRFInfo.getPulseEnergyWs());
            }

            // CHARM 382011
            // reset rSeqExpo buffer (might still contain multiple energy blocks from previous call of fSeqPrep() in binary search !!!)
#if defined __A_TSE_VFL_NAV
            if (rMrProt.NavigatorParam().getalFree()[Label_CheckBox_PrepScan] == Value_CheckBox_On)
                rSeqExpo.resetAllRFBlockInfos();     // note: return status MUST NOT be checked in this case: will return false, if no energy blocks available (that's the general case at this point!)
#endif

            rSeqExpo.setRFInfo(originalRFInfoInSequence + totalNavRFInfo);

            if (m_bDebug)
            {
                TRACE_PUT2(TC_INFO,TF_SEQ,"%s: RFEnergyInSequence_Ws (new)  %12.6f Ws",ptModule,rSeqExpo.getRFEnergyInSequence_Ws());
            }

#if defined __A_TSE_VFL_NAV
			// -----------
			// Additional support for RFSWD-Lookahead (charm 361972):
			// The RFSWD assumes, that rf-energy is constant during scan time.
			// In particular for short scan times (e.g. 20s), the 1.5s introduction without energy may cause the short time SAR limit (within 10s)
			// to exceed the limit.
			// In case of multiple concatenation: if number of slices is not an integer multiple of number of concats,
			// some concats will have more slices and thus higher energy deposition.
			// => To avoid this the RFSWD lookahead can be provided with different timing blocks containing different energy levels.
			MrProtocolData::SeqExpoRFInfo rfInfoAllSBBs =	originalRFInfoInSequence + totalNavRFInfo;
			if( rMrProt.intro() )
			{
				// first, reset all blocks
				// note: this also deletes the previous pSeqExpo->setRFInfo()
				//       (setRFInfo() is equivalent to specifing one single block)
				if (! rSeqExpo.resetAllRFBlockInfos() )
				{
					TRACE_PUT1(TC_ALWAYS, TF_SEQ,"%s: rSeqExpo.resetAllRFBlockInfos()  failed", ptModule);
					return SEQU_ERROR;
				}

				// specify two blocks: one contains the introduction interval without RF, the other contains the actual scanning period
				MeasNucleus myMeasNucleus(rMrProt.txSpec().nucleusInfoArray()[0].gettNucleus().c_str());

				double dMeasTimeSec_Block1 = 0.0;
				double dMeasTimeSec_Block2 = 0.0;
				double dMeasTimeSec_Block3 = 0.0;
				MrProtocolData::SeqExpoRFInfo rfInfo_Block1;
				MrProtocolData::SeqExpoRFInfo rfInfo_Block2;
				MrProtocolData::SeqExpoRFInfo rfInfo_Block3;
				bool   bTrace = false || IS_TR_INT(rSeqLim);     // tracing for debugging

				// Block 1: intro time without RF
				if( rMrProt.intro() )
				{
					dMeasTimeSec_Block1 = m_pRunLoopNav->getTokTokTokTime() * 1e-06;    // convert us to seconds!
				}
				if( m_pRunLoopNav->getPerformNoiseMeas() )
				{
					dMeasTimeSec_Block1 += m_pRunLoopNav->getNoiseMeasTime() * 1e-06;
				}

				// Block 3: remaining time, contains remaining RF
				dMeasTimeSec_Block3 = (m_pRunLoopNav->getTotalMeasTimeUsec(rMrProt, rSeqLim) * 1e-06) - dMeasTimeSec_Block2 - dMeasTimeSec_Block1;
				rfInfo_Block3    = rfInfoAllSBBs                                               -    rfInfo_Block2 -    rfInfo_Block1;

				// export energy blocks
				// Block1
				if ( (dMeasTimeSec_Block1 > 0) && ! rSeqExpo.addRFBlockInfo (dMeasTimeSec_Block1,                 // Duration in sec.
					SeqExpoRFBlockInfo::VALUETYPE_ACTUAL,
					rMrProt.txSpec().nucleusInfoArray()[0].gettNucleus().c_str(),
					rfInfo_Block1,                    // Energy
					SeqExpoRFBlockInfo::VALUETYPE_ACTUAL) )
				{
					TRACE_PUT1(TC_ALWAYS, TF_SEQ,"%s: rSeqExpo.addRFBlockInfo 1 failed  ", ptModule);
					return SEQU_ERROR;
				}
				// debug traces
				if( bTrace && rSeqLim.isContextNormal() )
				{
					TRACE_PUT0(TC_ALWAYS, TF_SEQ,"---Block1---");
					TRACE_PUT2(TC_ALWAYS, TF_SEQ,"dMeasTimeSec_Block1: %f, rfInfo_Block1.getPulseEnergyWs(): %f", dMeasTimeSec_Block1, rfInfo_Block1.getPulseEnergyWs());
				}

				// Block2
				if ( (dMeasTimeSec_Block2 > 0) && ! rSeqExpo.addRFBlockInfo (   dMeasTimeSec_Block2,                 // Duration in sec.
																				SeqExpoRFBlockInfo::VALUETYPE_ACTUAL,
																				rMrProt.txSpec().nucleusInfoArray()[0].gettNucleus().c_str(),
																				rfInfo_Block2,                    // Energy
																				SeqExpoRFBlockInfo::VALUETYPE_ACTUAL) )
				{
					TRACE_PUT1(TC_ALWAYS, TF_SEQ,"%s: rSeqExpo.addRFBlockInfo 2 failed  ", ptModule);
					return SEQU_ERROR;
				}
				// debug traces
				if( bTrace && rSeqLim.isContextNormal() )
				{
					TRACE_PUT0(TC_ALWAYS, TF_SEQ,"---Block2---");
					TRACE_PUT2(TC_ALWAYS, TF_SEQ,"dMeasTimeSec_Block2: %f, rfInfo_Block2.getPulseEnergyWs(): %f", dMeasTimeSec_Block2, rfInfo_Block2.getPulseEnergyWs());
				}

				// Block3
				{
					// export Block3 without separate concats
					if ( (dMeasTimeSec_Block3 > 0) && ! rSeqExpo.addRFBlockInfo (   dMeasTimeSec_Block3,                 // Duration in sec.
																					SeqExpoRFBlockInfo::VALUETYPE_ACTUAL,
																					rMrProt.txSpec().nucleusInfoArray()[0].gettNucleus().c_str(),
																					rfInfo_Block3,                    // Energy
																					SeqExpoRFBlockInfo::VALUETYPE_ACTUAL) )
					{
						TRACE_PUT1(TC_ALWAYS, TF_SEQ,"%s: rSeqExpo.addRFBlockInfo 3 failed  ", ptModule);
						return SEQU_ERROR;
					}
					// debug traces
					if( bTrace && rSeqLim.isContextNormal() )
					{
						TRACE_PUT0(TC_ALWAYS, TF_SEQ,"---Block3---");
						TRACE_PUT2(TC_ALWAYS, TF_SEQ,"dMeasTimeSec_Block3: %f, rfInfo_Block3.getPulseEnergyWs(): %f", dMeasTimeSec_Block3, rfInfo_Block3.getPulseEnergyWs());
					}
				}

				// finally check that sum of exported blocks are equal to exported total measurement time
				if( fabs(dMeasTimeSec_Block1 + dMeasTimeSec_Block2 + dMeasTimeSec_Block3 - (m_pRunLoopNav->getTotalMeasTimeUsec(rMrProt, rSeqLim) * 1e-06)) > 1e-6 )
				{
					TRACE_PUT1(TC_ALWAYS, TF_SEQ,"%s: Summed time of exported energy blocks not equal total measurement time", ptModule);
					TRACE_PUT2(TC_ALWAYS, TF_SEQ,"%s: dMeasTimeSec_Block1: %f", ptModule, dMeasTimeSec_Block1);
					TRACE_PUT2(TC_ALWAYS, TF_SEQ,"%s: dMeasTimeSec_Block2: %f", ptModule, dMeasTimeSec_Block2);
					TRACE_PUT2(TC_ALWAYS, TF_SEQ,"%s: dMeasTimeSec_Block3: %f", ptModule, dMeasTimeSec_Block3);
					TRACE_PUT2(TC_ALWAYS, TF_SEQ,"%s: m_pRunLoopNav->getTotalMeasTime [s]: %f", ptModule, m_pRunLoopNav->getTotalMeasTimeUsec(rMrProt, rSeqLim) * 1e-06);
					return SEQU_ERROR;
				}
				if( fabs(rfInfo_Block1.getPulseEnergyWs() + rfInfo_Block2.getPulseEnergyWs() + rfInfo_Block3.getPulseEnergyWs() - rfInfoAllSBBs.getPulseEnergyWs()) > 1e-6 )
				{
					TRACE_PUT1(TC_ALWAYS, TF_SEQ,"%s: Summed energy of exported energy blocks not equal total energy", ptModule);
					TRACE_PUT2(TC_ALWAYS, TF_SEQ,"%s: rfInfo_Block1.getPulseEnergyWs(): %f", ptModule, rfInfo_Block1.getPulseEnergyWs());
					TRACE_PUT2(TC_ALWAYS, TF_SEQ,"%s: rfInfo_Block2.getPulseEnergyWs(): %f", ptModule, rfInfo_Block2.getPulseEnergyWs());
					TRACE_PUT2(TC_ALWAYS, TF_SEQ,"%s: rfInfo_Block3.getPulseEnergyWs(): %f", ptModule, rfInfo_Block3.getPulseEnergyWs());
					TRACE_PUT2(TC_ALWAYS, TF_SEQ,"%s: rfInfoAllSBBs.getPulseEnergyWs(): %f", ptModule, rfInfoAllSBBs.getPulseEnergyWs());
					return SEQU_ERROR;
				}
			}
#endif // __A_TSE_VFL_NAV

            m_bDebug = false;
        }
    }
  
    mTRend;
    return lStatus;
}


//  -----------------------------------------------------------------
//  Implementation of NavigatorShell::check
//  -----------------------------------------------------------------
NLSStatus NavigatorShell::check(MrProt& rMrProt, SeqLim& rSeqLim, SeqExpo& rSeqExpo, SEQCheckMode* pSEQCheckMode)
{
  static const char* const ptModule = "NavigatorShell::check";
  NLS_STATUS lStatus = SEQU_NORMAL;

  mTRrun;
  if (m_bDebug) TRACE_PUT1(TC_INFO,TF_SEQ,"%s: Starting",ptModule);
  
  if (m_pRunLoopNav->m_sNav0.isNavPrepScan())
  {
    //  ----------------------------------------------------------------------------------
    //  In scout mode, the navigators are played out in a private loop in fSEQRun, and the
    //  inner imaging loops are not used.  In this case, we just check 4 executions of the
    //  navigator SBB.
    //  ----------------------------------------------------------------------------------
    for (long lI=0; lI<4; lI++)
    {
      sSLICE_POS* pSlcPos = NULL;

      //  Play out the navigator
      if (!m_pRunLoopNav->m_sNav0.run(rMrProt,rSeqLim,rSeqExpo,pSlcPos))
      {
          Trace (rSeqLim, ptModule, "m_pRunLoopNav->m_sNav0.run() FAILED");
          return(m_pRunLoopNav->m_sNav0.getNLSStatus());
      }
    }
  }
  else
  {
    //  -------------------------------------
    //  Just call the original check function
    //  -------------------------------------
#if defined BASE_SEQUENCE_HAS_OOD
    if (m_bDebug) TRACE_PUT1(TC_INFO, TF_SEQ,"%s: Calling BASE_TYPE::check()",ptModule);
    lStatus = BASE_TYPE::check(rMrProt, rSeqLim, rSeqExpo, pSEQCheckMode);
#else
    if (m_bDebug) TRACE_PUT1(TC_INFO, TF_SEQ,"%s: Calling (*m_pFctSeqCheck)()",ptModule);
    lStatus = (*m_pFctSeqCheck)(rMrProt, rSeqLim, rSeqExpo, pSEQCheckMode);
#endif
    if ( IsSevere(lStatus) )
    {
        Trace (rSeqLim, ptModule, "CHECK method of base sequence FAILED");
        return ( lStatus );
    }
  }
  
  //TRACE_PUT1(TC_INFO,TF_SEQ,"%s: Finished",ptModule);

  mTRend;
  return lStatus;
}

//  -----------------------------------------------------------------
//  Implementation of NavigatorShell::run
//  -----------------------------------------------------------------
NLSStatus NavigatorShell::run(MrProt& rMrProt, SeqLim& rSeqLim, SeqExpo& rSeqExpo)
{
  static const char* const ptModule = "NavigatorShell::run";
  NLS_STATUS lStatus = SEQU_NORMAL;
  sSLICE_POS* pSlcPos = NULL; 

  m_bTraceAlways = true;
  //m_bDebug = true;
  
  mTRrun;
  if (m_bDebug) TRACE_PUT1(TC_INFO, TF_SEQ,"%s: Starting",ptModule);
  
  //  Initialze the relevant ADC counter
  m_pRunLoopNav->setNumberOfRelevantReadoutsDelivered(0);
  
  //  Run the sequence
  if (m_pRunLoopNav->m_sNav0.isNavPrepScan())
  {
    bool bWaitForFeedback = true;
    long lPollInterval_us = rMrProt.NavigatorParam().getalFree()[Label_Long_Arr7_FeedbackPollInterval_ms]*1000;
    long lPause = 1000*m_pRunLoopNav->m_sNav0.getNavPrepTR_ms() -
                       m_pRunLoopNav->m_sNav0.getDurationPerRequest() -
                  1000*m_pRunLoopNav->m_sNav0.getNavSleepDuration_ms();

    double dTimestampOfPreviousRelevantADC_ms = m_pRunLoopNav->m_sNav0.getTimestampAfterNav_ms();
    for (long lI=0; lI<m_lNumberOfPrepLoops; lI++)
    {
      //  Deactivate the ADC for the Prep-Prep loops, and reactivate them when the Prep-Prep loops are done.
      if (m_lNumberOfPrepPrepLoops > 0)
      {
        if (lI==0)
        {
          fRTSetReadoutEnable(0);
          bWaitForFeedback = false;
        }
        else if (lI==m_lNumberOfPrepPrepLoops)
        {
          fRTSetReadoutEnable(1);
          bWaitForFeedback = true;  
        }
      }
      else
      {
        bWaitForFeedback = true;
      }

      // Set the current ADC as relevant
      if ((!m_bTriggering) &&
          (m_pRunLoopNav->m_sNav0.getTimestampAfterNav_ms() - dTimestampOfPreviousRelevantADC_ms) > 
           m_dTimeBetweenRelevantADCs_ms)
      {
        m_pRunLoopNav->m_sNav0.setRelevantADC(true);
        dTimestampOfPreviousRelevantADC_ms = m_pRunLoopNav->m_sNav0.getTimestampAfterNav_ms();
      }
      else
      {
        m_pRunLoopNav->m_sNav0.setRelevantADC(false);
      }
                    
      //  Set a flag for the ICE program on the very last navigator
      if (lI==m_lNumberOfPrepLoops-1)
      {
          if (m_bDebug) TRACE_PUT1(TC_INFO, TF_SEQ,"%s: LAST SCAN is now being played out (SCOUT)",ptModule);
          m_pRunLoopNav->m_sNav0.setNavFlagForLastHPScan();
          //bWaitForFeedback = false;
          //m_pRunLoopNav->m_sNav0.setNavFlagForNoFeedback();
      }
      
      //  Set m_pRunLoopNav->m_lNavPercentComplete
      m_pRunLoopNav->setNavPercentComplete((long) (100.0 * (double)lI/ (double)m_lNumberOfPrepLoops));
      m_pRunLoopNav->m_sNav0.setNavPercentComplete(m_pRunLoopNav->getNavPercentComplete());
      
      //  Reset the semaphore for fSEQReceive, and then set a flag to indicate that we
      //  are ready to receive the next feedback.
      if (!m_pRunLoopNav->m_sNav0.semaphoreRelease(&m_pRunLoopNav->m_sSemaphore))
      {
          Trace (rSeqLim, ptModule, "m_pRunLoopNav->m_sNav0.semaphoreRelease() FAILED");
          return(m_pRunLoopNav->m_sNav0.getNLSStatus());
      }

      if (!m_pRunLoopNav->m_sNav0.semaphoreAcquire(&m_pRunLoopNav->m_sSemaphore))
      {
          Trace (rSeqLim, ptModule, "m_pRunLoopNav->m_sNav0.semaphoreAcquire() FAILED");
          return(m_pRunLoopNav->m_sNav0.getNLSStatus());
      }
      
      //  Play out the navigator
      if (!m_pRunLoopNav->m_sNav0.run(rMrProt,rSeqLim,rSeqExpo,pSlcPos))
      {
          Trace (rSeqLim, ptModule, "m_pRunLoopNav->m_sNav0.run() FAILED");
          return(m_pRunLoopNav->m_sNav0.getNLSStatus());
      }
      
      //  Before checking for feedback, add some delay time to allow for data transfer, processing, etc.
      if (!m_pRunLoopNav->m_sNav0.runPause(1000*m_pRunLoopNav->m_sNav0.getNavSleepDuration_ms(),true))
      {
          Trace (rSeqLim, ptModule, "m_pRunLoopNav->m_sNav0.runPause() FAILED");
          return(m_pRunLoopNav->m_sNav0.getNLSStatus());
      }
      
      //  Wait until the "WAKEUP" code is seen by the DSPs
      if (!m_pRunLoopNav->m_sNav0.waitForWakeup())
      {
          Trace (rSeqLim, ptModule, "m_pRunLoopNav->m_sNav0.waitForWakeup() FAILED");
          return(m_pRunLoopNav->m_sNav0.getNLSStatus());
      }
      
      if (bWaitForFeedback)
      {
          if (m_bDebug) TRACE_PUT1(TC_INFO, TF_SEQ,"%s: WAITING FOR FEEDBACK (SCOUT)",ptModule);
#ifdef WIN32
          {
              //  When the sequence is run in simulation mode, execute fSEQReceive manually, so that the function
              //  can be tested.
              SEQData rSEQData;
              NavigatorFBData sNavigatorFBData;
              
              float fIceResult_1 = 128.;
              
              // Prepare the feedback buffer
              sNavigatorFBData.lNumber = m_pRunLoopNav->m_sNav0.getNavNumber();
              sNavigatorFBData.lCount  = m_pRunLoopNav->m_sNav0.getNavCount();
              sNavigatorFBData.fResult = fIceResult_1;
              
              rSEQData.setID("NAV");
              
              // "Deliver" the feedback
              if( !rSEQData.setData (&sNavigatorFBData, sizeof(sNavigatorFBData)) )
              {
                  Trace (rSeqLim, ptModule, "rSEQData.setData() FAILED");
                  return( SEQU_ERROR );
              }
              
              // "Receive" the feedback
              NLS_STATUS lStatus = receive(rSeqLim,rSeqExpo,rSEQData);
              if ( IsSevere(lStatus) )
              {
                  Trace (rSeqLim, ptModule, "receive() FAILED");
                  return( lStatus );
              }
          }
#endif
        
          //  Wait for the feedback from the ICE program (see fSEQReceive)
          if (!m_pRunLoopNav->m_sNav0.waitForFeedback(&m_pRunLoopNav->m_sSemaphore,lPollInterval_us))
          {
              Trace (rSeqLim, ptModule, "m_pRunLoopNav->m_sNav0.waitForFeedback() FAILED");
              return(m_pRunLoopNav->m_sNav0.getNLSStatus());
          }
      }
      
      //  Add TRFill
      if (!m_pRunLoopNav->m_sNav0.runPause(lPause,false))
      {
          Trace (rSeqLim, ptModule, "m_pRunLoopNav->m_sNav0.runPause() FAILED");
          return(m_pRunLoopNav->m_sNav0.getNLSStatus());
      }
    }
  }
  else
  {
    //  ------------------------------
    //  Just run the original sequence
    //  ------------------------------
#if defined BASE_SEQUENCE_HAS_OOD
    if (m_bDebug) TRACE_PUT1(TC_INFO, TF_SEQ,"%s: Calling BASE_TYPE::run()",ptModule);
    lStatus = BASE_TYPE::run(rMrProt, rSeqLim, rSeqExpo);
#else
    if (m_bDebug) TRACE_PUT1(TC_INFO, TF_SEQ,"%s: Calling (*m_pFctSeqRun)()",ptModule);
    lStatus = (*m_pFctSeqRun)(rMrProt, rSeqLim, rSeqExpo);
#endif
    if ( IsSevere(lStatus) )
    {
        Trace (rSeqLim, ptModule, "RUN method of base sequence FAILED");
        return ( lStatus );
    }
    
    //  --------------------------------------
    //  100% of the data has now been acquired
    //  --------------------------------------
    m_pRunLoopNav->m_sNav0.setNavPercentComplete(100);
    
#ifndef WIN32
    //  Some special steps are needed when RT feedback is selected: a) an adc with the RT feedback flag
    //  is delivered, causing the ice program to flush its profile buffer into the global object; b) a 
    //  second adc is delivered with the HP feedback flag, allowing all remaining global images to be
    //  delivered.
    if (m_pRunLoopNav->m_sNav0.getNavFBMode() == Value_NavFBMode_RT)
    {

      if (m_bDebug) TRACE_PUT1(TC_INFO, TF_SEQ,"%s: LASTRT SCAN is now being played out (IMAGING)",ptModule);
      m_pRunLoopNav->m_sNav0.setNavFlagForNoFeedback();
      m_pRunLoopNav->m_sNav0.setNavFlagForLastRTScan();
      
      //  Play out the navigator and the pause
      if (!m_pRunLoopNav->m_sNav0.run(rMrProt,rSeqLim,rSeqExpo,pSlcPos))
      {
          Trace (rSeqLim, ptModule, "m_pRunLoopNav->m_sNav0.run() FAILED");
          return(m_pRunLoopNav->m_sNav0.getNLSStatus());
      }

      if (!m_pRunLoopNav->m_sNav0.runPause(1000*m_pRunLoopNav->m_sNav0.getNavSleepDuration_ms(),false))
      {
          Trace (rSeqLim, ptModule, "m_pRunLoopNav->m_sNav0.runPause() FAILED");
          return(m_pRunLoopNav->m_sNav0.getNLSStatus());
      }
    }
#endif
    
    /*  Not needed for VB13A
    //  The final navigator is played out only to flush the data out of the ice program.  Since it
    //  executes outside of the fSEQRun function, it cannot be seen by the unit test, and so it is
    //  played out only when the unit test is not active.
    if ( !mIsUnittestActive() )
    {
      TRACE_PUT1(TC_INFO, TF_SEQ,"%s: LASTHP SCAN is now being played out (IMAGING)",ptModule);
      m_pRunLoopNav->m_sNav0.setNavFlagForNoFeedback();
      m_pRunLoopNav->m_sNav0.setNavFlagForLastHPScan();
      
      //  Play out the navigator and the pause
      if (!m_pRunLoopNav->m_sNav0.runIR(rMrProt,rSeqLim,rSeqExpo,pSlcPos))
      {
          Trace (rSeqLim, ptModule, "m_pRunLoopNav->m_sNav0.runIR() FAILED");
          return(m_pRunLoopNav->m_sNav0.getNLSStatus());
      }

      if (!m_pRunLoopNav->m_sNav0.run(rMrProt,rSeqLim,rSeqExpo,pSlcPos))
      {
          Trace (rSeqLim, ptModule, "m_pRunLoopNav->m_sNav0.run() FAILED");
          return(m_pRunLoopNav->m_sNav0.getNLSStatus());
      }

      if (!m_pRunLoopNav->m_sNav0.runPause(1000*m_pRunLoopNav->m_sNav0.getNavSleepDuration_ms(),false))
      {
          Trace (rSeqLim, ptModule, "m_pRunLoopNav->m_sNav0.runPause() FAILED");
          return(m_pRunLoopNav->m_sNav0.getNLSStatus());
      }
    }
    */
    
  }
  
  //  Display some statistics
  if (true) //m_bDebug)  Lets leave this here.
  {
      if ( mIsUnittestActive() ) TRACE_PUT1(TC_INFO,TF_SEQ,"%s UNIT TEST MEASUREMENT COMPLETED\n",ptModule);
      if (m_pRunLoopNav->m_sNav0.isNavPrepScan())
      {
          TRACE_PUT1(TC_INFO,TF_SEQ,"%s PREP SCANS ONLY WERE PERFORMED\n",ptModule);
      }
      else
      {
          TRACE_PUT1(TC_INFO,TF_SEQ,"%s IMAGING WAS PERFORMED\n",ptModule);
      }
      TRACE_PUT1(TC_INFO,TF_SEQ,"%s: Navigator Statistics",ptModule);
      
      char ptNavMode[32];
      strcpy(ptNavMode,"UNKNOWN");
      if (m_pRunLoopNav->m_sNav0.getNavMode() == Value_NavModeProspectiveGating) strcpy(ptNavMode,"ProspectiveGating");
      if (m_pRunLoopNav->m_sNav0.getNavMode() == Value_NavModeFollowSlice)       strcpy(ptNavMode,"FollowSlice");
      if (m_pRunLoopNav->m_sNav0.getNavMode() == Value_NavModeMonitorOnly)       strcpy(ptNavMode,"MonitorOnly");
      if (m_pRunLoopNav->m_sNav0.getNavMode() == Value_NavModeOff)               strcpy(ptNavMode,"OFF");
      TRACE_PUT1(TC_INFO,TF_SEQ,"    Navigator Mode    = %s", ptNavMode);
      TRACE_PUT1(TC_INFO,TF_SEQ,"    Nav duration      = %ld",m_pRunLoopNav->m_sNav0.getTotalDuration_us());
      TRACE_PUT1(TC_INFO,TF_SEQ,"    Nav energy        = %12.6f Ws",m_pRunLoopNav->m_sNav0.getTotalRFInfo().getPulseEnergyWs());
      TRACE_PUT1(TC_INFO,TF_SEQ,"    Nav duration NoIR = %ld",m_pRunLoopNav->m_sNav0.getTotalDuration_ExcludingIR_us());
      TRACE_PUT1(TC_INFO,TF_SEQ,"    Nav energy NoIR   = %12.6f Ws",m_pRunLoopNav->m_sNav0.getTotalRFInfo_ExcludingIR().getPulseEnergyWs());
      TRACE_PUT1(TC_INFO,TF_SEQ,"    Number of calls   = %ld",m_pRunLoopNav->m_sNav0.getNavCountTotal());
      TRACE_PUT1(TC_INFO,TF_SEQ,"    Number of ADCs    = %ld",m_pRunLoopNav->m_sNav0.getNavCount());
      TRACE_PUT1(TC_INFO,TF_SEQ,"    Repeat counter    = %ld",m_pRunLoopNav->getNavRepeatCount(0));
      TRACE_PUT1(TC_INFO,TF_SEQ,"    Min feedback time = %12.3f [ms]",m_pRunLoopNav->m_sNav0.getMinNavFeedbackTime_ms());
      TRACE_PUT1(TC_INFO,TF_SEQ,"    Max feedback time = %12.3f [ms]",m_pRunLoopNav->m_sNav0.getMaxNavFeedbackTime_ms());
  }
  
  if (m_bDebug) TRACE_PUT1(TC_INFO,TF_SEQ,"%s: Finished",ptModule);

  mTRend;
  return lStatus;
}

//  -----------------------------------------------------------------
//  Implementation of NavigatorShell::receive
//  -----------------------------------------------------------------
NLSStatus NavigatorShell::receive(SeqLim& rSeqLim, SeqExpo& rSeqExpo, SEQData& rSEQData)
{
  static const char* const ptModule = "NavigatorShell::receive";
  NLS_STATUS lStatus = SEQU_NORMAL;

  mTRrun;

  //TRACE_PUT1(TC_INFO,TF_SEQ,"%s: HERE I AM",ptModule);
  if (!m_pRunLoopNav->receive(rSeqLim, rSeqExpo, rSEQData))
  {
      Trace (rSeqLim, ptModule, "m_pRunLoopNav->receive() FAILED");
      return(SEQU_ERROR);
  }

  mTRend;
  return lStatus;
}

NLSStatus NavigatorShell::convProt(const MrProt &rMrProtSrc, MrProt &rMrProtDst)
{
    static const char *ptModule = {"NavigatorShell::convProt"};

    NLSStatus lStatus = SEQU__NORMAL;
    //unsigned int ui = 0;
    
    if( rMrProtSrc.getConvFromVersion() < 21310005 )
    {
        
        //  Change of units from sec to msec
        rMrProtDst.NavigatorParam().getalFree()[Label_Long_Arr5_ICEImageSendInterval_Msec] = 400;
    }
    if( rMrProtSrc.getConvFromVersion() < 21510006 )
    {
        //  Sleep time for feedback is always 10 ms.  Previously, it was 10 ms (imaging) and 50 ms (scout)
        rMrProtDst.NavigatorParam().getalFree()[Label_Long_NavSleepDuration_ms] = 10;
    }

#if defined BASE_SEQUENCE_HAS_OOD
    if (m_bDebug) TRACE_PUT1(TC_INFO, TF_SEQ,"%s: Calling BASE_TYPE::convProt()",ptModule);
    lStatus = BASE_TYPE::convProt(rMrProtSrc, rMrProtDst);
#else
    if (m_pFctSeqConvProt)
    {
        if (m_bDebug) TRACE_PUT1(TC_INFO, TF_SEQ,"%s: Calling (*m_pFctSeqConvProt)()",ptModule);
        lStatus = (*m_pFctSeqConvProt)(rMrProtSrc, rMrProtDst);
    }
#endif
        
    return(SEQU__NORMAL);
}

//  -----------------------------------------------------------------
//  Implementation of NavigatorShell::IsSevere
//  -----------------------------------------------------------------
bool NavigatorShell::IsSevere (NLS_STATUS lStatus) const
{
    return ( ( NLS_SEV & lStatus) == NLS_SEV ); 
}


//  -----------------------------------------------------------------
//  Implementation of NavigatorShell::Trace
//  -----------------------------------------------------------------
void NavigatorShell::Trace (SeqLim& rSeqLim, const char* ptModule, const char* ptErrorText) const
{
    if ( (! rSeqLim.isContextPrepForBinarySearch()) || (m_bTraceAlways) )  {
          TRACE_PUT2(TC_ALWAYS, TF_SEQ,"%s: %s", ptModule, ptErrorText);
    }
}


#if defined __A_TRUFI_CV_NAV
// * -------------------------------------------------------------------------- *
// *                                                                            *
// * Name        :  NavigatorShell::i_CreateRunLoop                             *
// *                                                                            *
// * Description :  Performs a "new" on the desired Runloop                     *
// *                                                                            *
// * Return      :  NLS status                                                  *
// *                                                                            *
// * -------------------------------------------------------------------------- *
NLS_STATUS NavigatorShell::i_CreateRunLoop (SeqLim& rSeqLim)
{
    
    static const char *ptModule = {"NavigatorShell::i_CreateRunLoop"};
    mTRrun;
    NLS_STATUS lStatus = SEQU__NORMAL;

    // Call i_CreateRunLoop of the base sequence
    if ( IsSevere (lStatus = BASE_TYPE::i_CreateRunLoop(rSeqLim) ) )  {
        Trace (rSeqLim, ptModule, "Initialization failed: BASE_TYPE::i_CreateRunLoop(...)");
        return ( lStatus );
    }

    // Create instance of SeqLoopNav
    m_pRunLoop = NULL;

    delete m_pRunLoopNav;
    m_pRunLoopNav = NULL;

    try 
    {
        m_pRunLoopNav = new SeqLoopNav();
        //TRACE_PUT2(TC_ALWAYS, TF_SEQ,"%s: Address of SeqLoopNav %8.8p",ptModule,m_pRunLoopNav);
    }
    catch (...)  
    {
        m_pRunLoopNav = NULL;
        TRACE_PUT1(TC_ALWAYS, TF_SEQ,"%s: Cannot instantiate m_pRunLoopNav !", ptModule);
        return ( SEQU_ERROR );
    }

    lStatus = chooseRunLoop((LocalSeqLoopCV *) m_pRunLoopNav, rSeqLim);
    if ( IsSevere(lStatus) )  {
        Trace (rSeqLim, ptModule, "choosing of m_pRunLoopNav failed");
        return ( lStatus );
    }

    if ( IsSevere (lStatus = i_InitializeRunLoop(rSeqLim) ) )  {
        Trace (rSeqLim, ptModule, "Initialization (m_pRunLoopNav) failed: i_InitializeRunLoop(...)");
        return ( lStatus );
    }

    mTRend;
    return SEQU_NORMAL;
    
}

// * -------------------------------------------------------------------------- *
// *                                                                            *
// * Name        :  NavigatorShell::p_Select                                          *
// *                                                                            *
// * Description :  Selects Kernel, ReorderInfo and RunLoop                     *
// *                                                                            *
// * Return      :  NLS status                                                  *
// *                                                                            *
// * -------------------------------------------------------------------------- *
NLS_STATUS NavigatorShell::p_Select (MrProt& rMrProt, SeqLim& rSeqLim, SeqExpo& rSeqExpo)
{
    static const char *ptModule = {"NavigatorShell::p_Select"};
    NLS_STATUS   lStatus = SEQU__NORMAL;
    mTRrun;

    if ( IsSevere (lStatus = BASE_TYPE::p_Select(rMrProt, rSeqLim, rSeqExpo) ) )  
    {
        Trace (rSeqLim, ptModule, "Initialization failed: BASE_TYPE::p_Select(...)");
        return ( lStatus );
    }

    if (IsSevere( chooseRunLoop( (LocalSeqLoopCV *) m_pRunLoopNav, rSeqLim) )) {
        TRACE_PUT1(TC_ALWAYS, TF_SEQ,"%s: choosing SeqLoop m_pRunLoopNav failed!", ptModule);
        return ( lStatus );
    }

    // Check, if every pointer is valid
    if ((! m_pReorder)   || (! m_pRunLoop)   || (! m_pKernel) || (! m_pUIContainer) ||
        (! m_pCVReorder) || (! m_pCVRunLoop) || (! m_pCVKernel) )
    {
        if (!m_pReorder)     Trace (rSeqLim, ptModule, "NULL pointer m_pReorder found"); 
        if (!m_pRunLoop)     Trace (rSeqLim, ptModule, "NULL pointer m_pRunLoop found");
        if (!m_pKernel)      Trace (rSeqLim, ptModule, "NULL pointer m_pKernel found");
        if (!m_pCVReorder)   Trace (rSeqLim, ptModule, "NULL pointer m_pCVReorder found"); 
        if (!m_pCVRunLoop)   Trace (rSeqLim, ptModule, "NULL pointer m_pCVRunLoop found");
        if (!m_pCVKernel)    Trace (rSeqLim, ptModule, "NULL pointer m_pCVKernel found");
        if (!m_pUIContainer) Trace (rSeqLim, ptModule, "NULL pointer m_pUIContainer found"); 

        return ( SEQU_ERROR );
    }

    m_pCVRunLoop->setpCppSequence ( this );
    
    
    mTRend;
    return ( lStatus );

}

// * -------------------------------------------------------------------------- *
// *                                                                            *
// * Name        :  NavigatorShell:p_SeqLoopMisc                                *
// *                                                                            *
// * Description :  Sets miscellaneous parameters of SeqLoop                    *
// *                                                                            *
// * Return      :  NLS status                                                  *
// *                                                                            *
// * -------------------------------------------------------------------------- *
NLS_STATUS NavigatorShell::p_SeqLoopMisc (MrProt& rMrProt, SeqLim& rSeqLim, SeqExpo& rSeqExpo)
{

    static const char *ptModule = {"NavigatorShell::p_SeqLoopMisc"};
    mTRrun;

    NLS_STATUS   lStatus = SEQU__NORMAL;

    // Call p_SeqLoopMisc of the base class
    if ( IsSevere (lStatus = BASE_TYPE::p_SeqLoopMisc(rMrProt, rSeqLim, rSeqExpo) ) )  {
        Trace (rSeqLim, ptModule, "Configuration of RunLoop failed: BASE_TYPE::p_SeqLoopMisc(...)");
        return ( lStatus );
    }

    // Override trig halt mode -> enforce NO single shot.
    m_pCVRunLoop->setTrigHaltSingleShot(false);

    // inform runLoopNav about the PSIR settings that changes the loop behavior
    m_pRunLoopNav->setIsPSIR (rMrProt.reconstructionMode() == SEQ::RECONMODE_PSIR);

    return ( lStatus );
}


// * -------------------------------------------------------------------------- *
// *                                                                            *
// * Name        :  NavigatorShell:p_KernelParameter                            *
// *                                                                            *
// * Description :  Sets the parameter that are required to calculate the       *
// *                kernel timing                                               *
// *                                                                            *
// * Return      :  NLS status                                                  *
// *                                                                            *
// * -------------------------------------------------------------------------- *
NLS_STATUS NavigatorShell::p_KernelParameter (MrProt& rMrProt, SeqLim& rSeqLim, SeqExpo& rSeqExpo)
{

    static const char *ptModule = {"NavigatorShell::p_KernelParameter"};
    mTRrun;

    NLS_STATUS   lStatus = SEQU__NORMAL;

    // Call p_KernelParameter of the base class
    if ( IsSevere (lStatus = BASE_TYPE::p_KernelParameter(rMrProt, rSeqLim, rSeqExpo) ) )  {
        Trace (rSeqLim, ptModule, "Setting of kernel parameters failed: BASE_TYPE::p_KernelParameter(...)");
        return ( lStatus );
    }

    // set the readout gradient type to constant to reduce the echo-spacing
    if ((IS_CV_CINE_OFF) && (IS_CV_CONTRAST_FLASH) && (rMrProt.kSpace().dimension() == SEQ::DIM_3))
    {
        m_pCVKernel->seteReadOutGradType     ( SBBGREKernel::Constant);
    }

    return ( lStatus );
}


// * -------------------------------------------------------------------------- *
// *                                                                            *
// * Name        :  NavigatorShell:rk_ExecKernel                                *
// *                                                                            *
// * Description :  Sets the parameter that are required to calculate the       *
// *                kernel timing                                               *
// *                                                                            *
// * Return      :  NLS status                                                  *
// *                                                                            *
// * -------------------------------------------------------------------------- *
NLS_STATUS NavigatorShell::rk_ExecKernel ( MrProt &rMrProt, SeqLim &rSeqLim, SeqExpo &rSeqExpo, long &lKernelMode, long &lSlice, long &lPartition, long &lLine )
{
    static const char *ptModule = {"NavigatorShell::rk_ExecKernel"};
    mTRrun;

    NLS_STATUS   lStatus = SEQU__NORMAL;

    // Transfer nav info from SeqLoop to Kernel
    lStatus = TransferNavInfoFromSeqLoopToKernel();
    if ( IsSevere (lStatus) ) {
        Trace (rSeqLim, ptModule, "TransferNavInfoFromSeqLoopToKernel(...) failed");
        return ( lStatus );
    }

    // No relevant ADCs are ever delivered by the imaging sequence (only do this if navigators are active)
    const bool wasRelevant = m_pCVKernel->ADC(0).isRelevantForMeasTime ();
    if(rMrProt.NavigatorParam().getlRespComp() != SEQ::RESP_COMP_OFF)
    {
        m_pCVKernel->ADC(0).setRelevantForMeasTime (false);
    }

    // Call rk_ExecKernel of the base class
    if ( IsSevere (lStatus = BASE_TYPE::rk_ExecKernel(rMrProt, rSeqLim, rSeqExpo, lKernelMode, lSlice, lPartition, lLine) ) )  {
        Trace (rSeqLim, ptModule, "BASE_TYPE::rk_ExecKernel(...) failed");
        return ( lStatus );
    }

    // Restore relevant ADC flag (only do this if navigators are active)
    if(rMrProt.NavigatorParam().getlRespComp() != SEQ::RESP_COMP_OFF)
    {    
        m_pCVKernel->ADC(0).setRelevantForMeasTime (wasRelevant);
    }

    return (lStatus);
}

NLS_STATUS NavigatorShell::runPreScans(MrProt &rMrProt,SeqLim &rSeqLim, SeqExpo &rSeqExpo, long lKernelMode, long lSlice, long lPartition, long lLine)
{
    static const char *ptModule  = {"NavigatorShell::runPreScans"};
    NLS_STATUS         lStatus   = SEQU__NORMAL;

    // No relevant ADCs are ever delivered by the imaging sequence (only do this if navigators are active)  
    const bool wasRelevant = m_pCVKernel->ADC(0).isRelevantForMeasTime ();
    if(rMrProt.NavigatorParam().getlRespComp() != SEQ::RESP_COMP_OFF)
    {    
        m_pCVKernel->ADC(0).setRelevantForMeasTime (false);
    }

    // Transfer nav info from SeqLoop to Kernel
    lStatus = TransferNavInfoFromSeqLoopToKernel();
    if ( IsSevere (lStatus) ) {
        Trace (rSeqLim, ptModule, "TransferNavInfoFromSeqLoopToKernel(...) failed");
        return ( lStatus );
    }

    // Call runPreScans of the base class
    if ( IsSevere (lStatus = BASE_TYPE::runPreScans(rMrProt, rSeqLim, rSeqExpo, lKernelMode, lSlice, lPartition, lLine) ) )  {
        Trace (rSeqLim, ptModule, "BASE_TYPE::runPreScans(...) failed");
        return ( lStatus );
    }

    // Restore relevant ADC flag (only do this if navigators are active)
    if(rMrProt.NavigatorParam().getlRespComp() != SEQ::RESP_COMP_OFF)
    {
	    m_pCVKernel->ADC(0).setRelevantForMeasTime (wasRelevant);
    }
    return (lStatus);
}

NLS_STATUS NavigatorShell::runPostScans(MrProt &rMrProt,SeqLim &rSeqLim, SeqExpo &rSeqExpo, long lKernelMode, long lSlice, long lPartition, long lLine)
{
    static const char *ptModule  = {"NavigatorShell::runPostScans"};
    NLS_STATUS         lStatus   = SEQU__NORMAL;

    // No relevant ADCs are ever delivered by the imaging sequence (only do this if navigators are active)
    const bool wasRelevant = m_pCVKernel->ADC(0).isRelevantForMeasTime ();
    if(rMrProt.NavigatorParam().getlRespComp() != SEQ::RESP_COMP_OFF)
    {
         m_pCVKernel->ADC(0).setRelevantForMeasTime (false);
    }

    // Transfer nav info from SeqLoop to Kernel
    lStatus = TransferNavInfoFromSeqLoopToKernel();
    if ( IsSevere (lStatus) ) {
        Trace (rSeqLim, ptModule, "TransferNavInfoFromSeqLoopToKernel(...) failed");
        return ( lStatus );
    }

    // Call runPostScans of the base class
    if ( IsSevere (lStatus = BASE_TYPE::runPostScans(rMrProt, rSeqLim, rSeqExpo, lKernelMode, lSlice, lPartition, lLine) ) )  {
        Trace (rSeqLim, ptModule, "BASE_TYPE::runPostScans(...) failed");
        return ( lStatus );
    }

    // Restore relevant ADC flag (only do this if navigators are active)
    if(rMrProt.NavigatorParam().getlRespComp() != SEQ::RESP_COMP_OFF)
    {
        m_pCVKernel->ADC(0).setRelevantForMeasTime (wasRelevant);
    }

    return (lStatus);
}


#endif

#if defined __A_TRUFI_CV_NAV

NLS_STATUS NavigatorShell::TransferNavInfoFromSeqLoopToKernel(void)
{
    static const char *ptModule  = {"NavigatorShell::TransferNavInfoFromSeqLoopToKernel"};
    NLS_STATUS         lStatus   = SEQU__NORMAL;

    if (m_pRunLoopNav->TraceNavFeedback())
    {
        TRACE_PUT3(TC_INFO,TF_SEQ,"%s: IsValidNav %s; ShiftAmountCorrected %8.3f",
            ptModule,(m_pRunLoopNav->ValidNavShiftVector()?"True":"False"),m_pRunLoopNav->getNavShiftAmountCorrected());
    }

    // Transfer Nav slice shift info from SeqLoopNav into the Kernel
    m_pCVKernel->setValidNavShiftVector(m_pRunLoopNav->ValidNavShiftVector());
    m_pCVKernel->setNavShiftAmountCorrected(m_pRunLoopNav->getNavShiftAmountCorrected());
    m_pCVKernel->setNavShiftVector(m_pRunLoopNav->getNavShiftVector());

    // Debug flags
    m_pCVKernel->setTraceNavFeedback(m_pRunLoopNav->TraceNavFeedback());

    return (lStatus);
}

#endif
