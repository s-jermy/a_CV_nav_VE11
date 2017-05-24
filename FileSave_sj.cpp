#include "FileSaveIB.h"





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

#include  "MrServers/MrImaging/seq/SeqDebug.h"                  // for debugging macros
#include  "MrServers/MrMeasSrv/SeqIF/Sequence/sequmsg.h"              // for SEQU_ ... errors codes
#include  "MrServers/MrMeasSrv/SeqIF/Sequence/Sequence.h"
#include  "MrServers/MrImaging/libSBB/SBBTSat.h"	// for T-Sat object
//#include  "MrServers/MrImaging/seq/nav_newspiral_DB2/LocalSeqLoop.h"	// for SeqLoop class
#include  "MrServers/MrMeasSrv/SeqFW/libSSL/libSSL.h"        // for fSSL functions.
#include  "MrServers/MrImaging/libSeqUtil/libSeqUtil.h" // for fSU functions.
#include  "MrServers/MrPerAns/PerProxies/GCProxy.h"     // for GC proxy :-)
#include  "MrServers/MrPerAns/PerProxies/GPAProxy.h"    // for GPA proxy 
#include  "MrServers/MrPerAns/PerProxies/MSUProxy.h"    // for MSU Proxy, contains nominal Bo
#include  "MrServers/MrMeasSrv/SeqIF/csequence.h"            // Defines for RF spoiling
#include  "MrServers/MrImaging/ut/libSeqUT.h"                   // for mSEQTest
//JK: #include  "MrServers/MrImaging/seq/Kernels/SBBGREFCKernel.h"    // includes the GRE Flow compensated Kernel
//#include  "MrServers/MrImaging/seq/nav_newspiral_DB2/SBBGREFCKernel.h"    //JK
#include  "MrServers/MrImaging/seq/SystemProperties.h "                  // for siemens system properties
#include  "MrServers/MrMeasSrv/MeasUtils/MeasMath.h"  // minimum/maximum

#ifndef VXWORKS
#include  "MrServers/MrProtSrv/MrProtocol/libUILink/StdRoutines.h"
#include "MrServers/MrImaging/libUICtrl/UICtrl.h"
#include "MrServers/MrProtSrv/MrProtocol/libUILink/UILinkLimited.h"
#include "MrServers/MrProtSrv/MrProtocol/libUILink/UILinkSelection.h"	//JK
#include "MrServers/MrProtSrv/MrProtocol/libUILink/UILinkArray.h"		//JK
#include "MrServers/MrMeasSrv/SeqIF/Sequence/Sequence.h"				//JK
#include <vector>														//JK: Sequence Special Card
#include "MrServers/MrProtSrv/MrProtocol/UILink/StdProtRes/StdProtRes.h"
#include "MrServers/MrProtSrv/MrProtocol/UILink/MrStdNameTags.h"
#endif

// * ------------------------------------------------------------------------------ *
// * supprot iPAT                                                                   *
// * ------------------------------------------------------------------------------ *
#ifdef SUPPORT_iPAT
    #include   "MrServers/MrImaging/seq/common/iPAT/iPAT.h"
#endif





FILE *MyFPtr;


   FileSaveIB::FileSaveIB(){}
   FileSaveIB::~FileSaveIB(){}


void FileSaveIB::FSIB_open()
{

	//cout<<"28-09#############################-------in--------##########################"<<endl;
	
	NavCountersv 	= 0;
	CSCountersv		= 0;
	FFTCountersv 	= 0;
	ExtraCountersv 	= 0;
	
		
	for (int OutCounter = 0; OutCounter<10000;OutCounter++) 
		{
		LogNav[OutCounter] 		= 0;	//zero all
		LogCSOut[OutCounter] 	= 0;
		LogFFT[OutCounter] 		= 0;
		LogExtra[OutCounter]	= 1;
		
		}
	
	time_t				ctime; 
	tm					*stime;  
	//	get current date and time in a string (format: YYYYMMDD_HHhMMmSSs)
	time(&ctime);
	stime=localtime(&ctime);
	strftime(datetime,20,"%Y%m%d_%Hh%Mm%Ss",stime);
	sprintf(myfilename,"save_data_");
	strcat(myfilename,datetime);
	strcat(myfilename,".txt");
	if((MyFPtr=fopen(myfilename,"w"))==NULL)
		{cout<<"error opening oufile"<<endl;};
}	

//ib-START - record the data to the file and close it		
void FileSaveIB::FSIB_close()
{
	//cout<<"close---------------------"<<endl;
	for (OutCounter = 0;OutCounter<NavCountersv;OutCounter++)
		{
			fprintf(MyFPtr,"%6.2f\n",LogNav[OutCounter]);
		}
		fprintf(MyFPtr,"-----------------\n\n");
	for (OutCounter = 0;OutCounter<CSCountersv;OutCounter++)
		{
			fprintf(MyFPtr,"%6.2f\n",LogCSOut[OutCounter]);
		}
		fprintf(MyFPtr,"-----------------\n\n");
	for (OutCounter = 0;OutCounter<FFTCountersv;OutCounter++)
		{
			fprintf(MyFPtr,"%6.2f\n",LogFFT[OutCounter]);
		}
		fprintf(MyFPtr,"-----------------\n\n");
	for (OutCounter = 0;OutCounter<ExtraCountersv;OutCounter++)
		{
			fprintf(MyFPtr,"%6.2f\n",LogExtra[OutCounter]);
		}
	fclose(MyFPtr);
}

//record the data to the file and close it
void FileSaveIB::FSIB_acces(double results, int nameOfFileib)
{
	if (nameOfFileib == 1)
	{
	LogNav[NavCountersv] = results;
	NavCountersv++;
	}
	else if (nameOfFileib == 2)
	{
	LogCSOut[CSCountersv] = results;
	CSCountersv++;
	}
	else if (nameOfFileib == 3)
	{
	LogFFT[FFTCountersv] = results;
	FFTCountersv++;
	}
	else if (nameOfFileib == 4)
	{
	LogExtra[ExtraCountersv] = results;
	ExtraCountersv++;
	}
}
