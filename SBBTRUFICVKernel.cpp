//    -----------------------------------------------------------------------------
//      Copyright (C) Siemens AG 1998  All Rights Reserved.
//    -----------------------------------------------------------------------------
//
//     Project: NUMARIS/4
//        File: \n4_servers1\pkg\MrServers\MrImaging\seq\Kernels\SBBTRUFICVKernel.cpp
//     Version: \main\78
//      Author: Clinical
//        Date: 2013-08-28 14:24:07 +02:00
//            derived from \n4\pkg\MrServers\MrCv\seq\Kernels\SBBTrueFispKernel.cpp@@\main\4b11a\2
//
//        Lang: C++
//
//     Descrip: MR::MrServers::MrImaging::seq::Kernels
//
//     Classes:
//
//   EGA Requirement Key: As shown on the following lines:
//
//    Abbrev.   Translation                                         Relevant for
//    -------   -----------                                         ------------
//    EGA-All   All of the following keys:                          All EGA requirements
//    EGA-01    {:IMPLEMENT:000_EGA_BildOri_SW_SequenzROVz::}       GR/GP   polarity
//    EGA-02    {:IMPLEMENT:000_EGA_BildPos_SW_SequenzSSelVz::}     GS      polarity
//    EGA-03    {:IMPLEMENT:000_EGA_BildMass_SW_SequenzROPC::}      GR/GP   amplitude
//    EGA-04    {:IMPLEMENT:000_EGA_BildPos_SW_SequenzSSel::}       GS      amplitude
//    EGA-05    {:IMPLEMENT:000_EGA_BildPos_SW_NCOFrequenzSSel::}   SRF     frequency
//    EGA-06    {:IMPLEMENT:000_EGA_BildPos_SW_NCOFrequenzRO::}     Readout frequency
//    EGA-07    {:IMPLEMENT:000_EGA_BildOri_SW_OrientierungTest::}  Image orientation
//
//
//    -----------------------------------------------------------------------------

// MrProt
#include "ProtBasic/Interfaces/MrFastImaging.h"
#include "MrServers/MrProtSrv/MrProt/MrSlice.h"
// MrProt

#include "MrServers/MrImaging/seq/Kernels/SBBTRUFICVKernel.h"

#include "MrServers/MrImaging/seq/Kernels/SBBReadOut.h"

#include "MrServers/MrImaging/seq/a_CV/mat3D.h"



#define DEBUG_ORIGIN                        DEBUG_SBB

#ifndef SEQ_NAMESPACE
    #error SEQ_NAMESPACE not defined
#endif

using namespace SEQ_NAMESPACE;

SBBTRUFICVKernel::SBBTRUFICVKernel (SBBList* pSBBList)
:   SBBTRUFIKernel                      (pSBBList)
,   m_bContrastIsTrufi                  (true)
,   m_ekSpaceTrajectory                 (Cartesian)
,   m_dAzimuthalAngle                   (0.0)
,   m_dPolarAngle                       (0.0)
,   m_dPELine                           (0.0)
,   m_sRFSpoil                          ()
,   m_dPhaseIncrement                   (0.0)
,   m_dPhase                            (0.0)
,   m_ePhaseCycleMode                   (UndefinedScan)
,   m_uiNumberVFL                       (0)
,   m_lNumberPSIRPulses                 (0)  // ycc
,   m_lVarRFPrepScanOffset              (0)  // ycc
,   m_lTrajectoryIndex                  (0)
,   m_dDeltaMomentRO                    (0.0)
,   m_bUseTrajectoryCorrection          (false)
,   m_dTimeDelay                        (0.0)
,   m_dRORampUpTime                     (0.0)
,   m_lClosestLineToKSpaceCenter        (-1)
,   m_bPrintClockDelay                  (true)
,   m_dPDFlipAnlge                      (5.0)
,   m_SRForPostScanSuppress               (3)   // 3-period spoiler rotation scheme
#if defined SUPPORT_NAV
,   m_dNavShiftAmountCorrected          (0.0)
,   m_bValidNavShiftVector              (false)
,   m_bTraceNavFeedback                 (false)
#endif
{
    unsigned int lI = 0;

    for (lI=0; lI < MAX_NO_VFL; lI++ )
    {
        m_sadFlipAnglesVFL[lI] = -1.0;
    }

    // ycc
    for (lI=0; lI < MAX_NO_PSIRPulses; lI++)
        m_sadFlipAnglesPSIRPulses[lI] = 0.;

    setClockADCShift ( 0.0, 0.0 );
    setROGradShift   ( 0.0, 0.0 );

    initClockADCShiftAndROGradShiftCorrectionValues();
}


SBBTRUFICVKernel::~SBBTRUFICVKernel ()
{
}


void SBBTRUFICVKernel::initClockADCShiftAndROGradShiftCorrectionValues()
{
    // * ---------------------------------------------------------------------- *
    ///
    /// - Handle non cartesian k-Space
    // * ---------------------------------------------------------------------- *
    // read clockshift (ADC->NCO) from MeasPerm and pass info to kernel
    // kernel requires shift NCO->ADC with slope in t_os, offset in s
    setClockADCShift  ( -SysProperties::get_flADCtoNCOShiftDwellFrac(), -1.e-6 * SysProperties::get_flADCtoNCOShiftConst()); 
                
    //TRACE_PUT3(TC_ALWAYS, TF_SEQ,"%s: clockshift NCO->ADC from measperm: %f*dwell + %f us", ptModule, -SysProperties::get_flADCtoNCOShiftDwellFrac(), - SysProperties::get_flADCtoNCOShiftConst());
        
    // values for VD11 hardware:    -0.5, -0.6e-6    
    // values for VB hardware:      -0.5, -2.0e-6
    // values for VA hardware:      -0.5, -0.8e-6

    // Avanto
    if(      SysProperties::isQengine() )
        setROGradShift    (0.40,  -3.8e-6); //slope in t_os, offset in s
    else if( SysProperties::isSQengine() )
        setROGradShift    (0.35,   0.0e-6); //slope in t_os, offset in s 
    // Espree
    else if( SysProperties::isZengine() )
        setROGradShift    (0.47,  -3.3e-6); //slope in t_os, offset in s
    else if( SysProperties::isDZengine() )
        setROGradShift    (0.40,  -3.8e-6); //slope in t_os, offset in s
    // SaTS
    else if( SysProperties::isSymphony() )
        setROGradShift    (0.39,  -3.3e-6); //slope in t_os, offset in s
    // TaTS
    else if( SysProperties::isTrio() )
        setROGradShift    (0.42,  -3.7e-6); //slope in t_os, offset in s
    // Verio
    else if( SysProperties::isVQengine() )
        setROGradShift    (0.50,   0.5e-6); //slope in t_os, offset in s 
    else if( SysProperties::isAxx60() )
    {
        // THESE VALUES ARE NOT CONFIRMED FOR AXX60
        setROGradShift    (0.42,  -3.7e-6); //slope in t_os, offset in s
    }
    else if(    SysProperties::isSkyraXQgradient() || SysProperties::isSkyraXJgradient() 
             || SysProperties::isAeraXQgradient()  || SysProperties::isAeraXJgradient() )
    {
        // grad_delay calibration corrected for ADC sampling shift in VD11D
        double grad_delay_corr = 3.75e-6;
        setROGradShift    (0.5,  -3.1e-6 + grad_delay_corr); //slope in t_os, offset in s
    }
    else
    {
        // set default value which is suitable for VD line 
        double grad_delay_corr = 3.75e-6;
        setROGradShift    (0.5,  -3.1e-6 + grad_delay_corr); //slope in t_os, offset in s
    }
}


void SBBTRUFICVKernel::switchToGREContrast()
{
    // switch off trufi
    setContrastTrufi(false);
    setePreScanMode(SEQ_NAMESPACE::SBBTRUFIBase::None);
    setbPerformTrueFispPhaseCycle(false);
    setePostScanMode( SBBTRUFIKernel::PostScanOff );

    // switch off psir
    setlNoOfVarRFPreScans(0);
    setlVarRFPrepScanOffset(0);

    seteReadOutGradType( SBBGREKernel::Spoiler );
}


bool SBBTRUFICVKernel::updateGradPerf (enum SEQ::Gradients eGradientMode)
{
    static const char *ptModule = "SBBTRUFICVKernel::updateGradPerf";

    // * ---------------------------------------------------------------------- *
    ///  Update gradient performance of the TRUFI kernel                        *
    // * ---------------------------------------------------------------------- *
    if (! SBBTRUFIKernel::updateGradPerf (eGradientMode) )
    {
        TRACE_PUT1(TC_ALWAYS, TF_SEQ,"%s: Update of gradient performance failed. ", ptModule);
        setNLSStatus ( SEQU_ERROR );
        return ( false );
    }

    // * ---------------------------------------------------------------------- *
    ///  Update additional gradients used for TrueFISP pre scans                *
    // * ---------------------------------------------------------------------- *
    m_sP_SSPostD.setMinRiseTime  ( getMinRiseTime  ( eGradientMode, SBBGREKernel_GRAD_GROUP_SS_TRUE_FISP) );
    m_sP_SSPostD.setMaxMagnitude ( getMaxMagnitude ( eGradientMode, SBBGREKernel_GRAD_GROUP_SS_TRUE_FISP) );

    return ( true );

}

#pragma message ("TODO: clean up overloading of SBBGREKernel::runSetMdhCeco(...) in SBBGREKernel")
void SBBTRUFICVKernel::runSetMdhCeco (long Ceco)
{
    // **************************************************************************
    // * Name        : runSetMdhCeco                                            *
    // *                                                                        *
    // * Class       : SBBGREKernel                                             *
    // *                                                                        *
    // * Description : Sets the Mdh Ceco index                                  *
    // *    AND SETS Clin for Noncart. TRAJECTORY to view number                *
    // *    THIS IS A BAD HACK until runSetMdh() includes setting Clin....      *
    // *                                                                        *
    // * Return      : bool                                                     *
    // *                                                                        *
    // **************************************************************************

  if(m_ekSpaceTrajectory != Cartesian)
  {
      m_aRO[Ceco].Mdh().setClin ( (unsigned short) m_lTrajectoryIndex );
  }

  SBBTRUFIKernel::runSetMdhCeco (Ceco);
}

// -----------------------------------------------------------------------------
//
//   Name        :  SBBTRUFICVKernel::calculateInit
//
//   Description :
///  \brief        Resets members to ensure consistent timing calculations
///
///                Calls base class method.
///
// -----------------------------------------------------------------------------

bool SBBTRUFICVKernel::calculateInit (MrProt &rMrProt, SeqLim &rSeqLim, SeqExpo &rSeqExpo)
{
    static const char *ptModule = "SBBTRUFICVKernel::calculateInit";

    // * ---------------------------------------------------------------------- *
    // * Initialization of base class                                           *
    // * ---------------------------------------------------------------------- *
    if ( ! SBBTRUFIKernel::calculateInit(rMrProt, rSeqLim, rSeqExpo) )  {
        TRACE_PUT1(TC_ALWAYS, TF_SEQ,"%s: Initialization of base class failed", ptModule);
        return (false);
    }

    if (true) // disable this for debugging not cleanly set members
    {
        m_dPELine           = 0.0;
        m_dPhaseIncrement   = 0.0;
        m_dPhase            = 0.0;
        m_dDeltaMomentRO    = 0.0;

        m_lClosestLineToKSpaceCenter = -1;
        m_lVarRFPrepScanOffset       = 0L;

        if (!
               (   m_sP_3D.prepAmplitude(0L, 0L, 0L, 0.0)
                && m_sP_3D_FC.prepAmplitude(0L, 0L, 0L, 0.0)
                && m_sTB_3DR.prepAmplitude(0L, 0L, 0L, 0.0)
                && m_sP_ROP.prepAmplitude(0L, 0L, 0L, 0.0)
                && m_sP_ROP_FC.prepAmplitude(0L, 0L, 0L, 0.0)
                && m_sP_ROD.prepAmplitude(0L, 0L, 0L, 0.0)
                && m_sTB_PER.prepAmplitude(0L, 0L, 0L, 0.0)
                && m_sP_SSP.prepAmplitude(0L, 0L, 0L, 0.0)
                && m_sP_SSPR.prepAmplitude(0L, 0L, 0L, 0.0)
                && m_sP_SSPostR.prepAmplitude(0L, 0L, 0L, 0.0)
                && m_sP_SS_SP.prepAmplitude(0L, 0L, 0L, 0.0)
                && m_sP_SSPostD.prepAmplitude(0L, 0L, 0L, 0.0)
               )
            )
        {
            setNLSError();
            return false;
        }
    }

    return ( true );

}


// -----------------------------------------------------------------------------
//
//   Name        :  SBBTRUFICVKernel::calculatePrepWEPulse
//
//   Description :
///  \brief        Calculates and prepares the excitation SBB
///
///                In addition to the base class, a VFL approach is supported.
///                If a valid array has been defined, it is prepared with
///                the vfl algorithm to support a better usage of the
///                magnetization in segmented FLASH sequences.
///
// -----------------------------------------------------------------------------

bool SBBTRUFICVKernel::calculatePrepWEPulse (MrProt &rMrProt, SeqLim &rSeqLim, SeqExpo &rSeqExpo)
{

    static const char *ptModule = "SBBTRUFICVKernel::calculatePrepWEPulse";

    ///
    /// checks for NULL pointers
    if ( (m_pRF_WE == NULL) || (m_pRF_Exc == NULL) )
    {
        setNLSStatus(SEQU_ERROR, ptModule, "m_pRF_WE == NULL or m_pRF_Exc == NULL \n");
        return ( false );
    }

    ///
    /// If m_uiNumberVFL > 0, then calculate the flip angle train for VFL.
    if (m_uiNumberVFL)
    {
        long lSeg; // must be signed value, because of comparison with -1

        //  fill zeroes
        for (lSeg = m_uiNumberVFL - 1; lSeg < MAX_NO_VFL; lSeg++)
        {
            m_sadFlipAnglesVFL[lSeg] = -0.1;
        }

        // calculate...
        for (lSeg = (long) m_uiNumberVFL - 1; lSeg > -1 ; lSeg--)
        {
            if (lSeg == (long) m_uiNumberVFL - 1)
            {
                m_sadFlipAnglesVFL[lSeg] = rMrProt.flipAngle();
            }
            else
            {
            // the following is a recursive formula as provided by J. Mugler
                m_sadFlipAnglesVFL[lSeg] = atan(sin(m_sadFlipAnglesVFL[lSeg+1]/180* M_PI))*180/M_PI;
            }
        }

        if (IS_TR_LAND(rSeqLim))
        {
            std::cout << ptModule << ": Calculated flip angle array for VFL: ";
            for (unsigned int ui = 0; ui < m_uiNumberVFL ; ui++)
                std::cout << m_sadFlipAnglesVFL[ui] << " ";
            std::cout << std::endl;
        }

        if ( ! m_pRF_WE->setFlipAngleArray(m_sadFlipAnglesVFL, m_uiNumberVFL ) )
        {
            setNLSStatus(m_pRF_WE->getNLSStatus(), ptModule, "m_pRF_WE->setFlipAngleArray failed");
            return ( false );
        }
    }
    else // ycc
    if (m_lNumberPSIRPulses)
    {
        m_sadFlipAnglesPSIRPulses[0] = rMrProt.flipAngle();
        m_sadFlipAnglesPSIRPulses[1] = getPDScanFlipAngle();

        if ( ! m_pRF_WE->setFlipAngleArray(m_sadFlipAnglesPSIRPulses, m_lNumberPSIRPulses ) )
        {
            setNLSStatus(m_pRF_WE->getNLSStatus(), ptModule, "m_pRF_WE->setFlipAngleArray failed");
            return ( false );
        }
    }
    else /// Otherwise, set the flip angle from the protocol into the excitation SBB
    {
        m_pRF_WE->setFlipAngle(rMrProt.flipAngle());
    }

    ///
    /// Calls original method SBBTRUFIKernel::calculatePrepWEPulse
    if (! SBBTRUFIKernel::calculatePrepWEPulse (rMrProt, rSeqLim, rSeqExpo) )
    {
        if (rSeqLim.isContextNormal())
            TRACE_PUT1(TC_ALWAYS, TF_SEQ,"%s: SBBTRUFIKernel::calculatePrepWEPulse failed. ", ptModule);
        if ((getNLSStatus() & NLS_SEV) != NLS_SEV)
            setNLSStatus ( SEQU_ERROR );
        return ( false );
    }

    return true;
}

// ycc
bool SBBTRUFICVKernel::calculateVarRFPrepScanFlipAngles()
{
    static const char *ptModule = "SBBTRUFICVKernel::calculateVarRFPrepScanFlipAngles";

    if ( !SBBTRUFIKernel::calculateVarRFPrepScanFlipAngles() )
        return false;

    if (m_lNumberPSIRPulses)
    {
        long lJ=1;
        for (long lI=0; lI<m_lNoOfVarRFPreScans; lI++)
            m_adFlipAnglesVarRFPrepScans[lJ * m_lNoOfVarRFPreScans + lI] = m_adFlipAnglesVarRFPrepScans[lI]
                        * ( m_sadFlipAnglesPSIRPulses[1] / m_sadFlipAnglesPSIRPulses[0]);
    }

    m_lFlipAngleArraySizeVarRFPrepScans = m_lNoOfVarRFPreScans * (m_lNumberPSIRPulses == 0 ? 1 : m_lNumberPSIRPulses);
    return true;

}


// ycc
bool SBBTRUFICVKernel::setlPSIRPulsesRunIndex (long lRunIndex)
{
    if ( m_lNumberPSIRPulses )
    {
        if (((long) m_pRF_WE->getNmbrOfFlipAngles()) != m_lNumberPSIRPulses)
        {
            setNLSStatus(SEQU_ERROR, ptModule, "Error: m_pRF_WE->getNmbrOfFlipAngles() != m_lNumberPSIRPulses");
            return ( false );
        }

        if (lRunIndex >= ((long) m_pRF_WE->getNmbrOfFlipAngles()))
        {
            setNLSStatus(SEQU_ERROR, ptModule, "Error: uiRunIndex >= m_pRF_WE->getNmbrOfFlipAngles()");
            return ( false );
        }

        if (! m_pRF_WE->setRunIndex(lRunIndex))
        {
            setNLSStatus(m_pRF_WE->getNLSStatus(), ptModule, "Error in m_pRF_WE->setRunIndex(...) at runtime");
            return ( false );
        }
    }

    return true;
}


// -----------------------------------------------------------------------------
//
//   Name        :  SBBTRUFICVKernel::runVarRFPrepScans
//
//   Description :
///  \brief        Runs the ramp of RF pulses for TrueFisp.
///
///                Replaces the base class method SBBTRUFIKernel::runVarRFPrepScans,
///                but is identical to it except for the use of m_lVarRFPrepScanOffset which is
///                needed for PSIR.
///
///
// -----------------------------------------------------------------------------

bool SBBTRUFICVKernel::runVarRFPrepScans (MrProt &rMrProt, SeqLim &rSeqLim, SeqExpo &rSeqExpo, sSLICE_POS* pSLC)
{

    static const char *ptModule = "SBBTRUFIKernel::runVarRFPrepScans";
    NLS_STATUS   lStatus        = SEQU__NORMAL;

    SeqBuildBlockBinomialPulses* pOrigWEPulse = NULL;



    // * -------------------------------------------------------------------------- *
    // * Reset TrueFISP phase cycle                                                 *
    // * -------------------------------------------------------------------------- *
    m_lTogglePhase = 0;



    // * -------------------------------------------------------------------------- *
    // * Save previous SDC status and disable ADC                                   *
    // * -------------------------------------------------------------------------- *
    bool bOrigReadoutStatus = fRTIsReadoutEnabled();
    fRTSetReadoutEnable ( 0 );


    // * -------------------------------------------------------------------------- *
    // * Apply the spoiler prior to the variable RF preparing scans                 *
    // * -------------------------------------------------------------------------- *

    // * ------------------------------------------------------------------------------------------------------------------- *
    // * |             Start Time        |     NCO     |    SRF   |    ADC   |          Gradient Events          | Sync    | *
    // * |               (usec)          |    Event    |   Event  |   Event  |   phase   |   read    |   slice   | Event   | *
    // * ------------------------------------------------------------------------------------------------------------------- *
    fRTEBInit( pSLC->getROT_MATRIX());
    fRTEI(0                              ,             0,         0,         0,          0,          0,&m_sP_SS_SP,        0);
    fRTEI(m_sP_SS_SP.getTotalTime()      ,             0,         0,         0,          0,          0,          0,        0);

    mSEQTest(rMrProt, rSeqLim, rSeqExpo, m_ulUTIdent, 30, 0, pSLC->getSliceIndex(), 0, 0);   /*! EGA-All !*/
    lStatus = fRTEBFinish();

    if ( setNLSStatus( lStatus ) )  {
        TRACE_PUT1_NLS(TC_INFO, TF_SEQ, "%s : fRTEBFinish() [*0030*] returned with error ", ptModule, lStatus);
        return ( false );
    }



    // * -------------------------------------------------------------------------- *
    // * Save the original rf-pulse intended for imaging                            *
    // * -------------------------------------------------------------------------- *
    pOrigWEPulse = m_pRF_WE;



    // * -------------------------------------------------------------------------- *
    // * Run m_lNoOfVarRFPreScans preparing scans with variable rf amplitudes       *
    // * -------------------------------------------------------------------------- *
    for (long lI=m_lVarRFPrepScanOffset; lI< (m_lNoOfVarRFPreScans + m_lVarRFPrepScanOffset); lI++ )
    {
        m_pRF_WE = &m_VarRF_PRE;

        if (! m_pRF_WE->setRunIndex(lI))
        {
            setNLSStatus(m_pRF_WE->getNLSStatus(), ptModule, "Error in m_pRF_WE->setRunIndex(...) at runtime");
            return ( false );
        }

        if ( ! run (rMrProt, rSeqLim, rSeqExpo, pSLC) )
        {
            setNLSError();
            return ( false );
        }

    }



    // * -------------------------------------------------------------------------- *
    // * Reset rf-pulse pointer                                                     *
    // * -------------------------------------------------------------------------- *
    m_pRF_WE = pOrigWEPulse;



    // * -------------------------------------------------------------------------- *
    // * Recover ADC status                                                         *
    // * -------------------------------------------------------------------------- *
    fRTSetReadoutEnable ( bOrigReadoutStatus ? 1 : 0 );



    return ( true );

}



// -----------------------------------------------------------------------------
//
//   Name        :  SBBTRUFICVKernel::calculateTEFill
//
//   Description :
///  \brief        Calculation of the TE fill times
///
///                If GRE contrast, the
///                TEFillTime calculation must be performed by the
///                GREKernel, which does not halve the TE as the trufi does.
///
// -----------------------------------------------------------------------------
bool SBBTRUFICVKernel::calculateTEFill (MrProt &rMrProt)
{
    if (getContrastTrufi())
    {
        return SBBTRUFIKernel::calculateTEFill (rMrProt);
    }
    else
    {
        return SBBGREKernel::calculateTEFill (rMrProt);
    }
}

// -----------------------------------------------------------------------------
//
//   Name        :  SBBTRUFICVKernel::calculateTRMin
//
//   Description :
///  \brief        Calculation of the Minimum TR time
///
///                Needs to be calculated by
///                SBBGREKernel or SBBTRUFIKernel, depending on the contrast
///
// -----------------------------------------------------------------------------
bool SBBTRUFICVKernel::calculateTRMin (MrProt &rMrProt)
{
    if (getContrastTrufi())
    {
        return SBBTRUFIKernel::calculateTRMin (rMrProt);
    }
    else
    {
        return SBBGREKernel::calculateTRMin (rMrProt);
    }
}

// -----------------------------------------------------------------------------
//
//   Name        :  SBBTRUFICVKernel::getlMinimalKernelTE
//
//   Description :
///  \brief        returns the minimum TE time for each contrast. Kernel timing has to have
///                been calculated first.
///
///                 This funtion returns the minimal possible TE time in us for each contrast, including the
///                 TEFill time introduced in case of TrueFisp-Accoustic-Resonance avoidance.
///                 Hence, can be sightly higher than m_alTEMin[iIndex]
//
//                  Background: If m_bAvoidAcousticResonances is active, then the Kernel is streched to reach
//                    a non-resonant TR. For Trufi, this requires, that the additional time is filled
//                    in both before and after the readout. However, the Kernel m_alTEMin only contains
//                    the time without that fillup, so to get the really obtainable minimum TE, we need to
//                    add the fill time.
//                    For gre or non-AcousticResonance-avoidance, the TEFill is simply the difference
//                    between the protocol TE and the minimum TE, so we must not add it.
//
///
// -----------------------------------------------------------------------------
long SBBTRUFICVKernel::getlMinimalKernelTE (int iIndex) const
{
    if ( m_bContrastIsTrufi && m_bAvoidAcousticResonances )
    {
        return (m_alTEMin[iIndex] + m_alTEFill[iIndex]);
    }
    else
    {
        return m_alTEMin[iIndex];
    }
}


// -----------------------------------------------------------------------------
//
//   Name        :  SBBTRUFICVKernel::calculateOverTime
//
//   Description :
///  \brief        Calculation of the duration that gradient ramps are
///                allowed to stick out of the event block
///
///                Needs to be calculated by
///                SBBGREKernel or SBBTRUFIKernel, depending on the contrast
///
// -----------------------------------------------------------------------------
bool SBBTRUFICVKernel::calculateOverTime (MrProt &rMrProt, SeqLim &rSeqLim)
{
    if (getContrastTrufi())
    {
        return SBBTRUFIKernel::calculateOverTime (rMrProt, rSeqLim);
    }
    else
    {
        return SBBGREKernel::calculateOverTime (rMrProt, rSeqLim);
    }
}

// -----------------------------------------------------------------------------
//
//   Name        :  SBBTRUFICVKernel::calculateRORampDownTime
//
//   Description :
///  \brief          Calculation of the read out ramp down times
///
///                Needs to be calculated by
///                SBBGREKernel or SBBTRUFIKernel, depending on the contrast
///
// -----------------------------------------------------------------------------
bool SBBTRUFICVKernel::calculateRORampDownTime (MrProt &rMrProt, SeqLim &rSeqLim, SeqExpo &rSeqExpo)
{
    if (getContrastTrufi())
    {
        return SBBTRUFIKernel::calculateRORampDownTime (rMrProt, rSeqLim, rSeqExpo);
    }
    else
    {
        return SBBGREKernel::calculateRORampDownTime (rMrProt, rSeqLim, rSeqExpo);
    }
}



// -----------------------------------------------------------------------------
//
//   Name        :  SBBTRUFICVKernel::prepPostScan
//
//   Description :
///  \brief        Prepares the post scan used magnetization prapred cine
///
///                Calls base class first, but appends an additional
///                dephasing gradient m_sP_SSPostD and calculates
///                m_lPostScanDurationPerRequest_us again from scratch.
///                Also prepares the post suppression module for long-T1 
///                artifact post suppression (PostScanSuppress) and 
///                calculates m_lPostScanDurationPerRequest_us.
///
// -----------------------------------------------------------------------------
bool SBBTRUFICVKernel::prepPostScan (MrProt &rMrProt, SeqLim &rSeqLim, SeqExpo &rSeqExpo)
{
    static const char *ptModule = "SBBTRUFICVKernel::prepPostScan";
    //TRACE_PUT1(TC_ALWAYS,TF_SEQ,"running module %s",ptModule);            


    // * ---------------------------------------------------------------------- *
    // * Prepare and check gradient pulse for slice select refocussing          *
    // * ---------------------------------------------------------------------- *
    if (! SBBTRUFIKernel::prepPostScan (rMrProt, rSeqLim, rSeqExpo) )
        return ( false );

    // * ---------------------------------------------------------------------- *
    // * Prepare and check gradient pulse for slice select refocussing          *
    // * ---------------------------------------------------------------------- *
    if (! m_sP_SSPostD.prepAmplitude(m_sP_SSPostD.getRampUpTime(), m_sP_SSPostD.getDuration(), m_sP_SSPostD.getRampDownTime(), m_sP_SSPostD.getAmplitude() ))
    {
        setNLSStatus (m_sP_SSPostD.getNLSStatus(), "m_sP_SSPostD.prepAmplitude");
        return ( false );
    }

    if (! m_sP_SSPostD.check() )
    {
        setNLSStatus (m_sP_SSPostD.getNLSStatus(), "m_sP_SSPostD.check");
        return ( false );
    }

    m_lPostScanDurationPerRequest_us   =  m_RF_WE121_Post.getRequiredLeadTime()
                                        + m_RF_WE121_Post.getDurationPerRequest()
                                        + m_sP_SSPostR.getTotalTime();

    m_lPostScanDurationPerRequest_us +=  m_RF_WE121_Post_half.getRequiredLeadTime()
                                       + m_RF_WE121_Post_half.getDurationPerRequest()
                                       - m_RF_WE121_Post_half.getGSData().getLastRampDownTime()
                                       + m_sP_SSPostD.getTotalTime();

 
    // * ---------------------------------------------------------------------- *
    // * saturation pulse for post suppression (PostScanSuppress)               *
    // * ---------------------------------------------------------------------- *

    if (m_ePostScanMode == PostScanSuppress)
    {
        m_SRForPostScanSuppress.setbSpoilCrusher    ( true );
        // increase spoiler duration
        m_SRForPostScanSuppress.Set_SpoilDuration   (1, 1000);
        m_SRForPostScanSuppress.Set_SpoilDuration   (2, 1550);    
        m_SRForPostScanSuppress.Set_SpoilAmplitude  (18);          
        m_SRForPostScanSuppress.setlPulseInterval   (2934);        
        
        if(!m_SRForPostScanSuppress.prepare(rMrProt, rSeqLim, rSeqExpo))
        {
            TRACE_PUT1(TC_ALWAYS,TF_SEQ,"%s: Prepare of SBBTRUFICVKernel::m_SRForPostScanSuppress failed!",ptModule);            
            setNLSStatus(m_SRForPostScanSuppress.getNLSStatus()); 
            return ( false );
        }

        m_lPostScanDurationPerRequest_us = m_SRForPostScanSuppress.getDurationPerRequest();
        m_PostScanRFInfoPerRequest       = m_SRForPostScanSuppress.getRFInfoPerRequest();
    }
    else
        m_SRForPostScanSuppress.resetPrepared();

    return ( true );

}

// -----------------------------------------------------------------------------
//
//   Name        :  SBBTRUFICVKernel::runPreScan
//
//   Description :
///  \brief        Executes the timing table of the pre scan
///
// -----------------------------------------------------------------------------



bool SBBTRUFICVKernel::runPreScan (MrProt &rMrProt, SeqLim &rSeqLim, SeqExpo &rSeqExpo, sSLICE_POS* pSLC)
{
    static const char *ptModule = "SBBTRUFICVKernel::runPreScan";
    setPhaseCycleMode(PreScan);
    runPhaseCycle();

    // ycc
    if (getbSkipOnRun() == true)
    {
        fSBBFillTimeRun (getlPreScanDurationPerRequest_us());
        return true;
    }
    else
    {
        if(!SBBTRUFIKernel::runPreScan(rMrProt, rSeqLim, rSeqExpo, pSLC))
        {
            TRACE_PUT1(TC_ALWAYS, TF_SEQ,"%s: runPreScan failed. ", ptModule);
            return ( false );
        }
    }

    return true;
}

// ---------------------------------------------------------------------------------
//
//   Name        :  SBBTRUFICVKernel::runPostScan
//
//   Description :
///  \brief        Executes the timing table of the post scan
///                runs either restore (flip back) pulse and gradients (PostScanAlphaOverTwo)
///                or the long-T1 artifact post suppression (PostScanSuppress)
///
// ---------------------------------------------------------------------------------

bool SBBTRUFICVKernel::runPostScan (MrProt &rMrProt, SeqLim &rSeqLim, SeqExpo &rSeqExpo, sSLICE_POS* pSLC)
{

    static const char *ptModule = "SBBTRUFICVKernel::runPostScan";
    //TRACE_PUT1(TC_ALWAYS, TF_SEQ, "running module %s\n", ptModule);

    NLS_STATUS   lStatus = SEQU__NORMAL;

    double dPhaseAngle = 0.0;
    long            lT = 0;
    switch ( m_ePostScanMode )
    {
        case PostScanAlphaOverTwo:
            
            if (getbSkipOnRun() == true)
            {
                fSBBFillTimeRun (getlPostScanDurationPerRequest_us());
                return ( true );
            }


            lT = 0;

            if ( IS_TR_RUN(rSeqLim) )
                TRACE_PUT1(TC_INFO, TF_SEQ, "%s running\n", ptModule);

            // * ------------------------------------------------------------------------- *
            // * Control of the phase of the postscan pulses                               *
            // * ------------------------------------------------------------------------- *

            dPhaseAngle = runPhaseCycle();
            m_RF_WE121_Post.setAdditionalPhase      ( dPhaseAngle );

            setPhaseCycleMode(PostScan);
            dPhaseAngle = runPhaseCycle();
            m_RF_WE121_Post_half.setAdditionalPhase ( dPhaseAngle );

            // * ------------------------------------------------------------------------- *
            // * Execute the postscan for TrueFisp                                         *
            // * ------------------------------------------------------------------------- *
            // play out the alpha pulse and the refocusing gradient lobe
            lT = 0;
            fRTEBInit(pSLC->getROT_MATRIX());
            m_RF_WE121_Post.setStartTimeInEventBlock ( m_RF_WE121_Post.getRequiredLeadTime() );
            m_RF_WE121_Post.run(rMrProt, rSeqLim, rSeqExpo, pSLC);

            // * ------------------------------------------------------------------------------------------------------------------- *
            // * |             Start Time        |     NCO     |    SRF   |    ADC   |          Gradient Events          | Sync    | *
            // * |               (usec)          |    Event    |   Event  |   Event  |   phase   |   read    |   slice   | Event   | *
            // * ------------------------------------------------------------------------------------------------------------------- *
            // CHARM 303844
            lT = m_RF_WE121_Post.getRequiredLeadTime() + m_RF_WE121_Post.getDurationPerRequest();
            fRTEI(lT                            ,            0,         0,         0,          0,          0, &m_sP_SSPostR,     0);
            lT += m_sP_SSPostR.getTotalTime() + m_lPostScanFillTime;
            fRTEI(lT                            ,            0,         0,         0,          0,          0,             0,     0);

        #ifdef WIN32
            mSEQTest( rMrProt, rSeqLim, rSeqExpo, m_ulUTIdent, 20, 0, pSLC->getSliceIndex(), 0, 0); /*! EGA-All !*/
        #endif
            lStatus = fRTEBFinish();

            if ( setNLSStatus(lStatus) )
            {
                TRACE_PUT1_NLS(TC_INFO, TF_SEQ, "%s : fRTEBFinish() [*0020*] returned with error ", ptModule, lStatus);
                return ( false );
            }

            // play out the alpha/2 pulse and the (optional) crusher
            lT = 0;
            fRTEBInit(pSLC->getROT_MATRIX());
            lT =  m_RF_WE121_Post_half.getRequiredLeadTime();
            m_RF_WE121_Post_half.setStartTimeInEventBlock ( lT );
            m_RF_WE121_Post_half.run(rMrProt, rSeqLim, rSeqExpo, pSLC);

            // * ------------------------------------------------------------------------------------------------------------------- *
            // * |             Start Time        |     NCO     |    SRF   |    ADC   |          Gradient Events          | Sync    | *
            // * |               (usec)          |    Event    |   Event  |   Event  |   phase   |   read    |   slice   | Event   | *
            // * ------------------------------------------------------------------------------------------------------------------- *

            // note: m_lTrueFispSSDephasingDuration controls both pre scan dephasing and post scan dephasing!
            if ( m_lTrueFispSSDephasingDuration != 0 )
            {
                lT += m_RF_WE121_Post_half.getDurationPerRequest() - m_RF_WE121_Post_half.getGSData().getLastRampDownTime();
                fRTEI(lT                         ,            0,         0,         0,          0,          0,&m_sP_SSPostD,     0);
                fRTEI(lT+= m_sP_SSPostD.getTotalTime(),       0,         0,         0,          0,          0,          0,       0);
            }
            else
            {
                fRTEI(lT+= m_RF_WE121_Post_half.getDurationPerRequest(),       0,         0,         0,          0,          0,          0,       0);
            }
        #ifdef WIN32
            mSEQTest(rMrProt,rSeqLim,rSeqExpo,m_ulUTIdent      ,10,0,pSLC->getSliceIndex(),0,0); /*! EGA-All !*/
        #endif
            lStatus = fRTEBFinish();

            if ( setNLSStatus(lStatus) )
            {
                TRACE_PUT1_NLS(TC_INFO, TF_SEQ, "%s : fRTEBFinish() [*0010*] returned with error ", ptModule, lStatus);
                return ( false );
            }

            if ( IS_TR_END(rSeqLim) )
                TRACE_PUT1(TC_INFO, TF_SEQ, "%s finished\n", ptModule);
        

            break;

        case PostScanSuppress:

            if ( getbSkipOnRun() )
                m_SRForPostScanSuppress.setbSkipOnRun(true);

            // * ------------------------------------------------------------------ *
            // * Excecute saturation module for post suppression (PostScanSuppress) *
            // * ------------------------------------------------------------------ *
            if( !m_SRForPostScanSuppress.run(rMrProt, rSeqLim, rSeqExpo, pSLC)) 
            {
                TRACE_PUT1( TC_ALWAYS, TF_SEQ,"%s: Error encountered in m_SRForPostScanSuppress.run(...).", ptModule );
                setNLSStatus ( m_SRForPostScanSuppress.getNLSStatus() );
                return ( false );
            }
            m_SRForPostScanSuppress.setbSkipOnRun(false);

            break;


        default:   // nothing to do
            break;
    }

    
    return ( true );


}  // end - SBBTRUFICVKernel::runPostScan




// -----------------------------------------------------------------------------
//
//   Name        :  SBBTRUFICVKernel::setVFLRunIndex
//
//   Description :
///  \brief        Modifies the flip angle for VFL sequence by passing an index
///                 to the excitation SBB
///
///                The index from the KernelLoop should be passed to this method,
///                 as this coincides with the current segment.
///                For VFL, the counter is simply passed to m_pRF_WE, being the
///                 excitation pulse SBB.
///                Preparation pulses (or dummy scans) are not affected by this method.
///
// -----------------------------------------------------------------------------

bool SBBTRUFICVKernel::setVFLRunIndex (unsigned int uiRunIndex)
{
    // If VFL is active, modify the flip angle according to the current segment
    if ( getNumberVFL() )
    {
        if (m_pRF_WE->getNmbrOfFlipAngles() != getNumberVFL())
        {
            setNLSStatus(SEQU_ERROR, ptModule, "Error: m_pRF_WE->getNmbrOfFlipAngles() != getNumberVFL()");
            return ( false );
        }

        if (uiRunIndex >= m_pRF_WE->getNmbrOfFlipAngles())
        {
            setNLSStatus(SEQU_ERROR, ptModule, "Error: uiRunIndex >= m_pRF_WE->getNmbrOfFlipAngles()");
            return ( false );
        }

        if (! m_pRF_WE->setRunIndex(uiRunIndex))
        {
            setNLSStatus(m_pRF_WE->getNLSStatus(), ptModule, "Error in m_pRF_WE->setRunIndex(...) at runtime");
            return ( false );
        }
    }

    return true;
}


// -----------------------------------------------------------------------------
//
//   Name        :  SBBTRUFICVKernel::calculateCheckSetting
//
//   Description :
///  \brief        Checks whether the parameter settings are valid to preceed in
///                kernel calculation.
///                The function sets a NLS status and returns a false if the
///                settings are not valid.
///
//   Return      : bool
///
// -----------------------------------------------------------------------------

bool SBBTRUFICVKernel::calculateCheckSetting (MrProt &rMrProt, SeqLim &rSeqLim)
{

    static const char *ptModule = "SBBTRUFICVKernel::calculateCheckSetting";

    bool bStatus;

    if (m_bContrastIsTrufi)
    {
        bStatus = SBBTRUFIKernel::calculateCheckSetting (rMrProt, rSeqLim);
    }
    else
    {
        bStatus = SBBGREKernel::calculateCheckSetting (rMrProt, rSeqLim);
    }

    return ( bStatus );

  //## end SBBTRUFIKernel::calculateCheckSetting%3BA1B7680020.body
}




bool SBBTRUFICVKernel::setTrajectoryParameters ( double dAzimuthalAngle, double dPolarAngle, double dPELine, long lMDHIndex )
{
    static const char *ptModule = "SBBTRUFICVKernel::setTrajectoryParameters";

    m_lTrajectoryIndex = lMDHIndex;
    switch ( m_ekSpaceTrajectory )
    {
    case RadialStack:
    case Radial:
        m_dAzimuthalAngle   = dAzimuthalAngle;
        m_dPolarAngle       = dPolarAngle;
        m_dPELine           = 0.0;
        break;
    case Propeller:
        m_dAzimuthalAngle   = dAzimuthalAngle;
        m_dPolarAngle       = 0.0;
        m_dPELine           = dPELine;
        break;
    case Cartesian:
        m_dAzimuthalAngle   = 0.0;
        m_dPolarAngle       = 0.0;
        m_dPELine           = 0.0;
        break;
    default:
        setNLSStatus(SEQU_ERROR, ptModule, " unimplemented trajectory \n");
        return ( false );
    }
    return true;
}

// -----------------------------------------------------------------------------
//
//   Name        :  SBBTRUFICVKernel::run
//
//   Description :
///  \brief        run method of the Kernel.
///
///                - If SkipOnRun is active, an empty event block is played.
///
///                - If Cartesian trajectory is selected, the standard ::run is called.
///
///                - If Radial trajectory is selected, runPropeller is called.
///
// -----------------------------------------------------------------------------

bool SBBTRUFICVKernel::run (MrProt &rMrProt, SeqLim &rSeqLim, SeqExpo &rSeqExpo, sSLICE_POS *pSLC)
{
    static const char *ptModule = "SBBTRUFICVKernel::run";
    setPhaseCycleMode(Scan);


    if (getbSkipOnRun() == true)
    {
        fSBBFillTimeRun (getlTRMin());
        return true;
    }


    //-------------------------------------------------------------------------
    // Set the mdh slice information here for two reasons:
    //  - for radial imaging the slice info must be entered before the slice is rotated
    //  - for slices that are measured not in an anatomical order sREADOUT does the wrong thing
    //
    // The following section follows (libRT:)sREADOUT::process(...)
    //-------------------------------------------------------------------------

    // comment on syntax: the GREKernel defines an enum GradientAxis::Slice. Since we are within a
    // derived kernel, we have to access the class 'Slice' via it's namespace.
    // Since in the header file the class slices is declared without a namespace
    // it belongs to the "unnamed" namespace and is addressed by ::Slice

    // ::Slice slice = pMrProt->sliceSeries().chronological(lSlice);

    long sSliceIndexInProt = pSLC->getSliceIndex();
    if( (sSliceIndexInProt < 0) || (sSliceIndexInProt >= rMrProt.sliceSeries().size()) )
    {
        TRACE_PUT2(TC_INFO, TF_SEQ, "%s: slice index %d in slice object not in range [0:slices[", ptModule, sSliceIndexInProt);

        // sSliceIndexInProt = 0;
        setNLSStatus(SEQU_ERROR, ptModule, "slice index in slice object out of range");
        return ( false );
    }

    ::Slice slice = rMrProt.sliceSeries()[sSliceIndexInProt];

    // Fill the slice data structure
    sSliceData  SliceData;
    SliceData.sSlicePosVec.flSag  = (float) slice.position().sag();
    SliceData.sSlicePosVec.flCor  = (float) slice.position().cor();
    SliceData.sSlicePosVec.flTra  = (float) slice.position().tra();

    // Calculate and store Gp, Gr and Gs (in patient coordinate system
    double adRotMat[3][3] =
    { { 0.0,                   0.0,                   0.0},                    // PE
      { 0.0,                   0.0,                   0.0},                    // RO
      { slice.normal().sag(),  slice.normal().cor(),  slice.normal().tra()}    // SL
    };

    fGSLCalcPRS(adRotMat[0], adRotMat[1], adRotMat[2], slice.rotationAngle());

    // Set the rotational matrix and SliceData object
    MdhProxy& rMDH = ADC(0).getMDH();

    rMDH.setRotMatrix(adRotMat, SliceData);
    rMDH.setSliceData(SliceData);

    //-------------------------------------------------------------------------
    // If the rotation angle is greater than 45grad and less than 135grad, or
    // less than -45grad and greater than -135grad we say it is swapped
    // (Charm: 361777).
    // 45grad  = 0.785398rad
    // 135grad = 2.356194rad
    // This definition must match the libRT definition in MrMeasSrv\SeqIF\libRT\PCIe\RTEventProcessorImpl.cpp
    //-------------------------------------------------------------------------
    rMDH.setPRSwapped((fabs(slice.rotationAngle()) > 0.785398) &&
                      (fabs(slice.rotationAngle()) < 2.356194));
                                    
                                    
#if defined SUPPORT_NAV
    // A local slice that is shifted by the navigator 
    ::Slice sliceNav;

    // Make a copy of the current slice
    sliceNav.copyFrom(slice);
    
    // Get the slice position
    VectorPat<double> sPosVect = sliceNav.position();
    
    // Apply the shift that is derived from the navigator signal to the current slice position.
    if (ValidNavShiftVector())
    {
        if (TraceNavFeedback())
        {
            const double dZ = m_dNavShiftAmountCorrected;
            TRACE_PUT5(TC_INFO,TF_SEQ,"%s: ShiftAmountCorrected %8.3f; OLD sPosVect [x%8.3f;y%8.3f;z%8.3f]\n",ptModule,dZ,sPosVect.dSag,sPosVect.dCor,sPosVect.dTra);
        }

        // Add the shift vector
        sPosVect += m_sNavShiftVector;

        if (TraceNavFeedback())
        {
            const double dZ = m_dNavShiftAmountCorrected;
            TRACE_PUT5(TC_INFO,TF_SEQ,"%s: ShiftAmountCorrected %8.3f; NEW sPosVect [x%8.3f;y%8.3f;z%8.3f]\n",ptModule,dZ,sPosVect.dSag,sPosVect.dCor,sPosVect.dTra);
        }

        // Update the slice data
        sliceNav.position(sPosVect);

        // Prep the slice again
        if (!pSLC->prep(rMrProt,rSeqLim,sliceNav,sSliceIndexInProt))
        {
            Trace (rSeqLim, ptModule, "pSLC[sSliceIndexInProt].prep (sliceNav) FAILED");
            setNLSStatus( pSLC[sSliceIndexInProt].getNLSStatus() );
            return false;
        }

        // Fill the slice data structure (again, like we did just a few lines above)
        sSliceData  SliceDataNav;
        SliceDataNav.sSlicePosVec.flSag  = (float) sliceNav.position().sag();
        SliceDataNav.sSlicePosVec.flCor  = (float) sliceNav.position().cor();
        SliceDataNav.sSlicePosVec.flTra  = (float) sliceNav.position().tra();
        
        // Calculate and store Gp, Gr and Gs (in patient coordinate system
        double adRotMat[3][3] =
        { 
            { 0.0,                      0.0,                      0.0},                       // PE
            { 0.0,                      0.0,                      0.0},                       // RO
            { sliceNav.normal().sag(),  sliceNav.normal().cor(),  sliceNav.normal().tra()}    // SL
        };
        
        fGSLCalcPRS(adRotMat[0], adRotMat[1], adRotMat[2], sliceNav.rotationAngle());
        
        // Set the rotational matrix and SliceData object (THIS PART IS SKIPPED, Ice Soda must not depend on Navs)
        //MdhProxy& rMDH = ADC(0).getMDH();
        
        //rMDH.setRotMatrix(adRotMat, SliceData);
        //rMDH.setSliceData(SliceData);
        
        //-------------------------------------------------------------------------
        // If the rotation angle is greater than 45grad and less than 135grad, or
        // less than -45grad and greater than -135grad we say it is swapped
        // (Charm: 361777).
        // 45grad  = 0.785398rad
        // 135grad = 2.356194rad
        // This definition must match the libRT definition in MrMeasSrv\SeqIF\libRT\PCIe\RTEventProcessorImpl.cpp
        //-------------------------------------------------------------------------
        //rMDH.setPRSwapped((fabs(slice.rotationAngle()) > 0.785398) &&
        //                  (fabs(slice.rotationAngle()) < 2.356194));
    }
    else
    {
        if (TraceNavFeedback())
        {
            const double dZ = m_dNavShiftAmountCorrected;
            TRACE_PUT5(TC_INFO,TF_SEQ,"%s: ShiftAmountCorrected %8.3f;     sPosVect [x%8.3f;y%8.3f;z%8.3f]\n",ptModule,dZ,sPosVect.dSag,sPosVect.dCor,sPosVect.dTra);
        }
    }
    
    //This is an interesting output that displays the currently-active slice shift.
    //TRACE_PUT3(TC_ALWAYS,TF_SEQ,"%s: Sliceshift for slice %d = %f",ptModule,pSLC->getSliceIndex(),pSLC->getSliceShift());

#endif

    // stop (libRT:)sREADOUT::process(...) from overwriting the slice info
    ADC(0).setSliceDataValid(true);


    //-------------------------------------------------------------------------
    // call one of the run functions
    //-------------------------------------------------------------------------
    switch ( m_ekSpaceTrajectory )
    {
    case RadialStack:
    case Radial:
        if(m_dPELine != 0.0)
        {
            setNLSStatus(SEQU_ERROR, ptModule, "PE != 0 for radial trajectory \n");
            return ( false );
        }
        // no break here on purpose!
    case Propeller:
        if (! runPropeller (rMrProt, rSeqLim, rSeqExpo, pSLC, m_dAzimuthalAngle, m_dPolarAngle, m_dPELine, adRotMat))
        {
            TRACE_PUT1(TC_INFO, TF_SEQ, "%s: runPropeller(...) failed.", ptModule);

            if ((getNLSStatus() & NLS_SEV) != NLS_SEV)
                setNLSStatus(SEQU_ERROR, ptModule, " kernel call fails but does not set NLS ERROR");

            return ( false );
        }
        break;

    case Cartesian:
        if(m_dPELine != 0.0 || m_dAzimuthalAngle != 0.0 || m_dPolarAngle != 0.0)
        {
            setNLSStatus(SEQU_ERROR, ptModule, "PE != 0, AzimuthalAngle != 0 or PolarAngle != 0 for cartesian trajectory \n");
            return ( false );
        }
        if (! SBBTRUFIKernel::run (rMrProt, rSeqLim, rSeqExpo, pSLC) )
        {
            if ((getNLSStatus() & NLS_SEV) != NLS_SEV)
                setNLSStatus(SEQU_ERROR, ptModule, " kernel call fails but does not set NLS ERROR");

            TRACE_PUT1(TC_INFO, TF_SEQ, "%s: cartesian run(...) failed.", ptModule);
            return ( false );
        }
        break;
    default:
        setNLSStatus(SEQU_ERROR, ptModule, " unimplemented trajectory \n");
        return ( false );
    }
    return true;
}






// -----------------------------------------------------------------------------
//
//   Name        :  SBBTRUFICVKernel::runPropeller
//
//   Description :
///  \brief        run method for radial imaging
///
///
///
// -----------------------------------------------------------------------------

bool SBBTRUFICVKernel::runPropeller (MrProt &rMrProt, SeqLim &rSeqLim, SeqExpo &rSeqExpo, sSLICE_POS *pSLC,
                                             double dAzimuthalAngle, double dPolarAngle, double dPELine, double adRotMat[3][3])
{
    static const char *ptModule = "SBBTRUFICVKernel::runPropeller";

    if( !(m_ekSpaceTrajectory == Radial || m_ekSpaceTrajectory == RadialStack) )
    {
        setNLSStatus(SEQU_ERROR, ptModule, " Propeller Mode not initialized \n");
        return ( false );
    }

    // set specified line number is relative to centerline
    setlLineNumber ( getlKSpaceCenterLine() + (long) dPELine );

    if (m_ekSpaceTrajectory != RadialStack)
        setlPartNumber ( getlKSpaceCenterPartition() );


    //  Reprepare global sSLICE_POS object for view rotation
    //////////////////////////////////////////////////////////////
    long sSliceIndexInProt = pSLC->getSliceIndex();
    ::Slice slice = rMrProt.sliceSeries()[sSliceIndexInProt];
    ::Slice * pSlice = & slice;

    MdhProxy& rMDH = ADC(0).getMDH();

    static sSLICE_POS asRotSLC;

    if (m_ekSpaceTrajectory == Radial && rMrProt.kSpace().getucDimension() == SEQ::DIM_3)
    {
        double adGr[3];                 //  readout vector
        double adGs[3];                 //  slice select vector
        double adGrPRS[3];              //  desired readout vector in PRS
        double adGsPRS[3];              //  desired slice select vector in PRS
        double adGrRad[3];              //  new readout vector in SCT for Radial trajectory
        double adGsRad[3];              //  new slice select vector in SCT for Radial trajectory
        matrix3 (iadRotMat);            //  inverse rotation matrix

        adGr[0] = adRotMat[1][0];        adGr[1] = adRotMat[1][1];        adGr[2] = adRotMat[1][2];
        adGs[0] = adRotMat[2][0];        adGs[1] = adRotMat[2][1];        adGs[2] = adRotMat[2][2];

        adGrPRS[0] = sin(dAzimuthalAngle) * sin(dPolarAngle);   // Phase (ky)
        adGrPRS[1] = cos(dAzimuthalAngle) * sin(dPolarAngle);   // Read  (kx)
        adGrPRS[2] = cos(dPolarAngle);                          // Slice (kz)

        // create a slice selection vector in PRS adGsPRS which is perpendicular to the desired readout vector in PRS adGrPRS
        // by making the cross product of the original slice selection vector adGs and adGrPRS
        // if adGs and adGrPRS are parallel, use adGr, which is by definition perpendicular to adGs, to create adGsPRS
        if (Mat3D::Parallel(adGs, adGrPRS))
        {
            Mat3D::CrossProduct(adGr, adGrPRS, adGsPRS);
        }
        else
        {
            Mat3D::CrossProduct(adGs, adGrPRS, adGsPRS);
        }

        Mat3D::Normalize(adGsPRS, 1e-16);

        // Calculate the inverse of the rotation matrix adRotMat
        Mat3D::CalculateInverse( iadRotMat, adRotMat);

        // Test if the rotation matrix adRotMat and the inverse rotation matrix iadRotMat are inverse
        if (!Mat3D::IsInverse(adRotMat, iadRotMat))
        {
            std::cout << "The rotation matrix iadRotMat and the inverse rotation matrix iadRotMat are not inverse" << std::endl;
            return false;
        }

        // Rotate adGrPRS and adGsPRS to adGrRad and adGsRad in SCT
        Mat3D::Mult( adGrRad, iadRotMat, adGrPRS );
        Mat3D::Mult( adGsRad, iadRotMat, adGsPRS );

        // Test if the readout and slice vectors are perpendicular throughout the rotation
        if (!( Mat3D::IsPerpendicular(adGr, adGs) &&  Mat3D::IsPerpendicular(adGrPRS, adGsPRS) && Mat3D::IsPerpendicular(adGrRad, adGsRad)))
        {
            std::cout << "Readout and Slice Vectors are not perpendicular" << std::endl;
            return false;
        }

        // Prepare slice object with new Slice Normal adGsRad and Readout Vector adGrRad
        bool bNormalInverted;

        PrepSliceFromNormalAndReadout(pSlice, adGsRad, adGrRad, bNormalInverted);

//        if(bNormalInverted)     rMDH.setFreeParameterByIndex ( MDH_FREESHORT_REFLECT, 1 );
//        else                    rMDH.setFreeParameterByIndex ( MDH_FREESHORT_REFLECT, 0 );

        // For UTE put acquisition into iso center and deal with off center in ICE
        // if (UTE) slice.position(0,0,0);

        // Prepare rotated slice object
        asRotSLC.prep(rMrProt, rSeqLim, slice, sSliceIndexInProt);

        // Force update of rotation matrix
        updateRotationMatrix();

        // Set the readout offcentre value - for ramp samling phase correction
        rMDH.setReadOutOffcentre ((float) asRotSLC.getSliceOffCenterRO());
    }
    else
    {
        // get inplane rotation angle
        double oldAngle = slice.rotationAngle();
        slice.rotationAngle(oldAngle+dAzimuthalAngle);
        //prepare rotated slice object
        asRotSLC.prep(rMrProt, rSeqLim, slice, sSliceIndexInProt);
        // Reset protocol rotation angle
        slice.rotationAngle(oldAngle);
        //   Force update of rotation matrix
        updateRotationMatrix();
    }

    // * ---------------------------------------------------------------------- *
    // * Execute Kernel                                                         *
    // * ---------------------------------------------------------------------- *
    return SBBTRUFIKernel::run(rMrProt, rSeqLim, rSeqExpo, &asRotSLC);

}

// -----------------------------------------------------------------------------
//
//   Name        :  SBBTRUFICVKernel::runPhaseCycle
//
//   Description :
///  \brief        Controls RF phase cycling for Trufi
///
///                Does not call base class, which only toggles between 180 and 0.
///                 Instead, m_dPhaseIncrement plus 180 deg is used
///
///
// -----------------------------------------------------------------------------

double SBBTRUFICVKernel::runPhaseCycle ()
{

    if ( m_bPerformTrueFispPhaseCycle == true)
    {
        switch(getPhaseCycleMode())
        {
            case PreScan:
                resetPhaseCycle();                            // preScan: the phase is set to zero
                m_dPhase = m_dPhase - m_dPhaseIncrement/2.0;  // the scan after preScan needs only a phase increment of m_dPhaseIncrement/2
                break;
            case PostScan:
                m_dPhase = m_dPhase + m_dPhaseIncrement/2.0;  // postScan needs only a phase increment of m_dPhaseIncrement/2
                break;
            case Scan:
                m_dPhase = m_dPhase + m_dPhaseIncrement;      // add m_dPhaseIncrement for "normal Scan"
                break;
            case UndefinedScan:
            default:
                break;
        }
        m_dPhase = m_dPhase + 180.0;                              // thats the conventional trueFISP phase cycling
        // avoid huge values in m_dPhase
        while ( m_dPhase > (double)RFMAXPHASEdeg )
        {
            m_dPhase -= (double)RFMAXPHASEdeg;
        }
    }

    return m_dPhase;

}


// -----------------------------------------------------------------------------
//
//   Name        :  SBBTRUFICVKernel::setRFSpoilPhase
//
//   Description :
///  \brief        Controls RF spoil phase.
///
///                - uses class m_sRFSpoil
///
///
// -----------------------------------------------------------------------------
bool SBBTRUFICVKernel::setRFSpoilPhase (MrProt &rMrProt, SeqLim &rSeqLim,
                                                  long lSlice, bool bFirstSliceInConcat)
{
    static const char *ptModule = "SBBTRUFICVKernel::setRFSpoilPhase";

    double dRFSpoilPhase = 0.0;
    NLS_STATUS lStatus = SEQU__NORMAL;

    if ( rMrProt.getsFastImaging().getulEnableRFSpoiling() )
    {
        lStatus = m_sRFSpoil.getRFPhase(
            dRFSpoilPhase,
            rMrProt,                                 // * IMP: user choice parameters  *
            rSeqLim,                                 // * IMP: limits from fSEQInit()  *
            bFirstSliceInConcat,                     // * IMP: copy data   true/false  *
            lSlice                                   // * IMP: Anatomic slice number   *
        );
        if (  ( NLS_SEV & lStatus) == NLS_SEV  )
        {
            TRACE_PUT1(TC_INFO, TF_SEQ, "%s: m_sRFSpoil.getRFPhase() failed.", ptModule);
            return false;
        }
    }

    setdRFSpoilPhase ( dRFSpoilPhase );
    return true;
}



// -----------------------------------------------------------------------------
//
//   Name        :  SBBTRUFICVKernel::getdROTotalM0
//
//   Description :
///  \brief        Calculates total Mo in read direction
///
///
///
// -----------------------------------------------------------------------------

bool SBBTRUFICVKernel::getdROTotalM0 (double & dROTotalM0)
{
      static const char *ptModule = "SBBTRUFICVKernel::getdROTotalM0";

    dROTotalM0 = 0.0;

    if (!isPrepared())
    {
        TRACE_PUT1(TC_INFO, TF_SEQ, "%s: Error: Kernel not yet prepared!", ptModule);
        return false;
    }

    if (m_eReadOutGradType != Constant)
    {
        TRACE_PUT1(TC_INFO, TF_SEQ, "%s: Error: RO spoiler not constant!", ptModule);
        return false;
    }
    if (m_bContrastIsTrufi)
    {
        TRACE_PUT1(TC_INFO, TF_SEQ, "%s: Error: Contrast is Trufi!", ptModule);
        return false;
    }

    dROTotalM0 = m_aRO[0].getdROMomentOut() + m_sP_ROD.getAmplitude() * (m_sP_ROD.getDuration() - 0.5*m_sP_ROD.getRampDownTime());
    return true;
}




RFSpoilParams::RFSpoilParams()
{
    dPhase                   = 0.0;
    dPhasePrevSlice          = 0.0;
    dIncrement               = 0.0;
    dIncrementPrevSlice      = 0.0;
    dPrevSlicePosSag         = 999999.0;
    dPrevSlicePosCor         = 999999.0;
    dPrevSlicePosTra         = 999999.0;
    dPrevSliceNormalSag      = 999999.0;
    dPrevSliceNormalCor      = 999999.0;
    dPrevSliceNormalTra      = 999999.0;
    dRFSPOIL_INCREMENTdeg    = RFSPOIL_INCREMENTdeg;
}

RFSpoilParams::~RFSpoilParams(){}

// * ---------------------------------------------------------------------- *
// * RF spoiling control                                                    *
// * ---------------------------------------------------------------------- *
NLS_STATUS RFSpoilParams::getRFPhase(double & dSpoilPhase, MrProt &rMrProt,SeqLim &rSeqLim, bool isbFirstSliceInConcat, long lSlice)
{
    static const char *ptModule = "RFSpoilParams::getRFPhase";

    NLS_STATUS lStatus = SEQU__NORMAL;

    if ( rMrProt.getsFastImaging().getulEnableRFSpoiling() )
    {
        lStatus = fSUVerifyRFSpoil (
                    rMrProt,                             // * IMP: user choice parameters  *
                    rSeqLim,                             // * IMP: limits from fSEQInit()  *
                    isbFirstSliceInConcat,     // * IMP: copy data   true/false  *
                    lSlice,                              // * IMP: Anatomic slice number   *
                    &dPhase,                       // * EXP: RF spoiling parameter   *
                    &dIncrement,                   // * EXP: RF spoiling parameter   *
                    &dPhasePrevSlice,              // * EXP: RF spoiling parameter   *
                    &dIncrementPrevSlice,          // * EXP: RF spoiling parameter   *
                    &dPrevSlicePosSag,                  // * EXP: RF spoiling parameter   *
                    &dPrevSlicePosCor,                  // * EXP: RF spoiling parameter   *
                    &dPrevSlicePosTra,                  // * EXP: RF spoiling parameter   *
                    &dPrevSliceNormalSag,               // * EXP: RF spoiling parameter   *
                    &dPrevSliceNormalCor,               // * EXP: RF spoiling parameter   *
                    &dPrevSliceNormalTra                // * EXP: RF spoiling parameter   *
                  );

        if (  ( NLS_SEV & lStatus) == NLS_SEV  )
        {
            TRACE_PUT1(TC_INFO, TF_SEQ, "%s: fSUVerifyRFSpoil(...) failed.", ptModule);
            return ( lStatus );
        }

        dPhase     += (dIncrement += dRFSPOIL_INCREMENTdeg);
        dPhase      = fmod(dPhase    ,(double)RFMAXPHASEdeg);
        dIncrement  = fmod(dIncrement,(double)RFMAXPHASEdeg);
        dSpoilPhase = dPhase;
    }
    else
        dSpoilPhase = 0.0;

    return lStatus;


};

// -----------------------------------------------------------------------------
//
//   Name        :  SBBTRUFICVKernel::calculateAddOnPost
//
//   Description :
///  \brief        Calculates additional timings.
///
///                - Calls base class
///
///                - Calculates the timing of the postscans
///
///
// -----------------------------------------------------------------------------

bool SBBTRUFICVKernel::calculateAddOnPost (MrProt &rMrProt, SeqLim &rSeqLim, SeqExpo &rSeqExpo)
{
    if (! SBBTRUFIKernel::calculateAddOnPost ( rMrProt, rSeqLim, rSeqExpo ) )
    {
        if ( ! rSeqLim.isContextPrepForBinarySearch() )
        {
            setNLSStatus ( SEQU_ERROR, "SBBTRUFIKernel::calculateAddOnPost failed");
        }
        else
        {
            setNLSStatus ( SEQU_ERROR );
        }
        return ( false );
    }

    if ( m_ePostScanMode == PostScanAlphaOverTwo )
    {
        m_sP_SSPostD.setAmplitude   ( m_RF_WE121_Post_half.getGSData().getdFirstAmplitude() );
        m_sP_SSPostD.setRampUpTime  ( m_RF_WE121_Post_half.getGSData().getLastRampDownTime() );
        m_sP_SSPostD.setDuration    ( m_lTrueFispSSDephasingDuration );
        m_sP_SSPostD.setRampDownTime( m_sP_SSPostD.getRampUpTime()  );
    }
    else
    {
        m_sP_SSPostD.setAmplitude   ( 0.0 );
        m_sP_SSPostD.setRampUpTime  (  0L );
        m_sP_SSPostD.setDuration    (  0L );
        m_sP_SSPostD.setRampDownTime(  0L );
    }

    return ( true );

}

// -----------------------------------------------------------------------------
//
//   Name        :  SBBTRUFICVKernel::runAdditionalTiming
//
//   Description :
///  \brief        Runs additional methods for correction etc.
///
///                - corrects the Mdh for shared phases
///
///                - corrects clock shift dependent ADC phase for radial imaging
///
///
// -----------------------------------------------------------------------------

bool SBBTRUFICVKernel::runAdditionalTiming(MrProt &rMrProt, SeqLim &rSeqLim, SeqExpo &rSeqExpo, sSLICE_POS* pSLC, long* plDuration)
{
    static const char *ptModule = "runAdditionalTiming";

    int lI;

    // MDH correction for shared phases
    if (m_lClosestLineToKSpaceCenter != -1)
    {
        for ( lI=0; lI<m_lNumberOfEchoes; lI++ )
        {
            m_aRO[lI].Mdh().setKSpaceCentreLineNo ( (unsigned short) m_lClosestLineToKSpaceCenter );
        }
    }


    // ADC clock correction for radial imaging
    // we only want to perform corrections if flag is set appropriate
    if (IsTrajectoryCorrectionUsed())
    {
        // clock shift correction
        /////////////////////////
        // modify ADC phase to compensate for delay between ADC and clock
        if( !(rSeqLim.getReadoutOSFactor() == 2) )
        {
            std::cout << ptModule << ": This Kernel can only be used with OS = 2" << std::endl;
            return false;
        }

        double dLinDelay = 0.0, dConstDelay = 0.0, dTos_us = 0.0, dClockFreq_Hz = 0.0, dAddPhase = 0.0;
        getClockADCShift(dLinDelay, dConstDelay);



        for ( int lI = 0; lI < m_lNumberOfEchoes; lI++ )
        {
            dTos_us       = ADC(lI).getDwellTime() * 1.e-3 / rSeqLim.getReadoutOSFactor(); // ns -> us
            dClockFreq_Hz = m_aRO[lI].getdFrequency();
            dAddPhase     = dClockFreq_Hz * (dLinDelay * 1e-6 * dTos_us + dConstDelay) * 360.0;
            m_aRO[lI].increaseADCPhase (dAddPhase);
        }

    #if defined DEBUG_RADIALCORRECTION
		#ifdef DEBUG
			bool bWorkAsDebug = true;
		#else
			bool bWorkAsDebug = SeqUT.isUnitTestActive();
		#endif
	    if ( bWorkAsDebug )
        {
	        if(m_bPrintClockDelay)
	        {
	            cout.setf(ios::scientific);
	            std::cout << "*****************" << std::endl;
	            std::cout << "ClockDelays = " << dLinDelay << ", " << dConstDelay << std::endl;
	            std::cout << "dTos [us] = " << dTos_us << std::endl;
	            std::cout << "Freq [Hz] = " << dClockFreq_Hz << std::endl;
	            std::cout << "dAddPhase [deg] = " << dAddPhase << std::endl;
	        }
        }
    #endif
        m_bPrintClockDelay = false;
    }

    return SBBTRUFIKernel::runAdditionalTiming(rMrProt, rSeqLim, rSeqExpo, pSLC, plDuration);
}



// -----------------------------------------------------------------------------
//
//   Name        :  SBBTRUFICVKernel::calculatePrepRO
//
//   Description :
///  \brief        Runs additional methods for correction etc.
///
///                - Set Trajectory correction in SBBReadout for radial multi-echo
///
///                - Pass on correction parameters
///
///                - Call base class
// -----------------------------------------------------------------------------
bool SBBTRUFICVKernel::calculatePrepRO (MrProt &rMrProt, SeqLim &rSeqLim, SeqExpo &rSeqExpo)
{
    static const char *ptModule = "SBBTRUFICVKernel::calculatePrepRO";

    long lI = 0;

    // * ---------------------------------------------------------------------- *
    // * Specify settings for the various contrasts                             *
    // * ---------------------------------------------------------------------- *
    for ( lI = 0; lI < m_lNumberOfEchoes; lI++ )  
    {
        // * ------------------------------------------------------------------ *
        // * Use Radial Trajectory Correction                                   *
        // * ------------------------------------------------------------------ *
        m_aRO[lI].setbUseTrajectoryCorrection(IsTrajectoryCorrectionUsed());
        m_aRO[lI].setROGradShift(m_dROGradShift[0], m_dROGradShift[1]);
    }

    bool bStatus = SBBTRUFIKernel::calculatePrepRO (rMrProt, rSeqLim, rSeqExpo);


    return bStatus;

}

// -----------------------------------------------------------------------------
//
//   Name        :  SBBTRUFICVKernel::calculateROP
//
//   Description :
///  \brief        Runs additional methods for correction etc.
///
///                - calls base class and corrects m_sP_ROP for radial.
///
///                - 
///
///
// -----------------------------------------------------------------------------
bool SBBTRUFICVKernel::calculateROP (MrProt &rMrProt, SeqLim &rSeqLim)
{
    static const char *ptModule = "SBBTRUFICVKernel::calculateROP";


    bool bStatus = SBBTRUFIKernel::calculateROP (rMrProt, rSeqLim );

    if(!bStatus)
    {
        return bStatus;
    }

    m_dRORampUpTime = m_aRO[0].getlRampUpTime();

    // correction of m_dMomentROP for Gradient shift
    if (IsTrajectoryCorrectionUsed())
    {
        m_dDeltaMomentRO = m_aRO[0].getdTimeDelay() * m_aRO[0].getdROAmplitude(); // needed for ROD
        if (m_bUseRampSampling)
        {
            m_dTimeDelay = m_aRO[0].getdTimeDelay(); // For UTE correction takes place in ICE
            if ( (rMrProt.sequenceType() == SEQ::SEQUENCE_TYPE_TRUFI) && (rMrProt.contrasts() > 1) )
            {
                m_dTimeDelay += 2.0; // Empirical value
            }
        }
        else
        {
            m_dTimeDelay  = 0.0; // Symmetric Echo the delay should be zero because the ICE Program does not need to correct anything
        }


    }

    return bStatus;
}

// -----------------------------------------------------------------------------
//
//   Name        :  SBBTRUFICVKernel::calculateROD
//
//   Description :
///  \brief        calls base class and corrects m_sP_ROD for radial.
///
///
// -----------------------------------------------------------------------------


bool SBBTRUFICVKernel::calculateROD (MrProt &rMrProt)
{
    static const char *ptModule = "SBBTRUFICVKernel::calculateROD";

    bool bStatus = SBBTRUFIKernel::calculateROD (rMrProt);

    // return in case of failure
    // or if ROD is no rewinder but a dephaser
    // because ROD already runs with maximized amplitude and correction might create an amplitude overflow
    // besides, correction is not needed on a dephaser anyway...
    if(!bStatus || (( m_eReadOutGradType != Symmetric_Rewinder ) && ( m_eReadOutGradType != Rewinder )) )
    {
        return bStatus;
    }


    // for bipolar trufi read out and sym echo we need to swap the ROD polarity
    if ((m_eReadOutGradType == Symmetric_Rewinder) && (rMrProt.contrasts() > 1) && (rMrProt.readOutMode() == SEQ::READOUT_BIPOLAR))
    {
        long lI = m_lNumberOfEchoes - 1;                     // * Index of the last echo *
        long lSign = (m_aRO[lI].geteROPolarity() == SeqBuildBlockReadOut::Positive) ? 1 : -1;
        // set the ROD polarity
        m_sP_ROD.setAmplitude(lSign * m_sP_ROP.getAmplitude());
    }
    else
    {
         // nothing to do
    }


    if (IsTrajectoryCorrectionUsed())
    {
        if (!m_bUseRampSampling && (rMrProt.contrasts() == 1) )
        {
            double dMomentROD = m_sP_ROD.getMomentumTOT();

#if defined DEBUG && defined DEBUG_RADIALCORRECTION
            // correction of m_dMomentROP for Gradient shift
            TRACE_PUT3(TC_ALWAYS, TF_SEQ,"%s: increasing ROD moment %lf by %lf ", ptModule, dMomentROD, 2*m_dDeltaMomentRO);
#endif
            // * ---------------------------------------------------------------------- *
            // * Calcution of the shortest possible prephasing gradient in              *
            // * readout direction                                                      *
            // * ---------------------------------------------------------------------- *

            // * Keep the timing but set the amplitude for a moment m_dMomentROP *
            bStatus = m_sP_ROD.prepMomentumTOT (dMomentROD + 2*m_dDeltaMomentRO);
            // adjust the min rise time to accomodate for increased amplitude
            // this is ok because the gre type sequences anyway restrict the slew rate to avoid stimulation pop ups
            m_sP_ROD.setMinRiseTime( m_sP_ROD.getMinRiseTime() * minimum( 1.0, 1.0 - fabs(2*m_dDeltaMomentRO/dMomentROD) ) );
        }
    }

    return bStatus;
}

// -----------------------------------------------------------------------------
//
//   Name        :  SBBTRUFICVKernel::dump
//
//   Description :
///  \brief        Dumps info on members of the sequence.
///
///
// -----------------------------------------------------------------------------

bool SBBTRUFICVKernel::dump (MrProt& /*rMrProt*/)
{

  // SeqBuildBlock:
  std::cout << std::endl << "SeqBuildBlock " << std::endl;
  std::cout << " isPrepared()                " << isPrepared()                   << std::endl
       << " isTimingCalculated()        " << isTimingCalculated()           << std::endl
       << " isRTEStartTimesCalculated() " << isRTEStartTimesCalculated()    << std::endl
       << " m_bLastRampDownOutside      " << m_bLastRampDownOutside         << std::endl
       << " m_lRampTimeOutsideSBB_us    " << m_lRampTimeOutsideSBB_us       << std::endl
       << " m_dFlipAngle_deg            " << m_dFlipAngle_deg               << std::endl
       << " m_lSBBDurationPerRequest_us " << m_lSBBDurationPerRequest_us    << std::endl
       << " m_RFInfoPerRequest.getPulseEnergyWs() " << m_RFInfoPerRequest.getPulseEnergyWs() << std::endl
       << " m_lRequestsPerMeasurement   " << m_lRequestsPerMeasurement      << std::endl
       << " m_lDebugLevel               " << m_lDebugLevel                  << std::endl
       << " m_pSBBList                  " << m_pSBBList                     << std::endl
       << " m_pCalcLimits               " << m_pCalcLimits                  << std::endl
       << " m_bTraceAlways              " << m_bTraceAlways                 << std::endl
       << " getbSkipOnRun()             " << getbSkipOnRun()                << std::endl
       << "                             " << std::endl;

// GREKernel:
  std::cout << "GREKernel " << std::endl;
  std::cout << " m_dEpsilon                        " << m_dEpsilon                         <<std::endl
       << " m_pRF_WE                          " << m_pRF_WE                           <<std::endl
       << " m_pRF_Exc                         " << m_pRF_Exc                          <<std::endl
       << " m_dRFSpoilPhase                   " << m_dRFSpoilPhase                    <<std::endl
       << " m_dMomentum_TB_3D                 " << m_dMomentum_TB_3D                  <<std::endl
       << " m_dMomentum_TB_3DR                " << m_dMomentum_TB_3DR                 <<std::endl
       << " m_dMomentROP                      " << m_dMomentROP                       <<std::endl
       << " m_bRampsOutsideOfEventBlock       " << m_bRampsOutsideOfEventBlock        <<std::endl
       << " m_bUsePERewinder                  " << m_bUsePERewinder                   <<std::endl
       << " m_bBalancedGradMomentSliceSelect  " << m_bBalancedGradMomentSliceSelect   <<std::endl
       << " m_bSendOscBit                     " << m_bSendOscBit                      <<std::endl
       << " m_bSendWakeUpBit                  " << m_bSendWakeUpBit                   <<std::endl
       << " m_lStartTimeWakeUp                " << m_lStartTimeWakeUp                 <<std::endl
       << " m_bUpdateRotationMatrix           " << m_bUpdateRotationMatrix            <<std::endl
       << " m_bPerformNegativeTEFillCheck     " << m_bPerformNegativeTEFillCheck      <<std::endl
       << " m_bSuppressTraces                 " << m_bSuppressTraces                  <<std::endl
       << " m_eWEMode                         " << m_eWEMode                          <<std::endl
       << " m_bUseWEOffsetFrequency           " << m_bUseWEOffsetFrequency            <<std::endl
       << " m_dWEOffsetFrequency              " << m_dWEOffsetFrequency               <<std::endl
       << " m_dWEBandwidthTimeProduct         " << m_dWEBandwidthTimeProduct          <<std::endl
       << " m_dRelRODephasingMoment           " << m_dRelRODephasingMoment            <<std::endl
       << " m_dRelSSDephasingMoment           " << m_dRelSSDephasingMoment            <<std::endl
       << " m_eReadOutGradType                " << m_eReadOutGradType                 <<std::endl
       << " m_lNumberOfEchoes                 " << m_lNumberOfEchoes                  <<std::endl
       << " m_eROPolarity                     " << m_eROPolarity                      <<std::endl
       << " m_adROAsymmetryBefore[0]          " << m_adROAsymmetryBefore[0]           <<std::endl
       << " m_adROAsymmetryAfter[0]           " << m_adROAsymmetryAfter[0]            <<std::endl
       << " m_lFirstLineToMeasure             " << m_lFirstLineToMeasure              <<std::endl
       << " m_lKSpaceCenterLine               " << m_lKSpaceCenterLine                <<std::endl
       << " m_lLastLineToMeasure              " << m_lLastLineToMeasure               <<std::endl
       << " m_lLineNumber                     " << m_lLineNumber                      <<std::endl
       << " m_lFirstPartToMeasure             " << m_lFirstPartToMeasure              <<std::endl
       << " m_lKSpaceCenterPartition          " << m_lKSpaceCenterPartition           <<std::endl
       << " m_lLastPartToMeasure              " << m_lLastPartToMeasure               <<std::endl
       << " m_lPartNumber                     " << m_lPartNumber                      <<std::endl
       << " m_lExcitationAllTime              " << m_lExcitationAllTime               <<std::endl
       << " m_lTEFromExcitation               " << m_lTEFromExcitation                <<std::endl
       << " m_lPrephaseTime                   " << m_lPrephaseTime                    <<std::endl
       << " m_lRephaseTime                    " << m_lRephaseTime                     <<std::endl
       << " m_lReadoutTime                    " << m_lReadoutTime                     <<std::endl
       << " m_lOverTime                       " << m_lOverTime                        <<std::endl
       << " m_lPrephaseFillTimeRead           " << m_lPrephaseFillTimeRead            <<std::endl
       << " m_lPrephaseFillTimeSlice          " << m_lPrephaseFillTimeSlice           <<std::endl
       << " m_lPrephaseFillTimePhase          " << m_lPrephaseFillTimePhase           <<std::endl
       << " m_eLimitingPrephaseTime           " << m_eLimitingPrephaseTime            <<std::endl
       << " m_eLimitingRephaseTime            " << m_eLimitingRephaseTime             <<std::endl
       << " m_lMinDelayBetweenADCs            " << m_lMinDelayBetweenADCs             <<std::endl
       << " m_lMinDelayBetweenADCAndRF        " << m_lMinDelayBetweenADCAndRF         <<std::endl
       << " m_lMinDelayBetweenRFAndADC        " << m_lMinDelayBetweenRFAndADC         <<std::endl
       << " m_dFactorForPixelSizeRO           " << m_dFactorForPixelSizeRO            <<std::endl
       << " m_dFactorForUpperPixelSizeRO      " << m_dFactorForUpperPixelSizeRO       <<std::endl
       << " m_dFactorForPixelSizePE           " << m_dFactorForPixelSizePE            <<std::endl
       << " m_dFactorForPixelSize3D           " << m_dFactorForPixelSize3D            <<std::endl
       << " m_dFactorForSliceThickness        " << m_dFactorForSliceThickness         <<std::endl
       << " m_alTEMin[0]                      " << m_alTEMin[0]                       <<std::endl
       << " m_alTEFill[0]                     " << m_alTEFill[0]                      <<std::endl
       << " m_lTRMin                          " << m_lTRMin                           <<std::endl
       << " m_lTRFill                         " << m_lTRFill                          <<std::endl
       << " m_lTRFillEnd                      " << m_lTRFillEnd                       <<std::endl
       << " m_bLastScanInConcat               " << m_bLastScanInConcat                <<std::endl
       << " m_bLastScanInMeas                 " << m_bLastScanInMeas                  <<std::endl
       << " m_bFlowCompRead                   " << m_bFlowCompRead                    <<std::endl
       << " m_bFlowCompPhase                  " << m_bFlowCompPhase                   <<std::endl
       << " m_bFlowCompSliceSelect            " << m_bFlowCompSliceSelect             <<std::endl
       << " m_bFlowCompPartitionEncode        " << m_bFlowCompPartitionEncode         <<std::endl
       << " m_bAvoidAcousticResonances        " << m_bAvoidAcousticResonances         <<std::endl
       << " m_ulUTIdent                       " << m_ulUTIdent                        <<std::endl
       << " m_bsuccessfulAllocation           " << m_bsuccessfulAllocation            <<std::endl
       << " m_pRF_Pre                         " << m_pRF_Pre                          <<std::endl
       << " m_lTimeToFirstPreScanRF           " << m_lTimeToFirstPreScanRF            <<std::endl
       << " m_lPreScanFillTime                " << m_lPreScanFillTime                 <<std::endl
       << " m_lDelayAdjustSSPosition          " << m_lDelayAdjustSSPosition           <<std::endl
       << "                                   " <<std::endl;

    std::cout <<" m_sP_3D     " << m_sP_3D.getRampUpTime()     <<" "<< m_sP_3D.getDuration()     <<" "<< m_sP_3D.getRampDownTime()     <<" "<< m_sP_3D.getAmplitude()     <<std::endl;
    std::cout <<" m_sP_3D_FC  " << m_sP_3D_FC.getRampUpTime()  <<" "<< m_sP_3D_FC.getDuration()  <<" "<< m_sP_3D_FC.getRampDownTime()  <<" "<< m_sP_3D_FC.getAmplitude()  <<std::endl;
    std::cout <<" m_sTB_3DR   " << m_sTB_3DR.getRampUpTime()   <<" "<< m_sTB_3DR.getDuration()   <<" "<< m_sTB_3DR.getRampDownTime()   <<" "<< m_sTB_3DR.getAmplitude()   <<std::endl;
    std::cout <<" m_sP_ROP    " << m_sP_ROP.getRampUpTime()    <<" "<< m_sP_ROP.getDuration()    <<" "<< m_sP_ROP.getRampDownTime()    <<" "<< m_sP_ROP.getAmplitude()    <<std::endl;
    std::cout <<" m_sP_ROP_FC " << m_sP_ROP_FC.getRampUpTime() <<" "<< m_sP_ROP_FC.getDuration() <<" "<< m_sP_ROP_FC.getRampDownTime() <<" "<< m_sP_ROP_FC.getAmplitude() <<std::endl;
    std::cout <<" m_sP_ROD    " << m_sP_ROD.getRampUpTime()    <<" "<< m_sP_ROD.getDuration()    <<" "<< m_sP_ROD.getRampDownTime()    <<" "<< m_sP_ROD.getAmplitude()    <<std::endl;
    std::cout <<" m_sTB_PER   " << m_sTB_PER.getRampUpTime()   <<" "<< m_sTB_PER.getDuration()   <<" "<< m_sTB_PER.getRampDownTime()   <<" "<< m_sTB_PER.getAmplitude()   <<std::endl;

    std::cout <<" m_TB_3D  " << m_TB_3D.getDurationPerRequest()<<std::endl;
    std::cout <<" m_TB_PE  " << m_TB_PE.getDurationPerRequest()<<std::endl;
    std::cout <<" m_TB_PER " << m_TB_PER.getDurationPerRequest()<<std::endl;

    std::cout << "                                   " <<std::endl;



    // TRUFIKernel:
    std::cout << "TRUFIKernel " << std::endl;
    std::cout << " m_pRF_WE_Pre                        " << m_pRF_WE_Pre                       <<std::endl
        << " m_lNoOfVarRFPreScans                " << m_lNoOfVarRFPreScans               <<std::endl
        << " m_lFlipAngleArraySizeVarRFPrepScans " << m_lFlipAngleArraySizeVarRFPrepScans<<std::endl
        << " m_ePreScanMode                      " << m_ePreScanMode                     <<std::endl
        << " m_ePostScanMode                     " << m_ePostScanMode                    <<std::endl
        << " m_bPerformTrueFispPhaseCycle        " << m_bPerformTrueFispPhaseCycle       <<std::endl
        << " m_lTrueFispPreScanDelay             " << m_lTrueFispPreScanDelay            <<std::endl
        << " m_lTrueFispSSDephasingDuration      " << m_lTrueFispSSDephasingDuration     <<std::endl
        << " m_lMinTEPreScan                     " << m_lMinTEPreScan                    <<std::endl
        << " m_lPreScanFillTime                  " << m_lPreScanFillTime                 <<std::endl
        << " m_lPostScanFillTime                 " << m_lPostScanFillTime                <<std::endl
        << " m_lTogglePhase                      " << m_lTogglePhase                     <<std::endl
        << " m_dVarRFPreScansSpoilMoment         " << m_dVarRFPreScansSpoilMoment        <<std::endl
        << " m_bVarRFPreScanCalculated           " << m_bVarRFPreScanCalculated          <<std::endl
        << " m_bVarRFPreScanPrepared             " << m_bVarRFPreScanPrepared            <<std::endl
        << " m_lPostScanDurationPerRequest_us    " << m_lPostScanDurationPerRequest_us   <<std::endl
        << " m_PostScanRFInfoPerRequest.getPulseEnergyWs()      " << m_PostScanRFInfoPerRequest.getPulseEnergyWs()     <<std::endl
        << " m_lTEToleranceForWEPulse            " << m_lTEToleranceForWEPulse           <<std::endl;

    std::cout <<" m_sP_SSP     " << m_sP_SSP.getRampUpTime()     <<" "<< m_sP_SSP.getDuration()     <<" "<< m_sP_SSP.getRampDownTime()     <<" "<< m_sP_SSP.getAmplitude()    <<std::endl;
    std::cout <<" m_sP_SSPR    " << m_sP_SSPR.getRampUpTime()    <<" "<< m_sP_SSPR.getDuration()    <<" "<< m_sP_SSPR.getRampDownTime()    <<" "<< m_sP_SSPR.getAmplitude()   <<std::endl;
    std::cout <<" m_sP_SSPostR " << m_sP_SSPostR.getRampUpTime() <<" "<< m_sP_SSPostR.getDuration() <<" "<< m_sP_SSPostR.getRampDownTime() <<" "<< m_sP_SSPostR.getAmplitude()<<std::endl;
    std::cout <<" m_sP_SS_SP   " << m_sP_SS_SP.getRampUpTime()   <<" "<< m_sP_SS_SP.getDuration()   <<" "<< m_sP_SS_SP.getRampDownTime()   <<" "<< m_sP_SS_SP.getAmplitude()  <<std::endl;


// TRUFICVKernel:
  std::cout << "TRUFICVKernel " << std::endl;
  std::cout << " m_bContrastIsTrufi                  " << m_bContrastIsTrufi                 <<std::endl
       << " m_ekSpaceTrajectory                 " << m_ekSpaceTrajectory                <<std::endl
       << " m_dAzimuthalAngle                   " << m_dAzimuthalAngle                  <<std::endl
       << " m_dPolarAngle                       " << m_dPolarAngle                      <<std::endl
       << " m_dPELine                           " << m_dPELine                          <<std::endl
       << " m_dPhaseIncrement                   " << m_dPhaseIncrement                  <<std::endl
       << " m_dPhase                            " << m_dPhase                           <<std::endl
       << " m_ePhaseCycleMode                   " << m_ePhaseCycleMode                  <<std::endl
       << " m_uiNumberVFL                       " << m_uiNumberVFL                      <<std::endl
       << " m_lNumberPSIRPulses                 " << m_lNumberPSIRPulses                <<std::endl
       << " m_lVarRFPrepScanOffset              " << m_lVarRFPrepScanOffset             <<std::endl
       << " m_lTrajectoryIndex                  " << m_lTrajectoryIndex                 <<std::endl
       << " m_dDeltaMomentRO                    " << m_dDeltaMomentRO                   <<std::endl
       << " m_bUseTrajectoryCorrection          " << m_bUseTrajectoryCorrection         <<std::endl
       << " m_lClosestLineToKSpaceCenter        " << m_lClosestLineToKSpaceCenter       <<std::endl
       << "                                     " <<std::endl;

    std::cout   << " m_adFlipAnglesVarRFPrepScans " ;
    for (long lI=0; lI<m_lMaxNoOfVarRFPreScans; lI++ ) std::cout << m_adFlipAnglesVarRFPrepScans[lI] << " ";
    std::cout << std::endl;

    std::cout <<" m_sP_SSPostD   " << m_sP_SSPostD.getRampUpTime()   <<" "<< m_sP_SSPostD.getDuration()   <<" "<< m_sP_SSPostD.getRampDownTime()   <<" "<< m_sP_SSPostD.getAmplitude()  <<std::endl;

    std::cout << "                                     " <<std::endl;

    return true;
}


bool SBBTRUFICVKernel::checkCombinedGradients(bool /*bSilent*/)
{
    bool bStatus = true;

    // check superposition of
    // 1. readout prephaser and PE
    //  get max amplitudes of prephaser and pe table
    return bStatus;
}


bool SBBTRUFICVKernel::PrepSliceFromNormalAndReadout(   ::Slice * pSlice,
                       double adNormVec[], double adReadoutVec[], bool & bNormalInverted)
{
    static const char *ptModule             = {"PrepSliceFromNormalAndReadout"};

    // check for zero vectors
    if(    fabs(Mat3D::Length(adNormVec))    < DOUBLE_TOLERANCE
        || fabs(Mat3D::Length(adReadoutVec)) < DOUBLE_TOLERANCE)
    {
        std::cout << ptModule << ": One ore more input vectors are zero" << std::endl;
    }
    // normalize vectors
    if( (Mat3D::Length(adNormVec) - 1.0) > DOUBLE_TOLERANCE)
    {
        Mat3D::Normalize(adNormVec);
    }
    if( (Mat3D::Length(adReadoutVec) - 1.0) > DOUBLE_TOLERANCE)
    {
        Mat3D::Normalize(adReadoutVec);
    }

    NLS_STATUS lNormStatus = checkNormalVector (adNormVec[0],adNormVec[1],adNormVec[2]);

    if (lNormStatus == GSL_INV_SLICE_ORIENTATION )
    {
        bNormalInverted = true;
    }
    else
    {
        bNormalInverted = false;
    }

    // store the rotated normal in slice object
    pSlice->normal(adNormVec[0],adNormVec[1],adNormVec[2]);

    // check if normal and readout vector are parallel
    if ( fabs( ( adNormVec[0]*adReadoutVec[0]
                +adNormVec[1]*adReadoutVec[1]
                +adNormVec[2]*adReadoutVec[2] ) - 1.0 ) < DOUBLE_TOLERANCE )
    {
        std::cout    << ptModule << "(...): Input vectors are parallel" << std::endl;
        std::cout    << "   vectors are: Norm = "
                << adNormVec[0] << "/"
                << adNormVec[1] << "/"
                << adNormVec[2] << "\n"
                << "   and :    readout   = "
                << adReadoutVec[0] << "/"
                << adReadoutVec[1] << "/"
                << adReadoutVec[2] << "\n";
        return false;
    }
    // orthogonalize vectors (adjust readout)
    if ( fabs( ( adNormVec[0]*adReadoutVec[0]
                +adNormVec[1]*adReadoutVec[1]
                +adNormVec[2]*adReadoutVec[2] )) > DOUBLE_TOLERANCE )
    {
        double adDeltaVec[3];
        Mat3D::CalcProjection(adDeltaVec, adReadoutVec, adNormVec);
        adReadoutVec[0] -= adDeltaVec[0];
        adReadoutVec[1] -= adDeltaVec[1];
        adReadoutVec[2] -= adDeltaVec[2];
    }
    // prepare slice with inplane rotation angle = 0
    double adRO[3];
    double adPE[3];
    fGSLCalcPRS (adPE, adRO, adNormVec, 0);

    double dInplaneRot;

    // check the prepared readout vector compared to the rotated readout vector
    //case 1: A == B
    if (   fabs(adRO[0] - adReadoutVec[0]) < DOUBLE_TOLERANCE
        && fabs(adRO[1] - adReadoutVec[1]) < DOUBLE_TOLERANCE
        && fabs(adRO[2] - adReadoutVec[2]) < DOUBLE_TOLERANCE )
        dInplaneRot = 0.0;
    else
    // case 2: A == -B
    if (   fabs(adRO[0] + adReadoutVec[0]) < DOUBLE_TOLERANCE
        && fabs(adRO[1] + adReadoutVec[1]) < DOUBLE_TOLERANCE
        && fabs(adRO[2] + adReadoutVec[2]) < DOUBLE_TOLERANCE )
        dInplaneRot = M_PI;
    else
    // case 3: A != B
    {
        Mat3D::angleVec(dInplaneRot, adReadoutVec, adRO, adNormVec, true);
    }

    // store the rotated rotation angle in slice object
    pSlice->rotationAngle( dInplaneRot );
    return true;
}

