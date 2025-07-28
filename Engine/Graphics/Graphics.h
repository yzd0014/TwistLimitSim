/*
	This file declares the external interface for the graphics system
*/

#ifndef EAE6320_GRAPHICS_H
#define EAE6320_GRAPHICS_H

// Includes
//=========

#include "Configuration.h"

#include <cstdint>
#include <Engine/Results/Results.h>

#if defined( EAE6320_PLATFORM_WINDOWS )
	#include <Engine/Windows/Includes.h>
#endif
#include "Engine/Math/cMatrix_transformation.h"
#include "Effect.h"
#include "Mesh.h"
// Interface
//==========

namespace sca2025
{
	
	namespace GameCommon {
		class GameObject;
	}
	namespace Graphics
	{
		// Submission
		//-----------

		// These functions should be called from the application (on the application loop thread)

		// As the class progresses you will add your own functions for submitting data,
		// but the following is an example (that gets called automatically)
		// of how the application submits the total elapsed times
		// for the frame currently being submitted
		void SubmitElapsedTime( const float i_elapsedSecondCount_systemTime, const float i_elapsedSecondCount_simulationTime );
		void SubmitBGColor(const float *i_color);
		void SubmitObject(Math::sVector i_color, Math::cMatrix_transformation &i_localToWorldMat, Effect *i_pEffect, Mesh * i_pMesh);
		void SubmitCamera(Math::cMatrix_transformation &i_transform_worldToCamera, Math::cMatrix_transformation &i_transform_cameraToProjected);

		// When the application is ready to submit data for a new frame
		// it should call this before submitting anything
		// (or, said another way, it is not safe to submit data for a new frame
		// until this function returns successfully)
		cResult WaitUntilDataForANewFrameCanBeSubmitted( const unsigned int i_timeToWait_inMilliseconds );
		// When the application has finished submitting data for a frame
		// it must call this function
		cResult SignalThatAllDataForAFrameHasBeenSubmitted();

		// Render
		//-------

		// This is called (automatically) from the main/render thread.
		// It will render a submitted frame as soon as it is ready
		// (i.e. as soon as SignalThatAllDataForAFrameHasBeenSubmitted() has been called)
		void RenderFrame();

		// Initialization / Clean Up
		//--------------------------

		struct sInitializationParameters
		{
#if defined( EAE6320_PLATFORM_WINDOWS )
			HWND mainWindow = NULL;
	#if defined( EAE6320_PLATFORM_D3D )
			uint16_t resolutionWidth, resolutionHeight;
	#elif defined( EAE6320_PLATFORM_GL )
			HINSTANCE thisInstanceOfTheApplication = NULL;
	#endif
#endif
		};

		cResult Initialize( const sInitializationParameters& i_initializationParameters );
		cResult CleanUp();

		void UpdateSimulationBasedOnInput();
		void ClearDataBeingSubmittedByApplicationThread();

		extern Concurrency::cMutex renderBufferMutex;
		extern bool renderThreadNoWait;
		extern bool isDataSubmittedFromApplication;
	}
}

#endif	// EAE6320_GRAPHICS_H
