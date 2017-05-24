//  -----------------------------------------------------------------------------
//   Project: Myne
//   File: n4\pkg\MrServers\MrImaging\seq\a_cv_nav_ib\ControlSystemIB.h
//   Version: 1.0
//   Author: Ian Burger
//   Date: 28/09/2009
//   Language: C++
//   Descrip: save a file with data acquired in idea
//
//  -----------------------------------------------------------------------------

#ifndef ControlSystemIB2_H
#define ControlSystemIB2_H

#include "MrServers/MrImaging/Seq/a_cv_nav_ib/FileSaveIB.h"					//ib-c
	
#include <math.h>
#include <stdio.h>
#include <iostream>
using namespace std;


class ControlSystemIB2{
friend class ControlSystemIB;		//declare as a friend

public:

    //  Default constructor
    	ControlSystemIB2();
    
    //  Destructor
    	virtual ~ControlSystemIB2();
    	

	double 	SAMPLE_RATE;
	double 	Lp[2][1];
	double 	H[1][2];  
	double 	phi[2][2];
	double 	x[2][1];
	double	x_er;	
	double	yplant;
	double	Xfft[1024];/*scoutValue*/
	double	yCS;
	double 	ySave[10000];
	double 	sample_rate;    	
	int 	num ;
	double 	ZestR, ZestI,Zest1, Zest2;
	int		counterC;			//counter for how many samples and how many predictions						(Updateib)	(needs to be adjustable) nbnbnbn
	int		counterSVSet;		//counter for used to set scoutval = 0 [this is where the scout is saved]	(InitScoutVal)
	int		counterSV;			//counter for set value of scout											(GetNavVal)
	int		counterOut;			//counter for the output of the control system								(Outputsib)
	int		counterDead;		//counter to correct if the dead time was longer or if QRS was missed		(calculateTdead)
	
	double 	a;
	double  Ts;
	double 	k_sin;
	bool 	scoutOn;
	double	numOfNavs;
	double	navTr;
	double	imTr;
	double 	predictSamples;
	double	extraTime;
 	int		xcounter;
 	int		xcounter2;
 	double	ave;
//------------time calculations----------
	double Ta;
	double Tb;
	double Tc;
	double Tdead;
	double Twait;
	double samples_passed;
	double time_left;
	double Tprep;
	double Tacq;
	double Predict_Samples2;
//--------------------------------------- 	
 	 	
 	 FileSaveIB saveib;										//ib-s
 	
/*====================*
 * methods *
 *====================*/
//void PLayOutCS();//(int *yl);
void 	PlayOutCSib();
void 	GetDataib();

void 	InitializeConditionsib(long numOfNavs1, long navTr1, long imTr1);

void 	Startib();
  
void 	Outputsib();

void 	Updateib();			//receive the adress of yplant;

void 	Terminateib();

void 	CalculateSystemValuesib();

// void CalculatePeriodib(double *scoutval);

// double LowPassFilterib(double input);

void 	CalculateFFTib();

void 	Maxib(double *in);


void 	SetScoutOnib(bool scout);

bool 	GetScoutOnib();

void 	SetNavValib(double NavValueImport, bool IsScout);

void 	InitScoutValib();

void 	DrawSinib();

double 	xsamplevalib();

double 	modulusib(double a, double b);

long 	getTwait();

void 	CalculateAveib();

void 	SliceSelectionib();

void 	CloseCSib();

double 	getCSOut();

void 	calculations(long Ta1, long Tb1, long Tc1);

void 	calculateTdead(short Trr);

int 	getNoOfNavs();

};
 
#endif