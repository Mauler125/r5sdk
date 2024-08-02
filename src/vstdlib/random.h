//===== Copyright (c) 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: Random number generator
//
// $Workfile: $
// $NoKeywords: $
//===========================================================================//

#ifndef VSTDLIB_RANDOM_H
#define VSTDLIB_RANDOM_H

#include "tier0/basetypes.h"

#define NTAB 32

#pragma warning(push)
#pragma warning( disable:4251 )

//-----------------------------------------------------------------------------
// A generator of uniformly distributed random numbers
//-----------------------------------------------------------------------------
class IUniformRandomStream
{
public:
	// Sets the seed of the random number generator
	virtual void	SetSeed(const int iSeed) = 0;
	virtual int		GetSeed() const = 0;

	// Generates random numbers
	virtual float	RandomFloat(const float flMinVal = 0.0f, const float flMaxVal = 1.0f) = 0;
	virtual int		RandomInt(const int iMinVal, const int iMaxVal) = 0;
	virtual float	RandomFloatExp(const float flMinVal = 0.0f, const float flMaxVal = 1.0f, const float flExponent = 1.0f) = 0;
	virtual int		RandomShortMax() = 0;

	virtual ~IUniformRandomStream() {};
};


//-----------------------------------------------------------------------------
// The standard generator of uniformly distributed random numbers
//-----------------------------------------------------------------------------
class CUniformRandomStream : public IUniformRandomStream
{
public:
	CUniformRandomStream();

	// Sets the seed of the random number generator
	virtual void	SetSeed(const int iSeed);
	virtual int		GetSeed() const;

	// Generates random numbers
	virtual float	RandomFloat(const float flMinVal = 0.0f, const float flMaxVal = 1.0f);
	virtual int		RandomInt(const int iMinVal, const int iMaxVal);
	virtual float	RandomFloatExp(const float flMinVal = 0.0f, const float flMaxVal = 1.0f, const float flExponent = 1.0f);
	virtual int		RandomShortMax();

	virtual ~CUniformRandomStream() {};

private:
	int		GenerateRandomNumber();

	int m_idum;
	CThreadMutex m_mutex;
};


//-----------------------------------------------------------------------------
// A generator of gaussian distributed random numbers
//-----------------------------------------------------------------------------
class CGaussianRandomStream
{
public:
	// Passing in NULL will cause the gaussian stream to use the
	// installed global random number generator
	CGaussianRandomStream(IUniformRandomStream* const pUniformStream = NULL);

	// Attaches to a random uniform stream
	void	AttachToStream(IUniformRandomStream* const pUniformStream = NULL);

	// Generates random numbers
	float	RandomFloat(const float flMean = 0.0f, const float flStdDev = 1.0f);

private:
	IUniformRandomStream* m_pUniformStream;
	bool	m_bHaveValue;
	float	m_flRandomValue;

	CThreadMutex m_mutex;
};


//-----------------------------------------------------------------------------
// A couple of convenience functions to access the library's global uniform stream
//-----------------------------------------------------------------------------
void	RandomSeed(const int iSeed);
float	RandomFloat(const float flMinVal = 0.0f, const float flMaxVal = 1.0f);
float	RandomFloatExp(const float flMinVal = 0.0f, const float flMaxVal = 1.0f, const float flExponent = 1.0f);
int	RandomInt(const int iMinVal, const int iMaxVal);
int	RandomShortMax();
float	RandomGaussianFloat(const float flMean = 0.0f, const float flStdDev = 1.0f);


//-----------------------------------------------------------------------------
// Installs a global random number generator, which will affect the Random functions above
//-----------------------------------------------------------------------------
void	InstallUniformRandomStream(IUniformRandomStream* pStream);


#pragma warning(pop)

#endif // VSTDLIB_RANDOM_H
