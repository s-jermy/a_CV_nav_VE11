//    -----------------------------------------------------------------------------
//      Copyright (C) Siemens AG 1998  All Rights Reserved.
//    -----------------------------------------------------------------------------
//
//     Project: NUMARIS/4
//        File: \n4\pkg\MrServers\MrImaging\seq\common\Nav\NavUI.cpp
//     Version: \main\28
//      Author: Clinical
//        Date: 2014-06-05 00:01:44 +02:00
//
//        Lang: C++
//
//     Descrip: MR::Measurement::CSequence::libSeqUtil
//
//     Classes:
//
//    -----------------------------------------------------------------------------

#ifdef WIN32

#include "MrServers/MrImaging/seq/a_CV_nav_VE11/NavigatorShell_sj.h"
#include "MrServers/MrImaging/seq/a_CV_nav_VE11/NavUI_sj.h"

#include "MrServers/MrMeasSrv/SeqIF/Sequence/SeqIF.h"

//#include "MrServers/MrProtSrv/MrProt/MrProtArray.h"
//#include "MrServers/MrProtSrv/MrProt/MrProtTypedefs.h"
#include "MrServers/MrProtSrv/MrProtocol/UILink/MrStdNameTags.h"
#include "MrServers/MrProtSrv/MrProtocol/UILink/StdProtRes/StdProtRes.h"
#include "MrServers/MrProtSrv/MrProtocol/libUILink/UILinkString.h"
#include "MrServers/MrProtSrv/MrProtocol/libUILink/StdRoutines.h"

#include "ProtBasic/Interfaces/MrNavigator.h"

#ifndef SEQ_NAMESPACE
    #error SEQ_NAMESPACE not defined
#endif

using namespace SEQ_NAMESPACE;

#ifndef SEQUENCE_CLASS
    MrUILinkArray::PFctErase          s_pOrigNavEraseHandler = NULL;
    MrUILinkArray::PFctInsert         s_pOrigNavInsertHandler = NULL;
    MrUILinkArray::PFctIsAvailable    s_pOrigNavIsAvailableHandler = NULL;
    LINK_VECTOR_TYPE::PFctSetValue    s_pFctNavPosVecSetVal_orig = NULL;
    LINK_SELECTION_TYPE::PFctSetValue s_pFctNavPosSelSetVal_orig = NULL;
    LINK_DOUBLE_TYPE::PFctSetValue    s_pFctNavPosTraSetVal_orig = NULL;
    LINK_DOUBLE_TYPE::PFctSetValue    s_pFctNavPosCorSetVal_orig = NULL;
    LINK_DOUBLE_TYPE::PFctSetValue    s_pFctNavPosSagSetVal_orig = NULL;

    long s_lOriginalNavMode = 0;
#endif

//  ==================
//  Registry functions
//  ==================
//  If the registry key 'hKeyRoot\lpszPath' does not exists the functions creates it and
//  sets the data of sub-key 'lpszEntry' to ulDef and its type to REG_DWORD.
//  If the key is created by the function it is volatile. i.e. the information is stored
//  in memory and is not preserved when the system is restarted.
//
//  The subsequent operations depend on the value passed to the ulOptionParameter:
//  REG_INT_GET
//    The function retrieves the current value of the key and writes it to rVal.
//  REG_INT_SET
//    The function sets the data of the key to rVal.
//  REG_INT_PREFIX_INCR
//    The function increments the data of the key by the initial value of rVal. rVal
//    receives the value of the keys data after the inccrement operation.
//  REG_INT_POSTFIX_INCR
//    The function increments the data of the key by the initial value of rVal. rVal
//    receives the value of the keys data before the inccrement operation.
//
///////enum RegIntOperation{REG_INT_OPERATION_GET = 1, REG_INT_OPERATION_SET = 2, REG_INT_OPERATION_PREFIX_INCR, REG_INT_OPERATION_POSTFIX_INCR};
///////bool RegIntOperator(const char* lpszPath, const char* lpszEntry, RegIntOperation nOperation, unsigned long& rVal, unsigned long ulDef);


// =====================================
// ========== Selection Boxes ==========
// =====================================

// -------------------------------------------------------------
// Implementation of handler function _SSP_SELECTION_IsAvailable
// -------------------------------------------------------------
static bool _SSP_SELECTION_IsAvailable(LINK_SELECTION_TYPE* const pThis, long lIndex)
{
    MrProt rMrProt (pThis->prot());
    bool bRet = true;
    switch(lIndex)
    {
    default: bRet = true; break;

    case Label_SelectionBox_NavMode:
        if (rMrProt.NavigatorParam().getalFree()[Label_CheckBox_PrepScan] == Value_CheckBox_On)
        {
            bRet = true;  //CHARM 308989 (keep selection box active even when prep scans are active)
            //Previously, bRet was set false.
        }
        else
        {
            bRet = true;
        }
        break;

    case Label_SelectionBox_NavPulseType:
        if (rMrProt.NavigatorParam().getalFree()[Label_SelectionBox_NavMode] == Value_NavModeOff)
        {
            bRet = false;
        }
        else
        {
            bRet = true;
        }
        break;

    case Label_SelectionBox_NavFBMode:
        if (rMrProt.NavigatorParam().getalFree()[Label_SelectionBox_NavMode] == Value_NavModeOff)
        {
            bRet = false;
        }
        else
        {
            bRet = true;
        }
        break;

    case Label_SelectionBox_NavPosition:
        if (rMrProt.NavigatorParam().getalFree()[Label_SelectionBox_NavMode] == Value_NavModeOff)
        {
            bRet = false;
        }
        else
        {
            bRet = true;
        }
        break;

    }
    return bRet;
}

// ------------------------------------------------------------
// Implementation of handler function _SSP_SELECTION_GetLabelId
// ------------------------------------------------------------
static unsigned _SSP_SELECTION_GetLabelId(LINK_SELECTION_TYPE* const pThis, char* arg_list[], long lIndex)
{
    MrProt rMrProt (pThis->prot());
    UNUSED_PARAM(arg_list);
    switch (lIndex)
    {
    case Label_SelectionBox_NavPulseType : return MIR_STD_RF_PULSE_TYPE;  // What is MIR ????
    case Label_SelectionBox_NavMode      : return MIR_STD_RESP_COMP;
    case Label_SelectionBox_NavFBMode    : return MRI_STD_FEEDBACK_TYPE;
    case Label_SelectionBox_NavPosition  : return MIR_STD_CHRON_POS;
    default                              : return MRI_STD_EMPTY;
    }
    return MRI_STD_STRING;
}

// --------------------------------------------------------
// Implementation of handler function _SSP_SELECTION_Format
// --------------------------------------------------------
static int _SSP_SELECTION_Format(LINK_SELECTION_TYPE* const pThis, unsigned nID, char* arg_list[], long lIndex)
{
    MrProt rMrProt (pThis->prot());
    UNUSED_PARAM(arg_list);
    UNUSED_PARAM(nID);

    switch (lIndex)
    {
    case Label_SelectionBox_NavPulseType:
    case Label_SelectionBox_NavMode:
    case Label_SelectionBox_NavFBMode:
    case Label_SelectionBox_NavPosition:
        {
            return 0;
        }

    default:
        {
            return 1;
        }
    }
}

// ----------------------------------------------------------
// Implementation of handler function _SSP_SELECTION_GetValue
// ----------------------------------------------------------
static unsigned _SSP_SELECTION_GetValue(LINK_SELECTION_TYPE* const pThis, long lIndex)
{
    MrProt rMrProt (pThis->prot());
    switch (lIndex)
    {
    case Label_SelectionBox_NavPulseType:
        {
            switch(rMrProt.NavigatorParam().getalFree()[lIndex])
            {
            case Value_NavPulseType2DPencil:
                rMrProt.NavigatorParam().setucRFPulseType(SEQ::EXCIT_MODE_2D_PENCIL);
                return MIR_STD_2D_PENCIL;

            case Value_NavPulseTypeCrossedPair:
                rMrProt.NavigatorParam().setucRFPulseType(SEQ::EXCIT_MODE_CROSSED_PAIR);
                return MIR_STD_CROSSED_PAIR;

            default:
                return MRI_STD_EMPTY;  //  Error

            }
            break;
        }

    case Label_SelectionBox_NavMode:
        {
            switch(rMrProt.NavigatorParam().getalFree()[lIndex])
            {
            case Value_NavModeFollowSlice:
                rMrProt.NavigatorParam().setlRespComp(SEQ::RESP_COMP_GATE_AND_FOLLOW);
                return MIR_STD_GATE_FOLLOW;

            case Value_NavModeProspectiveGating:
                rMrProt.NavigatorParam().setlRespComp(SEQ::RESP_COMP_GATE);
                return MIR_STD_GATE;

            case Value_NavModeMonitorOnly:
                rMrProt.NavigatorParam().setlRespComp(SEQ::RESP_COMP_MONITOR_ONLY);
                return MRI_STD_MONITOR_ONLY;

            case Value_NavModeOff:
                rMrProt.NavigatorParam().setlRespComp(SEQ::RESP_COMP_OFF);
                return MRI_STD_OFF;

            default:
                return MRI_STD_EMPTY;   //  Error
            }
            break;
        }

    case Label_SelectionBox_NavFBMode:
        {
            switch(rMrProt.NavigatorParam().getalFree()[lIndex])
            {
            case Value_NavFBMode_HP: return MRI_STD_FEEDBACK_HP;
            case Value_NavFBMode_RT: return MRI_STD_FEEDBACK_RT;
            default: return MRI_STD_EMPTY;   //  Error
            }
            break;
        }

    case Label_SelectionBox_NavPosition:
        {
            switch(rMrProt.NavigatorParam().getalFree()[lIndex])
            {
            case Value_NavBefEchoTrain:       return MRI_STD_BEFORE_ECHO;
            case Value_NavAftEchoTrain:       return MRI_STD_AFTER_ECHO;
            case Value_NavBefAndAftEchoTrain: return MRI_STD_BEFORE_AND_AFTER_ECHO;
            default: return MRI_STD_EMPTY;//  Error
            }
            break;
        }

    default:
        {
            unsigned nRet = MRI_STD_STRING;
            SET_MODIFIER(nRet,(unsigned char)rMrProt.NavigatorParam().getalFree()[lIndex]);
            return nRet;
            break;
        }
    }
}

// ------------------------------------------------------------
// Implementation of handler function _SSP_SELECTION_GetOptions
// ------------------------------------------------------------
static bool _SSP_SELECTION_GetOptions(LINK_SELECTION_TYPE* const pThis, std::vector<unsigned>& rOptionVector,
                                      unsigned long& rulVerify, long lIndex)
{
    MrProt rMrProt (pThis->prot());
    switch (lIndex)
    {
    case Label_SelectionBox_NavPulseType:
        {
            rulVerify = LINK_SELECTION_TYPE::VERIFY_ON;

            switch (rMrProt.NavigatorParam().getalFree()[Label_SelectionBox_NavMode])
            {
            case Value_NavModeProspectiveGating:
            case Value_NavModeFollowSlice:
            case Value_NavModeMonitorOnly:

                rOptionVector.resize(2);
                rOptionVector[0] = MIR_STD_CROSSED_PAIR;
                rOptionVector[1] = MIR_STD_2D_PENCIL;
                break;

            case Value_NavModeOff:
                break;

            default:
                return false;  //  Error
            }

            return true;
        }

    case Label_SelectionBox_NavMode:
        {
            rulVerify = LINK_SELECTION_TYPE::VERIFY_ON;

            switch (rMrProt.NavigatorParam().getalFree()[Label_SelectionBox_NavPosition])
            {
            case Value_NavBefEchoTrain:
            case Value_NavBefAndAftEchoTrain:

                rOptionVector.resize(4);
                rOptionVector[0] = MIR_STD_GATE_FOLLOW;
                rOptionVector[1] = MIR_STD_GATE;
                rOptionVector[2] = MRI_STD_MONITOR_ONLY;
                rOptionVector[3] = MRI_STD_OFF;
                break;

            case Value_NavAftEchoTrain:

                rOptionVector.resize(3);
                rOptionVector[0] = MIR_STD_GATE;
                rOptionVector[1] = MRI_STD_MONITOR_ONLY;
                rOptionVector[2] = MRI_STD_OFF;
                break;

            default: return false;  //  Error
            }
            return true;
        }

    case Label_SelectionBox_NavFBMode:
        {
            rulVerify = LINK_SELECTION_TYPE::VERIFY_ON;

            switch (rMrProt.NavigatorParam().getalFree()[Label_SelectionBox_NavMode])
            {
            case Value_NavModeProspectiveGating:
            case Value_NavModeFollowSlice:
            case Value_NavModeMonitorOnly:

                rOptionVector.resize(2);
                rOptionVector[0] = MRI_STD_FEEDBACK_RT;
                rOptionVector[1] = MRI_STD_FEEDBACK_HP;
                break;

            case Value_NavModeOff: break;

            default: return false;  //  Error
            }
            return true;
        }

    case Label_SelectionBox_NavPosition:
        {
            rulVerify = LINK_SELECTION_TYPE::VERIFY_ON;
            switch (rMrProt.NavigatorParam().getalFree()[Label_SelectionBox_NavMode])
            {
            case Value_NavModeProspectiveGating:
            case Value_NavModeMonitorOnly:

                rOptionVector.resize(3);
                rOptionVector[0] = MRI_STD_BEFORE_ECHO;
                rOptionVector[1] = MRI_STD_AFTER_ECHO;
                rOptionVector[2] = MRI_STD_BEFORE_AND_AFTER_ECHO;
                break;

            case Value_NavModeFollowSlice:
                rOptionVector.resize(2);
                rOptionVector[0] = MRI_STD_BEFORE_ECHO;
                rOptionVector[1] = MRI_STD_BEFORE_AND_AFTER_ECHO;
                break;

            case Value_NavModeOff:
                break;

            default: return false;
            }
            return true;
        }

    default: return false;  //  Error
  }
}

// ----------------------------------------------------------
// Implementation of handler function _SSP_SELECTION_SetValue
// ----------------------------------------------------------
static unsigned _SSP_SELECTION_SetValue(LINK_SELECTION_TYPE* const pThis, unsigned nNewVal, long lIndex)
{
    MrProt rMrProt (pThis->prot());
    MrUILinkArray::PFctErase pOrigNavEraseHandler  = NULL;
    MrUILinkArray::PFctErase pOrigNavInsertHandler = NULL;
#ifdef SEQUENCE_CLASS
    NavigatorShell *pSeq = static_cast<NavigatorShell*>(pThis->sequence().getSeq());
    pOrigNavEraseHandler  = pSeq->m_pNavUI->m_pOrigNavEraseHandler;
    pOrigNavInsertHandler = pSeq->m_pNavUI->m_pOrigNavInsertHandler;
#else
    pOrigNavEraseHandler  = s_pOrigNavEraseHandler;
    pOrigNavInsertHandler = s_pOrigNavInsertHandler;
#endif
    long nOldVal = rMrProt.NavigatorParam().getalFree()[lIndex];
    long lI = 0;
    switch (lIndex)
    {
    case Label_SelectionBox_NavPulseType:
        {
            switch(nNewVal)
            {
            case MIR_STD_CROSSED_PAIR:
                rMrProt.NavigatorParam().getalFree()[lIndex] = Value_NavPulseTypeCrossedPair;
                rMrProt.NavigatorParam().setucRFPulseType(SEQ::EXCIT_MODE_CROSSED_PAIR);
                break;
            case MIR_STD_2D_PENCIL:
                rMrProt.NavigatorParam().getalFree()[lIndex] = Value_NavPulseType2DPencil;
                rMrProt.NavigatorParam().setucRFPulseType(SEQ::EXCIT_MODE_2D_PENCIL);
                break;
            default:  break;  //  Error
            }

            //  Crossed-pair pulse type needs multiples of 2 navigator GSP objects, while
            //  the 2D pencil uses multiples of 1 GSP objects.
            if( MrUILinkArray* pNavArray = _search<MrUILinkArray>(pThis,MR_TAG_NAVIGATOR_ARRAY) )
            {
                long lCurNmbrOfNav = pNavArray->size(0);
                switch(GET_MODIFIER(nNewVal))
                {
                case Value_NavPulseType2DPencil:
                    switch(lCurNmbrOfNav)
                    {
                        //case 4:
                        //      (*pOrigNavEraseHandler)(pNavArray,3,0);
                        //      (*pOrigNavEraseHandler)(pNavArray,2,0);
                        //      break;
                    case 2:
                        (*pOrigNavEraseHandler)(pNavArray,1,0);
                        break;
                    default:
                        //  do nothing
                        break;
                    }
                    break;
                    case Value_NavPulseTypeCrossedPair:
                        switch(lCurNmbrOfNav)
                        {
                        case 1:
                            (*pOrigNavInsertHandler)(pNavArray,1,0);
                            break;
                            //case 2:
                            //      (*pOrigNavInsertHandler)(pNavArray,2,0);
                            //      (*pOrigNavInsertHandler)(pNavArray,3,0);
                            //      break;
                        default:
                            //  do nothing
                            break;
                        }
                        break;
                        default:
                            break;
                }
            }

            //  When the basic pulse type is modified, set up appropriate defaults for the navigator objects
            if (nOldVal != (long)nNewVal)
            {
                NavigatorArray& rNaviArray = rMrProt.navigatorArray();
                switch(GET_MODIFIER(nNewVal))
                {
                case Value_NavPulseType2DPencil:
                    for (lI = 0; lI<rNaviArray.size(); lI++)
                    {
                        rNaviArray[lI].readoutFOV( 10.0 );
                        rNaviArray[lI].phaseFOV  ( 10.0 );
                        rNaviArray[lI].normal(0,0,1);
                        rNaviArray[lI].position  ( 0.0,0.0,0.0 );
                        //rNaviArray[lI].normal(VectorPat<double>::e_tra);  ...Another way...
                    }
                    break;
                case Value_NavPulseTypeCrossedPair:
                    for (lI = 0; lI<rNaviArray.size(); lI++)
                    {
                        rNaviArray[lI].readoutFOV( 400.0 );
                        rNaviArray[lI].phaseFOV  ( 400.0 );
                        rNaviArray[lI].position  ( 0.0,0.0,0.0 );
                        if(lI%2 == 0)
                        {
                            rNaviArray[lI].normal(VectorPat<double>::e_sag);
                        }
                        else
                        {
                            rNaviArray[lI].normal(SEQ::SAG_TO_COR_TO_TRA,30,0);
                        }
                    }
                    break;
                default:  break;  //  Error
                }
            }
            break;
        }

    case Label_SelectionBox_NavMode:
        {
            NavigatorArray& rNaviArray = rMrProt.navigatorArray();
            long lTmp = 0;

            if (nNewVal != MRI_STD_OFF)
            {
                //CHARM 329090: When navigator control is re-enabled and there are no nav pulses,
                //add new pulses to the GEOMETRY-NAVIGATOR card.
                if( MrUILinkArray* pNavArray = _search<MrUILinkArray>(pThis,MR_TAG_NAVIGATOR_ARRAY) )
                {
                    if (rNaviArray.size() == 0)
                    {
                        if (rMrProt.NavigatorParam().getalFree()[lIndex] == Value_NavPulseTypeCrossedPair)
                        {

                            (*pOrigNavInsertHandler)(pNavArray,0,0);
                            (*pOrigNavInsertHandler)(pNavArray,1,0);
                        }
                        else
                        {
                            (*pOrigNavInsertHandler)(pNavArray,0,0);
                        }
                    }
                }
            }

            switch(nNewVal)
            {
            case MIR_STD_GATE_FOLLOW:
                rMrProt.NavigatorParam().getalFree()[lIndex] = Value_NavModeFollowSlice;
                rMrProt.NavigatorParam().setlRespComp(SEQ::RESP_COMP_GATE_AND_FOLLOW);
                rMrProt.NavigatorParam().setucRespMotionAdaptiveGating(1);
                break;
            case MIR_STD_GATE:
                rMrProt.NavigatorParam().getalFree()[lIndex] = Value_NavModeProspectiveGating;
                rMrProt.NavigatorParam().setlRespComp(SEQ::RESP_COMP_GATE);
                rMrProt.NavigatorParam().setucRespMotionAdaptiveGating(1);
                break;
            case MRI_STD_MONITOR_ONLY:
                rMrProt.NavigatorParam().getalFree()[lIndex] = Value_NavModeMonitorOnly;
                rMrProt.NavigatorParam().setlRespComp(SEQ::RESP_COMP_MONITOR_ONLY);
                rMrProt.NavigatorParam().setucRespMotionAdaptiveGating(0);
                break;

            case MRI_STD_OFF:
                //CHARM 308989 (pass 3)
                if( LINK_BOOL_TYPE* pScoutMode = _search<LINK_BOOL_TYPE>(pThis,MR_TAG_SCOUT_MODE) )
                {
                    if( pScoutMode->isAvailable(Label_CheckBox_PrepScan) )
                    {
                        pScoutMode->value(false,Label_CheckBox_PrepScan);
                    }
                }

                //CHARM 329090
                lTmp = rMrProt.NavigatorParam().getalFree()[lIndex];
#ifdef SEQUENCE_CLASS
                pSeq->m_pNavUI->m_lOriginalNavMode = lTmp;
#else
                s_lOriginalNavMode = lTmp;
#endif
                rMrProt.NavigatorParam().getalFree()[lIndex] = Value_NavModeOff;
                rMrProt.NavigatorParam().setlRespComp(SEQ::RESP_COMP_OFF);
                rMrProt.NavigatorParam().setucRespMotionAdaptiveGating(0);

                //When navigator control is turned off, we also delete the selected navigator pulses from
                //the GEOMETRY card, but the old values are remembered so that they can be restored later
                //if it is turned on again.
                
                for (lI=rNaviArray.size()-1; lI>=0; lI--)
                {
                  rNaviArray.erase(lI);
                }

                //CHARM 308989 (pass 2 changes were removed)
                //rMrProt.NavigatorParam().getalFree()[Label_CheckBox_PrepScan] = Value_CheckBox_Off;  //CHARM 308989
                break;
            default:  break;  //  Error
            }
            break;
        }

    case Label_SelectionBox_NavFBMode:
        {
            switch(nNewVal)
            {
            case MRI_STD_FEEDBACK_RT:
                rMrProt.NavigatorParam().getalFree()[lIndex] = Value_NavFBMode_RT;
                break;
            case MRI_STD_FEEDBACK_HP:
                rMrProt.NavigatorParam().getalFree()[lIndex] = Value_NavFBMode_HP;
                break;
            default:  break;  //  Error
            }
            break;
        }

    case Label_SelectionBox_NavPosition:
        {
            switch(nNewVal)
            {
            case MRI_STD_BEFORE_ECHO:
                rMrProt.NavigatorParam().getalFree()[lIndex] = Value_NavBefEchoTrain;
                break;
            case MRI_STD_AFTER_ECHO:
                rMrProt.NavigatorParam().getalFree()[lIndex] = Value_NavAftEchoTrain;
                break;
            case MRI_STD_BEFORE_AND_AFTER_ECHO:
                rMrProt.NavigatorParam().getalFree()[lIndex] = Value_NavBefAndAftEchoTrain;
                break;
            default:  break;  //  Error
            }
        }

    default:
        {
        }

  }

  return pThis->value(lIndex);
}

// --------------------------------------------------------------
// Implementation of handler function _SSP_SELECTION_GetToolTipId
// --------------------------------------------------------------
static unsigned _SSP_SELECTION_GetToolTipId(LINK_SELECTION_TYPE* const pThis, char* arg_list[], long lIndex)
{
    unsigned nRet = MRI_STD_EMPTY;

    switch (lIndex)
    {
    case Label_SelectionBox_NavPulseType:
            nRet = MRI_STD_NAVUI_TOOLTIP_PULSE_TYPE;
            break;

    case Label_SelectionBox_NavMode:
            nRet = MRI_STD_NAVUI_TOOLTIP_NAV_MODE;
            break;

    case Label_SelectionBox_NavFBMode:
            nRet = MRI_STD_NAVUI_TOOLTIP_FEEDBACK_MODE;
            break;

    case Label_SelectionBox_NavPosition:
        {
            MrProt rMrProt (pThis->prot());
            switch (rMrProt.NavigatorParam().getalFree()[Label_SelectionBox_NavMode])
            {
            case Value_NavModeProspectiveGating:
            case Value_NavModeMonitorOnly:
                nRet = MRI_STD_NAVUI_TOOLTIP_CHRON_POSITION1;
                break;

            case Value_NavModeFollowSlice:
                nRet = MRI_STD_NAVUI_TOOLTIP_CHRON_POSITION2;
                break;

            default:
                nRet = MRI_STD_EMPTY;
                break;
            }
        }

    default:
        nRet = MRI_STD_EMPTY;
        break;
    }
    return nRet;
}

// =================================
// ========== Check Boxes ==========
// =================================

// ------------------------------------------------------------
// Implementation of handler function _SSP_CHECKBOX_IsAvailable
// ------------------------------------------------------------
static bool _SSP_CHECKBOX_IsAvailable(LINK_BOOL_TYPE* const pThis, long lIndex)
{
    MrProt rMrProt (pThis->prot());
    bool bRet = true;
    if (rMrProt.NavigatorParam().getalFree()[Label_SelectionBox_NavMode] == Value_NavModeOff)
    {
        rMrProt.NavigatorParam().getalFree()[Label_CheckBox_PrepScan] = Value_CheckBox_Off; //CHARM 308989
        switch (lIndex)
        {
        case Label_CheckBox_PrepScan: bRet = false; break;
        default:                      bRet = false; break;
        }
    }
    else
    {
        switch (lIndex)
        {
        case Label_CheckBox_PrepScan:
            bRet = true;
            break;

        default:
            bRet = true;
            break;
        }
    }
    return bRet;
}

// -----------------------------------------------------------
// Implementation of handler function _SSP_CHECKBOX_GetLabelId
// -----------------------------------------------------------
static unsigned _SSP_CHECKBOX_GetLabelId(LINK_BOOL_TYPE* const pThis, char* arg_list[], long lIndex)
{
    MrProt rMrProt (pThis->prot());
    UNUSED_PARAM(arg_list);

    switch (lIndex)
    {
    case Label_CheckBox_PrepScan    : return MRI_STD_SCOUT_MODE;
    default    : return MRI_STD_EMPTY;  // Error
    }
    return MRI_STD_STRING;
}

// -----------------------------------------------------------
// Implementation of handler function _SSP_CHECKBOX_GetOptions
// -----------------------------------------------------------
static bool _SSP_CHECKBOX_GetOptions(LINK_BOOL_TYPE* const pThis, std::vector<unsigned>& rOptionVector,
                                     unsigned long& rulVerify, long lIndex)
{
    MrProt rMrProt (pThis->prot());
    UNUSED_PARAM(lIndex);

    rulVerify = LINK_BOOL_TYPE::VERIFY_ON;
    rOptionVector.resize(2);
    rOptionVector[0] = false;
    rOptionVector[1] = true;
    return true;
}

// ---------------------------------------------------------
// Implementation of handler function _SSP_CHECKBOX_GetValue
// ---------------------------------------------------------
static bool _SSP_CHECKBOX_GetValue(LINK_BOOL_TYPE* const pThis, long lIndex)
{
    MrProt rMrProt (pThis->prot());
    return rMrProt.NavigatorParam().getalFree()[lIndex] != Value_CheckBox_Off;
}

// ---------------------------------------------------------
// Implementation of handler function _SSP_CHECKBOX_SetValue
// ---------------------------------------------------------
static bool _SSP_CHECKBOX_SetValue(LINK_BOOL_TYPE* const pThis, bool lNewVal, long lIndex)
{
    MrProt rMrProt (pThis->prot());
    switch (lIndex)
    {
    case Label_CheckBox_PrepScan:
        {
            if (lNewVal) rMrProt.NavigatorParam().getalFree()[lIndex] = Value_CheckBox_On;
            else         rMrProt.NavigatorParam().getalFree()[lIndex] = Value_CheckBox_Off;

            return true;
            break;
        }

    default: break;
    }
    return false;
}

// -------------------------------------------------------------
// Implementation of handler function _SSP_CHECKBOX_GetToolTipId
// -------------------------------------------------------------
static unsigned _SSP_CHECKBOX_GetToolTipId(LINK_BOOL_TYPE* const /*pThis*/, char* arg_list[], long lIndex)
{
    unsigned nRet = MRI_STD_EMPTY;

    switch (lIndex)
    {
    case Label_CheckBox_PrepScan:
        nRet = MRI_STD_NAVUI_TOOLTIP_SCOUTSCAN_CHECKBOX;
        break;

    default:
        nRet = MRI_STD_EMPTY;
        break;
    }
    return nRet;
}

// =====================================
// ========== LONG Parameters ==========
// =====================================

// -------------------------------------------------------
// Implementation of handler function _SSP_LONG_GetLabelId
// -------------------------------------------------------
static unsigned _SSP_LONG_GetLabelId(LINK_LONG_TYPE* const, char* arg_list[], long lIndex)
{
    switch(lIndex)
    {
    case Label_Long_NavMatrix               : arg_list[0] = "Navigator Matrix";     break;
    case Label_Long_NavFov                  : arg_list[0] = "Navigator FOV";        break;

	case Label_Long_NoOfNavs				: arg_list[0] = "Number of Navigators"; 		break; //JK2008
    case Label_Long_TimeStartAcq			: arg_list[0] = "Time to Start Acquisition";	break; //JK2008
    case Label_Long_TimeEndAcq				: arg_list[0] = "Time to End Acquisition";		break; //JK2008
    case Label_Long_TimeEndCardiac			: arg_list[0] = "Time Cardiac Cycle Ends";		break; //JK2008
    case Label_Long_NavTR_ms				: arg_list[0] = "Navigators TR(ms)"; 			break; //ib
    case Label_Long_PoleSensitivity			: arg_list[0] = "Pole Sensitivity"; 			break; //ib
    case Label_Long_ScoutLength				: arg_list[0] = "Scout Length"; 				break; //ib
    case Label_Long_SliceSelection			: arg_list[0] = "Slice Selection"; 				break; //ib

    case Label_Long_NavPrepTR_ms            : return MIR_STD_SCOUT_TR;
    case Label_Long_NavPrepDuration_sec     : return MIR_STD_SCOUT_DURATION;
    case Label_Long_NavSleepDuration_ms     : return MRI_STD_FEEDBACK_TIME;
    default                                 : return MRI_STD_EMPTY;  // Error
    }
    return MRI_STD_STRING;
}

// ------------------------------------------------------
// Implementation of handler function _SSP_LONG_GetUnitId
// ------------------------------------------------------
static unsigned _SSP_LONG_GetUnitId(LINK_LONG_TYPE* const, char* arg_list[], long lIndex)
{
    UNUSED_PARAM(arg_list);
    switch(lIndex)
    {
    case Label_Long_NavMatrix               : return MRI_STD_EMPTY;
    case Label_Long_NavFov                  : return MRI_STD_UNIT_MM;

	case Label_Long_NoOfNavs				: return MRI_STD_EMPTY;		//JK2008
    case Label_Long_TimeStartAcq			: return MRI_STD_EMPTY;		//JK2008
    case Label_Long_TimeEndAcq				: return MRI_STD_EMPTY;		//JK2008
    case Label_Long_TimeEndCardiac			: return MRI_STD_EMPTY;		//JK2008
    case Label_Long_NavTR_ms				: return MRI_STD_EMPTY;		//ib
    case Label_Long_PoleSensitivity			: return MRI_STD_EMPTY;		//ib
    case Label_Long_ScoutLength				: return MRI_STD_EMPTY;		//ib
    case Label_Long_SliceSelection			: return MRI_STD_EMPTY;		//ib

    case Label_Long_NavPrepTR_ms            : return MRI_STD_UNIT_MS;
    case Label_Long_NavPrepDuration_sec     : return MRI_STD_UNIT_SEC;
    case Label_Long_NavSleepDuration_ms     : return MRI_STD_UNIT_MS;
    default                                 : return MRI_STD_EMPTY;  // Error
    }
    return MRI_STD_STRING;
}

// --------------------------------------------------------
// Implementation of handler function _SSP_LONG_IsAvailable
// --------------------------------------------------------
static bool _SSP_LONG_IsAvailable(LINK_LONG_TYPE* const pThis, long lIndex)
{
    MrProt rMrProt (pThis->prot());
    bool bRet = true;
    if (rMrProt.NavigatorParam().getalFree()[Label_SelectionBox_NavMode] == Value_NavModeOff)
    {
        bRet = false;
    }
    else
    {
        switch(lIndex)
        {
        default                                 : bRet = true; break;
        case Label_Long_NavMatrix               : bRet = true; break;
        case Label_Long_NavFov                  : bRet = true; break;

		case Label_Long_NoOfNavs			    : bRet = true; break;	//JK2008
        case Label_Long_TimeStartAcq			: bRet = true; break;	//ib
        case Label_Long_TimeEndAcq				: bRet = true; break;	//ib
        case Label_Long_TimeEndCardiac			: bRet = true; break;	//ib
        case Label_Long_NavTR_ms			    : bRet = true; break;	//ib
        case Label_Long_PoleSensitivity			: bRet = true; break;	//ib
        case Label_Long_ScoutLength				: bRet = true; break;	//ib
        case Label_Long_SliceSelection		    : bRet = true; break;	//ib

        case Label_Long_NavSleepDuration_ms     : bRet = true; break;
        case Label_Long_NavPrepTR_ms            : bRet = true; break;

        case Label_Long_NavPrepDuration_sec:
            {
                if (rMrProt.NavigatorParam().getalFree()[Label_CheckBox_PrepScan] == Value_CheckBox_On)
                {
                    bRet = true;
                }
                else
                {
                    bRet = true;  //CHARM 308989 (keep dialog active even when prep scans are active)
                    //Previously, bRet was set false.
                }
                break;
            }
        }
    }
    return bRet;
}

// ------------------------------------------------------
// Implementation of handler function _SSP_LONG_GetLimits
// ------------------------------------------------------
static bool _SSP_LONG_GetLimits(LINK_LONG_TYPE* const pThis, std::vector<MrLimitLong>& rLimitVector,
                                unsigned long& rulVerify, long lIndex)
{
    MrProt rMrProt (pThis->prot());
    UNUSED_PARAM(pThis);

    long lMin, lMax, lInc;
    rulVerify = LINK_LONG_TYPE::VERIFY_BINARY_SEARCH;
    rLimitVector.resize(1);

    switch(lIndex)
    {
    case Label_Long_NavMatrix               : lMin= 128 ; lMax=512     ; lInc=128 ;
        return rLimitVector[0].setBase2(lMin,lMax);
        break;
    case Label_Long_NavFov                  : lMin= 128 ; lMax=512     ; lInc=1   ; break;

	case Label_Long_NoOfNavs                : lMin= 1	; lMax=10	   ; lInc=1   ; break; //JK2008
    case Label_Long_TimeStartAcq			: lMin= 200	; lMax=1500	   ; lInc=1   ; break; //ib
    case Label_Long_TimeEndAcq         		: lMin= 250	; lMax=2000	   ; lInc=1   ; break; //ib
    case Label_Long_TimeEndCardiac			: lMin= 300	; lMax=2000	   ; lInc=1   ; break; //ib
    case Label_Long_NavTR_ms              	: lMin= 40	; lMax=400	   ; lInc=10  ; break; //ib
    case Label_Long_PoleSensitivity       	: lMin= 1	; lMax=3	   ; lInc=1   ; break; //ib
    case Label_Long_ScoutLength           	: lMin= 256	; lMax=1024	   ; lInc=256 ; break; //ib
    case Label_Long_SliceSelection        	: lMin= 1	; lMax=2	   ; lInc=1	  ; break; //ib???

    case Label_Long_NavPrepTR_ms            : lMin= 50  ; lMax=10000   ; lInc=1   ; break;
    case Label_Long_NavPrepDuration_sec     : lMin= 2   ; lMax=6000    ; lInc=1   ; break;
    case Label_Long_NavSleepDuration_ms     : lMin= 0   ; lMax=9999    ; lInc=1   ; break;
    default                                 : lMin= 0   ; lMax=0       ; lInc=1   ; break;
    }

    rLimitVector[0].setEqualSpaced(lMin,lMax,lInc);
    return true;
}

// -----------------------------------------------------
// Implementation of handler function _SSP_LONG_GetValue
// -----------------------------------------------------
static long _SSP_LONG_GetValue(LINK_LONG_TYPE* const pThis, long lIndex)
{
    MrProt rMrProt (pThis->prot());
    return rMrProt.NavigatorParam().getalFree()[lIndex];
}

// -----------------------------------------------------
// Implementation of handler function _SSP_LONG_SetValue
// -----------------------------------------------------
static long _SSP_LONG_SetValue(LINK_LONG_TYPE* const pThis, long value, long lIndex)
{
    MrProt rMrProt (pThis->prot());
    return (rMrProt.NavigatorParam().getalFree()[lIndex] = value);
}


// ---------------------------------------------------------
// Implementation of handler function _SSP_LONG_GetToolTipId
// ---------------------------------------------------------
unsigned _SSP_LONG_GetToolTipId(LINK_LONG_TYPE* const pThis, char* arg_list[], long lIndex)
{
    unsigned nRet = MRI_STD_EMPTY;

    switch (lIndex)
    {
    case Label_Long_NavMatrix:
        nRet = MRI_STD_NAVUI_TOOLTIP_MATRIX_SIZE;
        break;

    case Label_Long_NavFov:
        nRet = MRI_STD_NAVUI_TOOLTIP_FOV;
        break;

	//JK2008 - start
	case Label_Long_NoOfNavs:
        //sprintf(tLine,  "The total number of pre navs played out (including FB one)."); strcat(tToolTip,tLine);
        break;
	//JK2008 - end

	//ib - start
	case Label_Long_TimeStartAcq:
        //sprintf(tLine,  "Enter the time that the acquisition should start"); strcat(tToolTip,tLine);
        break;

	case Label_Long_TimeEndAcq:
        //sprintf(tLine,  "Enter the time that the acquisition should end"); strcat(tToolTip,tLine);
        break;

	case Label_Long_TimeEndCardiac:
        //sprintf(tLine,  "enter the time that the acquisition should start"); strcat(tToolTip,tLine);
        break;

	case Label_Long_NavTR_ms:
        //sprintf(tLine,  "The total TR time for each navigator, recovery time before next navigator"); strcat(tToolTip,tLine); //ib
        break;

	case Label_Long_PoleSensitivity:
        //sprintf(tLine,  "Set the sensitivity of the poles"); strcat(tToolTip,tLine); //ib?
        break;

	case Label_Long_ScoutLength:
        //sprintf(tLine,  "How many points to collect for model"); strcat(tToolTip,tLine); //ib
        break;

	case Label_Long_SliceSelection:
        //sprintf(tLine,  "Use slice following or not"); strcat(tToolTip,tLine); //ib
        break;
	//ib - end

    case Label_Long_NavPrepTR_ms:
        nRet = MRI_STD_NAVUI_TOOLTIP_SCOUT_TR;
        break;

    case Label_Long_NavPrepDuration_sec:
        nRet = MRI_STD_NAVUI_TOOLTIP_SCOUT_DURATION;
        break;

    case Label_Long_NavSleepDuration_ms:
        nRet = MRI_STD_NAVUI_TOOLTIP_SLEEPDURATION;
        break;

    default: 
        nRet = MRI_STD_EMPTY;
        break;
    }

    return nRet;
}

// ===============================================================
// ========== LONG Parameters for Service (in an array) ==========
// ===============================================================

// ------------------------------------------------------------
// Implementation of handler function fNaviExtraSwitchesMaxSize
// ------------------------------------------------------------
long fNaviExtraSwitchesMaxSize(MrUILinkArray* const, long)
{
    return 8;
}

// ---------------------------------------------------------
// Implementation of handler function fNaviExtraSwitchesSize
// ---------------------------------------------------------
long fNaviExtraSwitchesSize(MrUILinkArray* const pThis, long)
{
    MrProt rMrProt (pThis->prot());
    return 8;
}

// ----------------------------------------------------------------
// Implementation of handler function fNaviExtraSwitchesIsAvailable
// ----------------------------------------------------------------
static bool fNaviExtraSwitchesIsAvailable(LINK_LONG_TYPE* const pThis, long)
{
    MrProt rMrProt (pThis->prot());
    return true;
}

// ---------------------------------------------------------------
// Implementation of handler function fNaviExtraSwitchesGetLabelId
// ---------------------------------------------------------------
static unsigned fNaviExtraSwitchesGetLabelId(MrUILinkBase* const, char* arg_list[], long lPos)
{
    static const char* const pszLabel[] =
    {
        "Rotate Plot"
            ,"SEQ Debug Flags"
            ,"ICE Debug Flags"
            ,"Smooth Length"
            ,"LSFit Length"
            ,"Image Send Interval"
            ,"Time Scale Interval"
            ,"FB Polling Interval"
    };
    arg_list[0] = (char*) pszLabel[lPos];
    return MRI_STD_STRING;
}

// --------------------------------------------------------------
// Implementation of handler function fNaviExtraSwitchesGetUnitId
// --------------------------------------------------------------
static unsigned fNaviExtraSwitchesGetUnitId(LINK_LONG_TYPE* const, char* arg_list[], long lPos)
{
    static const char* const pszLabel[] =
    {
        ""
            ,""
            ,""
            ,"[mm]"
            ,"[mm]"
            ,"[ms]"
            ,"[sec]"
            ,"[ms]"
    };
    arg_list[0] = (char*) pszLabel[lPos];
    return MRI_STD_STRING;
}

// -------------------------------------------------------------
// Implementation of handler function fNaviExtraSwitchesGetValue
// -------------------------------------------------------------
static long fNaviExtraSwitchesGetValue(LINK_LONG_TYPE* const pThis, long lPos)
{
    MrProt rMrProt (pThis->prot());
    switch (lPos)
    {
    case 0:
        return rMrProt.NavigatorParam().getalFree()[Label_Long_Arr0_RotatePlot];
        break;
    case 1:
        return rMrProt.NavigatorParam().getalFree()[Label_Long_Arr1_SEQDebugFlags];
        break;
    case 2:
        return rMrProt.NavigatorParam().getalFree()[Label_Long_Arr2_ICEDebugFlags];
        break;
    case 3:
        return rMrProt.NavigatorParam().getalFree()[Label_Long_Arr3_ICESmoothLength];
        break;
    case 4:
        return rMrProt.NavigatorParam().getalFree()[Label_Long_Arr4_ICELSQFitLength];
        break;
    case 5:
        return rMrProt.NavigatorParam().getalFree()[Label_Long_Arr5_ICEImageSendInterval_Msec];
        break;
    case 6:
        return rMrProt.NavigatorParam().getalFree()[Label_Long_Arr6_ICETimeScaleInterval_Sec];
        break;
    case 7:
        return rMrProt.NavigatorParam().getalFree()[Label_Long_Arr7_FeedbackPollInterval_ms];
        break;
    default:
        return 0;
    }
}

// -------------------------------------------------------------
// Implementation of handler function fNaviExtraSwitchesSetValue
// -------------------------------------------------------------
static long fNaviExtraSwitchesSetValue(LINK_LONG_TYPE* const pThis, long lNewVal, long lPos)
{
    MrProt rMrProt (pThis->prot());
    switch (lPos)
    {

    case 0:
        {
            long lOldVal = rMrProt.NavigatorParam().getalFree()[Label_Long_Arr0_RotatePlot];
            if (lOldVal != lNewVal)
            {
                rMrProt.NavigatorParam().getadFree()[Label_Double_NavCorrectionFactor] *= -1.0;
            }
            return (rMrProt.NavigatorParam().getalFree()[Label_Long_Arr0_RotatePlot] = lNewVal);
            break;
        }
    case 1:
        //printf("Prot Array getalFree()[%ld] set to %ld\n",Label_Long_Arr1_SEQDebugFlags,lNewVal);
        return (rMrProt.NavigatorParam().getalFree()[Label_Long_Arr1_SEQDebugFlags] = lNewVal);
        break;
    case 2:
        return (rMrProt.NavigatorParam().getalFree()[Label_Long_Arr2_ICEDebugFlags] = lNewVal);
        break;
    case 3:
        return (rMrProt.NavigatorParam().getalFree()[Label_Long_Arr3_ICESmoothLength] = lNewVal);
        break;
    case 4:
        return (rMrProt.NavigatorParam().getalFree()[Label_Long_Arr4_ICELSQFitLength] = lNewVal);
        break;
    case 5:
        return (rMrProt.NavigatorParam().getalFree()[Label_Long_Arr5_ICEImageSendInterval_Msec] = lNewVal);
        break;
    case 6:
        return (rMrProt.NavigatorParam().getalFree()[Label_Long_Arr6_ICETimeScaleInterval_Sec] = lNewVal);
        break;
    case 7:
        return (rMrProt.NavigatorParam().getalFree()[Label_Long_Arr7_FeedbackPollInterval_ms] = lNewVal);
        break;
    default:
        return 0;
        break;
    }
}

// --------------------------------------------------------------
// Implementation of handler function fNaviExtraSwitchesGetLimits
// --------------------------------------------------------------
static bool fNaviExtraSwitchesGetLimits(LINK_LONG_TYPE* const pThis, std::vector<MrLimitLong>& rLimitVector,
                                        unsigned long& rulVerify, long lIndex)
{
    MrProt rMrProt (pThis->prot());

    long lMin, lMax, lInc;
    rulVerify = LINK_LONG_TYPE::VERIFY_BINARY_SEARCH;
    rLimitVector.resize(1);

    switch (lIndex)
    {
    case 0:
        lMin = 0; lMax = 1; lInc = 1;
        break;
    case 1:
        lMin = 0; lMax = 0x7fffffff; lInc = 1;
        break;
    case 2:
        lMin = 0; lMax = 0x7fffffff; lInc = 1;
        break;
    case 3:
        lMin = 3; lMax = 256; lInc = 1;
        break;
    case 4:
        lMin = 3; lMax = 256; lInc = 1;
        break;
    case 5:
        lMin = 100; lMax = 30000; lInc = 100;
        break;
    case 6:
        lMin = 1; lMax = 30; lInc = 1;
        break;
    case 7:
        lMin = 1; lMax = 2000; lInc = 1;
        break;
    default:
        lMin = 0; lMax = 0x7fffffff; lInc = 1;
        break;
    }
    rLimitVector[0].setEqualSpaced(lMin,lMax,lInc);
    return true;
}

// ===============================================
// ========== DOUBLE Parameters (ARRAY) ==========
// ===============================================

// ---------------------------------------------------------
// Implementation of handler function fNaviConstArrayMaxSize
// ---------------------------------------------------------
long fNaviConstArrayMaxSize(MrUILinkArray* const, long)
{
    return 1;
}

// ---------------------------------------------------------------
// Implementation of handler function fNavAcceptanceWidthArraySize
// ---------------------------------------------------------------
long fNavAcceptanceWidthArraySize(MrUILinkArray* const pThis, long)
{
    MrProt rMrProt (pThis->prot());
    if (rMrProt.NavigatorParam().getalFree()[Label_SelectionBox_NavMode] == Value_NavModeOff)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

// ----------------------------------------------------------------
// Implementation of handler function fNavAcceptanceWidthGetLabelId
// ----------------------------------------------------------------
static unsigned fNavAcceptanceWidthGetLabelId(MrUILinkBase* const, char* arg_list[], long lPos)
{
    UNUSED_PARAM(arg_list);
    UNUSED_PARAM(lPos);

    return MIR_STD_ACCEPT_WND;
}

// ---------------------------------------------------------------
// Implementation of handler function fNavAcceptanceWidthGetUnitId
// ---------------------------------------------------------------
static unsigned fNavAcceptanceWidthGetUnitId(LINK_DOUBLE_TYPE* const, char* arg_list[], long lPos)
{
    UNUSED_PARAM(arg_list);
    UNUSED_PARAM(lPos);

    return MRI_STD_UNIT_MM;
}

// -----------------------------------------------------------------
// Implementation of handler function fNavAcceptanceWidthIsAvailable
// -----------------------------------------------------------------
static bool fNavAcceptanceWidthIsAvailable(LINK_DOUBLE_TYPE* const pThis, long)
{
    MrProt rMrProt (pThis->prot());
    bool bRet = true;
    if (rMrProt.NavigatorParam().getalFree()[Label_SelectionBox_NavMode] == Value_NavModeOff)
    {
        bRet = false;
    }
    else
    {
        bRet = true;
    }
    return bRet;
}

// ---------------------------------------------------------------
// Implementation of handler function fNavAcceptanceWidthGetLimits
// ---------------------------------------------------------------
static bool fNavAcceptanceWidthGetLimits(LINK_DOUBLE_TYPE* const pThis, std::vector<MrLimitDouble>& rLimitVector,
                                         unsigned long& rulVerify, long lIndex)
{
    MrProt rMrProt (pThis->prot());
    UNUSED_PARAM(lIndex);

    rulVerify = LINK_LONG_TYPE::VERIFY_BINARY_SEARCH;
    rLimitVector.resize(1);  // Number of limit intervals
    double dMin = 0.0;
    double dMax = rMrProt.NavigatorParam().getalFree()[Label_Long_NavMatrix] - 1.0;
    double dInc = 0.5;
    rLimitVector[0].setEqualSpaced(dMin,dMax,dInc);
    return true;
}

// --------------------------------------------------------------
// Implementation of handler function fNavAcceptanceWidthGetValue
// --------------------------------------------------------------
static double fNavAcceptanceWidthGetValue(LINK_DOUBLE_TYPE* const pThis, long lPos)
{
    MrProt rMrProt (pThis->prot());
    UNUSED_PARAM(lPos);
    return (double) rMrProt.NavigatorParam().getadFree()[Label_Double_NavAcceptanceWidth];
}

// --------------------------------------------------------------
// Implementation of handler function fNavAcceptanceWidthSetValue
// --------------------------------------------------------------
static double fNavAcceptanceWidthSetValue(LINK_DOUBLE_TYPE* const pThis, double dNewVal, long lPos)
{
    MrProt rMrProt (pThis->prot());
    UNUSED_PARAM(lPos);
    return (rMrProt.NavigatorParam().getadFree()[Label_Double_NavAcceptanceWidth] = dNewVal);
}

// ------------------------------------------------------------------
// Implementation of handler function fNavAcceptanceWidthGetToolTipId
// ------------------------------------------------------------------
static unsigned fNavAcceptanceWidthGetToolTipId(LINK_DOUBLE_TYPE* const pThis, char* arg_list[], long lIndex)
{
    unsigned nRet = MRI_STD_NAVUI_TOOLTIP_ACCEPTANCE_WIDTH;
    return nRet;
}

// -----------------------------------------------------------
// Implementation of handler function fNavSearchWidthArraySize
// -----------------------------------------------------------
long fNavSearchWidthArraySize(MrUILinkArray* const pThis, long)
{
    MrProt rMrProt (pThis->prot());
    if (rMrProt.NavigatorParam().getalFree()[Label_SelectionBox_NavMode] == Value_NavModeOff)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

// ------------------------------------------------------------
// Implementation of handler function fNavSearchWidthGetLabelId
// ------------------------------------------------------------
static unsigned fNavSearchWidthGetLabelId(MrUILinkBase* const, char* arg_list[], long lPos)
{
    UNUSED_PARAM(arg_list);
    UNUSED_PARAM(lPos);
    return MIR_STD_SEARCH_WINDOW;
}

// -----------------------------------------------------------
// Implementation of handler function fNavSearchWidthGetUnitId
// -----------------------------------------------------------
static unsigned fNavSearchWidthGetUnitId(LINK_DOUBLE_TYPE* const, char* arg_list[], long lPos)
{
    UNUSED_PARAM(arg_list);
    UNUSED_PARAM(lPos);
    return MRI_STD_UNIT_MM;
}

// -------------------------------------------------------------
// Implementation of handler function fNavSearchWidthIsAvailable
// -------------------------------------------------------------
static bool fNavSearchWidthIsAvailable(LINK_DOUBLE_TYPE* const pThis, long)
{
    MrProt rMrProt (pThis->prot());
    bool bRet = true;
    if (rMrProt.NavigatorParam().getalFree()[Label_SelectionBox_NavMode] == Value_NavModeOff)
    {
        bRet = false;
    }
    else
    {
        bRet = true;
    }
    return bRet;
}

// -----------------------------------------------------------
// Implementation of handler function fNavSearchWidthGetLimits
// -----------------------------------------------------------
static bool fNavSearchWidthGetLimits(LINK_DOUBLE_TYPE* const pThis, std::vector<MrLimitDouble>& rLimitVector,
                                     unsigned long& rulVerify, long lIndex)
{
    MrProt rMrProt (pThis->prot());
    UNUSED_PARAM(lIndex);

    rulVerify = LINK_LONG_TYPE::VERIFY_BINARY_SEARCH;
    rLimitVector.resize(1);  // Number of limit intervals
    double dMin = 0.0;
    double dMax = rMrProt.NavigatorParam().getalFree()[Label_Long_NavMatrix] - 1.0;
    double dInc = 0.5;
    rLimitVector[0].setEqualSpaced(dMin,dMax,dInc);
    return true;
}

// ----------------------------------------------------------
// Implementation of handler function fNavSearchWidthGetValue
// ----------------------------------------------------------
static double fNavSearchWidthGetValue(LINK_DOUBLE_TYPE* const pThis, long lPos)
{
    MrProt rMrProt (pThis->prot());
    UNUSED_PARAM(lPos);
    return (double) rMrProt.NavigatorParam().getadFree()[Label_Double_NavSearchWidth];
}

// ----------------------------------------------------------
// Implementation of handler function fNavSearchWidthSetValue
// ----------------------------------------------------------
static double fNavSearchWidthSetValue(LINK_DOUBLE_TYPE* const pThis, double dNewVal, long lPos)
{
    MrProt rMrProt (pThis->prot());
    UNUSED_PARAM(lPos);
    return (rMrProt.NavigatorParam().getadFree()[Label_Double_NavSearchWidth] = dNewVal);
}

// --------------------------------------------------------------
// Implementation of handler function fNavSearchWidthGetToolTipId
// --------------------------------------------------------------
static unsigned fNavSearchWidthGetToolTipId(LINK_DOUBLE_TYPE* const pThis, char* arg_list[], long lIndex)
{
    unsigned nRet = MRI_STD_NAVUI_TOOLTIP_SEARCH_WIDTH;
    return nRet;
}

// ----------------------------------------------------------------
// Implementation of handler function fNavCorrectionFactorArraySize
// ----------------------------------------------------------------
long fNavCorrectionFactorArraySize(MrUILinkArray* const pThis, long)
{
    MrProt rMrProt (pThis->prot());
    if (rMrProt.NavigatorParam().getalFree()[Label_SelectionBox_NavMode] == Value_NavModeOff)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

// -----------------------------------------------------------------
// Implementation of handler function fNavCorrectionFactorGetLabelId
// -----------------------------------------------------------------
static unsigned fNavCorrectionFactorGetLabelId(MrUILinkBase* const, char* arg_list[], long lPos)
{
    UNUSED_PARAM(arg_list);
    UNUSED_PARAM(lPos);

    return MIR_STD_TRACK_FACT;
}

// ----------------------------------------------------------------
// Implementation of handler function fNavCorrectionFactorGetUnitId
// ----------------------------------------------------------------
static unsigned fNavCorrectionFactorGetUnitId(LINK_DOUBLE_TYPE* const, char* arg_list[], long lPos)
{
    UNUSED_PARAM(arg_list);
    UNUSED_PARAM(lPos);

    return MRI_STD_EMPTY;  //  CHARM 321569 (should not have units) MRI_STD_UNIT_MM;
}

// ------------------------------------------------------------------
// Implementation of handler function fNavCorrectionFactorIsAvailable
// ------------------------------------------------------------------
static bool fNavCorrectionFactorIsAvailable(LINK_DOUBLE_TYPE* const pThis, long)
{
    MrProt rMrProt (pThis->prot());
    bool bRet = true;
    if (rMrProt.NavigatorParam().getalFree()[Label_SelectionBox_NavMode] == Value_NavModeOff)
    {
        bRet = false;
    }
    else
    {
        bRet = true;
    }
    return bRet;
}

// ----------------------------------------------------------------
// Implementation of handler function fNavCorrectionFactorGetLimits
// ----------------------------------------------------------------
static bool fNavCorrectionFactorGetLimits(LINK_DOUBLE_TYPE* const pThis, std::vector<MrLimitDouble>& rLimitVector,
                                          unsigned long& rulVerify, long lIndex)
{
    MrProt rMrProt (pThis->prot());
    UNUSED_PARAM(lIndex);

    rulVerify = LINK_LONG_TYPE::VERIFY_BINARY_SEARCH;
    rLimitVector.resize(1);  // Number of limit intervals
    double dMin = -2.0;
    double dMax = +2.0;
    double dInc = 0.01;
    rLimitVector[0].setEqualSpaced(dMin,dMax,dInc);
    return true;
}

// ---------------------------------------------------------------
// Implementation of handler function fNavCorrectionFactorGetValue
// ---------------------------------------------------------------
static double fNavCorrectionFactorGetValue(LINK_DOUBLE_TYPE* const pThis, long lPos)
{
    MrProt rMrProt (pThis->prot());
    UNUSED_PARAM(lPos);
    return (double) rMrProt.NavigatorParam().getadFree()[Label_Double_NavCorrectionFactor];
}

// ---------------------------------------------------------------
// Implementation of handler function fNavCorrectionFactorSetValue
// ---------------------------------------------------------------
static double fNavCorrectionFactorSetValue(LINK_DOUBLE_TYPE* const pThis, double dNewVal, long lPos)
{
    MrProt rMrProt (pThis->prot());
    UNUSED_PARAM(lPos);
    return (rMrProt.NavigatorParam().getadFree()[Label_Double_NavCorrectionFactor] = dNewVal);
}

// -------------------------------------------------------------------
// Implementation of handler function fNavCorrectionFactorGetToolTipId
// -------------------------------------------------------------------
static unsigned fNavCorrectionFactorGetToolTipId(LINK_DOUBLE_TYPE* const pThis, char* arg_list[], long lIndex)
{
    unsigned nRet = MRI_STD_NAVUI_TOOLTIP_TRACKING_FACTOR;
    return nRet;
}

// =======================================
// ========== DOUBLE Parameters ==========
// =======================================

// ---------------------------------------------------------
// Implementation of handler function _SSP_DOUBLE_GetLabelId
// ---------------------------------------------------------
static unsigned _SSP_DOUBLE_GetLabelId(LINK_DOUBLE_TYPE* const pThis, char* arg_list[], long lIndex)
{
    MrProt rMrProt (pThis->prot());
    UNUSED_PARAM(arg_list);

    switch(lIndex)
    {
    case Label_Double_NavAcceptancePosition : return MIR_STD_ACCEPT_POS;
    case Label_Double_NavSearchPosition     : return MIR_STD_SEARCH_POS;
    default                                 : return MRI_STD_EMPTY;  // Error
    }
    return MRI_STD_STRING;
}

// --------------------------------------------------------
// Implementation of handler function _SSP_DOUBLE_GetUnitId
// --------------------------------------------------------
static unsigned _SSP_DOUBLE_GetUnitId(LINK_DOUBLE_TYPE* const pThis, char* arg_list[], long lIndex)
{
    MrProt rMrProt (pThis->prot());
    UNUSED_PARAM(arg_list);

    switch(lIndex)
    {
    case Label_Double_NavAcceptancePosition : return MRI_STD_UNIT_MM;
    case Label_Double_NavSearchPosition     : return MRI_STD_UNIT_MM;
    default                                 : return MRI_STD_EMPTY;  // Error
    }
    return MRI_STD_STRING;
}

// ----------------------------------------------------------
// Implementation of handler function _SSP_DOUBLE_IsAvailable
// ----------------------------------------------------------
static bool _SSP_DOUBLE_IsAvailable(LINK_DOUBLE_TYPE* const pThis, long lIndex)
{
    MrProt rMrProt (pThis->prot());
    bool bRet = true;
    if (rMrProt.NavigatorParam().getalFree()[Label_SelectionBox_NavMode] == Value_NavModeOff)
    {
        bRet = false;
    }
    else
    {
        switch(lIndex)
        {
        default: bRet = true; break;
        case Label_Double_NavAcceptancePosition : bRet = true; break;
        case Label_Double_NavSearchPosition     : bRet = true; break;
        }
    }
    return bRet;
}

// --------------------------------------------------------
// Implementation of handler function _SSP_DOUBLE_GetLimits
// --------------------------------------------------------
static bool _SSP_DOUBLE_GetLimits(LINK_DOUBLE_TYPE* const pThis, std::vector<MrLimitDouble>& rLimitVector,
                                  unsigned long& rulVerify, long lIndex)
{
    MrProt rMrProt (pThis->prot());
    double dMin, dMax, dInc;
    rulVerify = LINK_DOUBLE_TYPE::VERIFY_BINARY_SEARCH;
    rLimitVector.resize(1);

    switch(lIndex)
    {
    case Label_Double_NavAcceptancePosition:
    case Label_Double_NavSearchPosition:
        dMin = 0.0;
        dMax = rMrProt.NavigatorParam().getalFree()[Label_Long_NavMatrix] - 1.0;
        dInc = 0.5;
        break;

    default:
        dMin= 0.0; dMax=   0.0; dInc=0.1;  break;
    }

    rLimitVector[0].setEqualSpaced(dMin,dMax,dInc);
    return true;
}

// -------------------------------------------------------
// Implementation of handler function _SSP_DOUBLE_GetValue
// -------------------------------------------------------
static double _SSP_DOUBLE_GetValue(LINK_DOUBLE_TYPE* const pThis, long lIndex)
{
    MrProt rMrProt (pThis->prot());
    return (double) rMrProt.NavigatorParam().getadFree()[lIndex];
}

// -------------------------------------------------------
// Implementation of handler function _SSP_DOUBLE_SetValue
// -------------------------------------------------------
static double _SSP_DOUBLE_SetValue(LINK_DOUBLE_TYPE* const pThis, double value, long lIndex)
{
    MrProt rMrProt (pThis->prot());
    switch(lIndex)
    {
    case Label_Double_NavSearchPosition:
        rMrProt.NavigatorParam().getadFree()[Label_Double_NavAcceptancePosition] = value;
        return (rMrProt.NavigatorParam().getadFree()[lIndex] = value);

    default:
        return (rMrProt.NavigatorParam().getadFree()[lIndex] = value);
        break;
    }
}


// -----------------------------------------------------------
// Implementation of handler function _SSP_DOUBLE_GetToolTipId
// -----------------------------------------------------------
static unsigned _SSP_DOUBLE_GetToolTipId(LINK_DOUBLE_TYPE* const pThis, char* arg_list[], long lIndex)
{
    unsigned nRet = MRI_STD_EMPTY;

    switch (lIndex)
    {
    case Label_Double_NavAcceptancePosition:
        nRet = MRI_STD_NAVUI_TOOLTIP_ACCEPTANCE_POSITION;
        break;

    case Label_Double_NavSearchPosition:
        nRet = MRI_STD_NAVUI_TOOLTIP_SEARCH_POSITION;
        break;

    default:
        nRet = MRI_STD_EMPTY;
        break;
    }

    return nRet;
}


// =======================================
// ========== Support Functions ==========
// =======================================

// -------------------------------------------------------------
// Implementation of support function fUILinkNaviSDimIsAvailable
// -------------------------------------------------------------
bool fUILinkNaviSDimIsAvailable(LINK_DOUBLE_TYPE* const pThis,long lPos)
{
    MrProt rMrProt (pThis->prot());
    UNUSED_PARAM(lPos);
    return ((rMrProt.NavigatorParam().getalFree()[Label_SelectionBox_NavPulseType] != Value_NavPulseType2DPencil) ||
        (rMrProt.NavigatorParam().getalFree()[Label_SelectionBox_NavMode]      == Value_NavModeOff));
}

// --------------------------------------------------------------
// Implementation of support function fUILinkNaviArrayIsAvailable
// --------------------------------------------------------------
bool fUILinkNaviArrayIsAvailable(MrUILinkArray* const pThis, long lPos)
{
    MrProt rMrProt (pThis->prot());
    MrUILinkArray::PFctIsAvailable pOrigNavIsAvailableHandler;
#ifdef SEQUENCE_CLASS
    NavigatorShell *pSeq = static_cast<NavigatorShell*>(pThis->sequence().getSeq());
    pOrigNavIsAvailableHandler = pSeq->m_pNavUI->m_pOrigNavIsAvailableHandler;
#else
    pOrigNavIsAvailableHandler = s_pOrigNavIsAvailableHandler;
#endif
    if (pOrigNavIsAvailableHandler != NULL)
    {
        if( !(*pOrigNavIsAvailableHandler)(pThis,lPos) )
        {
            return false;
        }
        else if(rMrProt.NavigatorParam().getalFree()[Label_SelectionBox_NavMode] == Value_NavModeOff)
        {
            return false;
        }
    }
    return true;
}


// --------------------------------------------------------
// Implementation of support function fUILinkNaviArrayErase
// --------------------------------------------------------
bool fUILinkNaviArrayErase(MrUILinkArray* const pThis, long lDoomed, long lPos)
{
    MrProt rMrProt (pThis->prot());
    MrUILinkArray::PFctErase pOrigNavEraseHandler = NULL;
#ifdef SEQUENCE_CLASS
    NavigatorShell *pSeq = static_cast<NavigatorShell*>(pThis->sequence().getSeq());
    pOrigNavEraseHandler = pSeq->m_pNavUI->m_pOrigNavEraseHandler;
#else
    pOrigNavEraseHandler = s_pOrigNavEraseHandler;
#endif
    bool bRet = true;

    if (pOrigNavEraseHandler != NULL)
    {
        if( !(*pOrigNavEraseHandler)(pThis,lDoomed,lPos) )
        {
            bRet = false;
        }
        else if( rMrProt.NavigatorParam().getalFree()[Label_SelectionBox_NavPulseType] == Value_NavPulseTypeCrossedPair )
        {
            if( !(*pOrigNavEraseHandler)(pThis,lDoomed==0||lDoomed==1 ? 0 : 2,lPos) )
            {
                bRet = false;
            }
        }
    }

    // CHARM 329090: If no navigator pulses remain, we must turn off navigator control in the PACE card.  In addition,
    // the option to activate navigator control is removed in the handler function _SSP_SELECTION_GetOptions.
    if (rMrProt.navigatorArray().size() == 0)
    {
        long lTmp = rMrProt.NavigatorParam().getalFree()[Label_SelectionBox_NavMode];
#ifdef SEQUENCE_CLASS
        pSeq->m_pNavUI->m_lOriginalNavMode = lTmp;
#else
        s_lOriginalNavMode = lTmp;
#endif
        //rMrProt.NavigatorParam().getalFree()[Label_SelectionBox_NavMode] = Value_NavModeOff;
        //rMrProt.NavigatorParam().setlRespComp(SEQ::RESP_COMP_OFF);
	    if(LINK_SELECTION_TYPE* pRespComp = _search< LINK_SELECTION_TYPE> (pThis, MR_TAG_RESP_COMP, Label_SelectionBox_NavMode))
	    {
	        pRespComp->value(MRI_STD_OFF, Label_SelectionBox_NavMode,MrUILinkBase::SET_FORCED);
	        pThis->addDependentParamPtr(pRespComp, Label_SelectionBox_NavMode);
	    }
	    else
	    {
	    	bRet = false;
	    }
    }

    return bRet;
}

// ---------------------------------------------------------
// Implementation of support function fUILinkNaviArrayInsert
// ---------------------------------------------------------
bool fUILinkNaviArrayInsert(MrUILinkArray* const pThis, long newPos, long dummy)
{
    MrProt rMrProt (pThis->prot());
    MrUILinkArray::PFctInsert pOrigNavInsertHandler;
#ifdef SEQUENCE_CLASS
    NavigatorShell *pSeq = static_cast<NavigatorShell*>(pThis->sequence().getSeq());
    pOrigNavInsertHandler = pSeq->m_pNavUI->m_pOrigNavInsertHandler;
#else
    pOrigNavInsertHandler = s_pOrigNavInsertHandler;
#endif
    NavigatorArray& rNaviArray = rMrProt.navigatorArray();

    if (rMrProt.NavigatorParam().getalFree()[Label_SelectionBox_NavPulseType] == Value_NavPulseTypeCrossedPair)
    {
        if (newPos >= 2) return false;  //Maximum of 2 pulses for crossed pair
    }
    else if (rMrProt.NavigatorParam().getalFree()[Label_SelectionBox_NavPulseType] == Value_NavPulseType2DPencil)
    {
        if (newPos >= 1) return false;  //Maximum of 1 pulse for pencil
    }

    if (pOrigNavInsertHandler != NULL)
    {
        if( !(*pOrigNavInsertHandler)(pThis,newPos,dummy) )
        {
            return false;
        }
        if( rMrProt.NavigatorParam().getalFree()[Label_SelectionBox_NavPulseType] == Value_NavPulseTypeCrossedPair )
        {
            if( !(*pOrigNavInsertHandler)(pThis,newPos+1,dummy) )
            {
                return false;
            }

            rNaviArray[newPos  ].thickness  ( pThis->seqLimits().getNavigatorThickness() [newPos  ].getDef() );
            rNaviArray[newPos  ].readoutFOV ( 400.0 );
            rNaviArray[newPos  ].phaseFOV   ( 400.0 );
            rNaviArray[newPos  ].position   ( 0.0,0.0,0.0 );
            rNaviArray[newPos  ].normal     ( VectorPat<double>::e_sag);

            rNaviArray[newPos+1].thickness  ( pThis->seqLimits().getNavigatorThickness() [newPos+1].getDef() );
            rNaviArray[newPos+1].readoutFOV ( 400.0 );
            rNaviArray[newPos+1].phaseFOV   ( 400.0 );
            rNaviArray[newPos+1].position   ( 0.0,0.0,0.0 );
            rNaviArray[newPos+1].normal     ( VectorPat<double>::e_sag);
            rNaviArray[newPos+1].normal     ( SEQ::SAG_TO_COR_TO_TRA,30,0);
        }
#ifdef SEQUENCE_CLASS
        long lOriginalNavMode = pSeq->m_pNavUI->m_lOriginalNavMode;
#else
        long lOriginalNavMode = s_lOriginalNavMode;
#endif
        if (lOriginalNavMode!=0)
        {
            rMrProt.NavigatorParam().getalFree()[Label_SelectionBox_NavMode] = lOriginalNavMode;
        }

        switch (lOriginalNavMode)
        {
            default:
                rMrProt.NavigatorParam().setlRespComp(SEQ::RESP_COMP_OFF);
                break;

            case Value_NavModeFollowSlice:
                rMrProt.NavigatorParam().setlRespComp(SEQ::RESP_COMP_GATE_AND_FOLLOW);
                break;

            case Value_NavModeProspectiveGating:
                rMrProt.NavigatorParam().setlRespComp(SEQ::RESP_COMP_GATE);
                break;

            case Value_NavModeMonitorOnly:
                rMrProt.NavigatorParam().setlRespComp(SEQ::RESP_COMP_MONITOR_ONLY);
                break;

            case Value_NavModeOff:
                rMrProt.NavigatorParam().setlRespComp(SEQ::RESP_COMP_OFF);
                break;
        }
    }
    return true;
}

// -------------------------------------------------------
// Implementation of support functions fNaviPos...SetValue
// -------------------------------------------------------


static VectorPat<double> fNaviPosSetValue(LINK_VECTOR_TYPE* const pThis, const VectorPat<double>& rNewVal, long lPos)
{
    MrProt rMrProt (pThis->prot());
    LINK_VECTOR_TYPE::PFctSetValue pFctNavPosVecSetVal_orig;
#ifdef SEQUENCE_CLASS
    NavigatorShell *pSeq = static_cast<NavigatorShell*>(pThis->sequence().getSeq());
    pFctNavPosVecSetVal_orig = pSeq->m_pNavUI->m_pFctNavPosVecSetVal_orig;
#else
    pFctNavPosVecSetVal_orig = s_pFctNavPosVecSetVal_orig;
#endif

    VectorPat<double> rRetVal = (*pFctNavPosVecSetVal_orig)(pThis,rNewVal,lPos);
    if( rMrProt.navigatorArray().size() == 2 && rMrProt.NavigatorParam().getalFree()[Label_SelectionBox_NavPulseType] == Value_NavPulseTypeCrossedPair )
    {
        if(lPos == 0)
            rRetVal = (*pFctNavPosVecSetVal_orig)(pThis,rNewVal,1);
        else
            rRetVal = (*pFctNavPosVecSetVal_orig)(pThis,rNewVal,0);
    }
    return rRetVal;
}


static unsigned fNaviPosSetValue(LINK_SELECTION_TYPE* const pThis, unsigned nID, long lPos)
{
    MrProt rMrProt (pThis->prot());
    LINK_SELECTION_TYPE::PFctSetValue pFctNavPosSelSetVal_orig;
#ifdef SEQUENCE_CLASS
    NavigatorShell *pSeq = static_cast<NavigatorShell*>(pThis->sequence().getSeq());
    pFctNavPosSelSetVal_orig = pSeq->m_pNavUI->m_pFctNavPosSelSetVal_orig;
#else
    pFctNavPosSelSetVal_orig = s_pFctNavPosSelSetVal_orig;
#endif

    unsigned iRetVal = (*pFctNavPosSelSetVal_orig)(pThis,nID,lPos);

    if( rMrProt.navigatorArray().size() == 2 && rMrProt.NavigatorParam().getalFree()[Label_SelectionBox_NavPulseType] == Value_NavPulseTypeCrossedPair )
    {
        if(lPos == 0)
            iRetVal = (*pFctNavPosSelSetVal_orig)(pThis,nID,1);
        else
            iRetVal = (*pFctNavPosSelSetVal_orig)(pThis,nID,0);
    }
    return iRetVal;
}


static double fNaviPosTraSetValue(LINK_DOUBLE_TYPE* const pThis, double dVal, long lPos)
{
    MrProt rMrProt (pThis->prot());
    LINK_DOUBLE_TYPE::PFctSetValue pFctNavPosTraSetVal_orig;
#ifdef SEQUENCE_CLASS
    NavigatorShell *pSeq = static_cast<NavigatorShell*>(pThis->sequence().getSeq());
    pFctNavPosTraSetVal_orig = pSeq->m_pNavUI->m_pFctNavPosTraSetVal_orig;
#else
    pFctNavPosTraSetVal_orig = s_pFctNavPosTraSetVal_orig;
#endif

    double   dRetVal = (*pFctNavPosTraSetVal_orig)(pThis,dVal,lPos);

    if( rMrProt.navigatorArray().size() == 2 && rMrProt.NavigatorParam().getalFree()[Label_SelectionBox_NavPulseType] == Value_NavPulseTypeCrossedPair )
    {
        if(lPos == 0)
            dRetVal = (*pFctNavPosTraSetVal_orig)(pThis,dVal,1);
        else
            dRetVal = (*pFctNavPosTraSetVal_orig)(pThis,dVal,0);
    }
    return dRetVal;
}


static double fNaviPosCorSetValue(LINK_DOUBLE_TYPE* const pThis, double dVal, long lPos)
{
    MrProt rMrProt (pThis->prot());
    LINK_DOUBLE_TYPE::PFctSetValue pFctNavPosCorSetVal_orig;
#ifdef SEQUENCE_CLASS
    NavigatorShell *pSeq = static_cast<NavigatorShell*>(pThis->sequence().getSeq());
    pFctNavPosCorSetVal_orig = pSeq->m_pNavUI->m_pFctNavPosCorSetVal_orig;
#else
    pFctNavPosCorSetVal_orig = s_pFctNavPosCorSetVal_orig;
#endif

    double   dRetVal = (*pFctNavPosCorSetVal_orig)(pThis,dVal,lPos);

    if( rMrProt.navigatorArray().size() == 2 && rMrProt.NavigatorParam().getalFree()[Label_SelectionBox_NavPulseType] == Value_NavPulseTypeCrossedPair )
    {
        if(lPos == 0)
            dRetVal = (*pFctNavPosCorSetVal_orig)(pThis,dVal,1);
        else
            dRetVal = (*pFctNavPosCorSetVal_orig)(pThis,dVal,0);
    }
    return dRetVal;
}


static double fNaviPosSagSetValue(LINK_DOUBLE_TYPE* const pThis, double dVal, long lPos)
{
    MrProt rMrProt (pThis->prot());
    LINK_DOUBLE_TYPE::PFctSetValue pFctNavPosSagSetVal_orig;
#ifdef SEQUENCE_CLASS
    NavigatorShell *pSeq = static_cast<NavigatorShell*>(pThis->sequence().getSeq());
    pFctNavPosSagSetVal_orig = pSeq->m_pNavUI->m_pFctNavPosSagSetVal_orig;
#else
    pFctNavPosSagSetVal_orig = s_pFctNavPosSagSetVal_orig;
#endif

    double   dRetVal = (*pFctNavPosSagSetVal_orig)(pThis,dVal,lPos);

    if( rMrProt.navigatorArray().size() == 2 && rMrProt.NavigatorParam().getalFree()[Label_SelectionBox_NavPulseType] == Value_NavPulseTypeCrossedPair )
    {
        if(lPos == 0)
            dRetVal = (*pFctNavPosSagSetVal_orig)(pThis,dVal,1);
        else
            dRetVal = (*pFctNavPosSagSetVal_orig)(pThis,dVal,0);
    }
    return dRetVal;
}

// ===============================
// Member Functions of Class NavUI
// ===============================

// =================================================================
// Implementation of member functions NavUI::NavUI and NavUI::~NavUI
// =================================================================
NavUI::NavUI()


#if defined DEBUG
  : m_Default_SelectionBox_NavMode(Value_NavModeMonitorOnly)
#else
  : m_Default_SelectionBox_NavMode(Value_NavModeFollowSlice)
#endif
  , m_Default_SelectionBox_NavPulseType(Value_NavPulseTypeCrossedPair)
  , m_Default_SelectionBox_NavFBMode(Value_NavFBMode_RT)
  , m_Default_SelectionBox_NavPosition(Value_NavBefEchoTrain)
  , m_Default_Long_NavMatrix(256)
  , m_Default_Long_NavFov(256)
  , m_Default_Long_NoOfNavs(4)				//JK2008
  , m_Default_Long_TimeStartAcq(760)		//ib
  , m_Default_Long_TimeEndAcq(900)			//ib
  , m_Default_Long_TimeEndCardiac(1000)		//ib
  , m_Default_Long_NavTR_ms(80)				//ib
  , m_Default_Long_PoleSensitivity(2)		//ib
  , m_Default_Long_ScoutLength(512)			//ib
  , m_Default_Long_SliceSelection(1)		//ib
  , m_Default_Double_NavAcceptancePosition(128.0)
  , m_Default_Double_NavAcceptanceWidth(4.0)
  , m_Default_Double_NavSearchPosition(128.0)
  , m_Default_Double_NavSearchWidth(32.0)
  , m_Default_Double_NavCorrectionFactor(0.6)
#if defined DEBUG
  , m_Default_CheckBox_PrepScan(Value_CheckBox_Off)
#else
  , m_Default_CheckBox_PrepScan(Value_CheckBox_On)
#endif
  , m_Default_Long_NavPrepTR_ms(100)
  , m_Default_Long_NavPrepDuration_sec(19)
  , m_Default_Long_NavSleepDuration_ms(10)
  , m_Default_Label_Long_Arr0_RotatePlot(1)
  , m_Default_Label_Long_Arr1_SEQDebugFlags(SEQDebug_ShowFeedbackData)
  , m_Default_Label_Long_Arr2_ICEDebugFlags(0)
  , m_Default_Label_Long_Arr3_ICESmoothLength(7)
  , m_Default_Label_Long_Arr4_ICELSQFitLength(7)
  , m_Default_Label_Long_Arr5_ICEImageSendInterval_Msec(400)
  , m_Default_Label_Long_Arr6_ICETimeScaleInterval_Sec(10)
  , m_Default_Label_Long_Arr7_FeedbackPollInterval_ms(50)
  , m_Default_Label_Long_Arr8_NoOfPrepPulses(0)
  , m_Default_Label_Long_Arr9_NoOfPrepBeats(0)

#ifdef SEQUENCE_CLASS
  , m_pOrigNavEraseHandler(NULL)
  , m_pOrigNavInsertHandler(NULL)
  , m_pOrigNavIsAvailableHandler(NULL)
  , m_pFctNavPosVecSetVal_orig(NULL)
  , m_pFctNavPosSelSetVal_orig(NULL)
  , m_pFctNavPosTraSetVal_orig(NULL)
  , m_pFctNavPosCorSetVal_orig(NULL)
  , m_pFctNavPosSagSetVal_orig(NULL)

  , m_lOriginalNavMode(0)
#endif

{
    if ( SeqUT.isUnitTestActive() )
    {
        m_Default_SelectionBox_NavMode =Value_NavModeMonitorOnly;
        m_Default_CheckBox_PrepScan =Value_CheckBox_Off;
    }
}

NavUI::~NavUI()
{}

// =============================================
// Implementation of member function NavUI::init
// =============================================
NLS_STATUS NavUI::init (SeqLim &rSeqLim)
{
    static const char * const ptModule = "init";
    //cout << ptModule << " begin" << endl;

    // ==================================
    // Register various support functions
    // ==================================
    if( LINK_DOUBLE_TYPE* pNaviSDim = _searchElm<LINK_DOUBLE_TYPE>(rSeqLim, MR_TAG_NAVIGATOR_ARRAY, MR_TAG_SDIM) )
    {
        pNaviSDim->registerIsAvailableHandler    ( fUILinkNaviSDimIsAvailable  );
    }

    if( MrUILinkArray* pArray = _search<MrUILinkArray>(rSeqLim,MR_TAG_NAVIGATOR_ARRAY) )
    {
      //MrUILinkArray::PFctIsAvailable pOrigNavIsAvailableHandler = pArray->registerIsAvailableHandler(fUILinkNaviArrayIsAvailable);
        MrUILinkArray::PFctErase       pOrigNavEraseHandler       = pArray->registerEraseHandler(fUILinkNaviArrayErase);
        MrUILinkArray::PFctInsert      pOrigNavInsertHandler      = pArray->registerInsertHandler(fUILinkNaviArrayInsert);
#ifdef SEQUENCE_CLASS
      //m_pOrigNavIsAvailableHandler = pOrigNavIsAvailableHandler;
        m_pOrigNavEraseHandler       = pOrigNavEraseHandler;
        m_pOrigNavInsertHandler      = pOrigNavInsertHandler;
#else
      //s_pOrigNavIsAvailableHandler = pOrigNavIsAvailableHandler;
        s_pOrigNavEraseHandler       = pOrigNavEraseHandler;
        s_pOrigNavInsertHandler      = pOrigNavInsertHandler;
#endif
    }

    // =============================================================================
    // Functions that serve to link the two GSP objects when crossed-pair RF is used
    // =============================================================================

    if( LINK_VECTOR_TYPE* pVect = _searchElm<LINK_VECTOR_TYPE>(rSeqLim, MR_TAG_NAVIGATOR_ARRAY, MR_TAG_POSITION) )
    {
        LINK_VECTOR_TYPE::PFctSetValue pFctNavPosVecSetVal_orig = pVect->registerSetValueHandler( fNaviPosSetValue  );
#ifdef SEQUENCE_CLASS
        m_pFctNavPosVecSetVal_orig = pFctNavPosVecSetVal_orig;
#else
        s_pFctNavPosVecSetVal_orig = pFctNavPosVecSetVal_orig;
#endif
    }

    if( LINK_SELECTION_TYPE* pSel = _searchElm<LINK_SELECTION_TYPE>(rSeqLim,MR_TAG_NAVIGATOR_ARRAY,MR_TAG_POSITION2) )
    {
        LINK_SELECTION_TYPE::PFctSetValue pFctNavPosSelSetVal_orig = pSel->registerSetValueHandler( fNaviPosSetValue  );
#ifdef SEQUENCE_CLASS
        m_pFctNavPosSelSetVal_orig = pFctNavPosSelSetVal_orig;
#else
        s_pFctNavPosSelSetVal_orig = pFctNavPosSelSetVal_orig;
#endif
    }

    if( LINK_DOUBLE_TYPE* pSel = _searchElm<LINK_DOUBLE_TYPE>(rSeqLim,MR_TAG_NAVIGATOR_ARRAY,MR_TAG_POS_SAG) )
    {
        LINK_DOUBLE_TYPE::PFctSetValue pFctNavPosSagSetVal_orig = pSel->registerSetValueHandler( fNaviPosSagSetValue  );
#ifdef SEQUENCE_CLASS
        m_pFctNavPosSagSetVal_orig = pFctNavPosSagSetVal_orig;
#else
        s_pFctNavPosSagSetVal_orig = pFctNavPosSagSetVal_orig;
#endif
    }

    if( LINK_DOUBLE_TYPE* pSel = _searchElm<LINK_DOUBLE_TYPE>(rSeqLim,MR_TAG_NAVIGATOR_ARRAY,MR_TAG_POS_COR) )
    {
        LINK_DOUBLE_TYPE::PFctSetValue pFctNavPosCorSetVal_orig = pSel->registerSetValueHandler( fNaviPosCorSetValue  );
#ifdef SEQUENCE_CLASS
        m_pFctNavPosCorSetVal_orig = pFctNavPosCorSetVal_orig;
#else
        s_pFctNavPosCorSetVal_orig = pFctNavPosCorSetVal_orig;
#endif
    }

    if( LINK_DOUBLE_TYPE* pSel = _searchElm<LINK_DOUBLE_TYPE>(rSeqLim,MR_TAG_NAVIGATOR_ARRAY,MR_TAG_POS_TRA) )
    {
        LINK_DOUBLE_TYPE::PFctSetValue pFctNavPosTraSetVal_orig = pSel->registerSetValueHandler( fNaviPosTraSetValue  );
#ifdef SEQUENCE_CLASS
        m_pFctNavPosTraSetVal_orig = pFctNavPosTraSetVal_orig;
#else
        s_pFctNavPosTraSetVal_orig = pFctNavPosTraSetVal_orig;
#endif
    }

    // ====================================
    // Register Physio/PACE card parameters
    // ====================================

    // --------------------------
    // Parameter MR_TAG_RESP_COMP
    // --------------------------
    if (LINK_SELECTION_TYPE* pSelection = _create< LINK_SELECTION_TYPE >(rSeqLim, MR_TAG_RESP_COMP, Label_SelectionBox_NavMode))
    {
        pSelection->registerGetLabelIdHandler(_SSP_SELECTION_GetLabelId);
        pSelection->registerGetOptionsHandler(_SSP_SELECTION_GetOptions);
        pSelection->registerIsAvailableHandler(_SSP_SELECTION_IsAvailable);
        pSelection->registerGetValueHandler(_SSP_SELECTION_GetValue);
        pSelection->registerSetValueHandler(_SSP_SELECTION_SetValue);
        pSelection->registerFormatHandler(_SSP_SELECTION_Format);
        pSelection->registerGetToolTipIdHandler(_SSP_SELECTION_GetToolTipId);
    }

    // -----------------------------
    // Parameter MR_TAG_RF_PULS_TYPE
    // -----------------------------
    if (LINK_SELECTION_TYPE* pSelection = _create< LINK_SELECTION_TYPE >(rSeqLim, MR_TAG_RF_PULS_TYPE, Label_SelectionBox_NavPulseType))
    {
        pSelection->registerGetLabelIdHandler(_SSP_SELECTION_GetLabelId);
        pSelection->registerGetOptionsHandler(_SSP_SELECTION_GetOptions);
        pSelection->registerIsAvailableHandler(_SSP_SELECTION_IsAvailable);
        pSelection->registerGetValueHandler(_SSP_SELECTION_GetValue);
        pSelection->registerSetValueHandler(_SSP_SELECTION_SetValue);
        pSelection->registerFormatHandler(_SSP_SELECTION_Format);
        pSelection->registerGetToolTipIdHandler(_SSP_SELECTION_GetToolTipId);
    }

    // ---------------------------
    // Parameter MR_TAG_ACCEPT_POS
    // ---------------------------
    if (LINK_DOUBLE_TYPE* pDouble = _create< LINK_DOUBLE_TYPE >(rSeqLim, MR_TAG_ACCEPT_POS, Label_Double_NavAcceptancePosition))
    {
        pDouble->registerGetLabelIdHandler(_SSP_DOUBLE_GetLabelId);
        pDouble->registerGetUnitIdHandler(_SSP_DOUBLE_GetUnitId);
        pDouble->registerIsAvailableHandler(_SSP_DOUBLE_IsAvailable);
        pDouble->registerGetLimitsHandler(_SSP_DOUBLE_GetLimits);
        pDouble->registerGetValueHandler(_SSP_DOUBLE_GetValue);
        pDouble->registerSetValueHandler(_SSP_DOUBLE_SetValue);
        pDouble->registerGetToolTipIdHandler(_SSP_DOUBLE_GetToolTipId);
    }

    // ---------------------------
    // Parameter MR_TAG_SEARCH_POS
    // ---------------------------
    if (LINK_DOUBLE_TYPE* pDouble = _create< LINK_DOUBLE_TYPE >(rSeqLim, MR_TAG_SEARCH_POS, Label_Double_NavSearchPosition))
    {
        pDouble->registerGetLabelIdHandler(_SSP_DOUBLE_GetLabelId);
        pDouble->registerGetUnitIdHandler(_SSP_DOUBLE_GetUnitId);
        pDouble->registerIsAvailableHandler(_SSP_DOUBLE_IsAvailable);
        pDouble->registerGetLimitsHandler(_SSP_DOUBLE_GetLimits);
        pDouble->registerGetValueHandler(_SSP_DOUBLE_GetValue);
        pDouble->registerSetValueHandler(_SSP_DOUBLE_SetValue);
        pDouble->registerGetToolTipIdHandler(_SSP_DOUBLE_GetToolTipId);
    }

    // --------------------------
    // Parameter MR_TAG_CHRON_POS
    // --------------------------
    if (LINK_SELECTION_TYPE* pSelection = _create< LINK_SELECTION_TYPE >(rSeqLim, MR_TAG_CHRON_POS, Label_SelectionBox_NavPosition))
    {
        pSelection->registerGetLabelIdHandler(_SSP_SELECTION_GetLabelId);
        pSelection->registerGetOptionsHandler(_SSP_SELECTION_GetOptions);
        pSelection->registerIsAvailableHandler(_SSP_SELECTION_IsAvailable);
        pSelection->registerGetValueHandler(_SSP_SELECTION_GetValue);
        pSelection->registerSetValueHandler(_SSP_SELECTION_SetValue);
        pSelection->registerFormatHandler(_SSP_SELECTION_Format);
        pSelection->registerGetToolTipIdHandler(_SSP_SELECTION_GetToolTipId);
    }

    // ---------------------------
    // Parameter MR_TAG_SCOUT_MODE
    // ---------------------------
    if (LINK_BOOL_TYPE* pBool = _create< LINK_BOOL_TYPE >(rSeqLim, MR_TAG_SCOUT_MODE, Label_CheckBox_PrepScan))
    {
        pBool->registerGetLabelIdHandler(_SSP_CHECKBOX_GetLabelId);
        pBool->registerIsAvailableHandler(_SSP_CHECKBOX_IsAvailable);
        pBool->registerGetOptionsHandler(_SSP_CHECKBOX_GetOptions);
        pBool->registerGetValueHandler(_SSP_CHECKBOX_GetValue);
        pBool->registerSetValueHandler(_SSP_CHECKBOX_SetValue);
        pBool->registerGetToolTipIdHandler(_SSP_CHECKBOX_GetToolTipId);
    }

    // -------------------------
    // Parameter MR_TAG_SCOUT_TR
    // -------------------------
    if (LINK_LONG_TYPE* pLong = _create< LINK_LONG_TYPE >(rSeqLim,  MR_TAG_SCOUT_TR, Label_Long_NavPrepTR_ms))
    {
        pLong->registerGetLabelIdHandler(_SSP_LONG_GetLabelId);
        pLong->registerGetUnitIdHandler(_SSP_LONG_GetUnitId);
        pLong->registerIsAvailableHandler(_SSP_LONG_IsAvailable);
        pLong->registerGetLimitsHandler(_SSP_LONG_GetLimits);
        pLong->registerGetValueHandler(_SSP_LONG_GetValue);
        pLong->registerSetValueHandler(_SSP_LONG_SetValue);
        pLong->registerGetToolTipIdHandler(_SSP_LONG_GetToolTipId);
    }

    // -------------------------------
    // Parameter MR_TAG_SCOUT_DURATION
    // -------------------------------
    if (LINK_LONG_TYPE* pLong = _create< LINK_LONG_TYPE >(rSeqLim, MR_TAG_SCOUT_DURATION, Label_Long_NavPrepDuration_sec))
    {
        pLong->registerGetLabelIdHandler(_SSP_LONG_GetLabelId);
        pLong->registerGetUnitIdHandler(_SSP_LONG_GetUnitId);
        pLong->registerIsAvailableHandler(_SSP_LONG_IsAvailable);
        pLong->registerGetLimitsHandler(_SSP_LONG_GetLimits);
        pLong->registerGetValueHandler(_SSP_LONG_GetValue);
        pLong->registerSetValueHandler(_SSP_LONG_SetValue);
        pLong->registerGetToolTipIdHandler(_SSP_LONG_GetToolTipId);
    }

    // ----------------------------------------------
    // Parameter MR_TAG_ACCEPT_WND (NOTE: ARRAY type)
    // ----------------------------------------------
    LINK_DOUBLE_TYPE* pDblElm = NULL;
    if( MrUILinkArray* pArray = _createArray<LINK_DOUBLE_TYPE>(rSeqLim,MR_TAG_ACCEPT_WND,fNaviConstArrayMaxSize,pDblElm) )
    {
        pArray->registerSizeHandler         (fNavAcceptanceWidthArraySize);  //  handler function which must return the actual array size
        pArray->registerGetLabelIdHandler   (fNavAcceptanceWidthGetLabelId); //  GetLabelId is registered once for Array, once for DblElm!
        pDblElm->registerGetLabelIdHandler  (fNavAcceptanceWidthGetLabelId);
        pDblElm->registerGetUnitIdHandler   (fNavAcceptanceWidthGetUnitId);
        pDblElm->registerIsAvailableHandler (fNavAcceptanceWidthIsAvailable);
        pDblElm->registerGetLimitsHandler   (fNavAcceptanceWidthGetLimits);
        pDblElm->registerGetValueHandler    (fNavAcceptanceWidthGetValue);
        pDblElm->registerSetValueHandler    (fNavAcceptanceWidthSetValue);
        pDblElm->registerGetToolTipIdHandler(fNavAcceptanceWidthGetToolTipId);
    }

    // -------------------------------------------------
    // Parameter MR_TAG_SEARCH_WINDOW (NOTE: ARRAY type)
    // -------------------------------------------------
    pDblElm = NULL;
    if( MrUILinkArray* pArray = _createArray<LINK_DOUBLE_TYPE>(rSeqLim,MR_TAG_SEARCH_WINDOW,fNaviConstArrayMaxSize,pDblElm) )
    {
        pArray->registerSizeHandler         (fNavSearchWidthArraySize);  //  handler function which must return the actual array size
        pArray->registerGetLabelIdHandler   (fNavSearchWidthGetLabelId); //  GetLabelId is registered once for Array, once for DblElm!
        pDblElm->registerGetLabelIdHandler  (fNavSearchWidthGetLabelId);
        pDblElm->registerGetUnitIdHandler   (fNavSearchWidthGetUnitId);
        pDblElm->registerIsAvailableHandler (fNavSearchWidthIsAvailable);
        pDblElm->registerGetLimitsHandler   (fNavSearchWidthGetLimits);
        pDblElm->registerGetValueHandler    (fNavSearchWidthGetValue);
        pDblElm->registerSetValueHandler    (fNavSearchWidthSetValue);
        pDblElm->registerGetToolTipIdHandler(fNavSearchWidthGetToolTipId);
    }

    // ----------------------------------------------
    // Parameter MR_TAG_TRACK_FAKT (NOTE: ARRAY type)
    // ----------------------------------------------
    pDblElm = NULL;
    if( MrUILinkArray* pArray = _createArray<LINK_DOUBLE_TYPE>(rSeqLim,MR_TAG_TRACK_FAKT,fNaviConstArrayMaxSize,pDblElm) )
    {
        pArray->registerSizeHandler         (fNavCorrectionFactorArraySize);  //  handler function which must return the actual array size
        pArray->registerGetLabelIdHandler   (fNavCorrectionFactorGetLabelId); //  GetLabelId is registered once for Array, once for DblElm!
        pDblElm->registerGetLabelIdHandler  (fNavCorrectionFactorGetLabelId);
        pDblElm->registerGetUnitIdHandler   (fNavCorrectionFactorGetUnitId);
        pDblElm->registerIsAvailableHandler (fNavCorrectionFactorIsAvailable);
        pDblElm->registerGetLimitsHandler   (fNavCorrectionFactorGetLimits);
        pDblElm->registerGetValueHandler    (fNavCorrectionFactorGetValue);
        pDblElm->registerSetValueHandler    (fNavCorrectionFactorSetValue);
        pDblElm->registerGetToolTipIdHandler(fNavCorrectionFactorGetToolTipId);
    }

    //  -------------------------------------------------------------------------------------
    //  Parameters displayed on the SEQUENCE/SPECIAL card, used for development purposes only
    //  The appearance of these parameters depends on a registry parameter.
    //  -------------------------------------------------------------------------------------
    unsigned long ulSeqSpecialCard = 0;
    ///////if( !RegIntOperator( "SOFTWARE\\Siemens\\Numaris4\\MRConfig\\Modality\\Sequence\\NAV", "SEQ_SPECIAL_CARD", REG_INT_OPERATION_GET, ulSeqSpecialCard, 0) )
    ///////{
    ///////    std::cout << "Registry flag SEQ_SPECIAL_CARD not found" << std::endl;
    ///////    ulSeqSpecialCard = 0;
    ///////}
    if( ulSeqSpecialCard != 0 )
    {
        // -----------------------------------------
        // Parameter MR_TAG_SEQ_WIP1 (Feedback mode)
        // -----------------------------------------
        /* No longer needed in VB13A
        if (LINK_SELECTION_TYPE* pSelection = _create< LINK_SELECTION_TYPE >(rSeqLim, MR_TAG_SEQ_WIP1, Label_SelectionBox_NavFBMode))
        {
            pSelection->registerGetLabelIdHandler(_SSP_SELECTION_GetLabelId);
            pSelection->registerGetOptionsHandler(_SSP_SELECTION_GetOptions);
            pSelection->registerIsAvailableHandler(_SSP_SELECTION_IsAvailable);
            pSelection->registerGetValueHandler(_SSP_SELECTION_GetValue);
            pSelection->registerSetValueHandler(_SSP_SELECTION_SetValue);
            pSelection->registerFormatHandler(_SSP_SELECTION_Format);
            pSelection->registerGetToolTipIdHandler(_SSP_SELECTION_GetToolTipId);
        }*/

        // ----------------------------------------------------------
        // Parameter MR_TAG_SEQ_WIP2 (Feedback time - Sleep duration)
        // ----------------------------------------------------------
        if (LINK_LONG_TYPE* pLong = _create< LINK_LONG_TYPE >(rSeqLim, MR_TAG_SEQ_WIP2, Label_Long_NavSleepDuration_ms))
        {
            pLong->registerGetLabelIdHandler(_SSP_LONG_GetLabelId);
            pLong->registerGetUnitIdHandler(_SSP_LONG_GetUnitId);
            pLong->registerIsAvailableHandler(_SSP_LONG_IsAvailable);
            pLong->registerGetLimitsHandler(_SSP_LONG_GetLimits);
            pLong->registerGetValueHandler(_SSP_LONG_GetValue);
            pLong->registerSetValueHandler(_SSP_LONG_SetValue);
            pLong->registerGetToolTipIdHandler(_SSP_LONG_GetToolTipId);
        }

        // ---------------------------------------
        // Parameter MR_TAG_SEQ_WIP3 (Debug array)
        // ---------------------------------------
        LINK_LONG_TYPE* pLongElm = NULL;
        if( MrUILinkArray* pArray = _createArray<LINK_LONG_TYPE>(rSeqLim,MR_TAG_SEQ_WIP3,fNaviExtraSwitchesMaxSize,pLongElm) )
        {
            pArray->registerSizeHandler          ( fNaviExtraSwitchesSize        );
            pArray->registerGetLabelIdHandler    ( fNaviExtraSwitchesGetLabelId  ); //  GetLabelId is registered once for Array, once for LongElm!
            pLongElm->registerGetLabelIdHandler  ( fNaviExtraSwitchesGetLabelId  ); 
            pLongElm->registerGetUnitIdHandler   ( fNaviExtraSwitchesGetUnitId   );
            pLongElm->registerGetValueHandler    ( fNaviExtraSwitchesGetValue    );
            pLongElm->registerSetValueHandler    ( fNaviExtraSwitchesSetValue    );
            pLongElm->registerGetLimitsHandler   ( fNaviExtraSwitchesGetLimits   );
            pLongElm->registerIsAvailableHandler ( fNaviExtraSwitchesIsAvailable ) ;
        }
    }

	//JK2008
	if (LINK_LONG_TYPE* pLong = _create< LINK_LONG_TYPE >(rSeqLim, MR_TAG_SEQ_WIP10, Label_Long_NoOfNavs))
		{
		pLong->registerGetLabelIdHandler(_SSP_LONG_GetLabelId);
        pLong->registerGetUnitIdHandler(_SSP_LONG_GetUnitId);
        pLong->registerIsAvailableHandler(_SSP_LONG_IsAvailable);
        pLong->registerGetLimitsHandler(_SSP_LONG_GetLimits);
        pLong->registerGetValueHandler(_SSP_LONG_GetValue);
        pLong->registerSetValueHandler(_SSP_LONG_SetValue);
        pLong->registerGetToolTipIdHandler(_SSP_LONG_GetToolTipId);
		}
	//JK2008
	//ib-start
	if (LINK_LONG_TYPE* pLong = _create< LINK_LONG_TYPE >(rSeqLim, MR_TAG_SEQ_WIP5, Label_Long_TimeStartAcq))
		{
		pLong->registerGetLabelIdHandler(_SSP_LONG_GetLabelId);
        pLong->registerGetUnitIdHandler(_SSP_LONG_GetUnitId);
        pLong->registerIsAvailableHandler(_SSP_LONG_IsAvailable);
        pLong->registerGetLimitsHandler(_SSP_LONG_GetLimits);
        pLong->registerGetValueHandler(_SSP_LONG_GetValue);
        pLong->registerSetValueHandler(_SSP_LONG_SetValue);
        pLong->registerGetToolTipIdHandler(_SSP_LONG_GetToolTipId);
		}
	if (LINK_LONG_TYPE* pLong = _create< LINK_LONG_TYPE >(rSeqLim, MR_TAG_SEQ_WIP12, Label_Long_TimeEndAcq))
		{
		pLong->registerGetLabelIdHandler(_SSP_LONG_GetLabelId);
        pLong->registerGetUnitIdHandler(_SSP_LONG_GetUnitId);
        pLong->registerIsAvailableHandler(_SSP_LONG_IsAvailable);
        pLong->registerGetLimitsHandler(_SSP_LONG_GetLimits);
        pLong->registerGetValueHandler(_SSP_LONG_GetValue);
        pLong->registerSetValueHandler(_SSP_LONG_SetValue);
        pLong->registerGetToolTipIdHandler(_SSP_LONG_GetToolTipId);
		}
	if (LINK_LONG_TYPE* pLong = _create< LINK_LONG_TYPE >(rSeqLim, MR_TAG_SEQ_WIP13, Label_Long_TimeEndCardiac))
		{
		pLong->registerGetLabelIdHandler(_SSP_LONG_GetLabelId);
        pLong->registerGetUnitIdHandler(_SSP_LONG_GetUnitId);
        pLong->registerIsAvailableHandler(_SSP_LONG_IsAvailable);
        pLong->registerGetLimitsHandler(_SSP_LONG_GetLimits);
        pLong->registerGetValueHandler(_SSP_LONG_GetValue);
        pLong->registerSetValueHandler(_SSP_LONG_SetValue);
        pLong->registerGetToolTipIdHandler(_SSP_LONG_GetToolTipId);
		}
	if (LINK_LONG_TYPE* pLong = _create< LINK_LONG_TYPE >(rSeqLim, MR_TAG_SEQ_WIP9, Label_Long_NavTR_ms))
		{
		pLong->registerGetLabelIdHandler(_SSP_LONG_GetLabelId);
        pLong->registerGetUnitIdHandler(_SSP_LONG_GetUnitId);
        pLong->registerIsAvailableHandler(_SSP_LONG_IsAvailable);
        pLong->registerGetLimitsHandler(_SSP_LONG_GetLimits);
        pLong->registerGetValueHandler(_SSP_LONG_GetValue);
        pLong->registerSetValueHandler(_SSP_LONG_SetValue);
        pLong->registerGetToolTipIdHandler(_SSP_LONG_GetToolTipId);
		}
	if (LINK_LONG_TYPE* pLong = _create< LINK_LONG_TYPE >(rSeqLim, MR_TAG_SEQ_WIP2, Label_Long_PoleSensitivity))
		{
		pLong->registerGetLabelIdHandler(_SSP_LONG_GetLabelId);
        pLong->registerGetUnitIdHandler(_SSP_LONG_GetUnitId);
        pLong->registerIsAvailableHandler(_SSP_LONG_IsAvailable);
        pLong->registerGetLimitsHandler(_SSP_LONG_GetLimits);
        pLong->registerGetValueHandler(_SSP_LONG_GetValue);
        pLong->registerSetValueHandler(_SSP_LONG_SetValue);
        pLong->registerGetToolTipIdHandler(_SSP_LONG_GetToolTipId);
		}
	if (LINK_LONG_TYPE* pLong = _create< LINK_LONG_TYPE >(rSeqLim, MR_TAG_SEQ_WIP3, Label_Long_ScoutLength))
		{
		pLong->registerGetLabelIdHandler(_SSP_LONG_GetLabelId);
        pLong->registerGetUnitIdHandler(_SSP_LONG_GetUnitId);
        pLong->registerIsAvailableHandler(_SSP_LONG_IsAvailable);
        pLong->registerGetLimitsHandler(_SSP_LONG_GetLimits);
        pLong->registerGetValueHandler(_SSP_LONG_GetValue);
        pLong->registerSetValueHandler(_SSP_LONG_SetValue);
        pLong->registerGetToolTipIdHandler(_SSP_LONG_GetToolTipId);
		}
	if (LINK_LONG_TYPE* pLong = _create< LINK_LONG_TYPE >(rSeqLim, MR_TAG_SEQ_WIP4, Label_Long_SliceSelection))
		{
		pLong->registerGetLabelIdHandler(_SSP_LONG_GetLabelId);
        pLong->registerGetUnitIdHandler(_SSP_LONG_GetUnitId);
        pLong->registerIsAvailableHandler(_SSP_LONG_IsAvailable);
        pLong->registerGetLimitsHandler(_SSP_LONG_GetLimits);
        pLong->registerGetValueHandler(_SSP_LONG_GetValue);
        pLong->registerSetValueHandler(_SSP_LONG_SetValue);
        pLong->registerGetToolTipIdHandler(_SSP_LONG_GetToolTipId);
		}
	//ib-end

    //  Look at registry to see if any debug flags are set.
    unsigned long ulSimulateICERawData = 0;
    ///////if( !RegIntOperator( "SOFTWARE\\Siemens\\Numaris4\\MRConfig\\Modality\\Sequence\\NAV", "ICE_SIMULATE_RAW", REG_INT_OPERATION_GET, ulSimulateICERawData, 0) )
    ///////{
    ///////    std::cout << "Registry flag ICE_SIMULATE_RAW not found" << std::endl;
    ///////    ulSimulateICERawData = 0;
    ///////}
    if (ulSimulateICERawData!=0) m_Default_Label_Long_Arr2_ICEDebugFlags = _ICEDEBUG_SIMULATE_FULL;
    else                         m_Default_Label_Long_Arr2_ICEDebugFlags = 0;

    unsigned long ulSEQDebugFlags = 0;
    ///////if( !RegIntOperator( "SOFTWARE\\Siemens\\Numaris4\\MRConfig\\Modality\\Sequence\\NAV", "SEQ_DEBUG_FLAGS", REG_INT_OPERATION_GET, ulSEQDebugFlags, 0) )
    ///////{
    ///////    std::cout << "Registry flag SEQ_DEBUG_FLAGS not found" << std::endl;
    ///////    ulSEQDebugFlags = 0;
    ///////}
    if (ulSEQDebugFlags!=0) m_Default_Label_Long_Arr1_SEQDebugFlags = SEQDebug_ShowFeedbackData;
    else                    m_Default_Label_Long_Arr1_SEQDebugFlags = 0;

    //In VD11A there is no registry support anymore.  Activate the following line for debugging info.
    //m_Default_Label_Long_Arr1_SEQDebugFlags = SEQDebug_ShowFeedbackData;

    return SEQU_NORMAL;
}

// =============================================
// Implementation of member function NavUI::prep
// =============================================
NLS_STATUS NavUI::prep (MrProt &rMrProt, SeqLim &rSeqLim, SeqExpo &rSeqExpo)
{
    UNUSED_PARAM(&rSeqLim);
    UNUSED_PARAM(&rSeqExpo);

    static const char * const ptModule = "NavUI::prep";

    if (rSeqLim.isContextPrepForMrProtUpdate() && rMrProt.NavigatorParam().getalFree()[Label_Dummy] == 0)
    {
        // ----------------------------------------
        // Set default values for the UI parameters
        // ----------------------------------------
        rMrProt.NavigatorParam().getalFree()[Label_Dummy]                              = 999;
        rMrProt.NavigatorParam().getalFree()[Label_SelectionBox_NavMode]               = m_Default_SelectionBox_NavMode;
        rMrProt.NavigatorParam().getalFree()[Label_SelectionBox_NavPulseType]          = m_Default_SelectionBox_NavPulseType;
        rMrProt.NavigatorParam().getalFree()[Label_SelectionBox_NavFBMode]             = m_Default_SelectionBox_NavFBMode;
        rMrProt.NavigatorParam().getalFree()[Label_Long_NavMatrix]                     = m_Default_Long_NavMatrix;
        rMrProt.NavigatorParam().getalFree()[Label_Long_NavFov]                        = m_Default_Long_NavFov;

		rMrProt.NavigatorParam().getalFree()[Label_Long_NoOfNavs]					   = m_Default_Long_NoOfNavs;				//JK2008
        rMrProt.NavigatorParam().getalFree()[Label_Long_TimeStartAcq]				   = m_Default_Long_TimeStartAcq;			//ib
        rMrProt.NavigatorParam().getalFree()[Label_Long_TimeEndAcq]					   = m_Default_Long_TimeEndAcq ;			//ib
        rMrProt.NavigatorParam().getalFree()[Label_Long_TimeEndCardiac]				   = m_Default_Long_TimeEndCardiac;			//ib
		rMrProt.NavigatorParam().getalFree()[Label_Long_NavTR_ms]					   = m_Default_Long_NavTR_ms;				//ib        
        rMrProt.NavigatorParam().getalFree()[Label_Long_PoleSensitivity]			   = m_Default_Long_PoleSensitivity;		//ib
        rMrProt.NavigatorParam().getalFree()[Label_Long_ScoutLength]				   = m_Default_Long_ScoutLength;			//ib
        rMrProt.NavigatorParam().getalFree()[Label_Long_SliceSelection]				   = m_Default_Long_SliceSelection;			//ib

        rMrProt.NavigatorParam().getalFree()[Label_SelectionBox_NavPosition]           = m_Default_SelectionBox_NavPosition;
        rMrProt.NavigatorParam().getadFree()[Label_Double_NavAcceptancePosition]       = m_Default_Double_NavAcceptancePosition;
        rMrProt.NavigatorParam().getadFree()[Label_Double_NavAcceptanceWidth]          = m_Default_Double_NavAcceptanceWidth;
        rMrProt.NavigatorParam().getadFree()[Label_Double_NavSearchPosition]           = m_Default_Double_NavSearchPosition;
        rMrProt.NavigatorParam().getadFree()[Label_Double_NavSearchWidth]              = m_Default_Double_NavSearchWidth;
        rMrProt.NavigatorParam().getadFree()[Label_Double_NavCorrectionFactor]         = m_Default_Double_NavCorrectionFactor;
        rMrProt.NavigatorParam().getalFree()[Label_CheckBox_PrepScan]                  = m_Default_CheckBox_PrepScan;
        rMrProt.NavigatorParam().getalFree()[Label_Long_NavPrepTR_ms]                  = m_Default_Long_NavPrepTR_ms;
        rMrProt.NavigatorParam().getalFree()[Label_Long_NavPrepDuration_sec]           = m_Default_Long_NavPrepDuration_sec;
        rMrProt.NavigatorParam().getalFree()[Label_Long_NavSleepDuration_ms]           = m_Default_Long_NavSleepDuration_ms;
        rMrProt.NavigatorParam().getalFree()[Label_Long_Arr0_RotatePlot]               = m_Default_Label_Long_Arr0_RotatePlot;
        rMrProt.NavigatorParam().getalFree()[Label_Long_Arr1_SEQDebugFlags]            = m_Default_Label_Long_Arr1_SEQDebugFlags;
        rMrProt.NavigatorParam().getalFree()[Label_Long_Arr2_ICEDebugFlags]            = m_Default_Label_Long_Arr2_ICEDebugFlags;
        rMrProt.NavigatorParam().getalFree()[Label_Long_Arr3_ICESmoothLength]          = m_Default_Label_Long_Arr3_ICESmoothLength;
        rMrProt.NavigatorParam().getalFree()[Label_Long_Arr4_ICELSQFitLength]          = m_Default_Label_Long_Arr4_ICELSQFitLength;
        rMrProt.NavigatorParam().getalFree()[Label_Long_Arr5_ICEImageSendInterval_Msec]= m_Default_Label_Long_Arr5_ICEImageSendInterval_Msec;
        rMrProt.NavigatorParam().getalFree()[Label_Long_Arr6_ICETimeScaleInterval_Sec] = m_Default_Label_Long_Arr6_ICETimeScaleInterval_Sec;
        rMrProt.NavigatorParam().getalFree()[Label_Long_Arr7_FeedbackPollInterval_ms]  = m_Default_Label_Long_Arr7_FeedbackPollInterval_ms;

        //
        //
        switch (rMrProt.NavigatorParam().getalFree()[Label_SelectionBox_NavMode])
        {
        default:
            rMrProt.NavigatorParam().setlRespComp(SEQ::RESP_COMP_OFF);
            break;
        case Value_NavModeProspectiveGating:
            rMrProt.NavigatorParam().setlRespComp(SEQ::RESP_COMP_GATE);
            break;
        case Value_NavModeFollowSlice:
            rMrProt.NavigatorParam().setlRespComp(SEQ::RESP_COMP_GATE_AND_FOLLOW);
            break;
        case Value_NavModeMonitorOnly:
            rMrProt.NavigatorParam().setlRespComp(SEQ::RESP_COMP_MONITOR_ONLY);
            break;
        case Value_NavModeOff:
            rMrProt.NavigatorParam().setlRespComp(SEQ::RESP_COMP_OFF);
            break;
        }

        switch (rMrProt.NavigatorParam().getalFree()[Label_SelectionBox_NavPulseType])
        {
        default:
            rMrProt.NavigatorParam().setucRFPulseType(SEQ::EXCIT_MODE_CROSSED_PAIR);
            break;
        case Value_NavPulseType2DPencil:
            rMrProt.NavigatorParam().setucRFPulseType(SEQ::EXCIT_MODE_2D_PENCIL);
            break;
        case Value_NavPulseTypeCrossedPair:
            rMrProt.NavigatorParam().setucRFPulseType(SEQ::EXCIT_MODE_CROSSED_PAIR);
            break;
        }

        //  --------------------------------------------------------------------
        //  Initialize the navigator objects in the protocol with default values
        //  --------------------------------------------------------------------
        NavigatorArray& rNaviArray = rMrProt.navigatorArray();

        for (long lI = 0; lI < rNaviArray.size(); lI++)
        {
            if( rNaviArray[lI].phaseFOV() == 0 )
            {
                //if ( !pSeqLim->isContextPrepForMrProtUpdate() ) return(SEQU_ERROR);

                switch (rMrProt.NavigatorParam().getalFree()[Label_SelectionBox_NavPulseType])
                {
                case Value_NavPulseType2DPencil:
                    rNaviArray[lI].thickness  ( 0.0 );
                    rNaviArray[lI].readoutFOV ( rSeqLim.getNavigatorReadFOV() [lI].getDef() );
                    rNaviArray[lI].phaseFOV   ( rSeqLim.getNavigatorPhaseFOV()[lI].getDef() );
                    rNaviArray[lI].position   ( 0.0,0.0,0.0 );
                    rNaviArray[lI].normal     ( VectorPat<double>::e_sag );
                    break;
                case Value_NavPulseTypeCrossedPair:
                    rNaviArray[lI].thickness  ( rSeqLim.getNavigatorThickness() [lI].getDef() );
                    rNaviArray[lI].readoutFOV ( 400.0 );
                    rNaviArray[lI].phaseFOV   ( 400.0 );
                    rNaviArray[lI].position   ( 0.0,0.0,0.0 );
                    if (lI % 2 == 0)
                    {
                        rNaviArray[lI].normal(VectorPat<double>::e_sag);
                    }
                    else
                    {
                        rNaviArray[lI].normal(SEQ::SAG_TO_COR_TO_TRA,30,0);
                    }
                    break;
                default:
                    std::cout << ptModule << " Label_SelectionBox_NavPulseType is not recognized" << std::endl;
                    break;
                }
            }
        }
    }

    //  Any modified registry value should also cause an update of the existing protocols.
    rMrProt.NavigatorParam().getalFree()[Label_Long_Arr1_SEQDebugFlags] = m_Default_Label_Long_Arr1_SEQDebugFlags;
    rMrProt.NavigatorParam().getalFree()[Label_Long_Arr2_ICEDebugFlags] = m_Default_Label_Long_Arr2_ICEDebugFlags;

    //  This parameter position was redefined as Msec, it was previously seconds.  So to remain
    //  compatible with old protocols...
    if (rMrProt.NavigatorParam().getalFree()[Label_Long_Arr5_ICEImageSendInterval_Msec] < 100)
    {
        rMrProt.NavigatorParam().getalFree()[Label_Long_Arr5_ICEImageSendInterval_Msec] =
            m_Default_Label_Long_Arr5_ICEImageSendInterval_Msec;
    }

    // Ensure the sleep duration is not too small
    if (rMrProt.NavigatorParam().getalFree()[Label_Long_NavSleepDuration_ms]<10)
        rMrProt.NavigatorParam().getalFree()[Label_Long_NavSleepDuration_ms]=10;

    // -----------------------------------------------------------------------
    // If the default values have not yet been set, force fSEQPrep to run with
    // isContextPrepForMrProtUpdate context by returning with SEQU_ERROR.
    // -----------------------------------------------------------------------
    if (rMrProt.NavigatorParam().getalFree()[Label_Dummy] == 0)
    {
        return SEQU_ERROR;
    }

    return SEQU_NORMAL;
}

#endif  // ifdef WIN32



#ifdef SDUSDFUDSFUSFDUFSDFSSSDFSDFSF_WIN32
// #### move this to e.g. libSeqSysProp??
#include <windows.h>
//  =============================
//  Windows NT Registry functions
//  =============================

// =================================
// Implementation of RegIncrOperator
// =================================
#define PRINT_ERROR(_file,_line,_id)                                   \
{                                                                      \
    LPVOID lpMsgBuf;                                                     \
    FormatMessageA(                                      \
    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,         \
    NULL,                                                                \
    _id,                                                                 \
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),                           \
    (char*) &lpMsgBuf,                                                   \
    0,                                                                   \
    NULL                                                                 \
    );                                                                   \
    std::cout << "Error: " << (char*) lpMsgBuf << std::endl;                       \
    LocalFree(lpMsgBuf);                                                 \
}

bool RegIntOperator(const char* lpszPath, const char* lpszEntry, RegIntOperation nOperation, unsigned long& rVal, unsigned long ulDef)
{
    if(!lpszPath || !lpszEntry)
    {
        std::cout << "RegIntOperator: ERROR: Invalid argument"  << std::endl;
        return false;
    }

    HKEY hKeyPath = NULL;
    DWORD dwDisposition  = 0;
    LONG lStatus = RegCreateKeyExA(
        HKEY_LOCAL_MACHINE,      //  handle to an open key
        lpszPath,                //  address of subkey name
        0,                       //  Reserved; must be zero.
        REG_NONE,                //  address of class string
        REG_OPTION_VOLATILE,     //  This key is volatile; the information is stored in memory and is not preserved when the system is restarted.
        KEY_SET_VALUE            //  Permission to set subkey data.
        |KEY_CREATE_SUB_KEY      //  Permission to create subkeys.
        |KEY_QUERY_VALUE,        //  Permission to query subkey data.
        NULL,                    //  desired security access: the returned handle can't be inherited by child processes
        &hKeyPath,               //  address of buffer for opened handle
        &dwDisposition           //  address of disposition value buffer
        );

    if(lStatus != ERROR_SUCCESS)
    {
        PRINT_ERROR(__FILE__,__LINE__,lStatus);
        return false;
    }
    DWORD dwCurVal = ulDef;
    if(dwDisposition == REG_CREATED_NEW_KEY && nOperation == REG_INT_OPERATION_GET)
    {
        nOperation = REG_INT_OPERATION_SET;
        rVal       = ulDef;
    }
    else if(dwDisposition == REG_OPENED_EXISTING_KEY && nOperation != REG_INT_OPERATION_SET)
    {
        DWORD dwType, dwCount = sizeof(DWORD);
        lStatus = RegQueryValueExA(
            hKeyPath,            //  handle to key to query
            lpszEntry,           //  address of name of value to query
            0,                   //  Reserved; must be NULL.
            &dwType,             //  Pointer to a variable that receives the type of data associated with the specified value.
            (LPBYTE)&dwCurVal,   //  Pointer to a buffer that receives the value's data.
            &dwCount             //  Pointer to a variable that specifies the size, in bytes, of the buffer pointed to by the lpData parameter. When the function returns, this variable contains the size of the data copied to lpData.
            );

        if( lStatus == ERROR_FILE_NOT_FOUND )
        {
            //  The entry does not exist
            dwCurVal = ulDef;
            if(nOperation == REG_INT_OPERATION_GET)
            {
                nOperation = REG_INT_OPERATION_SET;
                rVal       = ulDef;
            }
        }
        else if(lStatus != ERROR_SUCCESS || dwType != REG_DWORD || dwCount != sizeof(DWORD) )
        {
            PRINT_ERROR(__FILE__,__LINE__,lStatus);
            RegCloseKey(hKeyPath);
            return false;
        }
    }
    DWORD dwNewVal = dwCurVal;
    switch(nOperation)
    {
    case REG_INT_OPERATION_GET:
        rVal = dwCurVal;
        RegCloseKey(hKeyPath);
        return true;
    case REG_INT_OPERATION_SET:
        dwNewVal = rVal;
        break;
    case REG_INT_OPERATION_PREFIX_INCR:
        dwNewVal = dwCurVal+rVal;
        rVal     = dwNewVal;
        break;
    case REG_INT_OPERATION_POSTFIX_INCR:
        dwNewVal = dwCurVal+rVal;
        rVal     = dwCurVal;
        break;
    default:
        std::cout << "RegIntOperator: ERROR: Unknown argument # 4: " << nOperation << std::endl;
        RegCloseKey(hKeyPath);
        return false;
    }


    //  increment/initialize value of registry entry
    lStatus =  RegSetValueExA(
        hKeyPath,            //  handle to key to set value for
        lpszEntry,           //  name of the value to set
        NULL,                //  Reserved; must be zero.
        REG_DWORD,           //  flag for value type
        (LPBYTE)&dwNewVal,   //  address of value data
        sizeof(DWORD)        //  size of value data
        );

    if(lStatus != ERROR_SUCCESS)
    {
        PRINT_ERROR(__FILE__,__LINE__,lStatus);
        RegCloseKey(hKeyPath);
        return false;
    }
    RegCloseKey(hKeyPath);
    return true;
}
#endif
