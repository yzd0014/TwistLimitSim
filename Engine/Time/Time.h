/*
	This file manages time-related functionality
*/

#ifndef EAE6320_TIME_H
#define EAE6320_TIME_H

// Includes
//=========

#include <cstdint>
#include <Engine/Results/Results.h>

// Interface
//==========

namespace sca2025
{
	namespace Time
	{
		// Time
		//-----

		uint64_t GetCurrentSystemTimeTickCount();

		double ConvertTicksToSeconds( const uint64_t i_tickCount );
		uint64_t ConvertSecondsToTicks( const double i_secondCount );
		double ConvertRatePerSecondToRatePerTick( const double i_rate_perSecond );

		// Initialization / Clean Up
		//--------------------------

		cResult Initialize();
		cResult CleanUp();

		extern uint64_t tickCount_systemTime_elapsed;
		extern uint64_t tickCount_elapsedSinceLastLoop;
		extern uint64_t tickCount_previousLoop;

		extern uint64_t m_tickCountPerSecond;
	}
}

#endif	// EAE6320_TIME_H
