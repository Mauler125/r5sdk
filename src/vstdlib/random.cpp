//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: Random number generator
//
// $Workfile: $
// $NoKeywords: $
//===========================================================================//

#include "core/stdafx.h"
#include "vstdlib/random.h"
#include "tier0/dbg.h"

#define IA 16807
#define IM 2147483647
#define IQ 127773
#define IR 2836
#define NDIV (1+(IM-1)/NTAB)
#define MAX_RANDOM_RANGE 0x7FFFFFFFUL
#define MAX_RANDOM_RANGE_SHORT 0x7FFFUL

// fran1 -- return a random floating-point number on the interval [0,1)
//
#define AM (1.0f/IM)
#define EPS 1.2e-7f
#define RNMX (1.0f-EPS)

//-----------------------------------------------------------------------------
// globals
//-----------------------------------------------------------------------------
static CUniformRandomStream s_UniformStream;
static CGaussianRandomStream s_GaussianStream;
static IUniformRandomStream* s_pUniformStream = &s_UniformStream;


//-----------------------------------------------------------------------------
// Installs a global random number generator, which will affect the Random functions above
//-----------------------------------------------------------------------------
void InstallUniformRandomStream(IUniformRandomStream* pStream)
{
	s_pUniformStream = pStream ? pStream : &s_UniformStream;
}


//-----------------------------------------------------------------------------
// A couple of convenience functions to access the library's global uniform stream
//-----------------------------------------------------------------------------
void RandomSeed(const int iSeed)
{
	s_pUniformStream->SetSeed(iSeed);
}

float RandomFloat(const float flMinVal, const float flMaxVal)
{
	return s_pUniformStream->RandomFloat(flMinVal, flMaxVal);
}

float RandomFloatExp(const float flMinVal, const float flMaxVal, const float flExponent)
{
	return s_pUniformStream->RandomFloatExp(flMinVal, flMaxVal, flExponent);
}

int RandomInt(const int iMinVal, const int iMaxVal)
{
	return s_pUniformStream->RandomInt(iMinVal, iMaxVal);
}

int RandomShortMax()
{
	return s_pUniformStream->RandomShortMax();
}

float RandomGaussianFloat(const float flMean, const float flStdDev)
{
	return s_GaussianStream.RandomFloat(flMean, flStdDev);
}


//-----------------------------------------------------------------------------
//
// Implementation of the uniform random number stream
//
//-----------------------------------------------------------------------------
CUniformRandomStream::CUniformRandomStream()
{
	SetSeed(0);
}

void CUniformRandomStream::SetSeed(const int iSeed)
{
	AUTO_LOCK( m_mutex );
	m_idum = iSeed;
}

int CUniformRandomStream::GetSeed() const
{
	return m_idum;
}

int CUniformRandomStream::GenerateRandomNumber()
{
	AUTO_LOCK( m_mutex );

	if (m_idum <= 0)
	{
		if (-(m_idum) < 1)
			m_idum = 1;
		else
			m_idum = -(m_idum);
	}
	const int k = (m_idum) / IQ;
	m_idum = IA * (m_idum - k * IQ) - IR * k;
	if (m_idum < 0)
		m_idum += IM;

	return m_idum;
}

float CUniformRandomStream::RandomFloat(const float flLow, const float flHigh)
{
	// float in [0,1)
	const float fl = Min(AM * GenerateRandomNumber(), RNMX);
	return (fl * (flHigh - flLow)) + flLow; // float in [low,high)
}

float CUniformRandomStream::RandomFloatExp(const float flMinVal, const float flMaxVal, const float flExponent)
{
	// float in [0,1)
	float fl = Min(AM * GenerateRandomNumber(), RNMX);
	if (flExponent != 1.0f)
	{
		fl = powf(fl, flExponent);
	}
	return (fl * (flMaxVal - flMinVal)) + flMinVal; // float in [low,high)
}

int CUniformRandomStream::RandomInt(const int iLow, const int iHigh)
{
	Assert(lLow <= lHigh);

	unsigned int maxAcceptable;
	unsigned int x = iHigh - iLow + 1;
	unsigned int n;
	if (x <= 1 || MAX_RANDOM_RANGE < x - 1)
	{
		return iLow;
	}

	// The following maps a uniform distribution on the interval [0,MAX_RANDOM_RANGE]
	// to a smaller, client-specified range of [0,x-1] in a way that doesn't bias
	// the uniform distribution unfavorably. Even for a worst case x, the loop is
	// guaranteed to be taken no more than half the time, so for that worst case x,
	// the average number of times through the loop is 2. For cases where x is
	// much smaller than MAX_RANDOM_RANGE, the average number of times through the
	// loop is very close to 1.
	//
	maxAcceptable = MAX_RANDOM_RANGE - ((MAX_RANDOM_RANGE + 1) % x);
	do
	{
		n = GenerateRandomNumber();
	} while (n > maxAcceptable);

	return iLow + (n % x);
}

int CUniformRandomStream::RandomShortMax()
{
	return RandomInt(0, MAX_RANDOM_RANGE_SHORT);
}


//-----------------------------------------------------------------------------
//
// Implementation of the gaussian random number stream
// We're gonna use the Box-Muller method (which actually generates 2
// gaussian-distributed numbers at once)
//
//-----------------------------------------------------------------------------
CGaussianRandomStream::CGaussianRandomStream(IUniformRandomStream* const pUniformStream)
{
	AttachToStream(pUniformStream);
}


//-----------------------------------------------------------------------------
// Attaches to a random uniform stream
//-----------------------------------------------------------------------------
void CGaussianRandomStream::AttachToStream(IUniformRandomStream* const pUniformStream)
{
	AUTO_LOCK( m_mutex );

	m_pUniformStream = pUniformStream;
	m_bHaveValue = false;
}


//-----------------------------------------------------------------------------
// Generates random numbers
//-----------------------------------------------------------------------------
float CGaussianRandomStream::RandomFloat(const float flMean, const float flStdDev)
{
	AUTO_LOCK( m_mutex );

	IUniformRandomStream* pUniformStream = m_pUniformStream ? m_pUniformStream : s_pUniformStream;
	float fac, rsq, v1, v2;

	if (!m_bHaveValue)
	{
		// Pick 2 random #s from -1 to 1
		// Make sure they lie inside the unit circle. If they don't, try again
		do
		{
			v1 = 2.0f * pUniformStream->RandomFloat() - 1.0f;
			v2 = 2.0f * pUniformStream->RandomFloat() - 1.0f;
			rsq = v1 * v1 + v2 * v2;
		} while ((rsq > 1.0f) || (rsq == 0.0f));

		// The box-muller transformation to get the two gaussian numbers
		fac = sqrtf(-2.0f * log(rsq) / rsq);

		// Store off one value for later use
		m_flRandomValue = v1 * fac;
		m_bHaveValue = true;

		return flStdDev * (v2 * fac) + flMean;
	}
	else
	{
		m_bHaveValue = false;
		return flStdDev * m_flRandomValue + flMean;
	}
}


//-----------------------------------------------------------------------------
// Creates a histogram (for testing)
//-----------------------------------------------------------------------------
