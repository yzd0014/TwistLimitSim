// Includes
//=========

#include "../Time.h"

#include <Engine/Asserts/Asserts.h>
#include <Engine/Logging/Logging.h>
#include <Engine/Windows/Includes.h>
#include <Engine/Windows/Functions.h>

// Static Data Initialization
//===========================

namespace
{
	uint64_t s_tickCountPerSecond = 0;
}

namespace sca2025
{
	namespace Time
	{
		uint64_t m_tickCountPerSecond = 1;
	}
}

// Interface
//==========

// Time
//-----

uint64_t sca2025::Time::GetCurrentSystemTimeTickCount()
{
	LARGE_INTEGER totalTickCountSinceSystemBoot;
	const auto result = QueryPerformanceCounter( &totalTickCountSinceSystemBoot );
	// Microsoft claims that querying the counter will never fail on Windows XP or later:
	// https://msdn.microsoft.com/en-us/library/windows/desktop/dn553408(v=vs.85).aspx
	EAE6320_ASSERTF( result != FALSE, "QueryPerformanceCounter() failed" );
	return static_cast<uint64_t>( totalTickCountSinceSystemBoot.QuadPart );
}

double sca2025::Time::ConvertTicksToSeconds( const uint64_t i_tickCount )
{
	EAE6320_ASSERT( s_tickCountPerSecond > 0 );
	return static_cast<double>( i_tickCount ) / static_cast<double>( s_tickCountPerSecond );
}

uint64_t sca2025::Time::ConvertSecondsToTicks( const double i_secondCount )
{
	EAE6320_ASSERT( s_tickCountPerSecond > 0 );
	return static_cast<uint64_t>( ( i_secondCount * static_cast<double>( s_tickCountPerSecond ) ) + 0.5 );
}

double sca2025::Time::ConvertRatePerSecondToRatePerTick( const double i_rate_perSecond )
{
	EAE6320_ASSERT( s_tickCountPerSecond > 0 );
	return i_rate_perSecond / static_cast<double>( s_tickCountPerSecond );
}

// Initialization / Clean Up
//--------------------------

sca2025::cResult sca2025::Time::Initialize()
{
	auto result = Results::Success;

	// Get the frequency of the high-resolution performance counter
	{
		// Microsoft claims that querying the frequency will never fail on Windows XP or later:
		// https://msdn.microsoft.com/en-us/library/windows/desktop/dn553408(v=vs.85).aspx
		LARGE_INTEGER tickCountPerSecond;
		if ( QueryPerformanceFrequency( &tickCountPerSecond ) != FALSE )
		{
			if ( tickCountPerSecond.QuadPart != 0 )
			{
				s_tickCountPerSecond = static_cast<uint64_t>( tickCountPerSecond.QuadPart );
				m_tickCountPerSecond = s_tickCountPerSecond;
			}
			else
			{
				result = Results::Failure;
				EAE6320_ASSERT( false );
				Logging::OutputMessage( "This hardware doesn't support high resolution performance counters!" );
				goto OnExit;
			}
		}
		else
		{
			result = Results::Failure;
			const auto errorMessage = Windows::GetLastSystemError();
			EAE6320_ASSERTF( false, errorMessage.c_str() );
			Logging::OutputMessage( "Windows failed to query performance frequency: %s", errorMessage.c_str() );
			goto OnExit;
		}
	}

	Logging::OutputMessage( "Initialized time" );

OnExit:

	return result;
}

sca2025::cResult sca2025::Time::CleanUp()
{
	return Results::Success;
}
