// Includes
//=========

#include "../cRenderState.h"

#include "Includes.h"
#include "../sContext.h"

#include <Engine/Asserts/Asserts.h>
#include <Engine/Logging/Logging.h>

// Interface
//==========

// Render
//-------

void sca2025::Graphics::cRenderState::Bind() const
{
	auto* const direct3dImmediateContext = sContext::g_context.direct3dImmediateContext;
	EAE6320_ASSERT( direct3dImmediateContext );

	// Alpha Transparency
	{
		EAE6320_ASSERT( m_blendState );
		const float* const noBlendFactor = NULL;
		const unsigned int defaultSampleMask = 0xffffffff;
		direct3dImmediateContext->OMSetBlendState( m_blendState, noBlendFactor, defaultSampleMask );
	}
	// Depth Buffering
	{
		EAE6320_ASSERT( m_depthStencilState );
		const unsigned int unusedStencilReference = 0;
		direct3dImmediateContext->OMSetDepthStencilState( m_depthStencilState, unusedStencilReference );
	}
	// Draw Both Triangle Sides
	{
		EAE6320_ASSERT( m_rasterizerState );
		direct3dImmediateContext->RSSetState( m_rasterizerState );
	}
}

// Initialization / Clean Up
//--------------------------

sca2025::cResult sca2025::Graphics::cRenderState::CleanUp()
{
	auto result = Results::Success;

	if ( m_blendState )
	{
		m_blendState->Release();
		m_blendState = nullptr;
	}
	if ( m_depthStencilState )
	{
		m_depthStencilState->Release();
		m_depthStencilState = nullptr;
	}
	if ( m_rasterizerState )
	{
		m_rasterizerState->Release();
		m_rasterizerState = nullptr;
	}

	return result;
}

// Implementation
//===============

// Initialization / Clean Up
//--------------------------

sca2025::cResult sca2025::Graphics::cRenderState::InitializeFromBits()
{
	auto result = Results::Success;

	auto* const direct3dDevice = sContext::g_context.direct3dDevice;

	// Alpha Transparency
	{
		D3D11_BLEND_DESC blendStateDescription{};
		// Alpha-to-coverage isn't used
		blendStateDescription.AlphaToCoverageEnable = FALSE;
		// All render targets use the same blending
		// (in our class we will only ever use a single render target)
		blendStateDescription.IndependentBlendEnable = FALSE;
		{
			D3D11_RENDER_TARGET_BLEND_DESC& renderTargetBlendDescription = blendStateDescription.RenderTarget[0];
			if ( IsAlphaTransparencyEnabled() )
			{
				renderTargetBlendDescription.BlendEnable = TRUE;

				// result = ( source * source.a ) + ( destination * ( 1 - source.a ) )
				renderTargetBlendDescription.SrcBlend = D3D11_BLEND_SRC_ALPHA;
				renderTargetBlendDescription.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
				renderTargetBlendDescription.BlendOp = D3D11_BLEND_OP_ADD;
				renderTargetBlendDescription.SrcBlendAlpha = D3D11_BLEND_ONE;
				renderTargetBlendDescription.DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
				renderTargetBlendDescription.BlendOpAlpha = D3D11_BLEND_OP_ADD;
			}
			else
			{
				renderTargetBlendDescription.BlendEnable = FALSE;

				// (Since blending is disabled the remaining states don't matter,
				// but it doesn't hurt to set them to explicitly disable alpha transparency)
				renderTargetBlendDescription.SrcBlend = D3D11_BLEND_ONE;
				renderTargetBlendDescription.DestBlend = D3D11_BLEND_ZERO;
				renderTargetBlendDescription.BlendOp = D3D11_BLEND_OP_ADD;
				renderTargetBlendDescription.SrcBlendAlpha = D3D11_BLEND_ONE;
				renderTargetBlendDescription.DestBlendAlpha = D3D11_BLEND_ZERO;
				renderTargetBlendDescription.BlendOpAlpha = D3D11_BLEND_OP_ADD;
			}
			renderTargetBlendDescription.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		}
		const auto d3dResult = direct3dDevice->CreateBlendState( &blendStateDescription, &m_blendState );
		if ( FAILED( d3dResult ) )
		{
			result = Results::Failure;
			EAE6320_ASSERTF( false, "CreateBlendState() failed (HRESULT %#010x)", d3dResult );
			sca2025::Logging::OutputError( "Direct3D failed to create a blend render state object from %u with HRESULT %#010x", m_bits, d3dResult );
			goto OnExit;
		}
	}
	// Depth Buffering
	{
		D3D11_DEPTH_STENCIL_DESC depthStateDescription{};
		// We don't use the stencil buffer in our class
		depthStateDescription.StencilEnable = FALSE;
		depthStateDescription.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
		depthStateDescription.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
		depthStateDescription.FrontFace.StencilFunc = depthStateDescription.BackFace.StencilFunc =
			D3D11_COMPARISON_ALWAYS;
		depthStateDescription.FrontFace.StencilDepthFailOp = depthStateDescription.BackFace.StencilDepthFailOp =
			depthStateDescription.FrontFace.StencilPassOp = depthStateDescription.BackFace.StencilPassOp =
			depthStateDescription.FrontFace.StencilFailOp = depthStateDescription.BackFace.StencilFailOp =
			D3D11_STENCIL_OP_KEEP;
		if ( IsDepthBufferingEnabled() )
		{
			// The new fragment becomes a pixel if its depth is less than what has previously been written
			depthStateDescription.DepthEnable = TRUE;
			depthStateDescription.DepthFunc = D3D11_COMPARISON_LESS;
			// Write to the depth buffer
			depthStateDescription.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		}
		else
		{
			// Don't test the depth buffer
			depthStateDescription.DepthEnable = FALSE;
			depthStateDescription.DepthFunc = D3D11_COMPARISON_ALWAYS;
			// Don't write to the depth buffer
			depthStateDescription.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
		}
		const auto d3dResult = direct3dDevice->CreateDepthStencilState( &depthStateDescription, &m_depthStencilState );
		if ( FAILED( d3dResult ) )
		{
			result = Results::Failure;
			EAE6320_ASSERTF( false, "CreateDepthStencilState() failed (HRESULT %#010x)", d3dResult );
			sca2025::Logging::OutputError( "Direct3D failed to create a depth/stencil render state object from %u with HRESULT %#010x", m_bits, d3dResult );
			goto OnExit;
		}
	}
	// Draw Both Triangle Sides
	{
		D3D11_RASTERIZER_DESC rasterizerStateDescription{};
		// Draw solid geometry (i.e. not wireframe)
		rasterizerStateDescription.FillMode = D3D11_FILL_SOLID;
		// Triangles use left-handed winding order
		// (opposite from OpenGL)
		rasterizerStateDescription.FrontCounterClockwise = FALSE;
		// No depth bias
		rasterizerStateDescription.DepthBias = 0;
		rasterizerStateDescription.SlopeScaledDepthBias = 0.0f;
		rasterizerStateDescription.DepthBiasClamp = 0.0f;
		// Use default depth clipping
		rasterizerStateDescription.DepthClipEnable = TRUE;
		// Disable "scissoring" (i.e. render all pixels in the render target)
		rasterizerStateDescription.ScissorEnable = FALSE;
		// Disable multisampling/antialiasing
		rasterizerStateDescription.MultisampleEnable = FALSE;
		rasterizerStateDescription.AntialiasedLineEnable = FALSE;
		if ( ShouldBothTriangleSidesBeDrawn() )
		{
			// Don't cull any triangles
			rasterizerStateDescription.CullMode = D3D11_CULL_NONE;
		}
		else
		{
			// Cull triangles that are facing backwards
			rasterizerStateDescription.CullMode = D3D11_CULL_BACK;
		}
		const auto d3dResult = direct3dDevice->CreateRasterizerState( &rasterizerStateDescription, &m_rasterizerState );
		if ( FAILED( d3dResult ) )
		{
			result = Results::Failure;
			EAE6320_ASSERTF( false, "CreateRasterizerState() failed (HRESULT %#010x)", d3dResult );
			sca2025::Logging::OutputError( "Direct3D failed to create a rasterizer render state object from %u with HRESULT %#010x", m_bits, d3dResult );
			goto OnExit;
		}
	}

OnExit:

	return result;
}
