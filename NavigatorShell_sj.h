#ifndef __NavigatorShell_H
#define __NavigatorShell_H 1

// For special simulations
#define DO_NOT_PERTURB_SIMULATION
//#define LOCAL_UNIT_TEST

#include "MrServers/MrProtSrv/MrProt/MrProt.h"

#include "MrServers/MrMeasSrv/SeqIF/SeqBuffer/SeqLim.h"
#include "MrServers/MrMeasSrv/SeqIF/libRT/sSYNC.h"
#include "MrServers/MrMeasSrv/SeqIF/libRT/SEQSemaphore.h"
#include "MrServers/MrMeasSrv/SeqIF/csequence.h"

#include "MrServers/MrImaging/seq/a_CV_nav_VE11/NavUI_sj.h"
#include "MrServers/MrImaging/seq/a_CV_nav_VE11/SeqLoopNav_sj.h"
#include "MrServers/MrImaging/seq/a_CV_nav_VE11/SBBNavigator_sj.h"
#include "MrServers/MrImaging/seq/common/Nav/NavigatorICEProgDef.h"

#if defined __A_TRUFI_CV_NAV

    // OOD sequence trufi_cv_nav
    #include "MrServers/MrImaging/seq/a_trufi_cv.h"
    typedef SEQ_NAMESPACE::Trufi_cv INHERITED_SEQUENCE_BASE_TYPE;
    #define BASE_SEQUENCE_HAS_OOD
    #pragma message (__FILE__": BASE_SEQUENCE_HAS_OOD Trufi_cv")

#elif defined __A_FL3D_CE_NAV

    // OOD sequence fl3d_ce
    #include "MrServers/MrImaging/seq/a_twist.h"
    typedef SEQ_NAMESPACE::cTwist INHERITED_SEQUENCE_BASE_TYPE;
    #define BASE_SEQUENCE_HAS_OOD
    #pragma message (__FILE__": BASE_SEQUENCE_HAS_OOD cTwist")

#elif defined __A_TSE_NAV

    // OOD sequence tse
    #include "MrServers/MrImaging/seq/a_tse.h"
    typedef SEQ_NAMESPACE::Tse INHERITED_SEQUENCE_BASE_TYPE;
    #define BASE_SEQUENCE_HAS_OOD
    #pragma message (__FILE__": BASE_SEQUENCE_HAS_OOD Tse")

#elif defined __A_TSE_VFL_NAV

    // OOD sequence tse_vfl
    #include "MrServers/MrImaging/seq/a_tse_vfl.h"
    typedef SEQ_NAMESPACE::Tse_vfl INHERITED_SEQUENCE_BASE_TYPE;
    #define BASE_SEQUENCE_HAS_OOD
    #pragma message (__FILE__": BASE_SEQUENCE_HAS_OOD Tse_vfl")

#endif

//  -----------------------------------------------------------------
//  Defintion of sequence class
//  -----------------------------------------------------------------
namespace SEQ_NAMESPACE
{
#if defined BASE_SEQUENCE_HAS_OOD
class NavigatorShell : public INHERITED_SEQUENCE_BASE_TYPE
#else
class NavigatorShell
#endif
{

public:

    //  Default Constructor
    NavigatorShell();

    //  Destructor
    ~NavigatorShell();

#if defined BASE_SEQUENCE_HAS_OOD

    typedef INHERITED_SEQUENCE_BASE_TYPE BASE_TYPE;
    typedef NavigatorShell MY_TYPE;

#endif

    //  Sequence entry points
    virtual NLSStatus initialize(SeqLim &rSeqLim);
    virtual NLSStatus prepare(MrProt &rMrProt, SeqLim &rSeqLim, SeqExpo &rSeqExpo);
    virtual NLSStatus check(MrProt &rMrProt, SeqLim &rSeqLim, SeqExpo &rSeqExpo, SEQCheckMode* pSEQCheckMode);
    virtual NLSStatus run(MrProt &rMrProt, SeqLim &rSeqLim, SeqExpo &rSeqExpo);
    virtual NLSStatus convProt(const MrProt &rMrProtSrc, MrProt &rMrProtDst);
    virtual NLSStatus receive(SeqLim&, SeqExpo&, SEQData& rSEQData);

    //  Diagnostics
    bool IsSevere (NLS_STATUS lStatus) const;
    void Trace (SeqLim& rSeqLim, const char* ptModule, const char* ptErrorText) const;

    //  Overloaded methods of specific sequences
#if defined __A_TRUFI_CV_NAV

    virtual NLS_STATUS i_CreateRunLoop (SeqLim &rSeqLim);
    virtual NLS_STATUS p_Select (MrProt &rMrProt, SeqLim &rSeqLim, SeqExpo &rSeqExpo);
    virtual NLS_STATUS p_SeqLoopMisc (MrProt &rMrProt, SeqLim &rSeqLim, SeqExpo &rSeqExpo);
    virtual NLS_STATUS p_KernelParameter (MrProt &rMrProt, SeqLim &rSeqLim, SeqExpo &rSeqExpo);
    virtual NLS_STATUS rk_ExecKernel ( MrProt &rMrProt, SeqLim &rSeqLim, SeqExpo &rSeqExpo, long &, long &, long &, long &);

    virtual NLS_STATUS runPreScans (MrProt &rMrProt,SeqLim &rSeqLim, SeqExpo &rSeqExpo, long lKernelMode, long lSlice, long lPartition, long lLine);
    virtual NLS_STATUS runPostScans(MrProt &rMrProt,SeqLim &rSeqLim, SeqExpo &rSeqExpo, long lKernelMode, long lSlice, long lPartition, long lLine);

#endif

    // New methods of specific sequences
#if defined __A_TRUFI_CV_NAV

    virtual NLS_STATUS TransferNavInfoFromSeqLoopToKernel(void);

#endif

    //  Special UI components
#ifdef WIN32
    NavUI  m_sNavUI;
    NavUI* m_pNavUI;
#endif

protected:

    //  Diagnostics
    bool m_bDebug;
#if !defined __A_TRUFI_CV_NAV
    bool m_bTraceAlways;
#endif

    //  Pointer to SeqLoopNav
    SeqLoopNav* m_pRunLoopNav;
    //SeqLoopNav* m_pRunLoop;

    //  Number of times the Navigator 0 is executed in prep mode (not executed by SeqLoop)
    long m_lNumberOfPrepLoops;
    long m_lNumberOfPrepPrepLoops;

    //  Number of times the Navigator 0 is executed in imaging mode (SeqLoop)
    long m_lNumberOfTriggerHalts;

    //  Duration and energy of the navigators, per request
    long   m_lNavDurationPerRequest_us;
    MrProtocolData::SeqExpoRFInfo m_NavRFInfoPerRequest;

    //  Triggering active
    bool   m_bTriggering;

    //  Relevant ADCs
    long   m_lNumberOfRelevantADCsForPrep;
    double m_dTimeBetweenRelevantADCs_ms;

private:

    //  Pointers to entry points of the related original sequence
    //  (required only if the original sequence is NOT OOD)
#if !defined BASE_SEQUENCE_HAS_OOD

    FUNCSEQINIT     *m_pFctSeqInit;
    FUNCSEQPREP     *m_pFctSeqPrep;
    FUNCSEQCHECK    *m_pFctSeqCheck;
    FUNCSEQRUN      *m_pFctSeqRun;
    FUNCSEQCONVPROT *m_pFctSeqConvProt;

#endif


};
} // end namespace

#endif


