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

#define DEBUG_ORIGIN DEBUG_SBB                          // needed by mTRrun;

#define SIDELOBE_DISTANCE_mm     (200)
#define RESOLUTION_FREQUENCY_mm  (5)
#define RESOLUTION_PHASE_mm      (10)
#define MAG_HEARTBEATS           (20)

#include "MrServers/MrImaging/seq/a_cv_nav_ib/SBBNavigator_JK.h"

#include "MrServers/MrMeasSrv/MeasNuclei/Base/MeasNucleus.h" // for MeasNucleus
#include "MrServers/MrMeasSrv/SeqIF/csequence.h"
#include "MrServers/MrImaging/seq/SeqDebug.h"                   // for trace macros
#include "MrCommon/MrNFramework/MrTrace/MPCUTrace/MPCUTrace.h"           // for TRACE_PUT macros
#include "MrServers/MrMeasSrv/SeqFW/libSSL/libSSL.h"
#include "MrServers/MrImaging/ut/libSeqUT.h"
#include "MrServers/MrMeasSrv/MeasPatient/MeasPatient.h"
#include "MrServers/MrMeasSrv/SeqIF/libRT/RTController.h"       // for getAbsTimeOfEventBlockMSec()

#include "MrServers/MrImaging/libSBB/libSBBmsg.h"

//  -----------------------------------------------------------------
//  Implementation of SeqBuildBlockNavigator member functions
//  -----------------------------------------------------------------
SeqBuildBlockNavigator_JK::SeqBuildBlockNavigator_JK (SBBList* pSBBList, char* pIdent) //JK
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

, SeqBuildBlockNavigator(pSBBList, pIdent) //JK was seqbuildblock
{
    for (long lI=0; lI<__NAV_ICE_BUFFER_LENGTH; lI++)
    {
        m_adNavCurrIceResults[lI] = m_adNavPrevIceResults[lI] = -999.;
    }
    setIdent(pIdent);
    
#if defined INCLUDE_SUPPORT_FOR_2DRF_PULSES
    m_2DExcitation.setTrajectory(&m_Trajectory);
    m_2DExcitation.setProfile(&m_RectProfile);
#endif
}


SeqBuildBlockNavigator_JK::~SeqBuildBlockNavigator_JK()
{
}

//  -----------------------------------------------------------------
//  Implementation of SeqBuildBlockNavigator::init
//  -----------------------------------------------------------------
bool SeqBuildBlockNavigator_JK::init (SeqLim* /*pSeqLim*/)
{
#if defined INCLUDE_SUPPORT_FOR_2DRF_PULSES
    const char* const ptModule = "SeqBuildBlockNavigator::init";
    
    if (! m_2DExcitation.init((long) SIDELOBE_DISTANCE_mm,
        (long) RESOLUTION_FREQUENCY_mm,
        (long) RESOLUTION_PHASE_mm,
        1,                                 // 1=Excitation
        false))                            // Self-refocussed?
    {
        setNLSStatus(m_2DExcitation.getNLSStatus(),ptModule,"m_2DExcitation.init");
        return false;
    }
    
    
    m_2DExcitation.setFlipAngle(30);
    m_2DExcitation.setTypeExcitation();
    m_2DExcitation.setSelfRefocussing(false);
    m_2DExcitation.setStartTimeInEventBlock(150);
    //m_2DExcitation.setRFTimeOffset(1);          // Use +/- [1,5] us to reduce ghost intensity.
#endif
    
    return true;
}


//  -----------------------------------------------------------------
//  Implementation of SeqBuildBlockNavigator::prep
//  -----------------------------------------------------------------
bool SeqBuildBlockNavigator_JK::prep (MrProt* pMrProt, SeqLim* pSeqLim, SeqExpo* pSeqExpo)
{
    const char* const ptModule = "SeqBuildBlockNavigator::prep";
    NLS_STATUS lStatus = SEQU__NORMAL;
    long lI = 0;
    double dAmpl     = 0.0;
    double dShiftExc = 0.0;
    double dShiftRef = 0.0;
    double dThick    = 0.0;
    long   lRut      = 0;
    long   lRdt      = 0;
    long   lDuration = 0;
    double dNormal[3];
    char   tErrorText[132];
    
    NavigatorArray& rNaviArray = pMrProt->navigatorArray();
    
    mPrintTrace0(DEBUG_CALL,DEBUG_SBB,"Starting");
    m_lSBBDurationPerRequest_us = 0;
    m_dEnergyPerRequest_Ws = 0.;
    m_SelIRDurationPerRequest = 0;
    m_dSelIREnergyPerRequest  = 0.;

    tErrorText[0] = '\0';
    
    setNLSStatus(SBB_NORMAL,"SeqBuildBlockNavigator::prep",NULL);
    resetPrepared();
    
    //  Do nothing when navigator control is turned off.
    if (pMrProt->NavigatorParam().RespComp() == SEQ::RESP_COMP_OFF)
    {
        mPrintTrace0(DEBUG_CALL,DEBUG_SBB,"Nav mode is OFF; returning with no action");
        return true;
    }

    //  Do nothing when there are no GSP objects defined
    if (rNaviArray.size() <= 0)
    {
        mPrintTrace0(DEBUG_CALL,DEBUG_SBB,"No navigator GSP objects defined");
        return true;
    }

    mTRrun;
    
    MeasPatient theMeasPatient;
    theMeasPatient.getDirection (&m_iPatDirection);
    theMeasPatient.getPosition  (&m_iPatPosition);
    
    if (getRequestsPerMeasurement() <= 0)
    {
        setNLSStatus(SBB_NORMAL);
        return true;
    }
    
    // Here we branch into different sections of the prep function based on the selection
    // of the type of RF pulse.
    switch (pMrProt->NavigatorParam().RFPulseType())
    {
#if defined INCLUDE_SUPPORT_FOR_2DRF_PULSES
    case SEQ::EXCIT_MODE_2D_PENCIL:
        {
            // JUST SUPPORT 1 PULSE FOR NOW
            if (rNaviArray.size() != 1)
            {
                printf("%s: Improper number of navigator objects = %ld\n",ptModule,rNaviArray.size());
                setNLSStatus(SEQU_ERROR,ptModule,"Invalid number of navigator objects");
                return false;
            }
            
            // This case uses a 2D RF pulse
            if ((rNaviArray.size() < 1) || (rNaviArray.size() > 3))
            {
                mPrintTrace0(DEBUG_CALL,DEBUG_SBB,"Expecting maximum of 3 2D pencils");
                setNLSStatus(SEQU_ERROR,ptModule,"Expecting maximum of 3 2D pencils");
                return false; 
            }
            m_lNumberOfNavigatorInstances = rNaviArray.size();
            
            // Navigator instance 0 uses GSP object 0
            // Navigator instance 1 uses GSP object 1
            if ( (m_lNavNumber < 0) || (rNaviArray.size() < m_lNavNumber+1) )
            {
                mPrintTrace1(DEBUG_CALL,DEBUG_SBB,"No GSP Navigator object for m_lNavNumber %ld",
                    m_lNavNumber);
                setNLSStatus(SBB_NORMAL);  // Just return with no error.
                return true;
            }
            
            m_lNavIndexExc = m_lNavNumber;
            
            mPrintTrace2(DEBUG_CALL,DEBUG_SBB,"Navigator object %ld has index %ld",
                m_lNavNumber,m_lNavIndexExc);
            
                /*            
                for (lI = m_lNavIndexExc; lI <= m_lNavIndexRef; lI++)
                {
                cout << endl;
                cout << "\tNav object for " << ((lI%2==0)?"90":"180") << " degree pulse" << endl;
                cout << "\tNav thickness:     (" << lI << ") " << rNaviArray[lI].thickness()     << endl;
                cout << "\tNav readoutFOV:    (" << lI << ") " << rNaviArray[lI].readoutFOV()    << endl;
                cout << "\tNav phaseFOV:      (" << lI << ") " << rNaviArray[lI].phaseFOV()      << endl;
                cout << "\tNav position:      (" << lI << ") " << rNaviArray[lI].position()      << endl;
                cout << "\tNav normal:        (" << lI << ") " << rNaviArray[lI].normal()        << endl;
                cout << "\tNav rotationAngle: (" << lI << ") " << rNaviArray[lI].rotationAngle() << endl;
                }
            */
            
            // -----------------------
            // Prepare the 2D RF pulse
            // -----------------------
            if (!m_2DExcitation.prep(pMrProt, pSeqLim, pSeqExpo, &rNaviArray[0]))
            {
                setNLSStatus(m_2DExcitation.getNLSStatus(),ptModule,"m_2DExcitation.prep");
                return false;
            };
            
            // -------------------------
            // Prepare the ADC structure
            // -------------------------
            m_ADC.setDwellTime(4096*1000/m_lNavMatrix);
            m_ADC.setColumns(m_lNavMatrix);
            
            
            lRut      = 320;
            lRdt      = 320;
            lDuration = m_ADC.getRoundedDuration()+3074;										//lDuration = 4920;
            dAmpl     = 23488.0 * m_lNavMatrix / ((double)m_lNavFov * m_ADC.getDuration());     //dAmpl     = 2.89375 * (double) m_lNavMatrix / 256.0;
            if (! m_GPRO.prepAmplitude(lRut,lDuration,lRdt,dAmpl) )
            {
                setNLSStatus(m_GPRO.getNLSStatus(),ptModule,"m_GPRO");
                return false;
            }
            m_GPRO.setAxis(SEQ::AXIS_SLICE);
            
            lRut      = 320;
            lRdt      = 320;
            lDuration = 2240;
            dAmpl     = - m_GPRO.getAmplitude() * (double)(m_ADC.getDuration()/2. + lRut/2.) / lDuration;
            
            if (! m_GPRORW.prepAmplitude(lRut,lDuration,lRdt,dAmpl) )
            {
                setNLSStatus(m_GPRORW.getNLSStatus(),ptModule,"m_GPRORW");
                return false;
            }
            
            
            double dMomX = 0.;
            double dMomY = 0.;
            double dMomentRW = m_GPRORW.getMomentumTOT();
            m_2DExcitation.getRefocussingMoment(&dMomX, &dMomY);
            //cout << "momentx: "<<dMomX<<" momentY: "<<dMomY<<endl;
            double dMomentMaxRW = sqrt(dMomX*dMomX + dMomY*dMomY + dMomentRW*dMomentRW);
            
            if (!m_GPRORW.prepSymmetricTOTShortestTime(dMomentMaxRW))
            {
                setNLSStatus(m_GPRORW.getNLSStatus(),ptModule,"m_GPRORW");
                return false;
            }
            
            if (!m_GPRFX.prepSymmetricTOTShortestTime(dMomentMaxRW))
            {
                setNLSStatus(m_GPRFX.getNLSStatus(),ptModule,"m_GPRORW");
                return false;
            }
            
            if (!m_GPRFY.prepSymmetricTOTShortestTime(dMomentMaxRW))
            {
                setNLSStatus(m_GPRFY.getNLSStatus(),ptModule,"m_GPRORW");
                return false;
            }
            
            if (!m_GPRORW.prepMomentumTOT(dMomentRW))
            {
                setNLSStatus(m_GPRORW.getNLSStatus(),ptModule,"m_GPRORW");
                return false;
            }
            
            if (!m_GPRFX.prepMomentumTOT(dMomX))
            {
                setNLSStatus(m_GPRFX.getNLSStatus(),ptModule,"m_GPRORW");
                return false;
            }
            
            if (!m_GPRFY.prepMomentumTOT(dMomY))
            {
                setNLSStatus(m_GPRFY.getNLSStatus(),ptModule,"m_GPRORW");
                return false;
            }
            
            
            // Spoiler gradients
            if ( m_bGradSpoiling == true )
            {
                m_bUseSpoiler = true;
                
                if (! m_GPSp1.prepAmplitude(500,3570,500,6.0) )
                {
                    setNLSStatus(m_GPSp1.getNLSStatus(),ptModule,"m_GPSp1");
                    return false;
                }
                
                if (! m_GPSp2.prepAmplitude(500,3570,500,6.0) )
                {
                    setNLSStatus(m_GPSp2.getNLSStatus(),ptModule,"m_GPSp2");
                    return false;
                }
                
                if (! m_GPSp3.prepAmplitude(500,3570,500,6.0) )
                {
                    setNLSStatus(m_GPSp3.getNLSStatus(),ptModule,"m_GPSp3");
                    return false;
                }
            }
            else
            {
                m_bUseSpoiler = false;
                m_GPSp1.prepAmplitude(0,0,0,0.0);
                m_GPSp2.prepAmplitude(0,0,0,0.0);
                m_GPSp3.prepAmplitude(0,0,0,0.0);
            }
            
            m_GPRFX.setStartTime   (0);
            m_GPRFY.setStartTime   (0);
            m_GPRORW.setStartTime  (0);
            m_GPRO.setStartTime    (m_GPRORW.getTotalTime());
            m_ADC.setStartTime     (m_GPRO.getStartTime()+m_GPRO.getRampUpTime());
            
            m_GPSp1.setStartTime(m_GPRO.getStartTime()+m_GPRO.getTotalTime());
            m_GPSp2.setStartTime(m_GPRO.getStartTime()+m_GPRO.getTotalTime());
            m_GPSp3.setStartTime(m_GPRO.getStartTime()+m_GPRO.getTotalTime());
            
            m_GPRFX.setAxis (SEQ::AXIS_READOUT);
            m_GPRFY.setAxis (SEQ::AXIS_PHASE);
            m_GPRORW.setAxis(SEQ::AXIS_SLICE);
            m_GPRO.setAxis  (SEQ::AXIS_SLICE);
            
            m_GPSp1.setAxis(SEQ::AXIS_PHASE);
            m_GPSp2.setAxis(SEQ::AXIS_READOUT);
            m_GPSp3.setAxis(SEQ::AXIS_SLICE);
            
            m_dEnergyPerRequest_Ws = m_2DExcitation.getEnergyPerRequest();
            m_lSBBDurationPerRequest_us = m_2DExcitation.getDurationPerRequest() +
                m_GPRORW.getTotalTime() +
                m_GPRO.getTotalTime() +
                m_GPSp1.getTotalTime();
            
            break;
  }
#endif
  
  case SEQ::EXCIT_MODE_CROSSED_PAIR:
      {
          mPrintTrace1(DEBUG_CALL,DEBUG_SBB,"pProt->NavigatorParam().RFPulseType() Crossed Pair; instance %ld",m_lNavNumber);
          mPrintTrace1(DEBUG_CALL,DEBUG_SBB,"GSP Navigator objects defined: %ld",rNaviArray.size());
          
          if (rNaviArray.size() != 2)
          {
              sprintf(tErrorText,"Expecting only 2 GSP objects, but found %ld",rNaviArray.size());
              setNLSStatus(SEQU_ERROR,ptModule,tErrorText);
              return false;
          }
          
          if ((rNaviArray.size() < 2) || ((rNaviArray.size() % 2)!=0) || (rNaviArray.size() > 6))
          {
              setNLSStatus(SEQU_ERROR,ptModule,"Expecting multiples of 2 GSP Navigator objects");
              return false;
          }
          m_lNumberOfNavigatorInstances = rNaviArray.size()/2;
          
          // Navigator instance 0 uses GSP objects 0 and 1
          // Navigator instance 1 uses GSP objects 2 and 3, etc.
          if ((m_lNavNumber < 0) || (rNaviArray.size() < 2*(m_lNavNumber+1)))
          {
              mPrintTrace1(DEBUG_CALL,DEBUG_SBB,"No GSP Navigator object for m_lNavNumber %ld",
                  m_lNavNumber);
              setNLSStatus(SBB_NORMAL);  // Just return with no error.
              return true;
          }
          m_lNavIndexExc = 2*m_lNavNumber;
          m_lNavIndexRef = m_lNavIndexExc+1;
          
          mPrintTrace3(DEBUG_CALL,DEBUG_SBB,"Navigator object %ld has indices [%ld,%ld]",
              m_lNavNumber,m_lNavIndexExc,m_lNavIndexRef);
          
              /*            
              for (lI = m_lNavIndexExc; lI <= m_lNavIndexRef; lI++)
              {
              cout << endl;
              cout << "\tNav object for " << ((lI%2==0)?"90":"180") << " degree pulse" << endl;
              cout << "\tNav thickness:     (" << lI << ") " << rNaviArray[lI].thickness()     << endl;
              cout << "\tNav readoutFOV:    (" << lI << ") " << rNaviArray[lI].readoutFOV()    << endl;
              cout << "\tNav phaseFOV:      (" << lI << ") " << rNaviArray[lI].phaseFOV()      << endl;
              cout << "\tNav position:      (" << lI << ") " << rNaviArray[lI].position()      << endl;
              cout << "\tNav normal:        (" << lI << ") " << rNaviArray[lI].normal()        << endl;
              cout << "\tNav rotationAngle: (" << lI << ") " << rNaviArray[lI].rotationAngle() << endl;
              }
          */
          
          // --------------------------------------
          // Select the RF pulses for the navigator
          // --------------------------------------
          
          // 90-degree pulse
          lI     = m_lNavIndexExc;
          dThick = rNaviArray[lI].thickness();
          if      ((  3.0 <= dThick) && (dThick <=  40.5)) m_RF090.setFamilyName("SE2560A90.SE90_12A2_2");
          else if (( 40.5 <  dThick) && (dThick <=  65.5)) m_RF090.setFamilyName("SAT2560A.SAT_26A2_1");
          else if (( 65.5 <  dThick) && (dThick <=  90.5)) m_RF090.setFamilyName("SAT2560A.SAT_29A2_1");
          else if (( 90.5 <  dThick) && (dThick <= 110.5)) m_RF090.setFamilyName("SAT2560A.SAT_36A2_1");
          else if ((110.5 <  dThick) && (dThick <= 150.0)) m_RF090.setFamilyName("SAT2560A.SAT_49A2_1");
          else
          {
              setNLSStatus(SEQU_ERROR,ptModule,"90 Nav thickness not possible");
              return false;
          }
          
          // 180-degree pulse
          lI     = m_lNavIndexRef;
          dThick = rNaviArray[lI].thickness();
          if      ((  3.0 <= dThick) && (dThick <=  40.5)) m_RF180.setFamilyName("SE2560A180.SE180_12A2_2");
          else if (( 40.5 <  dThick) && (dThick <=  65.5)) m_RF180.setFamilyName("SAT2560A.SAT_26A2_1");
          else if (( 65.5 <  dThick) && (dThick <=  90.5)) m_RF180.setFamilyName("SAT2560A.SAT_29A2_1");
          else if (( 90.5 <  dThick) && (dThick <= 110.5)) m_RF180.setFamilyName("SAT2560A.SAT_36A2_1");
          else if ((110.5 <  dThick) && (dThick <= 150.0)) m_RF180.setFamilyName("SAT2560A.SAT_49A2_1");
          else
          {
              setNLSStatus(SEQU_ERROR,ptModule,"180 Nav thickness not possible");
              return false;
          }

          // Initialize the remaining RF pulse properties
          char string1[32];
          sprintf(string1, "%sNav090", getIdent());
          m_RF090.setIdent(string1);
          
          sprintf(string1, "%sNav180", getIdent());
          m_RF180.setIdent(string1);
          m_RF090.setTypeExcitation();   m_RF180.setTypeRefocussing();
          m_RF090.setFlipAngle( 90.0);   m_RF180.setFlipAngle(180.0);
          m_RF090.setDuration(5120);     m_RF180.setDuration(5120);
          m_RF090.setSamples(512);       m_RF180.setSamples(512);
          
          m_RF090.setThickness(rNaviArray[m_lNavIndexExc].thickness());
          m_RF180.setThickness(rNaviArray[m_lNavIndexRef].thickness());
          
          // Prepare the RF pulses
          if(! m_RF090.prepExternal(pMrProt,pSeqExpo))
          {
              setNLSStatus(m_RF090.getNLSStatus(),ptModule,"m_RF090.prepExternal");
              return false;
          }
          
          if(! m_RF180.prepExternal(pMrProt,pSeqExpo))
          {
              setNLSStatus(m_RF180.getNLSStatus(),ptModule,"m_RF180.prepExternal");
              return false;
          }
          
          // -------------------------
          // Prepare the ADC structure
          // -------------------------
          m_ADC.setDwellTime(4096*1000/m_lNavMatrix);
          m_ADC.setColumns(m_lNavMatrix);
          //m_ADC.setRelevantForMeasTime();
          
          // ---------------------------
          // Prepare the gradient pulses
          // ---------------------------
          
          // Gradients for the slice-select axis
          lRut      = 320;
          lRdt      = 320;
          lDuration = 6120;                                           //lRut + m_RF090.getDuration() + 500;
          dAmpl     = m_RF090.getGSAmplitude();
          if (! m_GPSS090.prepAmplitude(lRut,lDuration,lRdt,dAmpl) )
          {
              setNLSStatus(m_GPSS090.getNLSStatus(),ptModule,"m_GPSS090");
              return false;
          }
          
          lRut      = 320;
          lRdt      = 320;
          lDuration = 2240;                                           //1420;
          dAmpl     = -m_GPSS090.getAmplitude() * (double)(m_RF090.getDuration() + 
              m_GPSS090.getRampUpTime()) / (2.0 * lDuration);
          if (! m_GPSSR.prepAmplitude(lRut,lDuration,lRdt,dAmpl) )
          {
              setNLSStatus(m_GPSSR.getNLSStatus(),ptModule,"m_GPSSR");
              return false;
          }
          
          lRut      = 320;
          lRdt      = 320;
          lDuration = lRut + m_RF180.getDuration();                     //lRut + m_RF180.getDuration() + 2000;
          dAmpl     = m_RF180.getGSAmplitude();
          if (! m_GPSS180.prepAmplitude(lRut,lDuration,lRdt,dAmpl) )
          {
              setNLSStatus(m_GPSS180.getNLSStatus(),ptModule,"m_GPSS180");
              return false;
          }
          
          // Gradients for the readout axis
          lRut      = 320;
          lRdt      = 320;
          lDuration = m_ADC.getRoundedDuration()+3074;                                    //lDuration = 4920;
          dAmpl     = 23488.0 * m_lNavMatrix / ((double)m_lNavFov * m_ADC.getDuration());      //dAmpl     = 2.89375 * (double) m_lNavMatrix / 256.0;
          if (! m_GPRO.prepAmplitude(lRut,lDuration,lRdt,dAmpl) )
          {
              setNLSStatus(m_GPRO.getNLSStatus(),ptModule,"m_GPRO");
              return false;
          }
          
          
          lRut      = 320;
          lRdt      = 320;
          lDuration = 2000;
          //dAmpl     = 2.89375 * 2710.0 / lDuration * (double) m_lNavMatrix / 256.0;
          dAmpl     = -m_GPRO.getAmplitude() * (double)(m_GPRO.getDuration() - m_ADC.getDuration()/2. - lRut/2.) / lDuration;
          if (! m_GPRORW.prepAmplitude(lRut,lDuration,lRdt,dAmpl) )
          {
              setNLSStatus(m_GPRORW.getNLSStatus(),ptModule,"m_GPRORW");
              return false;
          }
          
          // Spoiler gradients
          if ( m_bGradSpoiling == true )
          {
              m_bUseSpoiler = true;
              
              if (! m_GPS01.prepAmplitude(320,2000,320,8.0) )
              {
                  setNLSStatus(m_GPSp1.getNLSStatus(),ptModule,"m_GPSp1");
                  return false;
              }
              if (! m_GPS02.prepAmplitude(320,2000,320,8.0) )
              {
                  setNLSStatus(m_GPSp1.getNLSStatus(),ptModule,"m_GPSp1");
                  return false;
              }
              
              if (! m_GPSp1.prepAmplitude(500,3570,500,6.0) )
              {
                  setNLSStatus(m_GPSp1.getNLSStatus(),ptModule,"m_GPSp1");
                  return false;
              }
              if (! m_GPSp2.prepAmplitude(500,3570,500,6.0) )
              {
                  setNLSStatus(m_GPSp2.getNLSStatus(),ptModule,"m_GPSp2");
                  return false;
              }
              if (! m_GPSp3.prepAmplitude(500,3570,500,6.0) )
              {
                  setNLSStatus(m_GPSp3.getNLSStatus(),ptModule,"m_GPSp3");
                  return false;
              }
          }
          else
          {
              m_bUseSpoiler = false;
              m_GPS01.prepAmplitude(0,0,0,0.0);
              m_GPS02.prepAmplitude(0,0,0,0.0);
              m_GPSp1.prepAmplitude(0,0,0,0.0);
              m_GPSp2.prepAmplitude(0,0,0,0.0);
              m_GPSp3.prepAmplitude(0,0,0,0.0);
          }
          
          // -----------------------------
          // Prepare the rotation matrices
          // -----------------------------
          // First rotation matrix applies to the 90 degree pulse
          // Second rotation matrix applies to the 180 degree pulse
          dNormal[0] = rNaviArray[m_lNavIndexExc].normal().sag();
          dNormal[1] = rNaviArray[m_lNavIndexExc].normal().cor();
          dNormal[2] = rNaviArray[m_lNavIndexExc].normal().tra();
          lStatus = fGSLCalcRotMat(dNormal,
              0.0,
              m_iPatDirection,
              m_iPatPosition,
              &m_RotMatrix090);
          if (setNLSStatus(lStatus,ptModule,"fGSLCalcRotMat")) return false;  
          
          dNormal[0] = rNaviArray[m_lNavIndexRef].normal().sag();
          dNormal[1] = rNaviArray[m_lNavIndexRef].normal().cor();
          dNormal[2] = rNaviArray[m_lNavIndexRef].normal().tra();
          lStatus = fGSLCalcRotMat(dNormal,
              0.0,
              m_iPatDirection,
              m_iPatPosition,
              &m_RotMatrix180);
          if (setNLSStatus(lStatus,ptModule,"fGSLCalcRotMat")) return false;  
          
          
          // --------------------------------------
          // Prepare the Frequency/Phase structures
          // --------------------------------------
          // First calculation applies to the (Tx) NCO properties of the 90 degree pulse
          dShiftExc = fSSLSlicePosCalc (
              rNaviArray[m_lNavIndexExc].position().sag(),
              rNaviArray[m_lNavIndexExc].position().cor(),
              rNaviArray[m_lNavIndexExc].position().tra(),
              rNaviArray[m_lNavIndexExc].normal().sag(),
              rNaviArray[m_lNavIndexExc].normal().cor(),
              rNaviArray[m_lNavIndexExc].normal().tra());
          
          MeasNucleus myMeasNucleus(pMrProt->txSpec().nucleusInfoArray()[0].nucleus());

          m_Txz090Set.setFrequency  (long(NINT(myMeasNucleus.getLarmorConst() *
              m_GPSS090.getAmplitude() * dShiftExc)));
          m_Txz090Set.setPhase    	( - m_Txz090Set.getFrequency() * 0.5 * 
              m_RF090.getDuration() * (360.0/1000000.0));
          m_Txz090Neg.setFrequency	(long(0.));
          m_Txz090Neg.setPhase    	(m_Txz090Set.getPhase());
          
          // Second calculation applies to the (Tx) NCO properties of the 180 degree pulse
          dShiftRef = fSSLSlicePosCalc (
              rNaviArray[m_lNavIndexRef].position().sag(),
              rNaviArray[m_lNavIndexRef].position().cor(),
              rNaviArray[m_lNavIndexRef].position().tra(),
              rNaviArray[m_lNavIndexRef].normal().sag(),
              rNaviArray[m_lNavIndexRef].normal().cor(),
              rNaviArray[m_lNavIndexRef].normal().tra());
          
          m_Txz180Set.setFrequency  (long(NINT(myMeasNucleus.getLarmorConst() *
              m_GPSS180.getAmplitude() * dShiftRef)));
          m_Txz180Set.setPhase    	( - m_Txz180Set.getFrequency() * 0.5 * 
              m_RF180.getDuration() * (360.0/1000000.0));
          m_Txz180Neg.setFrequency	(long(0.));
          m_Txz180Neg.setPhase    	(m_Txz180Set.getPhase());
          
          // Third calculation applies to the (Rx) NCO properties of the ADC
          m_RxzSet.setFrequency     (long(0));
          m_RxzSet.setPhase         ( m_RxzSet.getFrequency() * m_ADC.getDuration() *
              (360.0/1000000.0));
          

          // ---------------
          // Prepare exports
          // ---------------
          m_dEnergyPerRequest_Ws = m_RF090.getPulseEnergyWs() + m_RF180.getPulseEnergyWs();
          m_lSBBDurationPerRequest_us = 30000;


          //  Additional Preparation for selective inversion
		  if (m_bUseIRSel)
		  {
             //printf("%s: Preparing IRSel objects...\n",ptModule);

             // RF Pulse
             if (pMrProt->preparationPulses().darkBlood()   == true)
             {
               m_RFSelIR.setDuration     (10240);
               m_RFSelIR.setFlipAngle    (200);
               m_RFSelIR.setInitialPhase (0.0);
               m_RFSelIR.setSamples      (512);
               m_RFSelIR.setFamilyName   ("IR10240H180.GS7");
               m_RFSelIR.setThickness    (m_dScaleSSThickness * rNaviArray[m_lNavIndexExc].thickness());
             }
             else
             {
               m_RFSelIR.setDuration     (m_IRSelRFDuration);
               m_RFSelIR.setFlipAngle    (m_dIRSelFlipAngle_deg);
               m_RFSelIR.setInitialPhase (0.0);
               m_RFSelIR.setSamples      (512);
               m_RFSelIR.setFamilyName   ("IR10240H180.IR180_36B1_1");
               m_RFSelIR.setThickness    (m_dScaleSSThickness * rNaviArray[m_lNavIndexExc].thickness());
             }

             sprintf(string1, "%sRFSelIR", getIdent());
             m_RFSelIR.setIdent(string1);

             if(! m_RFSelIR.prepExternal(pMrProt,pSeqExpo) )  
			 {
               setNLSStatus(m_RFSelIR.getNLSStatus(),ptModule, "Prep of m_RFSelIR failed!");
               return false;
			 }

             lRut      = 800;
             lRdt      = 800;
             lDuration = m_RFSelIR.getDuration() + lRut;
             dAmpl     = m_RFSelIR.getGSAmplitude();
             if (! m_GPSelIR1.prepAmplitude(lRut,lDuration,lRdt,dAmpl) )
             {
                 setNLSStatus(m_GPSelIR1.getNLSStatus(),ptModule,"m_GPSelIR1");
                 return false;
             }

             lRut      = 600;
             lRdt      = 600;
             lDuration = 8560;
             dAmpl     = 8.0;
             if (! m_GPSelIR2.prepAmplitude(lRut,lDuration,lRdt,dAmpl) )
             {
                 setNLSStatus(m_GPSelIR2.getNLSStatus(),ptModule,"m_GPSelIR2");
                 return false;
             }

             m_RFSelIRzSet.setFrequency  (long(NINT(myMeasNucleus.getLarmorConst() *
                 m_GPSelIR1.getAmplitude() * dShiftExc)));
             m_RFSelIRzSet.setPhase    	( - m_RFSelIRzSet.getFrequency() * 0.5 * 
                 m_RFSelIR.getDuration() * (360.0/1000000.0));
             m_RFSelIRzNeg.setFrequency	(long(0.));
             m_RFSelIRzNeg.setPhase    	(m_RFSelIRzSet.getPhase());

             m_dSelIREnergyPerRequest  = m_RFSelIR.getPulseEnergyWs();
             m_SelIRDurationPerRequest = 21000;

          }

          
          break;
  }
  

  default:
      setNLSStatus(SEQU_ERROR,ptModule,"Undefined pulse type pProt->NavigatorParam().RFPulseType()");
      return false;
      break;
      
      
  } // switch pProt->NavigatorParam().RFPulseType()
  
  mTRend;
  setPrepared();
  return true;
}

//  -----------------------------------------------------------------
//  Implementation of SeqBuildBlockNavigator::run
//  -----------------------------------------------------------------
bool SeqBuildBlockNavigator_JK::run (MrProt* pMrProt, SeqLim* pSeqLim, SeqExpo* /*pSeqExpo*/, sSLICE_POS* /*pSLC*/)
{
    const char* const ptModule = "SBBNavigator::run";
    long lT = 0;
    long lADCDuration = (long)(m_ADC.getDuration() + 0.5);
    long lMdhFlags = 0;
    sSLICE_POS Dummy;
    bool bDebug = ((getSEQDebugFlags() & SEQDebug_ShowFeedbackData) != 0);
        //TRACE_PUT1(TC_ALWAYS, TF_SEQ,"%s: ###in the run loop###-1 ", ptModule); //meee trace
    if (!isPrepared()) return true;

    //TRACE_PUT1(TC_INFO,TF_SEQ,"%s: HERE I AM", ptModule);
    
    // Prepare for feedback
    m_ADC.Mdh.setEvalInfoMask(MDH_RTFEEDBACK);

    //Set relevant ADCs
    if (m_bRelevantADC)
    {
      m_ADC.setRelevantForMeasTime(true);
    }
    else
    {
      m_ADC.setRelevantForMeasTime(false);
    }
    
    //Commented out Jan 11, 2002:
    //m_ADC.Mdh.setIceProgramPara(MDH_ICEPARAM_NAVIGATORNUMBER   ,getNavNumber());
    //m_ADC.Mdh.setIceProgramPara(MDH_ICEPARAM_NAVIGATORCOUNT    ,m_lNavCount);

    m_ADC.Mdh.setIceProgramPara(MDH_ICEPARAM_PERCENT_COMPLETE  ,getNavPercentComplete());
    m_ADC.Mdh.setIceProgramPara(MDH_ICEPARAM_PERCENT_ACCEPTED  ,getNavPercentAccepted());

    if (bDebug)
    {
        char tBefAft[20];
        if (m_bNavIsAfterEcho) strcpy(tBefAft,"(AFTER ECHO)"); else strcpy(tBefAft,"(BEFOREECHO)");
        TRACE_PUT4(TC_INFO,TF_SEQ,"%s: %s Min/Max Feedback times: %8.3f/%8.3f ms",
               ptModule,tBefAft,getMinNavFeedbackTime_ms(),getMaxNavFeedbackTime_ms());
        TRACE_PUT3(TC_INFO,TF_SEQ,"%s: %%Complete %ld; %%Accept %ld",
               ptModule,getNavPercentComplete(),getNavPercentAccepted());

        mPrintTrace3(DEBUG_CALL,DEBUG_SBB,"%s Min/Max Feedback times: %8.3f/%8.3f ms",
               tBefAft,getMinNavFeedbackTime_ms(),getMaxNavFeedbackTime_ms());
        mPrintTrace2(DEBUG_CALL,DEBUG_SBB,"%%Complete %ld; %%Accept %ld",
               getNavPercentComplete(),getNavPercentAccepted());
    }
           
    if (getMaxNavFeedbackTime_ms()>0.0)
    {
    m_ADC.Mdh.setIceProgramPara(MDH_ICEPARAM_MAX_FEEDBACKTIME_MS,(short) (getMaxNavFeedbackTime_ms()+0.5));
    }

    lMdhFlags = 0;
    if (m_bNavIsLastHPScan)
    {
        //TRACE_PUT0(TC_INFO,TF_SEQ,"LAST SCAN");
        lMdhFlags |= MDH_FLAG_LASTHPSCAN;
        m_bNavIsLastHPScan = false;
    }
    
    if (m_bNavIsLastRTScan)
    {
        //TRACE_PUT0(TC_INFO,TF_SEQ,"LAST SCAN");
        lMdhFlags |= MDH_FLAG_LASTRTSCAN;
        m_bNavIsLastRTScan = false;
    }
    
    if (m_bNavIsAfterEcho)
    {
        //TRACE_PUT0(TC_INFO,TF_SEQ,"NO FEEDBACK");
        lMdhFlags |= MDH_FLAG_AFTERECHO;
        m_bNavIsAfterEcho = false;
    }

    if (m_bNoFeedback)
    {
        //TRACE_PUT0(TC_INFO,TF_SEQ,"NO FEEDBACK");
        lMdhFlags |= MDH_FLAG_NOFEEDBACK;
        m_bNoFeedback = false;
    }
    m_ADC.Mdh.setIceProgramPara(MDH_ICEPARAM_FLAGS, lMdhFlags);
    
    switch (pMrProt->NavigatorParam().RFPulseType())
    {
#if defined INCLUDE_SUPPORT_FOR_2DRF_PULSES
    case SEQ::EXCIT_MODE_2D_PENCIL:
        //TRACE_PUT1(TC_ALWAYS, TF_SEQ,"%s: ###in #if defined INCLUDE_SUPPORT_FOR_2DRF_PULSES### ", ptModule); //meee trace
        NavigatorArray& rNaviArray = pMrProt->navigatorArray();
        NLS_STATUS lStatus = SBB_NORMAL;
        
        //cout << ptModule << " pProt->NavigatorParam().RFPulseType() 2D Pencil" << endl;
        double dNormal[3];
        
        //  Calculate the rotation matrix used for the 2DRF pulse
        dNormal[0] = rNaviArray[0].normal().sag();
        dNormal[1] = rNaviArray[0].normal().cor();
        dNormal[2] = rNaviArray[0].normal().tra();
        lStatus = fGSLCalcRotMat(dNormal,
            0.0,
            m_iPatDirection,
            m_iPatPosition,
            &m_RotMatrix180);
        if (setNLSStatus(lStatus,ptModule,"fGSLCalcRotMat")) return false;
        
        //  Execute the 2DRF pulse
        if (! m_2DExcitation.run(pMrProt, pSeqLim, pSeqExpo, NULL) )
        {
            setNLSStatus(m_2DExcitation.getNLSStatus(),ptModule,"m_2DExcitation.run");
            return false;
        }
        
        //  Here we play out the rest of the GRE timing
        fRTEBInit(&m_RotMatrix180);lT=0;
        
        m_GPRFY.run();
        m_GPRFX.run();
        m_GPRORW.run();
        m_GPRO.run();
        
        // Set Frequency/Phase of the ADC
        m_RxNCOSet.setPhase     (0.0);
        m_RxNCOSet.setFrequency ((long)0);
        m_RxNCOSet.setStartTime (m_ADC.getStartTime());
        m_RxNCONeg.setPhase     (0.0);
        m_RxNCONeg.setFrequency ((long)0);
        m_RxNCONeg.setStartTime (m_ADC.getStartTime()+m_ADC.getRoundedDuration());
        
        m_RxNCOSet.run();
        m_ADC.run();
        m_RxNCONeg.run();
        m_GPSp1.run();
        m_GPSp2.run();
        m_GPSp3.run();
        fRTEI(m_GPSp1.getStartTime()+m_GPSp1.getTotalTime());
        
        mSEQTest(NULL,NULL,NULL,RTEB_ORIGIN_fSBBOscBit,0,0,0,0,0);

        setNLSStatus(fRTEBFinish());
        
        break;
#endif
        
    case SEQ::EXCIT_MODE_CROSSED_PAIR:

        mPrintTrace0(DEBUG_CALL,DEBUG_SBB,"SBBNavigator.run (crossed pair)");
                ///TRACE_PUT1(TC_ALWAYS, TF_SEQ,"%s: ###in #case SEQ::EXCIT_MODE_CROSSED_PAIR:### ", ptModule); //meee trace
        // Apply the RF spoiling increment to the 90 pulse
        m_TxNCOSet.setPhase     (m_Txz090Set.getPhase()     + m_dRFPhaseInc);
        m_TxNCOSet.setFrequency (m_Txz090Set.getFrequency()                );
        m_TxNCONeg.setPhase     (m_Txz090Neg.getPhase()     - m_dRFPhaseInc);
        m_TxNCONeg.setFrequency (m_Txz090Neg.getFrequency()                );
        
        fRTEBInit(&m_RotMatrix090); lT=0;
        /************************************ S E Q U E N C E   T I M I N G ************************************************/
        /*            Start Time      |    NCO   |   SRF  |  ADC  |     Gradient Events             | Sync                 */
        /*              (usec)        |   Event  |  Event | Event |   phase  |   read    |  slice   | Event                */
        /*fRTEI(                      ,          ,        ,       ,          ,           ,          ,         );   [ Clock]*/
        /*******************************************************************************************************************/
        fRTEI(lT+=                   0,          0,       0,      0,         0,         0,&m_GPSS090,        0); /*[     0]*/
        fRTEI(lT+=                1000,&m_TxNCOSet,&m_RF090,      0,         0,         0,         0,        0); /*[  1000]*/
        fRTEI(lT+m_RF090.getDuration(),&m_TxNCONeg,       0,      0,         0,         0,         0,        0); /*[ +5120]*/
        fRTEI(lT+=                5120,          0,       0,      0,         0,         0,         0,        0); /*[  6120]*/
        fRTEI(lT+=                 320,          0,       0,      0,         0,         0,  &m_GPSSR,        0); /*[  6440]*/
        fRTEI(lT+=                2560,          0,       0,      0,         0,         0,         0,        0); /*[  9000]*/
        mSEQTest(NULL,NULL,NULL,RTEB_ORIGIN_fSBBOscBit,0,0,0,0,0);
        setNLSStatus(fRTEBFinish());
        
        // Apply the RF spoiling increment to the 180 pulse
        m_TxNCOSet.setPhase     (m_Txz180Set.getPhase()     + m_dRFPhaseInc);
        m_TxNCOSet.setFrequency (m_Txz180Set.getFrequency()                );
        m_TxNCONeg.setPhase     (m_Txz180Neg.getPhase()     - m_dRFPhaseInc);
        m_TxNCONeg.setFrequency (m_Txz180Neg.getFrequency()                );
        
        // Set Frequency/Phase of the ADC
        m_RxNCOSet.setPhase     (0.0);
        m_RxNCOSet.setFrequency ((long)0);
        m_RxNCONeg.setPhase     (0.0);
        m_RxNCONeg.setFrequency ((long)0);
        
        fRTEBInit(&m_RotMatrix180); lT=0;
        /************************************ S E Q U E N C E   T I M I N G ************************************************/
        /*            Start Time      |    NCO   |   SRF  |  ADC  |     Gradient Events             | Sync                 */
        /*              (usec)        |   Event  |  Event | Event |   phase  |   read    |  slice   | Event                */
        /*fRTEI(                      ,          ,        ,       ,          ,           ,          ,         );   [ Clock]*/
        /*******************************************************************************************************************/
        fRTEI(lT+=                   0,          0,       0,      0,         0,         0,  &m_GPS01,       0); /*[  9000]*/
        fRTEI(lT+=                2000,          0,       0,      0,         0,         0,&m_GPSS180,       0); /*[ 11000]*/
        fRTEI(lT+=                 320,&m_TxNCOSet,&m_RF180,      0,         0,         0,         0,       0); /*[ 11320]*/
        fRTEI(lT+m_RF180.getDuration(),&m_TxNCONeg,       0,      0,         0,         0,         0,       0); /*[ +5120]*/
        fRTEI(lT+=                5120,          0,       0,      0,         0, &m_GPRORW,  &m_GPS02,       0); /*[ 16440]*/
        fRTEI(lT+=                2320,          0,       0,      0,         0,   &m_GPRO,         0,       0); /*[ 18760]*/
        fRTEI(lT+=                3072,&m_RxNCOSet,       0, &m_ADC,         0,         0,         0,       0); /*[ 21832]*/
        fRTEI(lT+         lADCDuration,&m_RxNCONeg,       0,      0,         0,         0,         0,       0); /*[ +4096]*/
        fRTEI(lT+=                4098,          0,       0,      0,  &m_GPSp1,  &m_GPSp2,  &m_GPSp3,       0); /*[ 25930]*/
        fRTEI(lT+=                4070,          0,       0,      0,         0,         0,         0,       0); /*[ 30000]*/
        mSEQTest(NULL,NULL,NULL,RTEB_ORIGIN_fSBBOscBit,0,0,0,0,0);
        setNLSStatus(fRTEBFinish());
        
        break;
        
    default:
        setNLSStatus(SEQU_ERROR,ptModule,"Invalid pulse type pProt->NavigatorParam().RFPulseType()");
        return false;
        break;
  }
  
  m_lNavCountTotal++;
  if (fRTIsReadoutEnabled()) m_lNavCount++;
  m_dTimestampAfterNav_ms = RTController::getInstance().getAbsTimeOfEventBlockMSec();
  m_ADC.setRelevantForMeasTime(false);
  
  mPrintTrace1(DEBUG_CALL,DEBUG_SBB,"SBBNavigator.run finished, Nav %1ld",m_lNavNumber);
  return true;
}

//  -----------------------------------------------------------------
//  Implementation of SeqBuildBlockNavigator::run
//  -----------------------------------------------------------------
bool SeqBuildBlockNavigator_JK::runIR (MrProt* /*pMrProt*/, SeqLim* pSeqLim, SeqExpo* /*pSeqExpo*/, sSLICE_POS* /*pSLC*/)
{
    const char* const ptModule = "SBBNavigator::runIR";
    long lT = 0;
    
    if (!isPrepared() || !m_bUseIRSel) return true;
    
    //TRACE_PUT1(TC_INFO,TF_SEQ,"%s: HERE I AM", ptModule);
    
    fRTEBInit(&m_RotMatrix090); lT=0;
    /************************************** S E Q U E N C E   T I M I N G ************************************************/
    /*              Start Time      |    NCO   |   SRF  |  ADC  |     Gradient Events             | Sync                 */
    /*                (usec)        |   Event  |  Event | Event |   phase  |   read    |  slice   | Event                */
    /*fRTEI(                        ,          ,        ,       ,          ,           ,          ,         );   [ Clock]*/
    /*********************************************************************************************************************/
    fRTEI(lT+=                     0,        0,       0,      0,         0,         0,&m_GPSelIR1,        0);
    fRTEI(lT+=                   800,&m_RFSelIRzSet,&m_RFSelIR, 0,       0,         0,          0,        0);
    fRTEI(lT+m_RFSelIR.getDuration(),&m_RFSelIRzNeg,  0,      0,         0,         0,          0,        0);
    fRTEI(lT+=                 11040,        0,       0,      0,         0,         0,&m_GPSelIR2,        0);
    fRTEI(lT+=                  9160,        0,       0,      0,         0,         0,          0,        0);
    mSEQTest(NULL,NULL,NULL,RTEB_ORIGIN_fSBBOscBit,0,0,0,0,0);
    setNLSStatus(fRTEBFinish());
    
    mPrintTrace1(DEBUG_CALL,DEBUG_SBB,"SBBNavigator.runIR finished, Nav %1ld",m_lNavNumber);
    return true;
}

//  -----------------------------------------------------------------
//  Implementation of SeqBuildBlockNavigator::runPause
//  -----------------------------------------------------------------
bool SeqBuildBlockNavigator_JK::runPause(unsigned long lPauseDuration_us, bool bUseWakeup)
{
    if (!isPrepared()) return true;
    fRTEBInit(&sROT_MATRIXUnity);
    if (bUseWakeup)
    {
        fRTEI(0, 0, 0, 0, 0, 0, 0, &m_WAKEUP);
        m_bWakeupDelivered = true;
    }
    fRTEI(lPauseDuration_us, 0, 0, 0, 0, 0, 0, 0);
    mSEQTest(NULL,NULL,NULL,RTEB_ORIGIN_fSBBOscBit,0,0,0,0,0);
    setNLSStatus(fRTEBFinish());
#if (defined VXWORKS) || (defined BUILD_PLATFORM_LINUX)
    //TRACE_PUT1(TC_INFO,TF_SEQ,"Timestamp after pause: %f ms\n",RTController::getInstance().getAbsTimeOfEventBlockMSec() );
#endif
    return true;
}

//  -----------------------------------------------------------------
//  Implementation of SeqBuildBlockNavigator::waitForWakeup
//  -----------------------------------------------------------------
bool SeqBuildBlockNavigator_JK::waitForWakeup()
{
    if (!isPrepared()) return true;
    if (m_bWakeupDelivered)
    {
        fRTWaitForWakeup();
        m_bWakeupDelivered = false;
    }
    return true;
}

//  -----------------------------------------------------------------
//  Implementation of SeqBuildBlockNavigator::semaphoreRelease
//  -----------------------------------------------------------------
bool SeqBuildBlockNavigator_JK::semaphoreRelease (SEQSemaphore* pSemaphore)
{
    if (!isPrepared()) return true;
    pSemaphore->release();
    return true;
}

//  -----------------------------------------------------------------
//  Implementation of SeqBuildBlockNavigator::semaphoreAcquire
//  -----------------------------------------------------------------
bool SeqBuildBlockNavigator_JK::semaphoreAcquire (SEQSemaphore* pSemaphore)
{
    if (!isPrepared()) return true;
    pSemaphore->acquire(0);
    return true;
}

//  -----------------------------------------------------------------
//  Implementation of SeqBuildBlockNavigator::waitForFeedback
//  -----------------------------------------------------------------
bool SeqBuildBlockNavigator_JK::waitForFeedback (SEQSemaphore* pSemaphore, long lPollInterval_us)
{
    const char* const ptModule = "waitForFeedback";
    long lTimeUntilTimeout_us   = 500000; // 0.5 seconds
    long lLoopCountUntilTimeout = (lTimeUntilTimeout_us+lPollInterval_us-1)/lPollInterval_us;
    long lLoopCount             = 0;
    
    if (!isPrepared()) return true;
    
    if (lLoopCountUntilTimeout<=0) lLoopCountUntilTimeout=1;
    while (! pSemaphore->acquire(0))  //  Loop until the feedback has arrived
    {
        // This loop times out after a predefined number of loops
        if (lLoopCount < lLoopCountUntilTimeout)
        {
#if (defined VXWORKS) || (defined BUILD_PLATFORM_LINUX)
            if ((lLoopCount>=0)) //&&(lLoopCount%10==0))
            {
                TRACE_PUT5(TC_INFO,TF_SEQ,"%s: [%ld,%ld] No feedback after %8.3f ms; waiting %ld ms\n",
                    ptModule,m_lNavNumber,m_lNavCount,
                    RTController::getInstance().getAbsTimeOfEventBlockMSec()-getTimestampAfterNav_ms(),
                    lPollInterval_us/1000);
            }
#endif
            if (! runPause(lPollInterval_us,true))
            {
                setNLSStatus(SEQU_ERROR,ptModule,"Error in runPause");
                return false;  
            }
            if (! waitForWakeup() )
            {
                setNLSStatus(SEQU_ERROR,ptModule,"Error in waitForWakeup");
                return false;
            }
            ++lLoopCount;
        }
        else
        {
            setNLSStatus(SEQU_ERROR,ptModule,"Timeout waiting for feedback");
            return false;
        }
    }
    
#if (defined VXWORKS) || (defined BUILD_PLATFORM_LINUX)
    double dMin = getMinNavFeedbackTime_ms();
    double dMax = getMaxNavFeedbackTime_ms();
    double dVal = getTimeSinceLastNav_ms();     // This method alters dmin and dmax!!
    if ((dVal<dMin)||(dVal>dMax))				// Display only changes in the extreme values
    {
        TRACE_PUT4(TC_INFO,TF_SEQ,"%s: [%ld,%ld] Feedback time: %8.3f ms\n",
            ptModule,m_lNavNumber,m_lNavCount,dVal);
    }
#endif
    return true;
}

//  -----------------------------------------------------------------
//  Implementation of SeqBuildBlockNavigator::decide
//  -----------------------------------------------------------------
bool SeqBuildBlockNavigator_JK::decide (long lPreOrPost, long lKernelMode, MrProt* pMrProt, SeqLim* /*pSeqLim*/, SeqExpo* /*pSeqExpo*/, sSLICE_POS* /*pSLC*/)
{
    const char* const ptModule = "decide";
    char tString1[256];
    char tString2[256];
    double dPosCurr = getNavCurrIceResults(0);
    double dPosPrev = getNavPrevIceResults(0);
    double dLow     = 0.0;
    double dHigh    = 0.0;
    
    bool bPre   = getNavExecuteBeforeKernel();
    bool bPost  = getNavExecuteAfterKernel();
    bool isPre  = (lPreOrPost == 0);
    bool isPost = (lPreOrPost == 1);
    
    bool bDebug = ((getSEQDebugFlags() & SEQDebug_ShowFeedbackData) != 0);
    
    if (bDebug)
    {
        sprintf(tString1,"%s: Pos[%6.2f,%6.2f];Win[%6.2f,%6.2f];%4s;Cnt%4ld;",
        ptModule,dPosCurr,dPosPrev,dLow,dHigh,(lPreOrPost==0?" Pre":"Post"),getNavCount());
    }
        
    
    if ( (!isPrepared()) || (lKernelMode == KERNEL_PREPARE) || isNavPrepScan() ||
        (pMrProt->NavigatorParam().RespComp() == SEQ::RESP_COMP_OFF) || 
        (pMrProt->NavigatorParam().RespComp() == SEQ::RESP_COMP_MONITOR_ONLY) )
    {
        m_eNavDecideCode = DoNothing;
        return true;
    }

    // There is a parallel definition of MAG_HEARTBEATS in the ICE world.
    // It is defined in IceProgramNav.cpp
    // If a different number of "learning" heartbeats are desired, make changes to 
    // MAG_HEARTBEATS in both locations.
    if (m_lNavCount <= MAG_HEARTBEATS )
    {
        dLow     = getNavAcceptancePosition_mm() - getNavAcceptanceWidth_mm();
        dHigh    = getNavAcceptancePosition_mm() + getNavAcceptanceWidth_mm();
    }
    else
    {
        dLow     = m_adNewAcceptPosition - getNavAcceptanceWidth_mm();
        dHigh    = m_adNewAcceptPosition + getNavAcceptanceWidth_mm();
    }
    
    switch (pMrProt->NavigatorParam().RespComp())
    {
        
    case SEQ::RESP_COMP_GATE:
        
        if ((bPre ^ bPost) || isPre)
        {
            // This section is used when one of the following conditions is true:
            // a) the nav UI selects navs either before after the kernel, but not both.
            // b) the nav executed before the kernel
            
            // Examine the results of the navigator, to see if it is inside
            // or outside of the acceptance window.
            if ((dPosCurr >= dLow) && (dPosCurr <= dHigh))
            {     
                // The detected point is inside the selected window.  In this case, the
                // measurement proceeds in the usual way.
                if (bDebug)
                {
                    sprintf(tString2,"%s: PreOrPost OK",ptModule);
                }
                m_lNavSuccessCount++;
                m_eNavDecideCode = DoNothing;
            }
            else
            {
                m_eNavDecideCode = BreakAndRepeatNavigator;
                if (bDebug)
                {
                    sprintf(tString2,"%s: BreakAndRepeatNavigator",ptModule);
                }
            }
        }
        else if (bPre && bPost && isPost)
        {
            // If there are two navigators (one pre-Kernel and post-Kernel), the data is accepted
            // only when both the post-kernel and the pre-kernel have acceptable feedback.          
            if ((dPosCurr >= dLow) && (dPosCurr <= dHigh) &&
                (dPosPrev >= dLow) && (dPosPrev <= dHigh))
            {     
                // Both detected points are inside the selected window.  In this case, the
                // measurement proceeds in the usual way.
                if (bDebug)
                {
                    sprintf(tString2,"%s: Pre++Post OK",ptModule);
                }
                m_lNavSuccessCount++;
                m_eNavDecideCode = DoNothing;
            }
            else
            {
                // Either detected point is outside of the selected window.  In this case,
                // the measurement loop is interrupted and another navigator is played out.
                if (bDebug)
                {
                    sprintf(tString2,"%s: BreakAndRepeatNavigator",ptModule);
                }
                m_eNavDecideCode = BreakAndRepeatNavigator;
            }
        }
        else
        {
            m_eNavDecideCode = DoNothing;
            if (bDebug)
            {
                sprintf(tString2,"%s: LOGIC ERROR PROSPECTIVE GATING",ptModule);
            }
            setNLSStatus(SEQU_ERROR,ptModule,"Logic error prospective gating method");
            return false;
        }
        break;
        
    case SEQ::RESP_COMP_GATE_AND_FOLLOW:
        
        if (bPost && !bPre)
        {
            m_eNavDecideCode = DoNothing;
            if (bDebug)
            {
                sprintf(tString2,"%s: LOGIC ERROR: SLICE-FOLLOW WITH ONLY POST ECHO-TRAIN NAV",ptModule);
            }
            setNLSStatus(SEQU_ERROR,ptModule,"Post-echotrain only with slice-follow method");
            return false;
        }
        else if (bPre && bPost && isPost && getNavReferencePositionOK())
        {
            // If there are two navigators (one pre-Kernel and post-Kernel), the data is accepted
            // only when both the post-kernel and the pre-kernel have acceptable feedback, and if
            // the difference between the two points are within the limit specified in the UI.
            if ((dPosCurr >= dLow) && (dPosCurr <= dHigh) &&
                (dPosPrev >= dLow) && (dPosPrev <= dHigh))
            {     
                // Both detected points are inside the selected window, and the difference between
                // them are within the maximum shift value specified in the UI.  In this case, the
                // measurement proceeds in the usual way.
                if (bDebug)
                {
                    sprintf(tString2,"%s: Pre++Post OK",ptModule);
                }
                m_lNavSuccessCount++;
                m_eNavDecideCode = DoNothing;
            }
            else
            {
                // Either detected point is outside of the selected window.  In this case,
                // the measurement loop is interrupted and another navigator is played out.
                if (bDebug)
                {
                    sprintf(tString2,"%s: BreakAndRepeatNavigator",ptModule);
                }
                m_eNavDecideCode = BreakAndRepeatNavigator;
            }
        }
        else if (bPre && bPost && isPost && !getNavReferencePositionOK())
        {
            //No reference was found in the first navigator, and so everything is repeated.
            m_eNavDecideCode = BreakAndRepeatNavigator;
            if (bDebug)
            {
                sprintf(tString2,"%s: BreakAndRepeatNavigator (no ref)",ptModule);
            }
        }
        else if (isPre && !getNavReferencePositionOK())
        {
            // This section is used when both of the following conditions are true:
            // a) the nav is executed before the kernel
            // b) the reference scan (first scan in the acceptance window) is not yet measured.
            
            // Examine the results of the navigator, to see if it is inside
            // or outside of the acceptance window.
            if ((dPosCurr >= dLow) && (dPosCurr <= dHigh))
            {     
                // The detected point is inside the selected window.  In this case, the
                // measurement proceeds in the usual way.
                m_lNavSuccessCount++;
                setNavReferencePosition_mm(dPosCurr);
                if (bDebug)
                {
                    if (bDebug)
                    {
                        sprintf(tString2,"%s: ShiftSlicePositionAndProceed; REFERENCE POSITION SET TO %3f",ptModule,dPosCurr);
                    }
                }
                m_eNavDecideCode = ShiftSlicePositionAndProceed;
            }
            else
            {
                m_eNavDecideCode = BreakAndRepeatNavigator;
                if (bDebug)
                {
                    sprintf(tString2,"%s: BreakAndRepeatNavigator",ptModule);
                }
            }
        }
        else if (isPre && getNavReferencePositionOK())
        {
            if ((dPosCurr >= dLow) && (dPosCurr <= dHigh))
            {     
               setNavShiftAmount_mm(dPosCurr);
               m_lNavSuccessCount++;
               m_eNavDecideCode = ShiftSlicePositionAndProceed;
               if (bDebug)
               {
                    sprintf(tString2,"%s: ShiftSlicePositionAndProceed; SHIFT = %f\n",ptModule,getNavShiftAmountCorrected_mm());
               }
            }
            else
            {
                m_eNavDecideCode = BreakAndRepeatNavigator;
                if (bDebug)
                {
                    sprintf(tString2,"%s: BreakAndRepeatNavigator",ptModule);
                }
            }
        }
        
        else
        {
            m_eNavDecideCode = DoNothing;
            if (bDebug)
            {
                sprintf(tString2,"%s: LOGIC ERROR SLICE FOLLOW",ptModule);
            }
            setNLSStatus(SEQU_ERROR,ptModule,"Logic error slice-follow method");
            return false;
        }
        break;
        
        //case Value_NavModeRope:
        //  m_eNavDecideCode = DoNothing;
        //  break;
        
        //case Value_NavModeRetrospective:
        //  m_eNavDecideCode = DoNothing;
        //  break;
        
    default:
        m_eNavDecideCode = DoNothing;
        break;
    }
#if (defined VXWORKS) || (defined BUILD_PLATFORM_LINUX)
    if (bDebug) TRACE_PUT1(TC_INFO,TF_SEQ,"%s",tString1);
    if (bDebug) TRACE_PUT1(TC_INFO,TF_SEQ,"%s",tString2);
#else
    if (bDebug) printf("%s\n",tString1);
    if (bDebug) printf("%s\n",tString2);
#endif
    
    return true;
}

//Formerly, inline functions:
unsigned long SeqBuildBlockNavigator_JK::getSEQDebugFlags()
{
  return m_ulSEQDebugFlags;
}

long SeqBuildBlockNavigator_JK::getNavMode()
{
  return m_lNavMode;
}

long SeqBuildBlockNavigator_JK::getNavPulseType()
{
  return m_lNavPulseType;
}

long SeqBuildBlockNavigator_JK::getNavFBMode()
{
  return m_lNavFBMode;
}

long SeqBuildBlockNavigator_JK::getNavPrepDuration_ms()
{
  return m_lNavPrepDuration_ms;
}

long SeqBuildBlockNavigator_JK::getNavPrepTR_ms()
{
  return m_lNavPrepTR_ms;
}

double SeqBuildBlockNavigator_JK::getNavAcceptancePosition_mm()
{
  return m_dNavAcceptancePosition_mm;
}

double SeqBuildBlockNavigator_JK::getNavAcceptanceWidth_mm()
{
  return m_dNavAcceptanceWidth_mm;
}

double SeqBuildBlockNavigator_JK::getNavSearchPosition_mm()
{
  return m_dNavSearchPosition_mm;
}

double SeqBuildBlockNavigator_JK::getNavSearchWidth_mm()
{
  return m_dNavSearchWidth_mm;
}

double SeqBuildBlockNavigator_JK::getNavCorrectionFactor()
{
  return m_dNavCorrectionFactor;
}

long SeqBuildBlockNavigator_JK::getNavMatrix()
{
  return m_lNavMatrix;
}

bool SeqBuildBlockNavigator_JK::isNavPrepScan()
{
  return m_bNavPrepScans;
}

void SeqBuildBlockNavigator_JK::setRelevantADC(bool bValue)
{
  m_bRelevantADC = bValue;
}

long SeqBuildBlockNavigator_JK::getNavSleepDuration_ms()
{
  if (isPrepared()) return m_lNavSleepDuration_ms;
  else return 0;
}

long SeqBuildBlockNavigator_JK::getNavPollInterval_ms()
{
  if (isPrepared()) return m_lNavPollInterval_ms;
  else return 0;
}


long SeqBuildBlockNavigator_JK::getNavNumber()
{
  return m_lNavNumber;
}

long SeqBuildBlockNavigator_JK::getNavCount()
{
  return m_lNavCount;
}

long SeqBuildBlockNavigator_JK::getNavCountTotal()
{
  return m_lNavCountTotal;
}

void SeqBuildBlockNavigator_JK::setSEQDebugFlags(unsigned long ulValue)
{
  m_ulSEQDebugFlags = ulValue;
}

void SeqBuildBlockNavigator_JK::setNavIceResults(long lIndex, double dValue)
{
  m_adNavPrevIceResults[lIndex] = m_adNavCurrIceResults[lIndex];
  m_adNavCurrIceResults[lIndex] = dValue;
}

double SeqBuildBlockNavigator_JK::getNavCurrIceResults(long lIndex)
{
  return m_adNavCurrIceResults[lIndex];
}

double SeqBuildBlockNavigator_JK::getNavPrevIceResults(long lIndex)
{
  return m_adNavPrevIceResults[lIndex];
}

void SeqBuildBlockNavigator_JK::setNavPhaseInc(double dValue)
{
  m_dRFPhaseInc = dValue;
}

void SeqBuildBlockNavigator_JK::setNavUseIRSel(bool bValue)
{
  m_bUseIRSel = bValue;
}


double SeqBuildBlockNavigator_JK::getTimestampAfterNav_ms()
{
  return m_dTimestampAfterNav_ms;
}

double SeqBuildBlockNavigator_JK::getTimeSinceLastNav_ms()
{
  double dInterval = RTController::getInstance().getAbsTimeOfEventBlockMSec() - m_dTimestampAfterNav_ms;
  if (dInterval > m_dMaxNavFeedbackTime_ms) m_dMaxNavFeedbackTime_ms = dInterval;
  if (dInterval < m_dMinNavFeedbackTime_ms) m_dMinNavFeedbackTime_ms = dInterval;
  return dInterval;
}

double SeqBuildBlockNavigator_JK::getMaxNavFeedbackTime_ms()
{
  return m_dMaxNavFeedbackTime_ms;
}

double SeqBuildBlockNavigator_JK::getMinNavFeedbackTime_ms()
{
  return m_dMinNavFeedbackTime_ms;
}

void SeqBuildBlockNavigator_JK::setNavFlagForLastRTScan()
{
  m_bNavIsLastRTScan = true;
  return;
}

void SeqBuildBlockNavigator_JK::setNavFlagForLastHPScan()
{
  m_bNavIsLastHPScan = true;
  return;
}

void SeqBuildBlockNavigator_JK::setNavFlagForAfterEcho()
{
  m_bNavIsAfterEcho = true;
  return;
}

void SeqBuildBlockNavigator_JK::setNavFlagForBeforeEcho()
{
  m_bNavIsAfterEcho = false;
  return;
}

void SeqBuildBlockNavigator_JK::setNavFlagForNoFeedback()
{
  m_bNoFeedback = true;
  return;
}

void SeqBuildBlockNavigator_JK::setNavPercentComplete(long lValue)
{
  m_lNavPercentComplete = lValue;
  return;
}

long SeqBuildBlockNavigator_JK::getNavPercentComplete()
{
  return m_lNavPercentComplete;
}

void SeqBuildBlockNavigator_JK::setNavPercentAccepted(long lValue)
{
  m_lNavPercentAccepted = lValue;
  return;
}

long SeqBuildBlockNavigator_JK::getNavPercentAccepted()
{
  return m_lNavPercentAccepted;
}

void SeqBuildBlockNavigator_JK::setNavNumber(long lValue)
{
  m_lNavNumber = lValue;
}

void SeqBuildBlockNavigator_JK::setNavMode (long lValue)
{
  m_lNavMode = lValue;
}

void SeqBuildBlockNavigator_JK::setNavPulseType (long lValue)
{
  m_lNavPulseType = lValue;
}

void SeqBuildBlockNavigator_JK::setNavMatrix (long lValue)
{
  m_lNavMatrix = lValue;
}

void SeqBuildBlockNavigator_JK::setNavFov (long lValue)
{
  m_lNavFov = lValue;
}

void SeqBuildBlockNavigator_JK::setNavAcceptancePosition_mm (double dValue)
{
  m_dNavAcceptancePosition_mm = dValue;
}

void SeqBuildBlockNavigator_JK::setNavAcceptanceWidth_mm (double dValue)
{
  m_dNavAcceptanceWidth_mm = dValue;
}

void SeqBuildBlockNavigator_JK::setNavSearchPosition_mm (double dValue)
{
  m_dNavSearchPosition_mm = dValue;
}

void SeqBuildBlockNavigator_JK::setNavSearchWidth_mm (double dValue)
{
  m_dNavSearchWidth_mm = dValue;
}

void SeqBuildBlockNavigator_JK::setNavCorrectionFactor (double dValue)
{
  m_dNavCorrectionFactor = dValue;
}

void SeqBuildBlockNavigator_JK::setNavPrepScansFlag (bool bValue)
{
  m_bNavPrepScans = bValue;
}

void SeqBuildBlockNavigator_JK::setNavPrepTR_ms (long lValue)
{
  m_lNavPrepTR_ms = lValue;
}

void SeqBuildBlockNavigator_JK::setNavPrepDuration_ms (long lValue)
{
  m_lNavPrepDuration_ms = lValue;
}

void SeqBuildBlockNavigator_JK::setNavSleepDuration_ms (long lValue)
{
  m_lNavSleepDuration_ms = lValue;
}
void SeqBuildBlockNavigator_JK::setNavPollInterval_ms (long lValue)
{
  m_lNavPollInterval_ms = lValue;
}

void SeqBuildBlockNavigator_JK::setNavFBMode (long lValue)
{
  m_lNavFBMode = lValue;
}

void SeqBuildBlockNavigator_JK::setNavExecuteBeforeKernel(bool bValue)
{
  m_bNavExecuteBeforeKernel = bValue;
}

void SeqBuildBlockNavigator_JK::setNavExecuteAfterKernel(bool bValue)
{
  m_bNavExecuteAfterKernel = bValue;
}

bool SeqBuildBlockNavigator_JK::getNavExecuteBeforeKernel()
{
  return m_bNavExecuteBeforeKernel;
}

bool SeqBuildBlockNavigator_JK::getNavExecuteAfterKernel()
{
  return m_bNavExecuteAfterKernel;
}

void SeqBuildBlockNavigator_JK::setNavReferencePosition_mm(double dValue)
{
  m_bNavReferencePositionOK  = true;
  m_dNavReferencePosition_mm = dValue;
}

void SeqBuildBlockNavigator_JK::setNewAcceptPosition (double dValue)
{
  m_adNewAcceptPosition = dValue;
}

double SeqBuildBlockNavigator_JK::getNewAcceptPosition()
{
  return m_adNewAcceptPosition;
}

double SeqBuildBlockNavigator_JK::getNavReferencePosition_mm()
{
  if (m_bNavReferencePositionOK) return m_dNavReferencePosition_mm;
  else                           return (-999.0);
}

bool SeqBuildBlockNavigator_JK::getNavReferencePositionOK()
{
  return m_bNavReferencePositionOK;
}

void SeqBuildBlockNavigator_JK::setNavShiftAmount_mm(double dValue)
{
  m_dNavShiftAmount_mm = dValue - m_dNavReferencePosition_mm;
}

double SeqBuildBlockNavigator_JK::getNavShiftAmountCorrected_mm()
{
  return m_dNavShiftAmount_mm * m_dNavCorrectionFactor;
}

eNavDecideCode SeqBuildBlockNavigator_JK::getNavDecideCode()
{
  return m_eNavDecideCode;
}

double SeqBuildBlockNavigator_JK::getSelIREnergyPerRequest()
{
  return m_dSelIREnergyPerRequest;
}

long SeqBuildBlockNavigator_JK::getSelIRDurationPerRequest()
{
  return m_SelIRDurationPerRequest;
}

long SeqBuildBlockNavigator_JK::getTotalDuration_us()
{
  return (getDurationPerRequest() + 1000*getNavSleepDuration_ms() + getSelIRDurationPerRequest());
}

long SeqBuildBlockNavigator_JK::getTotalDuration_ExcludingIR_us()
{
  return (getDurationPerRequest() + 1000*getNavSleepDuration_ms());
}

double SeqBuildBlockNavigator_JK::getTotalEnergy_Ws()
{
  return (getEnergyPerRequest() + getSelIREnergyPerRequest());
}

double SeqBuildBlockNavigator_JK::getTotalEnergy_ExcludingIR_Ws()
{
  return getEnergyPerRequest();
}
//JK2008
long SeqBuildBlockNavigator_JK::getNoOfNavs()
{
  return m_lNoOfNavs;
}

void SeqBuildBlockNavigator_JK::setNoOfNavs (long lValue)
{
  m_lNoOfNavs = lValue;
}

//JK2008
//ib-start
long SeqBuildBlockNavigator_JK::getNavTR()
{
  return m_lNavTR;
}

void SeqBuildBlockNavigator_JK::setNavTR (long lValue)
{
  m_lNavTR = lValue;
}

//ib-end
//ib-start
long SeqBuildBlockNavigator_JK::getCont1()
{
  return m_lCont1;
}

void SeqBuildBlockNavigator_JK::setCont1 (long lValue)
{
  m_lCont1 = lValue;
}

//ib-end
//ib-start
long SeqBuildBlockNavigator_JK::getTime_start_acq()
{
  return m_lTime_start_acq;
}

void SeqBuildBlockNavigator_JK::setTime_start_acq(long lValue)
{
  m_lTime_start_acq = lValue;
}
//ib-end
//ib-start
long SeqBuildBlockNavigator_JK::getTime_end_acq()
{
  return m_lTime_end_acq;
}

void SeqBuildBlockNavigator_JK::setTime_end_acq(long lValue)
{
  m_lTime_end_acq = lValue;
}
//ib-end
//ib-start
long SeqBuildBlockNavigator_JK::getTime_end_cardiac()
{
  return m_lTime_end_cardiac;
}

void SeqBuildBlockNavigator_JK::setTime_end_cardiac(long lValue)
{
  m_lTime_end_cardiac = lValue;
}
//ib-end
//ib-start
long SeqBuildBlockNavigator_JK::getScout_length()
{
  return m_lScout_length;
}

void SeqBuildBlockNavigator_JK::setScout_length (long lValue)
{
  m_lScout_length = lValue;
}

//ib-end