// Includes
//=========

#include "../cConstantBuffer.h"

#include "Includes.h"
#include "../cShader.h"
#include "../sContext.h"

#include <Engine/Asserts/Asserts.h>
#include <Engine/Logging/Logging.h>
#include <Engine/Math/Functions.h>

// Interface
//==========

// Render
//-------

void sca2025::Graphics::cConstantBuffer::Bind( const uint_fast8_t i_shaderTypesToBindTo ) const
{
	auto* const direct3dImmediateContext = sContext::g_context.direct3dImmediateContext;
	EAE6320_ASSERT( direct3dImmediateContext );

	EAE6320_ASSERT( m_buffer );

	constexpr unsigned int bufferCount = 1;
	if ( i_shaderTypesToBindTo & ShaderTypes::Vertex )
	{
		direct3dImmediateContext->VSSetConstantBuffers( static_cast<unsigned int>( m_type ), bufferCount, &m_buffer );
	}
	if ( i_shaderTypesToBindTo & ShaderTypes::Fragment )
	{
		direct3dImmediateContext->PSSetConstantBuffers( static_cast<unsigned int>( m_type ), bufferCount, &m_buffer );
	}
}

void sca2025::Graphics::cConstantBuffer::Update( const void* const i_data )
{
	auto* const direct3dImmediateContext = sContext::g_context.direct3dImmediateContext;
	EAE6320_ASSERT( direct3dImmediateContext );

	EAE6320_ASSERT( m_buffer );

	auto mustConstantBufferBeUnmapped = false;

	// Get a pointer from Direct3D that can be written to
	void* memoryToWriteTo;
	{
		D3D11_MAPPED_SUBRESOURCE mappedSubResource;
		{
			// Discard previous contents when writing
			constexpr unsigned int noSubResources = 0;
			constexpr D3D11_MAP mapType = D3D11_MAP_WRITE_DISCARD;
			constexpr unsigned int noFlags = 0;
			const auto d3dResult = direct3dImmediateContext->Map( m_buffer, noSubResources, mapType, noFlags, &mappedSubResource );
			if ( SUCCEEDED( d3dResult ) )
			{
				mustConstantBufferBeUnmapped = true;
			}
			else
			{
				EAE6320_ASSERT( false );
				Logging::OutputError( "Direct3D failed to map a constant buffer" );
				goto OnExit;
			}
		}
		memoryToWriteTo = mappedSubResource.pData;
	}
	// Copy the new data to the memory that Direct3D has provided
	memcpy( memoryToWriteTo, i_data, m_size );

OnExit:

	if ( mustConstantBufferBeUnmapped )
	{
		// Let Direct3D know that the memory contains the data
		// (the pointer will be invalid after this call)
		constexpr unsigned int noSubResources = 0;
		direct3dImmediateContext->Unmap( m_buffer, noSubResources );
	}
}

// Initialization / Clean Up
//--------------------------

sca2025::cResult sca2025::Graphics::cConstantBuffer::CleanUp()
{
	auto result = Results::Success;

	if ( m_buffer )
	{
		m_buffer->Release();
		m_buffer = nullptr;
	}

	return result;
}

// Implementation
//===============

// Initialization / Clean Up
//--------------------------

sca2025::cResult sca2025::Graphics::cConstantBuffer::Initialize_platformSpecific( const void* const i_initialData )
{
	auto* const direct3dDevice = sContext::g_context.direct3dDevice;
	EAE6320_ASSERT( direct3dDevice );

	D3D11_BUFFER_DESC bufferDescription{};
	{
		EAE6320_ASSERTF( m_size < ( uint64_t( 1u ) << ( sizeof( unsigned int ) * 8 ) ),
			"The constant buffer format's size (%u) is too large to fit into a D3D11_BUFFER_DESC", m_size );
		// The byte width must be rounded up to a multiple of 16
		bufferDescription.ByteWidth = Math::RoundUpToMultiple_powerOf2( static_cast<unsigned int>( m_size ), 16u );
		bufferDescription.Usage = D3D11_USAGE_DYNAMIC;	// The CPU must be able to update the buffer
		bufferDescription.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bufferDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;	// The CPU must write, but doesn't read
		bufferDescription.MiscFlags = 0;
		bufferDescription.StructureByteStride = 0;	// Not used
	}
	D3D11_SUBRESOURCE_DATA initialData{};
	{
		initialData.pSysMem = i_initialData;
		// (The other data members are ignored for non-texture buffers)
	}

	const auto d3dResult = direct3dDevice->CreateBuffer( &bufferDescription,
		i_initialData ? &initialData : nullptr,
		&m_buffer );
	if ( SUCCEEDED( d3dResult ) )
	{
		return Results::Success;
	}
	else
	{
		EAE6320_ASSERTF( false, "Couldn't create constant buffer creation (HRESULT %#010x)", d3dResult );
		sca2025::Logging::OutputError( "Direct3D failed to create a constant buffer with HRESULT %#010x", d3dResult );
		return Results::Failure;
	}
}
