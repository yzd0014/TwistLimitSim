// Includes
//=========

#include "Logging.h"

#include <cstdarg>
#include <cstdio>
#include <Engine/Asserts/Asserts.h>
#include <fstream>
#include <sstream>
#include <string>

// Helper Class Declaration
//=========================

namespace
{
	class cLogging
	{
		// Interface
		//----------

	public:

		// Logging
		sca2025::cResult OutputMessage( const std::string& i_message );
		void FlushLog();
		// Initialization / Clean Up
		sca2025::cResult InitializeIfNecesary();
		static void CleanUp();
		~cLogging();

		// Data
		//-----

	private:

		std::ofstream m_outputStream;
	};
}

namespace sca2025
{
	namespace Logging
	{
		std::fstream logStream;
		bool fileOpened = false;
	}
}

// Static Data Initialization
//===========================

namespace
{
	cLogging s_logger;
	auto s_hasTheLoggerBeenDestroyed = false;
	auto s_hasTheLogFileAlreadyBeenWrittenTo = false;
	auto s_cleanUpResult = sca2025::Results::Failure;
}

// Helper Function Declarations
//=============================

namespace
{
	sca2025::cResult OutputMessage( const char* const i_message, va_list io_insertions );
}

// Interface
//==========

// Output
//-------

sca2025::cResult sca2025::Logging::OutputMessage( const char* const i_message, ... )
{
	cResult result;
	{
		va_list insertions;
		va_start( insertions, i_message );
		result = ::OutputMessage( i_message, insertions );
		va_end( insertions );
	}
#ifdef EAE6320_LOGGING_FLUSHBUFFERAFTEREVERYMESSAGE
	s_logger.FlushLog();
#endif
	return result;
}

sca2025::cResult sca2025::Logging::OutputError( const char* const i_errorMessage, ... )
{
	cResult result;
	{
		va_list insertions;
		va_start( insertions, i_errorMessage );
		result = ::OutputMessage( i_errorMessage, insertions );
		va_end( insertions );
	}
	s_logger.FlushLog();
	return result;
}

// Initialization / Clean Up
//--------------------------

sca2025::cResult sca2025::Logging::Initialize()
{
	return s_logger.InitializeIfNecesary();
}

sca2025::cResult sca2025::Logging::CleanUp()
{
	cLogging::CleanUp();
	if (fileOpened)
	{
		logStream.close();
	}
	return s_cleanUpResult;
}

// Helper Class Definition
//========================

namespace
{
	// Interface
	//----------

	// Logging

	sca2025::cResult cLogging::OutputMessage( const std::string& i_message )
	{
		if ( InitializeIfNecesary() )
		{
			// Write the message to the file
			m_outputStream << i_message << "\n";

			return sca2025::Results::Success;
		}
		else
		{
			return sca2025::Results::Failure;
		}
	}

	inline void cLogging::FlushLog()
	{
		m_outputStream.flush();
	}

	// Initialization / Clean UP

	sca2025::cResult cLogging::InitializeIfNecesary()
	{
		// If there is an error when the application is exiting
		// the logger may have already been destroyed
		if ( !s_hasTheLoggerBeenDestroyed )
		{
			if ( m_outputStream.is_open() )
			{
				return sca2025::Results::Success;
			}
			else
			{
				if ( !s_hasTheLogFileAlreadyBeenWrittenTo )
				{
					EAE6320_ASSERTF( strlen( EAE6320_LOGGING_PATH ) > 0, "The path to log to is empty" );
					m_outputStream.open( EAE6320_LOGGING_PATH );
					if ( m_outputStream.is_open() )
					{
						s_hasTheLogFileAlreadyBeenWrittenTo = true;
						sca2025::Logging::OutputMessage( "Opened log file \"%s\"", EAE6320_LOGGING_PATH );
						FlushLog();
						return sca2025::Results::Success;
					}
					else
					{
						return sca2025::Results::Failure;
					}
				}
				else
				{
					m_outputStream.open( EAE6320_LOGGING_PATH, std::ofstream::app );
					const auto result = m_outputStream.is_open() ? sca2025::Results::Success : sca2025::Results::Failure;
					EAE6320_ASSERT( result );
					if ( result )
					{
						sca2025::Logging::OutputMessage( "Re-opened log file" );
						FlushLog();
					}
					return result;
				}
			}
		}
		else
		{
			// If the logger has already been destroyed it needs to be re-constructed in the same memory location
			{
				new ( this ) cLogging;
				// Register the CleanUp() function so that it still gets called when the application exits
				// (the destructor won't be called automatically when using placement new)
				{
					auto result = atexit( CleanUp );
					if ( result == 0 )
					{
						s_hasTheLoggerBeenDestroyed = false;
					}
					else
					{
						EAE6320_ASSERTF( false, "Calling atexit() to register logging clean up on a revived logger failed with a return value of %i", result );
						CleanUp();
						return sca2025::Results::Failure;
					}
				}
			}
			// Re-open the file
			m_outputStream.open( EAE6320_LOGGING_PATH, std::ofstream::app );
			const auto result = m_outputStream.is_open() ? sca2025::Results::Success : sca2025::Results::Failure;
			EAE6320_ASSERT( result );
			if ( result )
			{
				sca2025::Logging::OutputMessage( "Re-opened log file after it had been destroyed" );
				FlushLog();
			}
			return result;
		}
	}

	void cLogging::CleanUp()
	{
		if ( s_logger.m_outputStream.is_open() )
		{
			{
				sca2025::Logging::OutputMessage( "Closing log file" );
				s_logger.FlushLog();
			}
			s_logger.m_outputStream.close();
			s_cleanUpResult = !s_logger.m_outputStream.is_open() ? sca2025::Results::Success : sca2025::Results::Failure;
			EAE6320_ASSERTF( s_cleanUpResult, "Log file wasn't closed" );
			if ( !s_cleanUpResult )
			{
				sca2025::Logging::OutputError( "Error: Log file did not close" );
			}
			return;
		}

		s_cleanUpResult = sca2025::Results::Success;
	}

	cLogging::~cLogging()
	{
		s_hasTheLoggerBeenDestroyed = true;
		CleanUp();
	}
}

// Helper Function Definitions
//============================

namespace
{
	sca2025::cResult OutputMessage( const char* const i_message, va_list io_insertions )
	{
		constexpr size_t bufferSize = 512;
		char buffer[bufferSize];
		const auto formattingResult = vsnprintf( buffer, bufferSize, i_message, io_insertions );
		if ( formattingResult >= 0 )
		{
			if ( formattingResult < bufferSize )
			{
				return s_logger.OutputMessage( buffer );
			}
			else
			{
				EAE6320_ASSERTF( false, "The internal logging buffer of size %u was not big enough to hold the formatted message of length %i",
					bufferSize, formattingResult + 1 );
				std::ostringstream errorMessage;
				errorMessage << "FORMATTING ERROR! (The internal logging buffer of size " << bufferSize
					<< " was not big enough to hold the formatted message of length " << ( formattingResult + 1 ) << ".)"
					" Cut-off message is:\n\t" << buffer;
				s_logger.OutputMessage( errorMessage.str().c_str() );
				// Return failure regardless of whether the unformatted message was output
				return sca2025::Results::Failure;
			}
		}
		else
		{
			EAE6320_ASSERTF( false, "An encoding error occurred while logging the message \"%s\"", i_message );
			std::ostringstream errorMessage;
			errorMessage << "ENCODING ERROR! Unformatted message was:\n\t" << i_message;
			s_logger.OutputMessage( errorMessage.str().c_str() );
			// Return failure regardless of whether the unformatted message was output
			return sca2025::Results::Failure;
		}
	}
}
