// Includes
//=========

#include "OpenGl.h"

#include "Functions.h"

#include <Engine/Asserts/Asserts.h>
#include <sstream>

// Static Data Initialization
//===========================

namespace
{
	constexpr wchar_t* const s_hiddenWindowClass_name = L"EAE6320 Hidden OpenGL Context Window Class";
}

// Interface
//==========

sca2025::cResult sca2025::Windows::OpenGl::CreateHiddenContextWindow( HINSTANCE& io_applicationInstance, sHiddenWindowInfo& o_info, std::string* const o_errorMessage )
{
	auto result = Results::Success;

	// Create the hidden window
	{
		// Get the instance of the running program if it wasn't provided
		if ( io_applicationInstance == NULL )
		{
			constexpr DWORD flags =
				// Interpret the "module name" parameter as an address in the module
				GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS
				// Don't increment the modules reference count
				| GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT
				;
			static constexpr wchar_t* const hackyWayOfGettingAnAddressInsideThisModule = L"";
			if ( GetModuleHandleExW( flags, hackyWayOfGettingAnAddressInsideThisModule, &io_applicationInstance ) == FALSE )
			{
				const auto windowsErrorMessage = GetLastSystemError();
				result = Results::Failure;
				EAE6320_ASSERTF( false, "Couldn't get the current instance handle: %s", windowsErrorMessage.c_str() );
				if ( o_errorMessage )
				{
					std::ostringstream errorMessage;
					errorMessage << "Windows failed to get the current instance handle: " << windowsErrorMessage;
					*o_errorMessage = errorMessage.str();
				}
				goto OnExit;
			}
		}
		// Register the hidden window's class
		{
			WNDCLASSEXW wndClassEx{};
			{
				wndClassEx.cbSize = sizeof( WNDCLASSEX );
				wndClassEx.lpfnWndProc = DefWindowProc;
				wndClassEx.hInstance = io_applicationInstance;
				wndClassEx.lpszClassName = s_hiddenWindowClass_name;
			}
			o_info.windowClass = RegisterClassExW( &wndClassEx );
			if ( !o_info.windowClass )
			{
				const auto windowsErrorMessage = GetLastSystemError();
				result = Results::Failure;
				EAE6320_ASSERTF( false, "Couldn't register the hidden OpenGL context window's class: %s", windowsErrorMessage.c_str() );
				if ( o_errorMessage )
				{
					std::ostringstream errorMessage;
					errorMessage << "Windows failed to register the hidden OpenGL context window's class: " << windowsErrorMessage;
					*o_errorMessage = errorMessage.str();
				}
				goto OnExit;
			}
		}
		// Create the hidden window
		{
			constexpr wchar_t* const windowName = L"EAE6320 Hidden OpenGL Context Window";
			constexpr DWORD windowStyle = WS_POPUP
				| WS_MINIMIZE;	// Just in case
			constexpr DWORD windowStyle_extended = 0;
			constexpr int position = CW_USEDEFAULT;
			constexpr int dimension = CW_USEDEFAULT;
			constexpr HWND parent = NULL;
			constexpr HMENU menu = NULL;
			constexpr void* const userData = nullptr;
			o_info.window = CreateWindowExW( windowStyle_extended, s_hiddenWindowClass_name, windowName, windowStyle,
				position, position, dimension, dimension,
				parent, menu, io_applicationInstance, userData );
			if ( !o_info.window )
			{
				result = Results::Failure;
				EAE6320_ASSERTF( false, "Couldn't create the hidden OpenGL context window" );
				if ( o_errorMessage )
				{
					const std::string windowsErrorMessage = GetLastSystemError();
					std::ostringstream errorMessage;
					errorMessage << "Windows failed to create the hidden OpenGL context window: " << windowsErrorMessage;
					*o_errorMessage = errorMessage.str();
				}
				goto OnExit;
			}
		}
	}
	// Get the device context
	{
		o_info.deviceContext = GetDC( o_info.window );
		if ( !o_info.deviceContext )
		{
			result = Results::Failure;
			EAE6320_ASSERTF( false, "Couldn't get the hidden OpenGL device context" );
			if ( o_errorMessage )
			{
				*o_errorMessage = "Windows failed to get the hidden OpenGL device context";
			}
			goto OnExit;
		}
	}
	// Set the pixel format for the window
	{
		PIXELFORMATDESCRIPTOR desiredPixelFormat{};
		{
			desiredPixelFormat.nSize = sizeof( PIXELFORMATDESCRIPTOR );
			desiredPixelFormat.nVersion = 1;

			desiredPixelFormat.dwFlags = PFD_SUPPORT_OPENGL
				| PFD_DEPTH_DONTCARE | PFD_DOUBLEBUFFER_DONTCARE | PFD_STEREO_DONTCARE;
			desiredPixelFormat.iPixelType = PFD_TYPE_RGBA;
			desiredPixelFormat.cColorBits = 24;
			desiredPixelFormat.iLayerType = PFD_MAIN_PLANE;
		}
		// Get the ID of the desired pixel format
		int pixelFormatId;
		{
			pixelFormatId = ChoosePixelFormat( o_info.deviceContext, &desiredPixelFormat );
			if ( pixelFormatId == 0 )
			{
				const auto windowsErrorMessage = GetLastSystemError();
				result = Results::Failure;
				EAE6320_ASSERTF( false, "Couldn't choose the closest pixel format for the hidden OpenGL context window: %s", windowsErrorMessage.c_str() );
				if ( o_errorMessage )
				{
					std::ostringstream errorMessage;
					errorMessage << "Windows couldn't choose the closest pixel format"
						" for the hidden OpenGL context window: "
						<< windowsErrorMessage;
					*o_errorMessage = errorMessage.str();
				}
				goto OnExit;
			}
		}
		// Set it
		if ( SetPixelFormat( o_info.deviceContext, pixelFormatId, &desiredPixelFormat ) == FALSE )
		{
			const auto windowsErrorMessage = GetLastSystemError();
			result = Results::Failure;
			EAE6320_ASSERTF( false, "Couldn't set the desired pixel format (%i) for the hidden OpenGL context window: %s",
				pixelFormatId, windowsErrorMessage.c_str() );
			if ( o_errorMessage )
			{
				std::ostringstream errorMessage;
				errorMessage << "Windows couldn't set the desired pixel format " << pixelFormatId
					<< " for the hidden OpenGL context window: "
					<< windowsErrorMessage;
				*o_errorMessage = errorMessage.str();
			}
			goto OnExit;
		}
	}
	// Create the rendering context
	o_info.openGlRenderingContext = wglCreateContext( o_info.deviceContext );
	if ( !o_info.openGlRenderingContext )
	{
		const auto windowsErrorMessage = GetLastSystemError();
		result = Results::Failure;
		EAE6320_ASSERTF( false, "Couldn't create the hidden OpenGL rendering context: %s", windowsErrorMessage.c_str() );
		if ( o_errorMessage )
		{
			std::ostringstream errorMessage;
			errorMessage << "Windows failed to create a hidden OpenGL rendering context: "
				<< windowsErrorMessage;
			*o_errorMessage = errorMessage.str();
		}
		goto OnExit;
	}
	// Set it as the rendering context of this thread
	if ( wglMakeCurrent( o_info.deviceContext, o_info.openGlRenderingContext ) == FALSE )
	{
		const auto windowsErrorMessage = GetLastSystemError();
		result = Results::Failure;
		EAE6320_ASSERTF( false, "Couldn't set the current OpenGL rendering context for the hidden window: %s", windowsErrorMessage.c_str() );
		if ( o_errorMessage )
		{
			std::ostringstream errorMessage;
			errorMessage << "Windows failed to set the current OpenGL rendering context"
				" for the hidden window: "
				<< windowsErrorMessage;
			*o_errorMessage = errorMessage.str();
		}
		goto OnExit;
	}

OnExit:

	return result;
}

sca2025::cResult sca2025::Windows::OpenGl::FreeHiddenContextWindow( const HINSTANCE& i_applicationInstance, sHiddenWindowInfo& io_info, std::string* const o_errorMessage )
{
	auto result = Results::Success;

	if ( io_info.openGlRenderingContext )
	{
		EAE6320_ASSERTF( io_info.deviceContext, "If a rendering context exists then a device context should also" );
		if ( wglGetCurrentContext() == io_info.openGlRenderingContext )
		{
			if ( wglMakeCurrent( io_info.deviceContext, NULL ) == FALSE )
			{
				const auto windowsErrorMessage = GetLastSystemError();
				if ( result )
				{
					result = Results::Failure;
				}
				EAE6320_ASSERTF( false, "Couldn't unset the current hidden OpenGL device context: %s", windowsErrorMessage.c_str() );
				if ( o_errorMessage )
				{
					std::ostringstream errorMessage;
					errorMessage << "\nWindows failed to unset the current hidden OpenGL device context: " << windowsErrorMessage;
					*o_errorMessage += errorMessage.str();
				}
			}
		}
		if ( wglDeleteContext( io_info.openGlRenderingContext ) == FALSE )
		{
			const auto windowsErrorMessage = GetLastSystemError();
			if ( result )
			{
				result = Results::Failure;
			}
			EAE6320_ASSERTF( false, "Couldn't delete the hidden OpenGL rendering context: %s", windowsErrorMessage.c_str() );
			if ( o_errorMessage )
			{
				std::ostringstream errorMessage;
				errorMessage << "\nWindows failed to delete the hidden OpenGL rendering context: " << windowsErrorMessage;
				*o_errorMessage += errorMessage.str();
			}
		}
		io_info.openGlRenderingContext = NULL;
	}
	if ( io_info.deviceContext )
	{
		if ( io_info.window )
		{
			ReleaseDC( io_info.window, io_info.deviceContext );
			io_info.deviceContext = NULL;
		}
		else
		{
			EAE6320_ASSERTF( false, "A window handle is required to release a device context" );
		}
	}
	if ( io_info.window )
	{
		EAE6320_ASSERTF( io_info.windowClass, "If a window was created then a window class should also be provided" );
		if ( DestroyWindow( io_info.window ) == FALSE )
		{
			const auto windowsErrorMessage = GetLastSystemError();
			if ( result )
			{
				result = Results::Failure;
			}
			EAE6320_ASSERTF( false, "Couldn't destroy the hidden OpenGL context window: %s", windowsErrorMessage.c_str() );
			if ( o_errorMessage )
			{
				std::ostringstream errorMessage;
				errorMessage << "\nWindows failed to destroy the hidden OpenGL context window: " << windowsErrorMessage;
				*o_errorMessage += errorMessage.str();
			}			
		}
		io_info.window = NULL;
	}
	if ( io_info.windowClass )
	{
		EAE6320_ASSERTF( i_applicationInstance, "The provided instance handle is NULL" );
		if ( UnregisterClass( s_hiddenWindowClass_name, i_applicationInstance ) == FALSE )
		{
			const auto windowsErrorMessage = GetLastSystemError();
			if ( result )
			{
				result = Results::Failure;
			}
			EAE6320_ASSERTF( false, "Couldn't unregister the hidden OpenGL context window's class: %s", windowsErrorMessage.c_str() );
			if ( o_errorMessage )
			{
				std::ostringstream errorMessage;
				errorMessage << "\nWindows failed to unregister the hidden OpenGL context window's class \""
					<< io_info.windowClass << "\": " << windowsErrorMessage;
				*o_errorMessage += errorMessage.str();
			}
		}
		io_info.windowClass = NULL;
	}

	return result;
}
