//    -----------------------------------------------------------------------------
//      Copyright (C) Siemens AG 1998  All Rights Reserved.
//    -----------------------------------------------------------------------------
//
//     Project: NUMARIS/4
//        File: \n4\pkg\MrServers\MrImaging\seq\Kernels\SBBTRUFICVKernel.h
//     Version: \main\45
//      Author: Clinical
//        Date: 2013-02-27 18:47:00 +01:00
//
//        Lang: C++
//
//     Descrip: MR::Measurement::CSequence::libSeqUtil
//
//     Classes:
//
//    -----------------------------------------------------------------------------

#ifndef SBBTRUFICVKernel_h
#define SBBTRUFICVKernel_h 1

#include "MrServers/MrProtSrv/MrProt/MrSlice.h"
#include "MrServers/MrImaging/seq/Kernels/SBBTRUFIKernel.h"

#include "MrServers/MrImaging/libSBB/SBBBinomialPulses.h"
#include "MrServers/MrImaging/libSBB/SBBPulseSequel.h"

#ifdef BUILD_SEQU
#define __OWNER
#endif
#include "MrCommon/MrGlobalDefinitions/ImpExpCtrl.h"

#define MAX_NO_VFL 512
#define MAX_NO_PSIRPulses  4    // ycc

namespace SEQ_NAMESPACE
{


// class containing information for RF spoiling calculation
class RFSpoilParams
{
public:
    RFSpoilParams();
    ~RFSpoilParams();

    NLS_STATUS getRFPhase(double & SpoilPhase, MrProt &rMrProt,SeqLim &rSeqLim, bool isbFirstSliceInConcat, long lSlice);

protected:
    double dPhase;
    double dPhasePrevSlice;
    double dIncrement;
    double dIncrementPrevSlice;
    double dPrevSlicePosSag;
    double dPrevSlicePosCor;
    double dPrevSlicePosTra;
    double dPrevSliceNormalSag;
    double dPrevSliceNormalCor;
    double dPrevSliceNormalTra;
    double dRFSPOIL_INCREMENTdeg;
};


// kernel class
class __IMP_EXP SBBTRUFICVKernel : public SBBTRUFIKernel
{
public:

    // constructor for kernel configured for TRUFI contrast
    SBBTRUFICVKernel (SBBList* pSBBList);

    // destructor
    virtual ~SBBTRUFICVKernel();

    // take system and heuristic values to set correction values for time shifts
    void initClockADCShiftAndROGradShiftCorrectionValues();

    void switchToGREContrast();

    // * ------------------------------------------------------------------ *
    // * overloaded members of SBBTrufiKernel and SBBGREKernel              *
    // * ------------------------------------------------------------------ *

    virtual bool updateGradPerf (enum SEQ::Gradients eGradientMode);

    /// Resets members calculated in calculateTiming.
    virtual bool calculateInit (MrProt&, SeqLim&, SeqExpo&);

    /// Adds the preparation of variable flip angles (VFL) to the calculation of the Kernel excitation pulses.
    virtual bool calculatePrepWEPulse (MrProt &rMrProt, SeqLim &rSeqLim, SeqExpo &rSeqExpo);

    virtual bool calculateVarRFPrepScanFlipAngles();

    // bool calculateTiming (MrProt* pMrProt, SeqLim* pSeqLim, SeqExpo* pSeqExpo);
    virtual bool calculateAddOnPost (MrProt &rMrProt, SeqLim &rSeqLim, SeqExpo &rSeqExpo);

    // These methods differ for SBBTRUFIKernel and SBBGREKernel and are switched in the CVKernel
    // For explanation see SBBGREKernel and SBBTRUFIKernel.
    virtual bool calculateTEFill (MrProt &rMrProt);
    virtual bool calculateTRMin  (MrProt &rMrProt);

    /// This funtion returns the minimal possible TE time in us for each contrast, including the
    ///  TEFill time introduced in case of TrueFisp-Accoustic-Resonance avoidance.
    /// Hence, can be sightly higher than m_alTEMin[iIndex]
    virtual long getlMinimalKernelTE (int iIndex = 0) const;


    virtual bool calculateOverTime (MrProt &rMrProt, SeqLim &rSeqLim);
    virtual bool calculateRORampDownTime (MrProt &rMrProt, SeqLim &rSeqLim, SeqExpo &rSeqExpo);


    virtual bool runPreScan  (MrProt &rMrProt, SeqLim &rSeqLim, SeqExpo &rSeqExpo, sSLICE_POS* pSLC);
    
    virtual bool prepPostScan (MrProt &rMrProt, SeqLim &rSeqLim, SeqExpo &rSeqExpo);
    virtual bool runPostScan (MrProt &rMrProt, SeqLim &rSeqLim, SeqExpo &rSeqExpo, sSLICE_POS* pSLC);
    

    virtual bool calculatePrepRO    (MrProt &rMrProt, SeqLim& rSeqLim, SeqExpo &rSeqExpo);
    virtual bool calculateROP       (MrProt &rMrProt, SeqLim& rSeqLim);
    virtual bool calculateROD       (MrProt &rMrProt);
    virtual bool runAdditionalTiming(MrProt &rMrProt, SeqLim &rSeqLim, SeqExpo &rSeqExpo, sSLICE_POS* pSLC, long* plDuration);

    virtual bool checkCombinedGradients(bool bSilent = true);

    inline virtual void setlLineNumber (long value);
    inline virtual void setlPartNumber (long value);

    // * ------------------------------------------------------------------ *
    // * New members                                                        *
    // * ------------------------------------------------------------------ *


    // ENUMS
    // //////
    enum kSpaceTrajectory { Undefined, Radial, Cartesian, Propeller, RadialStack };
    enum PhaseCycleMode   { UndefinedScan, PreScan, PostScan, Scan  };

    // k-Space Trajectory
    // ///////////////////
    void setkSpaceTrajectory(kSpaceTrajectory eTraj);
    kSpaceTrajectory getkSpaceTrajectory();

    bool setTrajectoryParameters ( double dAzimuthalAngle, double dPolarAngle, double dPELine, long lMDHIndex );

    bool run (MrProt &rMrProt, SeqLim &rSeqLim, SeqExpo &rSeqExpo, sSLICE_POS *pSLC);
    bool runPropeller (MrProt &rMrProt, SeqLim &rSeqLim, SeqExpo &rSeqExpo,
                       sSLICE_POS *pSLC, double dAzimuthalAngle, double dPolarAngle, double PELine, double adRotMat[3][3]);

    virtual void runSetMdhCeco (long Ceco); // just a hack until setClin() is available in runSetMdh()

    // for radial imaging, some corrections are performed within the kernel: ClockShift and TrajectoryCorrection
    void UseTrajectoryCorrection(bool bValue);
    bool IsTrajectoryCorrectionUsed();

    // start(ADC) = start(Clock) + dLinDelay*T_os + dConstDelay
    void setClockADCShift(const double dLinDelay, const double dConstDelay);
    void getClockADCShift(double & dLinDelay, double & dConstDelay);

    // start(ADC) -= dLinDelay*BW + dConstDelay in principle
    // but really the RO dephaser amplitude is scaled
    void setROGradShift(const double dLinDelay, const double dConstDelay);
    void getROGradShift(double & dLinDelay, double & dConstDelay);

    void setTimeDelay(const double dTimeDelay);
    double getTimeDelay();

    double getRORampUpTime();

    // 3D radial rotation
    bool PrepSliceFromNormalAndReadout( ::Slice * pSlice, double adNormVec[], double adReadoutVec[], bool & bNormalInverted);

    // Make total M0 (i.e. M0 over whole TR) in read direction available to the sequence
    // Used for RF spoiling correction in TimCT
    bool getdROTotalM0(double & dROTotalM0);

    // contrast
    ///////////
    bool setContrastTrufi(bool bContrast);
    bool getContrastTrufi();

    void setPhaseCycleMode(PhaseCycleMode ePhaseCycleMode);
    PhaseCycleMode getPhaseCycleMode();

    bool setRFSpoilPhase (MrProt &rMrProt, SeqLim &rSeqLim,
                          long lSlice, bool bFirstSliceInConcat);

    virtual double runPhaseCycle ();
    virtual void resetPhaseCycle();

    void   setPhaseIncrement(double dIncrement);
    double getPhaseIncrement();

    // VFL
    ///////
    virtual bool setVFLRunIndex (unsigned int );
    ///
    /// Instructs the Kernel to use variable flip angles for each segment
    ///   (returns m_uiNumberVFL)
    void   setNumberVFL(unsigned int );

    // flip angle for proton density scans
    void setPDScanFlipAngle(const double dPDFlipAnlge);
    double getPDScanFlipAngle();


    // Misc
    ///////
    unsigned int getNumberVFL();

    virtual bool runVarRFPrepScans (MrProt&, SeqLim&, SeqExpo&, sSLICE_POS*);
    virtual bool setlPSIRPulsesRunIndex (long);
    bool   setlNumberPSIRPulses(long);
    long   getlNumberPSIRPulses();
    bool   setlVarRFPrepScanOffset(long);
    long   getlVarRFPrepScanOffset();


    /// Resets the flag "Kernel timing successfully calculated"
    //  NOTE: uses SBBGREKernel::m_bCalculated, should be using SeqBuildBlock::isTimingCalculated(), once the GREKernel supports that.
    void resetKernelTimingCalculated();

    const long getlPostScanDephaserRampDownTime_us () const;


    long getlClosestLineToKSpaceCenter () const;
    void setlClosestLineToKSpaceCenter (long);

#if defined SUPPORT_NAV
    bool ValidNavShiftVector();
    void setNavShiftAmountCorrected(const double dValue);
    void setValidNavShiftVector(const bool bValue);

    bool TraceNavFeedback();
    void setTraceNavFeedback(const bool bValue);

    void setNavShiftVector(VectorPat<double> sNavShiftVector);
    // void setNavLocSlice(Slice sSlice);
#endif

    bool dump (MrProt &rMrProt);

protected:
    
    ///
    /// module for wrap up suppression (post suppression)
    PulseSequel m_SRForPostScanSuppress;

    /// Array that contains the flip angles for the excitation SBB for vfl
    double              m_sadFlipAnglesVFL[MAX_NO_VFL];
    double              m_sadFlipAnglesPSIRPulses[MAX_NO_PSIRPulses];  // ycc

    // gradient lobe to dephase spins after flipback
    sGRAD_PULSE_TRAP    m_sP_SSPostD;

    bool                m_bContrastIsTrufi;

    /// Variable that tells Kernel to use VFL (variable flip angles),
    ///  If set to zero, no VFL is used.
    ///  Only allowed for non-cine FLASH.
    unsigned int        m_uiNumberVFL;
    long                m_lNumberPSIRPulses;
    long                m_lVarRFPrepScanOffset;

    PhaseCycleMode      m_ePhaseCycleMode;

    kSpaceTrajectory    m_ekSpaceTrajectory;

    long                m_lTrajectoryIndex; // for noncartesian sequences:
                                            //   stores index set in lLineNumber during run()

    RFSpoilParams       m_sRFSpoil;

    double              m_dPhaseIncrement;

    double              m_dPhase;

//    double m_dViewAngle; // additional inplane rotation angle (rad) for Propeller scanning
    double m_dAzimuthalAngle;   // in x-y plane (from 0 to 2pi) equivalent to m_dViewAngle
    double m_dPolarAngle;       // in z-axis (from 0 to pi) out of the x-y plane
    double m_dPELine;    // PE for Propeller scanning
                         // both parameters are ignored for cartesian scanning
    
    double              m_dPDFlipAnlge;

    bool                m_bUseTrajectoryCorrection;

    double              m_dClockADCShift[2];
    double              m_dROGradShift[2];
    double              m_dDeltaMomentRO;

    double              m_dTimeDelay;
    double              m_dRORampUpTime;


    long                m_lClosestLineToKSpaceCenter; // for shared phases radial the center line is not acquired with every phase

    /// Debug output only once
    bool                m_bPrintClockDelay;

    //    *****************************************************
    //    *                                                   *
    //    * Checks whether the parameter settings are valid   *
    //    * to preceed in kernel calculation                  *
    //    * The function sets a NLs status and returns a      *
    //    * FALSE if the settings are not valid               *
    //    *                                                   *
    //    * Overloaded to allow FLASH contrast                *
    //    *                                                   *
    //    *****************************************************
    virtual bool calculateCheckSetting ( MrProt &rMrProt, SeqLim& );

#if defined SUPPORT_NAV
    // Initialized as false, set to true the first time m_sShiftVector is filled with feedback data
    bool m_bValidNavShiftVector;

    // Uses TRACE_PUT to display changes in the slice position as a result of slice-following
    bool m_bTraceNavFeedback;

    // the shift vector that specifies how much to move the slice for slice-following
    double m_dNavShiftAmountCorrected;
    VectorPat<double> m_sNavShiftVector;

    // modified slice object to be calculated during ::run
    // this must be allocated before run to avoid xenomai mode switches
    //Slice m_sNavLocSlice;
#endif


private:


};


inline void SBBTRUFICVKernel::setkSpaceTrajectory(kSpaceTrajectory eTraj)
{
    m_ekSpaceTrajectory = eTraj;
}

inline SBBTRUFICVKernel::kSpaceTrajectory SBBTRUFICVKernel::getkSpaceTrajectory()
{
    return m_ekSpaceTrajectory;
}

inline void SBBTRUFICVKernel::UseTrajectoryCorrection(bool bValue)
{
    m_bUseTrajectoryCorrection = bValue;
}

inline bool SBBTRUFICVKernel::IsTrajectoryCorrectionUsed()
{
    return m_bUseTrajectoryCorrection;
}

inline void SBBTRUFICVKernel::setClockADCShift(const double dLinDelay, const double dConstDelay)
{
    m_dClockADCShift[0] = dLinDelay;
    m_dClockADCShift[1] = dConstDelay;
}

inline void SBBTRUFICVKernel::getClockADCShift(double & dLinDelay, double & dConstDelay)
{
    dLinDelay   = m_dClockADCShift[0];
    dConstDelay = m_dClockADCShift[1];
}

inline void SBBTRUFICVKernel::setROGradShift(const double dLinDelay, const double dConstDelay)
{
    m_dROGradShift[0] = dLinDelay;
    m_dROGradShift[1] = dConstDelay;
}

inline void SBBTRUFICVKernel::getROGradShift(double & dLinDelay, double & dConstDelay)
{
    dLinDelay   = m_dROGradShift[0];
    dConstDelay = m_dROGradShift[1];
}

inline void SBBTRUFICVKernel::setPhaseCycleMode(PhaseCycleMode ePhaseCycleMode)
{
    m_ePhaseCycleMode = ePhaseCycleMode;
}

inline SBBTRUFICVKernel::PhaseCycleMode SBBTRUFICVKernel::getPhaseCycleMode()
{
    return m_ePhaseCycleMode;
}

inline bool SBBTRUFICVKernel::setContrastTrufi(bool bContrast)
{
     m_bContrastIsTrufi = bContrast;
     return true;
}

inline bool SBBTRUFICVKernel::getContrastTrufi()
{
    return m_bContrastIsTrufi;
}

inline const long SBBTRUFICVKernel::getlPostScanDephaserRampDownTime_us () const
{
    return m_sP_SSP.getRampDownTime();
}

inline void SBBTRUFICVKernel::setPhaseIncrement(double dIncrement)
{
    m_dPhaseIncrement = dIncrement;
}

inline double SBBTRUFICVKernel::getPhaseIncrement()
{
    return m_dPhaseIncrement;
}

inline void SBBTRUFICVKernel::setNumberVFL(unsigned int uiValue)
{
    m_uiNumberVFL = uiValue;
}

inline unsigned int SBBTRUFICVKernel::getNumberVFL()
{
    return m_uiNumberVFL;
}

// ycc
inline bool SBBTRUFICVKernel::setlNumberPSIRPulses(long uiValue)
{
    if (uiValue < 0)
        return false;

    m_lNumberPSIRPulses = uiValue;
    return true;
}

inline long SBBTRUFICVKernel::getlNumberPSIRPulses()
{
    return m_lNumberPSIRPulses;
}

inline bool SBBTRUFICVKernel::setlVarRFPrepScanOffset(long lOffset)
{
    if (lOffset < 0)
        return false;

    if (m_lNoOfVarRFPreScans > 0)
    {
        if ((m_lVarRFPrepScanOffset % m_lNoOfVarRFPreScans) != 0)
            return false;
        else
            m_lVarRFPrepScanOffset = lOffset;
    }
    else
    {
        m_lVarRFPrepScanOffset = 0;
        if ( lOffset != 0 )
        {
            return false;
        }
    }

    return true;
}

inline long SBBTRUFICVKernel::getlVarRFPrepScanOffset()
{
    return m_lVarRFPrepScanOffset;
}

inline void SBBTRUFICVKernel::resetPhaseCycle()
{
    m_dPhase = 0.0;
}

inline void SBBTRUFICVKernel::resetKernelTimingCalculated()
{
    resetTimingCalculated();
}

inline long SBBTRUFICVKernel::getlClosestLineToKSpaceCenter () const
{
    return m_lClosestLineToKSpaceCenter;
}

inline void SBBTRUFICVKernel::setlClosestLineToKSpaceCenter (long lVal)
{
    m_lClosestLineToKSpaceCenter = lVal;
}

inline void SBBTRUFICVKernel::setlLineNumber (long value)
{
    SBBTRUFIKernel::setlLineNumber (value);
}

inline void SBBTRUFICVKernel::setlPartNumber (long value)
{
    SBBTRUFIKernel::setlPartNumber (value);
}

inline void SBBTRUFICVKernel::setTimeDelay(const double dTimeDelay)
{
    m_dTimeDelay = dTimeDelay;
}

inline double SBBTRUFICVKernel::getTimeDelay()
{
    return m_dTimeDelay;
}

inline double SBBTRUFICVKernel::getRORampUpTime()
{
    return m_dRORampUpTime;
}

inline void SBBTRUFICVKernel::setPDScanFlipAngle(const double dPDFlipAnlge)
{
    m_dPDFlipAnlge = dPDFlipAnlge;
}

inline double SBBTRUFICVKernel::getPDScanFlipAngle()
{
    return m_dPDFlipAnlge;
}

#if defined SUPPORT_NAV

inline bool SBBTRUFICVKernel::ValidNavShiftVector(void)
{
    return m_bValidNavShiftVector;
}

inline bool SBBTRUFICVKernel::TraceNavFeedback(void)
{
    return m_bTraceNavFeedback;
}

inline void SBBTRUFICVKernel::setValidNavShiftVector(const bool bValue)
{
    m_bValidNavShiftVector = bValue;
}

inline void SBBTRUFICVKernel::setTraceNavFeedback(const bool bValue)
{
    m_bTraceNavFeedback = bValue;
}

inline void SBBTRUFICVKernel::setNavShiftVector(VectorPat<double> sNavShiftVector)
{
    m_sNavShiftVector = sNavShiftVector;
}

inline void SBBTRUFICVKernel::setNavShiftAmountCorrected(const double dValue)
{
    m_dNavShiftAmountCorrected = dValue;
}

#endif

} // end namespace
#endif
