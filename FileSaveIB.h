//  -----------------------------------------------------------------------------
//   Project: Myne
//   File: n4\pkg\MrServers\MrImaging\seq\a_cv_nav_ib\FileSave_ib.h
//   Version: 1.0
//   Author: Ian Burger
//   Date: 28/09/2009
//   Language: C++
//   Descrip: save a file with data acquired in idea
//
//  -----------------------------------------------------------------------------

#ifndef FileSaveIB_1
#define FileSaveIB_1 1

#ifndef VXWORKS
#include "MrServers/MrMeasSrv/SeqIF/Sequence/Sequence.h"				
#include <vector>														
#include "MrServers/MrProtSrv/MrProtocol/UILink/StdProtRes/StdProtRes.h"
#include "MrServers/MrProtSrv/MrProtocol/UILink/MrStdNameTags.h"
#endif  

class FileSaveIB{

public:

    //  Default constructor
    	FileSaveIB();
    
    //  Destructor
    	virtual ~FileSaveIB();
    
    
    char myfilename[128];
	char datetime[20]; 
    double LogNav[10000];
    double LogCSOut[10000];
    double LogFFT[10000];
    double LogExtra[10000];
	
    //double Log[10000];
	int NavCountersv;
	int CSCountersv;
	int FFTCountersv;
	int ExtraCountersv;
	int	OutCounter;
	long * navlogpntr;
		
void FSIB_open();

void FSIB_close();

void FSIB_acces(double results, int nameOfFileib);

};
 
#endif