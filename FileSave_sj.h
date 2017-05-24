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

#ifdef WIN32
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
    
    
    char	m_cMyFilename[128];
	char	m_cDateTime[20]; 
    double	m_dLogNav[10000];
    double	m_dLogCSOut[10000];
    double	m_dLogFFT[10000];
    double	m_dLogExtra[10000];
	
    //double m_dLog[10000];
	int		m_iNavCountersv;
	int		m_iCSCountersv;
	int		m_iFFTCountersv;
	int		m_iExtraCountersv;
	int		m_iOutCounter;
	long	*m_pNavLog;

	void	FileSaveOpen();

	void	FileSaveClose();

	void	FileSaveAccess(double dResults, int iNameOfFile);

};
 
#endif