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
//   Porting Ian's control system over to VE11
//  -----------------------------------------------------------------------------

#ifndef ControlSystemIB2_H
#define ControlSystemIB2_H

#include "MrServers/MrImaging/seq/a_CV_nav_VE11/FileSave_sj.h"			//ib-c
	
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
    	

	//double	m_dSAMPLE_RATE;
	double	m_dLp[2][1];		// feedback gain matrix
	double	m_dH[1][2];			// output matrix
	double	m_dPhi[2][2];		// discrete state matrix
	double	m_dX[2][1];			// current state of the model
	double	m_dXer;				// error between plant and control system
	double	m_dYPlant;			// output of plant
	double	m_dXfft[1024];		//*scoutValue*/
	double	m_dyCS;				// output of control system
	double	m_dYSave[10000];
	//double	m_dSampleRate;
	//int		m_iNum;
	double	m_dZestR,m_dZestI,m_dZest1,m_dZest2;	// pole placement estimates

	int		m_iCounterC;		// counter for how many samples and how many predictions					(Updateib)	(needs to be adjustable) nbnbnbn
	int		m_iCounterSVSet;	// counter for used to set scoutval = 0 [this is where the scout is saved]	(InitScoutVal)
	int		m_iCounterSV;		// counter for set value of scout											(GetNavVal)
	int		m_iCounterOut;		// counter for the output of the control system								(Outputsib)
	int		m_iCounterDead;		// counter to correct if the dead time was longer or if QRS was missed		(calculateTdead)
	
	double	m_dA;
	double	m_dTs;				// sampling time
	double	m_dKsin;
	bool	m_bScoutOn;
	double	m_dNumOfNavs;		// number of navigators
	double	m_dNavTr;			// TR of navigator
	double	m_dImTr;			// TR of imaging?
	double	m_dPredictSamples;
	double	m_dExtraTime;
 	int		m_iXCounter1;
 	int		m_iXCounter2;
 	double	m_dAve;

//------------time calculations----------
	double	m_dTa;				// acquisition start
	double	m_dTb;				// acquisition end
	double	m_dTc;				// cardiac cycle end
	double	m_dTdead;			// dead time - time between end of acquisition and next r-wave
	double	m_dTwait;			// wait time - time between r-wave and first nav
	double	m_dSamplesPassed;	// number of samples gathered
	double	m_dTimeLeft;		// this is the time that passed extra to the time which fits into full samples
	double	m_dTprep;			// prep time
	double	m_dTacq;			// acquisition time
	double	m_dPredictSamples2;
//--------------------------------------- 	
 	 	
 	 FileSaveIB saveib;			//ib-s
 	
/*====================*
 * methods *
 *====================*/
	void	InitializeConditions_ib(long lNumOfNavs, long lNavTr, long lImTr); // set initial conditions for control system, and get navigator info
	void	InitScoutVal_ib();			// sets xfft to zero, sets counter to zero
	void	CalculateSystemValues_ib();	// set all the control system values
	void	SaveData_ib();				// save data
	void	Update_ib();				// creates the x(k+1) for the next period
	void	Outputs_ib();				// calculates the output of the control system, will return value for slice follow
	void	CloseCS_ib();				// close control system
	void	Calculations(long lTa, long lTb, long lTc);	// time calculations
	void	CalculateTdead(short sTRr);	// calculate dead time
	int		getNoOfNavs();
	long	getTwait();					// get Twait in ms
	void	setScoutOn_ib(bool bScout);	// set that scout is on
	bool	getScoutOn_ib();			// get whether or not scout is on
	double	getCSOut();					// return control system output to kernel via friend
	void	setNavVal_ib(double dNavValueImport, bool bScout);	// receives the navigator value assigns it to the fft or control system input
	//void	PLayOutCS();//(int *yl);	// just for debug..runs control system - modify for real use
	void	PlayOutCS_ib();				// just for debug..runs control system - modify for real use
	double	XSampleVal_ib();			// just for debug..feeds dummy navigator values
	double	Modulus_ib(double dA, double dB);
	void	CalculateAve_ib();
	void	CalculateFFT_ib();			// This computes an in-place complex-to-complex FFT
	void	Max_ib(double *dIn);		// calculate the frequency of the respiration from the fft of scout collected data
	//void	CalculatePeriod_ib(double *scoutval);
	//double	LowPassFilter_ib(double input);
	//void	DrawSin_ib();				//
	//void	Start_ib();					//
	//void	Terminate_ib();				//
	//void	SliceSelection_ib();		// calculates the slice select matrix
};
 
#endif