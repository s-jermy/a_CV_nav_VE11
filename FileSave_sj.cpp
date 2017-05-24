//  -----------------------------------------------------------------------------
//   Project: Myne
//   File: n4\pkg\MrServers\MrImaging\seq\a_cv_nav_ib\FileSave_ib.cpp
//   Version: 1.0
//   Author: Ian Burger
//   Date: 28/09/2009
//   Language: C++
//   Descrip: save a file with data acquired in idea
//
//  -----------------------------------------------------------------------------

//------------------------------------------------------------------------------
// include files                   
//------------------------------------------------------------------------------

#include "FileSave_sj.h"

//retro
#define private public
//#include "MrServers/MrImaging/seq/nav_newspiral_DB2/LocalSeqLoop.h"
#undef private

//JK added:
/*
#include "MrServers/MrImaging/seq/nav_newspiral_DB2/spiralx_bh.h"		//JK
#include "MrServers/MrImaging/seq/nav_newspiral_DB2/spiraly_bh.h"		//JK
*/
//ADS	My include
//#include "MrServers/MrImaging/seq/nav_newspiral_DB2/ADS_header.h"

#include  "MrServers/MrImaging/seq/SeqDebug.h"				// for debugging macros
#include  "MrServers/MrImaging/seq/SystemProperties.h"		// for siemens system properties
#include  "MrServers/MrImaging/libSBB/SBBTSat.h"			// for T-Sat object
#include  "MrServers/MrImaging/libSeqUtil/libSeqUtil.h"		// for fSU functions.
#include  "MrServers/MrImaging/ut/libSeqUT.h"				// for mSEQTest
//#include  "MrServers/MrImaging/seq/nav_newspiral_DB2/LocalSeqLoop.h"	// for SeqLoop class
//JK: #include  "MrServers/MrImaging/seq/Kernels/SBBGREFCKernel.h"    // includes the GRE Flow compensated Kernel
//#include  "MrServers/MrImaging/seq/nav_newspiral_DB2/SBBGREFCKernel.h"    //JK

#include  "MrServers/MrMeasSrv/SeqIF/Sequence/sequmsg.h"	// for SEQU_ ... errors codes
#include  "MrServers/MrMeasSrv/SeqIF/Sequence/Sequence.h"
#include  "MrServers/MrMeasSrv/SeqIF/csequence.h"			// Defines for RF spoiling
#include  "MrServers/MrMeasSrv/SeqFW/libSSL/libSSL.h"		// for fSSL functions.
#include  "MrServers/MrMeasSrv/MeasUtils/MeasMath.h"		// minimum/maximum

//#include  "MrServers/MrPerAns/PerProxies/GCProxy.h"		// for GC proxy :-)						sj - commented out
//#include  "MrServers/MrPerAns/PerProxies/GPAProxy.h"		// for GPA proxy						sj - commented out
//#include  "MrServers/MrPerAns/PerProxies/MSUProxy.h"		// for MSU Proxy, contains nominal Bo	sj - commented out

#ifdef WIN32
//#include "MrServers/MrImaging/libUICtrl/UICtrl.h"			//sj - commented out

#include "MrServers/MrProtSrv/MrProtocol/libUILink/StdRoutines.h"
#include "MrServers/MrProtSrv/MrProtocol/libUILink/UILinkLimited.h"
#include "MrServers/MrProtSrv/MrProtocol/libUILink/UILinkSelection.h"	//JK
#include "MrServers/MrProtSrv/MrProtocol/libUILink/UILinkArray.h"		//JK
#include "MrServers/MrProtSrv/MrProtocol/UILink/StdProtRes/StdProtRes.h"
#include "MrServers/MrProtSrv/MrProtocol/UILink/MrStdNameTags.h"

#include "MrServers/MrMeasSrv/SeqIF/Sequence/Sequence.h"				//JK
#include <vector>														//JK: Sequence Special Card
#endif

// * ------------------------------------------------------------------------------ *
// * supprot iPAT                                                                   *
// * ------------------------------------------------------------------------------ *
#ifdef SUPPORT_iPAT
    #include   "MrServers/MrImaging/seq/common/iPAT/iPAT.h"
#endif

FILE *MyFPtr;

FileSaveIB::FileSaveIB()
{
}

FileSaveIB::~FileSaveIB()
{
}

void FileSaveIB::FileSaveOpen()
{
	//cout<<"28-09#############################-------in--------##########################"<<endl;
	
	m_iNavCountersv 	= 0;
	m_iCSCountersv		= 0;
	m_iFFTCountersv 	= 0;
	m_iExtraCountersv	= 0;
	
		
	for (m_iOutCounter=0; m_iOutCounter<10000; m_iOutCounter++) 
	{
		m_dLogNav[m_iOutCounter]	= 0;	//zero all
		m_dLogCSOut[m_iOutCounter]	= 0;
		m_dLogFFT[m_iOutCounter]	= 0;
		m_dLogExtra[m_iOutCounter]	= 1;
	}
	
	time_t	ctime; 
	tm		*stime;  
	// get current date and time in a string (format: YYYYMMDD_HHhMMmSSs)
	time(&ctime);
	stime = localtime(&ctime);
	strftime(m_cDateTime,20, "%Y%m%d_%Hh%Mm%Ss", stime);
	sprintf(m_cMyFilename, "save_data_");
	strcat(m_cMyFilename, m_cDateTime);
	strcat(m_cMyFilename, ".txt");
	if( (MyFPtr=fopen(m_cMyFilename,"w"))==NULL )
	{
		std::cout<<"error opening outfile"<<std::endl;
	}
}	

//ib-START - record the data to the file and close it		
void FileSaveIB::FileSaveClose()
{
	//cout<<"close---------------------"<<endl;
	for (m_iOutCounter=0; m_iOutCounter<m_iNavCountersv; m_iOutCounter++)
		{
			fprintf(MyFPtr,"%6.2f\n", m_dLogNav[m_iOutCounter]);
		}
		fprintf(MyFPtr,"-----------------\n\n");
	for (m_iOutCounter=0; m_iOutCounter<m_iCSCountersv; m_iOutCounter++)
		{
			fprintf(MyFPtr,"%6.2f\n", m_dLogCSOut[m_iOutCounter]);
		}
		fprintf(MyFPtr,"-----------------\n\n");
	for (m_iOutCounter=0; m_iOutCounter<m_iFFTCountersv; m_iOutCounter++)
		{
			fprintf(MyFPtr,"%6.2f\n", m_dLogFFT[m_iOutCounter]);
		}
		fprintf(MyFPtr,"-----------------\n\n");
	for (m_iOutCounter=0; m_iOutCounter<m_iExtraCountersv; m_iOutCounter++)
		{
			fprintf(MyFPtr,"%6.2f\n", m_dLogExtra[m_iOutCounter]);
		}
	fclose(MyFPtr);
}

//record the data to the file and close it
void FileSaveIB::FileSaveAccess(double dResults, int iNameOfFile)
{
	if (iNameOfFile == 1)
	{
		m_dLogNav[m_iNavCountersv] = dResults;
		m_iNavCountersv++;
	}
	else if (iNameOfFile == 2)
	{
		m_dLogCSOut[m_iCSCountersv] = dResults;
		m_iCSCountersv++;
	}
	else if (iNameOfFile == 3)
	{
		m_dLogFFT[m_iFFTCountersv] = dResults;
		m_iFFTCountersv++;
	}
	else if (iNameOfFile == 4)
	{
		m_dLogExtra[m_iExtraCountersv] = dResults;
		m_iExtraCountersv++;
	}
}
