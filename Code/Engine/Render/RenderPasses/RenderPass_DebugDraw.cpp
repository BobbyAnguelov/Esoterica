#include "RenderPass_DebugDraw.h"
#include "Engine/Render/RenderSystem.h"
#include "Engine/Render/DebugMesh/DebugMeshRegistry.h"
#include "Engine/Render/RenderViewport.h"
#include "Engine/Render/Device/DeviceRenderWorld.h"
#include "Base/Types/StringID.h"
#include "Base/Profiling.h"
#include "Base/Memory/Memory.h"
#include "Base/Render/RHI.h"
#include "Base/Render/Settings/Settings_Render.h"
#include "Base/Math/ViewVolume.h"
#include "Base/Fonts/Font_RobotoMono_Regular.h"
#include "Base/ThirdParty/stb/stb_truetype.h"

// Shaders
#include "Engine/Render/Shaders/Debug/DebugDrawResolve.esf"
#include "Engine/Render/Shaders/Debug/DebugDraw.esf"
#include "Engine/Render/Shaders/Debug/DebugDrawMesh.esf"
#include "Engine/Render/Shaders/OIT/OITResolve.esf"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS

using namespace EE::DebugDrawInternal;

//-------------------------------------------------------------------------

namespace EE::Render
{
    // TODO: Make it configurable!
    static uint32_t const s_gDebugFontResolution = 256;

    static ShaderTypes::DebugDrawCommand_Raw* WriteTexts(
        ShaderTypes::DebugDrawCommand_Raw*          pDstCommand_WriteCombined,
        ShaderTypes::DebugDrawCommand_Raw const*    pDstCommandEnd_WriteCombined,
        TArray<TVector<uint8_t>, 2>&                fontCaches,
        TArray<RHI::TextureHandle, 2>               fontTextureHandles,
        Float2                                      fontPixelOffset,
        Float2                                      fontPixelSize,
        Float4                                      fontModulateColor,
        Viewport const&                             viewport,
        bool                                        writeBackground,
        TArrayView<TextCommand const>               textCommands )
    {
        EE_PROFILE_FUNCTION_RENDER();

        Math::ViewVolume const& viewVolume = viewport.GetViewVolume();

        Matrix reverseZ( Vector( 1.0f, 0.0f, 0.0f, 0.0f ),
                         Vector( 0.0f, 1.0f, 0.0f, 0.0f ),
                         Vector( 0.0f, 0.0f, -1.0f, 0.0f ),
                         Vector( 0.0f, 0.0f, 1.0f, 1.0f ) );

        Matrix viewProjectionMatrix = viewVolume.GetViewMatrix() * ( viewVolume.GetProjectionMatrix() * reverseZ );

        for ( TextCommand const& textCommand : textCommands )
        {
            uint32_t textColor = Color( textCommand.m_color.ToFloat4() * fontModulateColor );

            stbtt_bakedchar*   pBakedChars = reinterpret_cast<stbtt_bakedchar*>( fontCaches[(uint8_t) textCommand.m_font].data() );
            RHI::TextureHandle fontTextureHandle = fontTextureHandles[(uint8_t) textCommand.m_font];

            // Measure the whole text
            float measureX = 0.0F;
            float measureY = 0.0F;

            float currentX = 0.0F;
            float currentY = 0.0F;

            float measureLineY = 0.0F;
            for ( uint8_t letter : textCommand.m_text )
            {
                if ( letter == '\n' )
                {
                    stbtt_aligned_quad alignedQuad = {};
                    stbtt_GetBakedQuad( pBakedChars, s_gDebugFontResolution, s_gDebugFontResolution, 'A' - 32, &currentX, &currentY, &alignedQuad, 1 );

                    measureX = Math::Max( measureX, currentX );

                    currentX = 0;
                    measureY += alignedQuad.y0 + alignedQuad.y1;

                    measureLineY = Math::Min( measureLineY, alignedQuad.y0 );
                }
                else
                {
                    stbtt_aligned_quad alignedQuad = {};
                    stbtt_GetBakedQuad( pBakedChars, s_gDebugFontResolution, s_gDebugFontResolution, letter - 32, &currentX, &currentY, &alignedQuad, 1 );

                    measureX = Math::Max( measureX, currentX );
                    measureY = Math::Min( measureY, alignedQuad.y0 + alignedQuad.y1 );

                    measureLineY = Math::Min( measureLineY, alignedQuad.y0 );
                }
            }

            float initialScreenX = 0.0F;
            float initialScreenY = 0.0F;
            switch ( textCommand.m_alignment )
            {
                case DebugTextAlign::TopLeft:
                {
                    initialScreenX = fontPixelOffset.m_x;
                    initialScreenY = fontPixelOffset.m_y;
                }
                break;

                case DebugTextAlign::TopCenter:
                {
                    initialScreenX = fontPixelOffset.m_x - measureX * 0.5F;
                    initialScreenY = fontPixelOffset.m_y;
                }
                break;

                case DebugTextAlign::TopRight:
                {
                    initialScreenX = fontPixelOffset.m_x - measureX;
                    initialScreenY = fontPixelOffset.m_y;
                }
                break;

                case DebugTextAlign::MiddleLeft:
                {
                    initialScreenX = fontPixelOffset.m_x;
                    initialScreenY = fontPixelOffset.m_y + measureY * 0.5F;
                }
                break;

                case DebugTextAlign::MiddleCenter:
                {
                    initialScreenX = fontPixelOffset.m_x - measureX * 0.5F;
                    initialScreenY = fontPixelOffset.m_y + measureY * 0.5F;
                }
                break;

                case DebugTextAlign::MiddleRight:
                {
                    initialScreenX = fontPixelOffset.m_x - measureX;
                    initialScreenY = fontPixelOffset.m_y + measureY * 0.5F;
                }
                break;

                case DebugTextAlign::BottomLeft:
                {
                    initialScreenX = fontPixelOffset.m_x;
                    initialScreenY = fontPixelOffset.m_y + measureY;
                }
                break;

                case DebugTextAlign::BottomCenter:
                {
                    initialScreenX = fontPixelOffset.m_x - measureX * 0.5F;
                    initialScreenY = fontPixelOffset.m_y + measureY;
                }
                break;

                case DebugTextAlign::BottomRight:
                {
                    initialScreenX = fontPixelOffset.m_x - measureX;
                    initialScreenY = fontPixelOffset.m_y + measureY;
                }
                break;
            };

            float screenX = initialScreenX;
            float screenY = initialScreenY;

            bool isCulled = false;

            Vector textPosition = textCommand.m_position;
            if ( !textCommand.m_isScreenText )
            {
                Vector pointScreenSpace = viewProjectionMatrix.TransformVector3( textPosition );

                isCulled = pointScreenSpace.GetW() < 0.0F;

                pointScreenSpace /= pointScreenSpace.GetSplatW();
                pointScreenSpace += Vector::One;
                pointScreenSpace *= 0.5F;

                textPosition = pointScreenSpace * viewport.GetDimensions();
            }
            else
            {
                textPosition.SetY( viewport.GetDimensions().m_y - textPosition.GetY() );
            }

            for ( uint8_t letter : textCommand.m_text )
            {
                // HACK: Generate empty quad for EOL character and move 1 char down.
                if ( letter == '\n' )
                {
                    stbtt_aligned_quad alignedQuad = {};
                    stbtt_GetBakedQuad( pBakedChars, s_gDebugFontResolution, s_gDebugFontResolution, 'A' - 32, &screenX, &screenY, &alignedQuad, 1 );

                    screenX = initialScreenX;
                    screenY -= ( alignedQuad.y0 - alignedQuad.y1 ) * 1.25F;

                    ShaderTypes::DebugDrawTextLetterCommand textLetterCommand = {};
                    textLetterCommand.m_commandFlags = DEBUG_DRAW_COMMAND_FLAG_SKIP;

                    *reinterpret_cast<ShaderTypes::DebugDrawTextLetterCommand*>( pDstCommand_WriteCombined ) = textLetterCommand;
                    pDstCommand_WriteCombined++;

                    continue;
                }

                // TODO: We are "culling" text commands right now by generating empty quads, need to rework this to actually cull geometry.
                if ( isCulled )
                {
                    ShaderTypes::DebugDrawTextLetterCommand textLetterCommand = {};
                    textLetterCommand.m_commandFlags = DEBUG_DRAW_COMMAND_FLAG_SKIP;

                    *reinterpret_cast<ShaderTypes::DebugDrawTextLetterCommand*>( pDstCommand_WriteCombined ) = textLetterCommand;
                    pDstCommand_WriteCombined++;

                    continue;
                }

                EE_ASSERT( letter >= 32 && letter < 128 );
                if ( letter >= 32 && letter < 128 )
                {
                    stbtt_aligned_quad alignedQuad = {};
                    stbtt_GetBakedQuad( pBakedChars, s_gDebugFontResolution, s_gDebugFontResolution, letter - 32, &screenX, &screenY, &alignedQuad, 1 );

                    ShaderTypes::DebugDrawTextLetterCommand textLetterCommand = {};
                    textLetterCommand.m_commandFlags = DEBUG_DRAW_COMMAND_FLAG_TEXT_LETTER;
                    textLetterCommand.m_alignedQuadTopLeft[0] = alignedQuad.x0;
                    textLetterCommand.m_alignedQuadTopLeft[1] = alignedQuad.y0;
                    textLetterCommand.m_alignedQuadTopLeft[2] = alignedQuad.s0;
                    textLetterCommand.m_alignedQuadTopLeft[3] = alignedQuad.t0;

                    textLetterCommand.m_alignedQuadBottomRight[0] = alignedQuad.x1;
                    textLetterCommand.m_alignedQuadBottomRight[1] = alignedQuad.y1;
                    textLetterCommand.m_alignedQuadBottomRight[2] = alignedQuad.s1;
                    textLetterCommand.m_alignedQuadBottomRight[3] = alignedQuad.t1;

                    textLetterCommand.m_textColor = textColor;
                    textPosition.StoreFloat3( textLetterCommand.m_textPosition );
                    textLetterCommand.m_fontTextureHandle = fontTextureHandle;

                    *reinterpret_cast<ShaderTypes::DebugDrawTextLetterCommand*>( pDstCommand_WriteCombined ) = textLetterCommand;
                    pDstCommand_WriteCombined++;
                }
            }
        }

        return pDstCommand_WriteCombined;
    }

    static ShaderTypes::DebugDrawCommand_Raw* WriteTextsOutline(
        ShaderTypes::DebugDrawCommand_Raw*          pDstCommand_WriteCombined,
        ShaderTypes::DebugDrawCommand_Raw const*    pDstCommandEnd_WriteCombined,
        TArray<TVector<uint8_t>, 2>&                fontCaches,
        TArray<RHI::TextureHandle, 2>               fontTextureHandles,
        Float2                                      fontPixelSize,
        Viewport const&                             viewport,
        TArrayView<TextCommand const>      textCommands )
    {
        EE_PROFILE_FUNCTION_RENDER();

        pDstCommand_WriteCombined = WriteTexts( pDstCommand_WriteCombined,
                                                pDstCommandEnd_WriteCombined,
                                                fontCaches,
                                                fontTextureHandles,
                                                Float2( 1.0F, 1.0F ),
                                                fontPixelSize,
                                                Float4( 0.0F, 0.0F, 0.0F, 1.0F ),
                                                viewport,
                                                true,
                                                textCommands );
        pDstCommand_WriteCombined = WriteTexts( pDstCommand_WriteCombined,
                                                pDstCommandEnd_WriteCombined,
                                                fontCaches,
                                                fontTextureHandles,
                                                Float2::Zero,
                                                fontPixelSize,
                                                Float4::One,
                                                viewport,
                                                false,
                                                textCommands );
        return pDstCommand_WriteCombined;
    }

    static ShaderTypes::DebugDrawCommand_Raw* WriteTextBoxes(
        ShaderTypes::DebugDrawCommand_Raw*          pDstCommand_WriteCombined,
        ShaderTypes::DebugDrawCommand_Raw const*    pDstCommandEnd_WriteCombined,
        TArray<TVector<uint8_t>, 2>&                fontCaches,
        Float2                                      fontPixelSize,
        Viewport const&                             viewport,
        TArrayView<TextCommand const>      textCommands )
    {
        EE_PROFILE_FUNCTION_RENDER();

        Math::ViewVolume const& viewVolume = viewport.GetViewVolume();

        Matrix reverseZ( Vector( 1.0f, 0.0f, 0.0f, 0.0f ),
                         Vector( 0.0f, 1.0f, 0.0f, 0.0f ),
                         Vector( 0.0f, 0.0f, -1.0f, 0.0f ),
                         Vector( 0.0f, 0.0f, 1.0f, 1.0f ) );

        Matrix viewProjectionMatrix = viewVolume.GetViewMatrix() * ( viewVolume.GetProjectionMatrix() * reverseZ );

        for ( TextCommand const& textCommand : textCommands )
        {
            if ( !textCommand.m_hasBackground )
            {
                continue;
            }

            stbtt_bakedchar* pBakedChars = reinterpret_cast<stbtt_bakedchar*>( fontCaches[(uint8_t) textCommand.m_font].data() );

            // Measure the whole text
            float measureX = 0.0F;
            float measureY = 0.0F;

            float currentX = 0.0F;
            float currentY = 0.0F;

            float measureLineY = 0.0F;
            for ( uint8_t letter : textCommand.m_text )
            {
                if ( letter == '\n' )
                {
                    stbtt_aligned_quad alignedQuad = {};
                    stbtt_GetBakedQuad( pBakedChars, s_gDebugFontResolution, s_gDebugFontResolution, 'A' - 32, &currentX, &currentY, &alignedQuad, 1 );

                    measureX = Math::Max( measureX, currentX );

                    currentX = 0;
                    measureY += alignedQuad.y0 + alignedQuad.y1;

                    measureLineY = Math::Min( measureLineY, alignedQuad.y0 );
                }
                else
                {
                    stbtt_aligned_quad alignedQuad = {};
                    stbtt_GetBakedQuad( pBakedChars, s_gDebugFontResolution, s_gDebugFontResolution, letter - 32, &currentX, &currentY, &alignedQuad, 1 );

                    measureX = Math::Max( measureX, currentX );
                    measureY = Math::Min( measureY, alignedQuad.y0 + alignedQuad.y1 );

                    measureLineY = Math::Min( measureLineY, alignedQuad.y0 );
                }
            }

            Float2 fontPixelOffset = Float2::Zero;

            float initialScreenX = 0.0F;
            float initialScreenY = 0.0F;
            switch ( textCommand.m_alignment )
            {
                case DebugTextAlign::TopLeft:
                {
                    initialScreenX = fontPixelOffset.m_x;
                    initialScreenY = fontPixelOffset.m_y;
                }
                break;

                case DebugTextAlign::TopCenter:
                {
                    initialScreenX = fontPixelOffset.m_x - measureX * 0.5F;
                    initialScreenY = fontPixelOffset.m_y;
                }
                break;

                case DebugTextAlign::TopRight:
                {
                    initialScreenX = fontPixelOffset.m_x - measureX;
                    initialScreenY = fontPixelOffset.m_y;
                }
                break;

                case DebugTextAlign::MiddleLeft:
                {
                    initialScreenX = fontPixelOffset.m_x;
                    initialScreenY = fontPixelOffset.m_y + measureY * 0.5F;
                }
                break;

                case DebugTextAlign::MiddleCenter:
                {
                    initialScreenX = fontPixelOffset.m_x - measureX * 0.5F;
                    initialScreenY = fontPixelOffset.m_y + measureY * 0.5F;
                }
                break;

                case DebugTextAlign::MiddleRight:
                {
                    initialScreenX = fontPixelOffset.m_x - measureX;
                    initialScreenY = fontPixelOffset.m_y + measureY * 0.5F;
                }
                break;

                case DebugTextAlign::BottomLeft:
                {
                    initialScreenX = fontPixelOffset.m_x;
                    initialScreenY = fontPixelOffset.m_y + measureY;
                }
                break;

                case DebugTextAlign::BottomCenter:
                {
                    initialScreenX = fontPixelOffset.m_x - measureX * 0.5F;
                    initialScreenY = fontPixelOffset.m_y + measureY;
                }
                break;

                case DebugTextAlign::BottomRight:
                {
                    initialScreenX = fontPixelOffset.m_x - measureX;
                    initialScreenY = fontPixelOffset.m_y + measureY;
                }
                break;
            };

            float screenX = initialScreenX;
            float screenY = initialScreenY;

            Vector textPosition = textCommand.m_position;
            if ( !textCommand.m_isScreenText )
            {
                Vector pointScreenSpace = viewProjectionMatrix.TransformVector3( textPosition );
                pointScreenSpace /= pointScreenSpace.GetSplatW();
                pointScreenSpace += Vector::One;
                pointScreenSpace *= 0.5F;

                textPosition = pointScreenSpace * viewport.GetDimensions();
            }
            else
            {
                textPosition.SetY( viewport.GetDimensions().m_y - textPosition.GetY() );
            }

            ShaderTypes::DebugDrawTextBoxCommand textBoxCommand = {};
            textBoxCommand.m_commandFlags = DEBUG_DRAW_COMMAND_FLAG_TEXT_BOX;

            textBoxCommand.m_quadCoordinates[0] = screenX - 4.0F;
            textBoxCommand.m_quadCoordinates[1] = screenX + measureX + 4.0F;
            textBoxCommand.m_quadCoordinates[2] = screenY + measureLineY - 4.0F;
            textBoxCommand.m_quadCoordinates[3] = screenY - measureY;

            textBoxCommand.m_textColor = Color( Float4( 0.2F, 0.2F, 0.2F, 0.6F ) );
            textPosition.StoreFloat3( textBoxCommand.m_textPosition );

            *reinterpret_cast<ShaderTypes::DebugDrawTextBoxCommand*>( pDstCommand_WriteCombined ) = textBoxCommand;
            pDstCommand_WriteCombined++;
        }

        return pDstCommand_WriteCombined;
    }

    //-------------------------------------------------------------------------------------------------------

    static_assert( sizeof( PointCommand ) == sizeof( ShaderTypes::DebugDrawPointCommand ) );
    static_assert( sizeof( LineCommand ) == sizeof( ShaderTypes::DebugDrawLineCommand ) );
    static_assert( sizeof( TriangleCommand ) == sizeof( ShaderTypes::DebugDrawTriangleCommand ) );

    static_assert( sizeof( PointCommand ) == sizeof( ShaderTypes::DebugDrawCommand_Raw ) );
    static_assert( sizeof( LineCommand ) == sizeof( ShaderTypes::DebugDrawCommand_Raw ) );
    static_assert( sizeof( TriangleCommand ) == sizeof( ShaderTypes::DebugDrawCommand_Raw ) );

    static_assert( ( sizeof( PointCommand ) % 16 ) == 0 );
    static_assert( ( sizeof( LineCommand ) % 16 ) == 0 );
    static_assert( ( sizeof( TriangleCommand ) % 16 ) == 0 );

    //-------------------------------------------------------------------------------------------------------

    void DebugDrawRenderPass::Initialize( RenderPassContext const& context )
    {
        EE_ASSERT( m_pDebugMeshRegistry == nullptr );

        m_pRenderSettings = context.m_pRenderSettings;

        RHI::Context* pContextRHI = context.m_pRenderSystem->GetContextRHI();

        static StringID const s_DebugDrawShaderID( "DebugDraw" );
        static StringID const s_DebugDrawCullingID( "DebugDrawCulling" );
        static StringID const s_DebugDrawResolveID( "DebugDrawResolve" );
        static StringID const s_DebugDrawMeshID( "DebugDrawMesh" );

        m_pDebugDrawShader = context.m_pRenderSystem->FindSurfaceShader( s_DebugDrawShaderID );
        m_pDebugDrawMeshShader = context.m_pRenderSystem->FindSurfaceShader( s_DebugDrawMeshID );
        m_pDebugDrawResolveShader = context.m_pRenderSystem->FindComputeShader( s_DebugDrawResolveID );

        RHI::DataFormat pipelineColorFormats[] = { RHI::DataFormat::RGBA8_sRGB };

        RHI::DepthStencilState depthStencilStateDepthOn = {};
        depthStencilStateDepthOn.m_depthTest = true;
        depthStencilStateDepthOn.m_depthWrite = true;
        depthStencilStateDepthOn.m_depthCompareMode = RHI::CompareMode::GreaterEqual;

        RHI::DepthStencilState depthStencilStateDepthOnNoWrite = depthStencilStateDepthOn;
        depthStencilStateDepthOnNoWrite.m_depthWrite = false;

        RHI::DepthStencilState depthStencilStateDepthOff = {};
        depthStencilStateDepthOff.m_depthCompareMode = RHI::CompareMode::Always;

        RHI::DepthStencilState depthStencilStateDepthEquals = {};
        depthStencilStateDepthEquals.m_depthTest = true;
        depthStencilStateDepthEquals.m_depthWrite = false;
        depthStencilStateDepthEquals.m_depthCompareMode = RHI::CompareMode::Equal;

        RHI::MeshPipelineParameters opaqueDepthOnPipelineParameters = {};
        opaqueDepthOnPipelineParameters.m_colorFormats = pipelineColorFormats;
        opaqueDepthOnPipelineParameters.m_numRenderTargets = 1;
        opaqueDepthOnPipelineParameters.m_depthStencilFormat = RHI::DataFormat::D32_SFloat;
        opaqueDepthOnPipelineParameters.m_depthStencilState = depthStencilStateDepthOn;
        opaqueDepthOnPipelineParameters.m_rasterizerState.m_depthClip = true;
        opaqueDepthOnPipelineParameters.m_rasterizerState.m_scissor = true;
        opaqueDepthOnPipelineParameters.m_rasterizerState.m_cullMode = RHI::CullMode::None;
        opaqueDepthOnPipelineParameters.m_debugName = "RenderPass_DebugDraw Pipeline Opaque DepthOn";
        opaqueDepthOnPipelineParameters.m_pRootSignature = m_pDebugDrawShader->m_pRootSignature;
        opaqueDepthOnPipelineParameters.m_pShader = m_pDebugDrawShader->m_pShader;

        RHI::MeshPipelineParameters transparentDepthOnDepthPipelineParameters = opaqueDepthOnPipelineParameters;
        transparentDepthOnDepthPipelineParameters.m_blendState.m_writeMasks[0] = 0;
        transparentDepthOnDepthPipelineParameters.m_debugName = "RenderPass_DebugDraw Pipeline Transparent DepthOn_Depth";

        RHI::MeshPipelineParameters transparentDepthOnColorPipelineParameters = opaqueDepthOnPipelineParameters;
        transparentDepthOnColorPipelineParameters.m_depthStencilState = depthStencilStateDepthEquals;
        transparentDepthOnColorPipelineParameters.m_blendState.m_srcFactors[0] = RHI::BlendConstant::SrcAlpha;
        transparentDepthOnColorPipelineParameters.m_blendState.m_srcAlphaFactors[0] = RHI::BlendConstant::SrcAlpha;
        transparentDepthOnColorPipelineParameters.m_blendState.m_dstFactors[0] = RHI::BlendConstant::OneMinusSrcAlpha;
        transparentDepthOnColorPipelineParameters.m_blendState.m_dstAlphaFactors[0] = RHI::BlendConstant::OneMinusSrcAlpha;
        transparentDepthOnColorPipelineParameters.m_blendState.m_blendModes[0] = RHI::BlendMode::Add;
        transparentDepthOnColorPipelineParameters.m_blendState.m_blendModesAlpha[0] = RHI::BlendMode::Add;
        transparentDepthOnColorPipelineParameters.m_blendState.m_blendEnabled = true;
        transparentDepthOnColorPipelineParameters.m_debugName = "RenderPass_DebugDraw Pipeline Transparent DepthOn_Color";

        RHI::MeshPipelineParameters transparentDepthOnColorNoWritePipelineParameters = transparentDepthOnColorPipelineParameters;
        transparentDepthOnColorNoWritePipelineParameters.m_depthStencilState = depthStencilStateDepthOnNoWrite;

        RHI::MeshPipelineParameters transparentDepthOffPipelineParameters = transparentDepthOnColorPipelineParameters;
        transparentDepthOffPipelineParameters.m_depthStencilState = depthStencilStateDepthOff;
        transparentDepthOffPipelineParameters.m_debugName = "RenderPass_DebugDraw Pipeline Transparent DepthOff";

        m_pTransparentDepthOnDepthPipeline = RHI::CreatePipeline( pContextRHI, transparentDepthOnDepthPipelineParameters );
        m_pTransparentDepthOnColorPipeline = RHI::CreatePipeline( pContextRHI, transparentDepthOnColorPipelineParameters );
        m_pTransparentDepthOnNoWriteColorPipeline = RHI::CreatePipeline( pContextRHI, transparentDepthOnColorNoWritePipelineParameters );
        m_pTransparentDepthOffPipeline = RHI::CreatePipeline( pContextRHI, transparentDepthOffPipelineParameters );

        //---------------------------------------------------------------------------------------------------

        RHI::MeshPipelineParameters transparentDepthOnDepthPipelineParameters_Mesh = transparentDepthOnDepthPipelineParameters;
        transparentDepthOnDepthPipelineParameters_Mesh.m_pRootSignature = m_pDebugDrawMeshShader->m_pRootSignature;
        transparentDepthOnDepthPipelineParameters_Mesh.m_pShader = m_pDebugDrawMeshShader->m_pShader;

        RHI::MeshPipelineParameters transparentDepthOnColorPipelineParameters_Mesh = transparentDepthOnColorPipelineParameters;
        transparentDepthOnColorPipelineParameters_Mesh.m_pRootSignature = m_pDebugDrawMeshShader->m_pRootSignature;
        transparentDepthOnColorPipelineParameters_Mesh.m_pShader = m_pDebugDrawMeshShader->m_pShader;

        RHI::MeshPipelineParameters transparentDepthOnColorNoWritePipelineParameters_Mesh = transparentDepthOnColorNoWritePipelineParameters;
        transparentDepthOnColorNoWritePipelineParameters_Mesh.m_pRootSignature = m_pDebugDrawMeshShader->m_pRootSignature;
        transparentDepthOnColorNoWritePipelineParameters_Mesh.m_pShader = m_pDebugDrawMeshShader->m_pShader;

        RHI::MeshPipelineParameters transparentDepthOffPipelineParameters_Mesh = transparentDepthOffPipelineParameters;
        transparentDepthOffPipelineParameters_Mesh.m_pRootSignature = m_pDebugDrawMeshShader->m_pRootSignature;
        transparentDepthOffPipelineParameters_Mesh.m_pShader = m_pDebugDrawMeshShader->m_pShader;

        m_pTransparentDepthOnDepthPipeline_Mesh = RHI::CreatePipeline( pContextRHI, transparentDepthOnDepthPipelineParameters_Mesh );
        m_pTransparentDepthOnColorPipeline_Mesh = RHI::CreatePipeline( pContextRHI, transparentDepthOnColorPipelineParameters_Mesh );
        m_pTransparentDepthOnNoWriteColorPipeline_Mesh = RHI::CreatePipeline( pContextRHI, transparentDepthOnColorNoWritePipelineParameters_Mesh );
        m_pTransparentDepthOffPipeline_Mesh = RHI::CreatePipeline( pContextRHI, transparentDepthOffPipelineParameters_Mesh );

        //---------------------------------------------------------------------------------------------------

        TArray<TInlineString<256>, NUM_DEPTH_TEST_BUCKETS> argumentBufferNames =
        {
            "RenderPass_DebugDraw DebugArgumentBuffer Transparent DepthOn NoWrite",
            "RenderPass_DebugDraw DebugArgumentBuffer Transparent DepthOn Write",
            "RenderPass_DebugDraw DebugArgumentBuffer Transparent DepthSeparate Write",
        };

        TArray<TInlineString<256>, NUM_DEPTH_TEST_BUCKETS> counterBufferNames =
        {
            "RenderPass_DebugDraw DebugCounterBuffer Transparent DepthOn NoWrite",
            "RenderPass_DebugDraw DebugCounterBuffer Transparent DepthOn Write",
            "RenderPass_DebugDraw DebugCounterBuffer Transparent DepthSeparate Write",
        };

        TArray<TInlineString<256>, NUM_DEPTH_TEST_BUCKETS> commandsBufferNames =
        {
            "RenderPass_DebugDraw DebugCommandsBuffer Transparent DepthOn NoWrite",
            "RenderPass_DebugDraw DebugCommandsBuffer Transparent DepthOn Write",
            "RenderPass_DebugDraw DebugCommandsBuffer Transparent DepthSeparate Write",
        };

        TArray<uint32_t, NUM_DEPTH_TEST_BUCKETS> pickingSortPriorities =
        {
            400,
            600,
            200
        };

        for ( size_t bucketIndex = 0; bucketIndex < m_depthBuckets.size(); ++bucketIndex )
        {
            DepthTestBucket& depthBucket = m_depthBuckets[bucketIndex];
            depthBucket.m_pickingSortPriority = pickingSortPriorities[bucketIndex];

            if ( !depthBucket.m_argumentBuffer )
            {
                RHI::BufferParameters debugArgumentBufferParameters = {};
                debugArgumentBufferParameters.m_bufferSize = sizeof( ShaderTypes::DebugDrawArgument );
                debugArgumentBufferParameters.m_bufferStride = sizeof( ShaderTypes::DebugDrawArgument );
                debugArgumentBufferParameters.m_descriptorTypes.SetMultipleFlags( RHI::DescriptorTypeFlags::IndirectArgumentBuffer, RHI::DescriptorTypeFlags::RWBuffer );
                debugArgumentBufferParameters.m_debugName = argumentBufferNames[bucketIndex];

                depthBucket.m_argumentBuffer = RHI::CreateBuffer( pContextRHI, debugArgumentBufferParameters );

                depthBucket.m_commandsBuffer.Initialize( pContextRHI, commandsBufferNames[bucketIndex] );
            }
        }
    }

    void DebugDrawRenderPass::Shutdown( RenderSystem* pRenderSystem )
    {
        EE_ASSERT( m_pDebugMeshRegistry == nullptr );

        RHI::DestroyPipeline( pRenderSystem->GetContextRHI(), eastl::move( m_pTransparentDepthOnDepthPipeline ) );
        RHI::DestroyPipeline( pRenderSystem->GetContextRHI(), eastl::move( m_pTransparentDepthOnColorPipeline ) );
        RHI::DestroyPipeline( pRenderSystem->GetContextRHI(), eastl::move( m_pTransparentDepthOnNoWriteColorPipeline ) );
        RHI::DestroyPipeline( pRenderSystem->GetContextRHI(), eastl::move( m_pTransparentDepthOffPipeline ) );

        RHI::DestroyPipeline( pRenderSystem->GetContextRHI(), eastl::move( m_pTransparentDepthOnDepthPipeline_Mesh ) );
        RHI::DestroyPipeline( pRenderSystem->GetContextRHI(), eastl::move( m_pTransparentDepthOnColorPipeline_Mesh ) );
        RHI::DestroyPipeline( pRenderSystem->GetContextRHI(), eastl::move( m_pTransparentDepthOnNoWriteColorPipeline_Mesh ) );
        RHI::DestroyPipeline( pRenderSystem->GetContextRHI(), eastl::move( m_pTransparentDepthOffPipeline_Mesh ) );

        //---------------------------------------------------------------------------------------------------

        m_frameCommandBuffer.Clear();

        RHI::DestroyTexture( pRenderSystem->GetContextRHI(), eastl::move( m_fontCacheTextures[0] ) );
        RHI::DestroyTexture( pRenderSystem->GetContextRHI(), eastl::move( m_fontCacheTextures[1] ) );

        for ( DepthTestBucket& depthBucket : m_depthBuckets )
        {
            RHI::DestroyBuffer( pRenderSystem->GetContextRHI(), eastl::move( depthBucket.m_argumentBuffer ) );

            depthBucket.m_commandsBuffer.Shutdown( pRenderSystem->GetContextRHI() );
        }

        m_fontCaches[0].clear();
        m_fontCaches[1].clear();
    }

    void DebugDrawRenderPass::UpdateDeviceResources( RenderSystem* pRenderSystem )
    {
        EE_PROFILE_FUNCTION_RENDER();

        uint32_t            frameIndex = pRenderSystem->GetFrameIndex();
        RHI::Context*       pContextRHI = pRenderSystem->GetContextRHI();

        // Fonts
        //---------------------------------------------------------------------------------------------------

        if ( !m_fontCacheTextures[0] || !m_fontCacheTextures[1] )
        {
            TVector<uint8_t> fontBitmapData;
            fontBitmapData.resize( s_gDebugFontResolution * s_gDebugFontResolution );

            // Regular font
            //---------------------------------------------------------------------------------------------------

            Blob const fontBlob = Embed::Font_RobotoMono_Regular::GetFileData();

            m_fontCaches[0].clear();
            m_fontCaches[0].resize( 96 * sizeof( stbtt_bakedchar ) );

            int bakeResult = stbtt_BakeFontBitmap( fontBlob.data(), 0, 20,
                                                   fontBitmapData.data(), s_gDebugFontResolution, s_gDebugFontResolution,
                                                   32, 96,
                                                   reinterpret_cast<stbtt_bakedchar*>( m_fontCaches[0].data() ) );
            EE_ASSERT( bakeResult != -1 );

            RHI::TextureParameters fontTextureParameters = {};
            fontTextureParameters.m_width = s_gDebugFontResolution;
            fontTextureParameters.m_height = s_gDebugFontResolution;
            fontTextureParameters.m_format = RHI::DataFormat::R8_UNorm;
            fontTextureParameters.m_debugName = "RenderPass_DebugDraw FontCache Normal";

            auto CopyTextureMemory0 = [&fontBitmapData] ( uint8_t* pDstMemory_WriteCombined, size_t srcOffset, uint32_t rowStride, uint32_t row )
            {
                Memory::CopyToWriteCombined( pDstMemory_WriteCombined, fontBitmapData.data() + srcOffset, rowStride );
            };
            m_fontCacheTextures[0] = pRenderSystem->QueueTextureCreate( CopyTextureMemory0, fontTextureParameters );

            // Small font
            //-------------------------------------------------------------------------

            m_fontCaches[1].clear();
            m_fontCaches[1].resize( 96 * sizeof( stbtt_bakedchar ) );

            bakeResult = stbtt_BakeFontBitmap( fontBlob.data(), 0, 16,
                                               fontBitmapData.data(), s_gDebugFontResolution, s_gDebugFontResolution,
                                               32, 96,
                                               reinterpret_cast<stbtt_bakedchar*>( m_fontCaches[1].data() ) );
            EE_ASSERT( bakeResult != -1 );

            fontTextureParameters.m_debugName = "RenderPass_DebugDraw FontCache Small";

            auto CopyTextureMemory1 = [&fontBitmapData] ( uint8_t* pDstMemory_WriteCombined, size_t srcOffset, uint32_t rowStride, uint32_t row )
            {
                Memory::CopyToWriteCombined( pDstMemory_WriteCombined, fontBitmapData.data() + srcOffset, rowStride );
            };
            m_fontCacheTextures[1] = pRenderSystem->QueueTextureCreate( CopyTextureMemory1, fontTextureParameters );

            fontBitmapData = {};
        }

        m_pDebugMeshRegistry->UpdateDeviceResources();
    }

    void DebugDrawRenderPass::UpdateViewportDeviceResources( RenderSystem* pRenderSystem,
                                                             DeviceRenderWorld const& deviceRenderWorld,
                                                             DebugDrawSystem* pDebugDrawingSystem, Seconds const deltaTime,
                                                             RenderViewport* pRenderViewport )
    {
        uint32_t            frameIndex = pRenderSystem->GetFrameIndex();
        RHI::Context*       pContextRHI = pRenderSystem->GetContextRHI();

        //---------------------------------------------------------------------------------------------------

        // Reset the frame buffer for a new frame, flush old commands and only keep ones with a valid TTL
        m_frameCommandBuffer.Reset( deltaTime );

        // Fill the command buffer from the various debug drawing systems
        pDebugDrawingSystem->ReflectFrameCommandBuffer( m_frameCommandBuffer );
        pRenderViewport->GetDebugDrawSystem()->ReflectFrameCommandBuffer( m_frameCommandBuffer );

        //---------------------------------------------------------------------------------------------------

        Int2 textureSize = pRenderViewport->GetSize();
        uint32_t textureWidth = uint32_t( textureSize.m_x );
        uint32_t textureHeight = uint32_t( textureSize.m_y );

        if ( pRenderViewport->TextureNeedsResize( pRenderViewport->m_DebugDraw_DepthTexture ) )
        {
            pRenderSystem->QueueResourceDelete( eastl::move( pRenderViewport->m_DebugDraw_DepthTexture ) );

            RHI::TextureParameters depthParameters = {};
            depthParameters.m_width = textureWidth;
            depthParameters.m_height = textureHeight;
            depthParameters.m_format = RHI::DataFormat::D32_SFloat;
            depthParameters.m_descriptorTypes.SetMultipleFlags( RHI::DescriptorTypeFlags::RenderTarget );
            depthParameters.m_debugName.sprintf( "DebugDraw Separate Depth Target %dx%d", textureWidth, textureHeight );

            pRenderViewport->m_DebugDraw_DepthTexture = RHI::CreateTexture( pRenderSystem->GetContextRHI(), depthParameters );
        }

        //---------------------------------------------------------------------------------------------------

        auto ComputeNumCommands = [] ( CommandBuffer const& commandBuffer )
        {
            uint32_t numCommands = 0;
            numCommands += uint32_t( commandBuffer.m_lineCommands.size() );
            numCommands += uint32_t( commandBuffer.m_triangleCommands.size() );
            numCommands += uint32_t( commandBuffer.m_pointCommands.size() );

            for ( TextCommand const& textCommand : commandBuffer.m_textCommands )
            {
                numCommands += uint32_t( textCommand.m_text.size() * 2 );
                if ( textCommand.m_hasBackground )
                {
                    numCommands++;
                }
            }

            return numCommands;
        };

        auto ComputeNumMeshThreadGroups = [this] ( CommandBuffer const& commandBuffer )
        {
            uint32_t numThreadGroups = 0;
            for ( MeshCommand const& meshCommand : commandBuffer.m_meshCommands )
            {
                DebugMeshRegistry::RegisteredMesh const* pDebugMesh = m_pDebugMeshRegistry->FindMesh( meshCommand.m_meshID );
                if ( !pDebugMesh )
                {
                    EE_LOG_WARNING( LogCategory::Render, "Debug Draw", "No registered debug mesh" );
                    continue;
                }

                numThreadGroups += uint32_t( pDebugMesh->m_clustersHandle.m_data.size() ); // 1 group per cluster
            }
            return numThreadGroups;
        };

        pRenderViewport->m_numCommands_TransparentDepthOnNoWrite = ComputeNumCommands( m_frameCommandBuffer.m_transparentDepthOnNoWrite );
        pRenderViewport->m_numCommands_TransparentDepthOnWrite = ComputeNumCommands( m_frameCommandBuffer.m_transparentDepthOnWrite );
        pRenderViewport->m_numCommands_TransparentDepthSeparateWrite = ComputeNumCommands( m_frameCommandBuffer.m_transparentDepthSeparateWrite );

        uint32_t numDebugCommands_Total =
            pRenderViewport->m_numCommands_TransparentDepthOnNoWrite +
            pRenderViewport->m_numCommands_TransparentDepthOnWrite +
            pRenderViewport->m_numCommands_TransparentDepthSeparateWrite;

        uint32_t numMeshDebugCommands_Total =
            uint32_t( m_frameCommandBuffer.m_transparentDepthOnNoWrite.m_meshCommands.size() ) +
            uint32_t( m_frameCommandBuffer.m_transparentDepthOnWrite.m_meshCommands.size() ) +
            uint32_t( m_frameCommandBuffer.m_transparentDepthSeparateWrite.m_meshCommands.size() );

        // Shader debug
        //----------------------------------------------------------------------------------------------------

        if ( !pRenderViewport->m_shaderDebugDrawBuffers[frameIndex] )
        {
            RHI::BufferParameters debugBufferParameters = {};
            debugBufferParameters.m_bufferSize = sizeof( ShaderTypes::ShaderDebugDrawBuffer );
            debugBufferParameters.m_bufferStride = sizeof( ShaderTypes::ShaderDebugDrawBuffer );
            debugBufferParameters.m_memoryType = RHI::ResourceMemoryType::HostToDevice;
            debugBufferParameters.m_descriptorTypes = RHI::DescriptorTypeFlags::Buffer;
            debugBufferParameters.m_flags = RHI::BufferFlags::PersistentMap;
            debugBufferParameters.m_debugName.sprintf( "RenderPass_DebugDraw ShaderDebugDrawBuffer %i", frameIndex );

            pRenderViewport->m_shaderDebugDrawBuffers[frameIndex] = RHI::CreateBuffer( pContextRHI, debugBufferParameters );
        }

        // Debug buffers
        //---------------------------------------------------------------------------------------------------

        auto UpdateBuffer_DebugCommands = [pContextRHI, frameIndex] ( RHI::Buffer* && pOldBuffer, size_t newBufferSize )
        {
            RHI::DestroyBuffer( pContextRHI, eastl::move( pOldBuffer ) );

            RHI::BufferParameters commandsBufferParameters = {};
            commandsBufferParameters.m_bufferSize = newBufferSize;
            commandsBufferParameters.m_bufferStride = sizeof( ShaderTypes::DebugDrawCommand_Raw );
            commandsBufferParameters.m_descriptorTypes.SetMultipleFlags( RHI::DescriptorTypeFlags::Buffer, RHI::DescriptorTypeFlags::Raw );
            commandsBufferParameters.m_memoryType = RHI::ResourceMemoryType::HostToDevice;
            commandsBufferParameters.m_debugName.sprintf( "RenderPass_DebugDraw DebugCommands Buffer %i", frameIndex );
            commandsBufferParameters.m_flags = RHI::BufferFlags::PersistentMap;

            return RHI::CreateBuffer( pContextRHI, commandsBufferParameters );
        };

        pRenderViewport->m_debugCommandsBuffers[frameIndex].UpdateDeviceResources
        (
            Math::Max( numDebugCommands_Total, 1U ) * sizeof( ShaderTypes::DebugDrawCommand_Raw ),
            UpdateBuffer_DebugCommands
        );

        if ( !pRenderViewport->m_debugParametersBuffers[frameIndex] )
        {
            RHI::BufferParameters debugParameters = {};
            debugParameters.m_bufferSize = sizeof( ShaderTypes::DebugDrawParameters );
            debugParameters.m_bufferStride = sizeof( ShaderTypes::DebugDrawParameters );
            debugParameters.m_memoryType = RHI::ResourceMemoryType::HostToDevice;
            debugParameters.m_descriptorTypes = RHI::DescriptorTypeFlags::ConstantBuffer;
            debugParameters.m_flags = RHI::BufferFlags::PersistentMap;
            debugParameters.m_debugName.sprintf( "RenderPass_DebugDraw DebugParameters %i", frameIndex );

            pRenderViewport->m_debugParametersBuffers[frameIndex] = RHI::CreateBuffer( pContextRHI, debugParameters );
        }

        //---------------------------------------------------------------------------------------------------

        auto UpdateBuffer_DebugMeshTransform = [pContextRHI, frameIndex] ( RHI::Buffer* && pOldBuffer, size_t newBufferSize )
        {
            RHI::DestroyBuffer( pContextRHI, eastl::move( pOldBuffer ) );

            RHI::BufferParameters transformBufferParameters = {};
            transformBufferParameters.m_bufferSize = newBufferSize;
            transformBufferParameters.m_bufferStride = sizeof( ShaderTypes::DebugDrawMeshParameters );
            transformBufferParameters.m_descriptorTypes = RHI::DescriptorTypeFlags::Buffer;
            transformBufferParameters.m_memoryType = RHI::ResourceMemoryType::HostToDevice;
            transformBufferParameters.m_debugName.sprintf( "RenderPass_DebugDraw DebugMeshTransforms Buffer %i", frameIndex );
            transformBufferParameters.m_flags = RHI::BufferFlags::PersistentMap;

            return RHI::CreateBuffer( pContextRHI, transformBufferParameters );
        };

        pRenderViewport->m_meshParametersBuffers[frameIndex].UpdateDeviceResources
        (
            Math::Max( numMeshDebugCommands_Total, 1U ) * sizeof( ShaderTypes::DebugDrawMeshParameters ),
            UpdateBuffer_DebugMeshTransform
        );

        //---------------------------------------------------------------------------------------------------

        auto UpdateBuffer_DebugMeshArgument = [pContextRHI, frameIndex] ( RHI::Buffer* && pOldBuffer, size_t newBufferSize )
        {
            RHI::DestroyBuffer( pContextRHI, eastl::move( pOldBuffer ) );

            RHI::BufferParameters argumentBufferParameters = {};
            argumentBufferParameters.m_bufferSize = newBufferSize;
            argumentBufferParameters.m_bufferStride = sizeof( ShaderTypes::DebugDrawMeshArgument );
            argumentBufferParameters.m_flags.SetMultipleFlags( RHI::BufferFlags::NoDescriptors, RHI::BufferFlags::PersistentMap );
            argumentBufferParameters.m_memoryType = RHI::ResourceMemoryType::HostToDevice;
            argumentBufferParameters.m_debugName.sprintf( "RenderPass_DebugDraw DebugMeshArguments Buffer %i", frameIndex );


            return RHI::CreateBuffer( pContextRHI, argumentBufferParameters );
        };

        pRenderViewport->m_meshArgumentBuffers[frameIndex].UpdateDeviceResources
        (
            Math::Max( numMeshDebugCommands_Total, 1U ) * sizeof( ShaderTypes::DebugDrawMeshArgument ),
            UpdateBuffer_DebugMeshArgument
        );

        if ( !pRenderViewport->m_meshArgumentCounterBuffers[frameIndex] )
        {
            RHI::BufferParameters counterBufferParameters = {};
            counterBufferParameters.m_bufferSize = NUM_DEPTH_TEST_BUCKETS * sizeof( uint32_t );
            counterBufferParameters.m_bufferStride = sizeof( uint32_t );
            counterBufferParameters.m_flags.SetMultipleFlags( RHI::BufferFlags::NoDescriptors, RHI::BufferFlags::PersistentMap );
            counterBufferParameters.m_memoryType = RHI::ResourceMemoryType::HostToDevice;
            counterBufferParameters.m_debugName.sprintf( "RenderPass_DebugDraw DebugMeshArgumentCounter Buffer %i", frameIndex );

            pRenderViewport->m_meshArgumentCounterBuffers[frameIndex] = RHI::CreateBuffer( pContextRHI, counterBufferParameters );
        }

        // Picking and debug buffers
        //---------------------------------------------------------------------------------------------------

        ForEachBucket( [pRenderSystem, frameIndex] ( DeviceBufferState& argumentBuffer, DeviceAppendBuffer<void>& commandsBuffer )
        {
            commandsBuffer.UpdateBuffers( pRenderSystem, frameIndex, sizeof( ShaderTypes::DebugDrawCommand_Raw ), TBitFlags<RHI::DescriptorTypeFlags>( RHI::DescriptorTypeFlags::Raw, RHI::DescriptorTypeFlags::RWBuffer ) );
        } );

        pRenderViewport->m_debugDrawPickingResultsBuffer.UpdateBuffers( pRenderSystem, frameIndex, sizeof( ShaderTypes::PickingResult ), RHI::DescriptorTypeFlags::RWBuffer );

        //---------------------------------------------------------------------------------------------------

        Math::ViewVolume const& viewVolume = pRenderViewport->GetViewVolume();

        alignas( 32 ) ShaderTypes::DebugDrawParameters debugDrawParameters = {};
        viewVolume.GetViewPosition().StoreFloat3( debugDrawParameters.m_cameraPosition );
        viewVolume.GetViewForwardVector().StoreFloat3( debugDrawParameters.m_cameraForwardDirection );
        viewVolume.GetViewUpVector().StoreFloat3( debugDrawParameters.m_cameraUpDirection );
        viewVolume.GetViewRightVector().StoreFloat3( debugDrawParameters.m_cameraRightDirection );

        Vector mouseClipSpace = pRenderViewport->ScreenSpaceToClipSpace( pRenderViewport->m_lastKnownPickingMousePosition );
        float pickingRadiusClipSpace = float( pRenderViewport->m_lastKnownPickingPixelRadius ) / pRenderViewport->GetDimensions().GetMax();

        debugDrawParameters.m_pickingInput[0] = mouseClipSpace.GetX();
        debugDrawParameters.m_pickingInput[1] = mouseClipSpace.GetY();
        debugDrawParameters.m_pickingInput[2] = pickingRadiusClipSpace;
        debugDrawParameters.m_pickingInput[3] = 0.0F;

        Memory::CopyToWriteCombined( pRenderViewport->m_debugParametersBuffers[frameIndex]->m_pMappedAddress_WriteCombined, &debugDrawParameters, sizeof( ShaderTypes::DebugDrawParameters ) );
    }

    void DebugDrawRenderPass::ClearBuffers( DeviceResourceStates& resourceStates, RHI::CommandBuffer* pCommandBuffer, uint32_t frameIndex )
    {
        ForEachBucket( [&resourceStates, pCommandBuffer, frameIndex] ( DeviceBufferState& argumentBuffer, DeviceAppendBuffer<void>& commandsBuffer )
        {
            commandsBuffer.Clear( resourceStates, pCommandBuffer, frameIndex );
        } );
    }

    void DebugDrawRenderPass::DrawToViewport( RenderViewport const*     pRenderViewport,
                                              DeviceRenderWorld const&  deviceRenderWorld,
                                              DeviceTextureState&       finalRenderTarget,
                                              RHI::BufferHandle         clusterBuffer,
                                              RHI::BufferHandle         renderViewBuffer,
                                              uint32_t                  mainCameraViewIndex,
                                              DeviceResourceStates&     resourceStates,
                                              RHI::CommandBuffer*       pCommandBuffer,
                                              uint32_t                  frameIndex )
    {
        EE_PROFILE_FUNCTION_RENDER();

        EE_ASSERT( !resourceStates.HasPendingBarriers() );

        Math::ViewVolume const& viewVolume = pRenderViewport->GetViewVolume();

        m_fontPixelSize = Float2( 1.0F / pRenderViewport->GetDimensions().m_x, 1.0F / pRenderViewport->GetDimensions().m_y );
        m_fontPixelSize.m_x *= viewVolume.GetAspectRatio();

        TArray<uint32_t, NUM_DEPTH_TEST_BUCKETS> numValidMeshCommandsPerBucket = {};

        if ( !m_frameCommandBuffer.IsEmpty() )
        {
            EE_PROFILE_SCOPE_RENDER( "CopyDebugDrawCommands" );

            TArray<RHI::TextureHandle, 2> fontTextureHandles =
            {
                RHI::GetTextureHandle( m_fontCacheTextures[0], RHI::DescriptorTypeFlags::Texture, 0 ),
                RHI::GetTextureHandle( m_fontCacheTextures[1], RHI::DescriptorTypeFlags::Texture, 0 ),
            };

            ShaderTypes::DebugDrawCommand_Raw* pDstCommands_WriteCombined = reinterpret_cast<ShaderTypes::DebugDrawCommand_Raw*>( pRenderViewport->m_debugCommandsBuffers[frameIndex].m_buffer->m_pMappedAddress_WriteCombined );
            ShaderTypes::DebugDrawCommand_Raw const* pDstCommandsEnd_WriteCombined = pDstCommands_WriteCombined + ( pRenderViewport->m_debugCommandsBuffers[frameIndex].m_buffer->m_size / pRenderViewport->m_debugCommandsBuffers[frameIndex].m_buffer->m_stride );

            ShaderTypes::DebugDrawMeshArgument* pDstMeshArguments_WriteCombined = reinterpret_cast<ShaderTypes::DebugDrawMeshArgument*>( pRenderViewport->m_meshArgumentBuffers[frameIndex].m_buffer->m_pMappedAddress_WriteCombined );
            ShaderTypes::DebugDrawMeshParameters* pDstMeshParameters_WriteCombined = reinterpret_cast<ShaderTypes::DebugDrawMeshParameters*>( pRenderViewport->m_meshParametersBuffers[frameIndex].m_buffer->m_pMappedAddress_WriteCombined );

            ShaderTypes::DebugDrawMeshArgument const* pDstMeshArgumentsEnd_WriteCombined = pDstMeshArguments_WriteCombined + ( pRenderViewport->m_meshArgumentBuffers[frameIndex].m_buffer->m_size / pRenderViewport->m_meshArgumentBuffers[frameIndex].m_buffer->m_stride );
            ShaderTypes::DebugDrawMeshParameters const* pDstMeshParametersEnd_WriteCombined = pDstMeshParameters_WriteCombined + ( pRenderViewport->m_meshParametersBuffers[frameIndex].m_buffer->m_size / pRenderViewport->m_meshParametersBuffers[frameIndex].m_buffer->m_stride );

            uint32_t* pDstMeshArgumentCounter_WriteCombined = reinterpret_cast<uint32_t*>( pRenderViewport->m_meshArgumentCounterBuffers[frameIndex]->m_pMappedAddress_WriteCombined );
            uint32_t const* pDstMeshArgumentCounterEnd_WriteCombined = pDstMeshArgumentCounter_WriteCombined + NUM_DEPTH_TEST_BUCKETS;

            uint64_t dstParametersDeviceAddress = pRenderViewport->m_meshParametersBuffers[frameIndex].m_buffer->m_deviceAddress;
            uint32_t dstBucketIndex = 0;

            // TODO: All these writes can be done in parallel, if this ever shows up in the CPU profiler just offload them to worker 
            //-----------------------------------------------------------------

            auto CopyCommands =
                [
                    &pDstCommands_WriteCombined, pDstCommandsEnd_WriteCombined,
                    &pDstMeshArguments_WriteCombined, pDstMeshArgumentsEnd_WriteCombined,
                    &pDstMeshParameters_WriteCombined, pDstMeshParametersEnd_WriteCombined,
                    &pDstMeshArgumentCounter_WriteCombined, pDstMeshArgumentCounterEnd_WriteCombined,
                    &dstBucketIndex, &dstParametersDeviceAddress,
                    this,
                    &fontTextureHandles,
                    frameIndex,
                    clusterBuffer,
                    renderViewBuffer,
                    mainCameraViewIndex,
                    pRenderViewport,
                    &numValidMeshCommandsPerBucket
                ] ( CommandBuffer const& commandBuffer )
            {
                // Regular commands
                //-------------------------------------------------------------

                EE_ASSERT( ( pDstCommands_WriteCombined + commandBuffer.m_pointCommands.size() ) <= pDstCommandsEnd_WriteCombined );
                Memory::CopyToWriteCombined
                (
                    pDstCommands_WriteCombined,
                    commandBuffer.m_pointCommands.data(),
                    commandBuffer.m_pointCommands.size() * sizeof( ShaderTypes::DebugDrawPointCommand )
                );
                pDstCommands_WriteCombined += commandBuffer.m_pointCommands.size();

                EE_ASSERT( ( pDstCommands_WriteCombined + commandBuffer.m_lineCommands.size() ) <= pDstCommandsEnd_WriteCombined );
                Memory::CopyToWriteCombined
                (
                    pDstCommands_WriteCombined,
                    commandBuffer.m_lineCommands.data(),
                    commandBuffer.m_lineCommands.size() * sizeof( ShaderTypes::DebugDrawLineCommand )
                );
                pDstCommands_WriteCombined += commandBuffer.m_lineCommands.size();

                EE_ASSERT( ( pDstCommands_WriteCombined + commandBuffer.m_triangleCommands.size() ) <= pDstCommandsEnd_WriteCombined );
                Memory::CopyToWriteCombined
                (
                    pDstCommands_WriteCombined,
                    commandBuffer.m_triangleCommands.data(),
                    commandBuffer.m_triangleCommands.size() * sizeof( ShaderTypes::DebugDrawTriangleCommand )
                );
                pDstCommands_WriteCombined += commandBuffer.m_triangleCommands.size();

                pDstCommands_WriteCombined = WriteTextBoxes( pDstCommands_WriteCombined, pDstCommandsEnd_WriteCombined, m_fontCaches, m_fontPixelSize, *pRenderViewport, commandBuffer.m_textCommands );
                pDstCommands_WriteCombined = WriteTextsOutline( pDstCommands_WriteCombined, pDstCommandsEnd_WriteCombined, m_fontCaches, fontTextureHandles, m_fontPixelSize, *pRenderViewport, commandBuffer.m_textCommands );

                EE_ASSERT( pDstCommands_WriteCombined <= pDstCommandsEnd_WriteCombined );

                // Mesh commands
                //-----------------------------------------------------------------

                uint32_t numValidMeshCommands = 0;
                for ( MeshCommand const& meshCommand : commandBuffer.m_meshCommands )
                {
                    DebugMeshRegistry::RegisteredMesh const* pDebugMesh = m_pDebugMeshRegistry->FindMesh( meshCommand.m_meshID );
                    if ( !pDebugMesh )
                    {
                        EE_LOG_WARNING( LogCategory::Render, "Debug Draw", "No registered debug mesh" );
                        continue;
                    }

                    ShaderTypes::DebugDrawMeshRootConstant meshRootConstant = {};
                    meshRootConstant.m_renderViewBuffer = renderViewBuffer;
                    meshRootConstant.m_clusterBuffer = clusterBuffer;
                    meshRootConstant.m_clusterVertexBuffer = RHI::GetBufferHandle( pDebugMesh->m_pClusterVertexBuffer, RHI::DescriptorTypeFlags::Buffer );
                    meshRootConstant.m_clusterTriangleBuffer = RHI::GetBufferHandle( pDebugMesh->m_pClusterTriangleBuffer, RHI::DescriptorTypeFlags::Buffer );
                    meshRootConstant.m_mainCameraRenderView = mainCameraViewIndex;
                    meshRootConstant.m_packedColorTint = meshCommand.m_tintColor;
                    meshRootConstant.m_wireframe = meshCommand.m_isWireframe;
                    meshRootConstant.m_fakeLighting = meshCommand.m_useFakeLighting;
                    meshRootConstant.m_clusterOffset = uint32_t( pDebugMesh->m_clustersHandle.m_handle.m_offset );
                    meshRootConstant.m_pickingSortPriority = m_depthBuckets[dstBucketIndex].m_pickingSortPriority;

                    ShaderTypes::DebugDrawMeshArgument meshArgument = {};
                    meshArgument.m_rootConstant = meshRootConstant;
                    meshArgument.m_rootCBV = pRenderViewport->m_debugParametersBuffers[frameIndex]->m_deviceAddress;
                    meshArgument.m_rootSRV = dstParametersDeviceAddress;
                    meshArgument.m_dispatchArguments[0] = uint32_t( pDebugMesh->m_clustersHandle.m_data.size() );
                    meshArgument.m_dispatchArguments[1] = 1;
                    meshArgument.m_dispatchArguments[2] = 1;

                    ShaderTypes::DebugDrawMeshParameters meshParameters = {};
                    meshParameters.m_hitTestID = meshCommand.m_hitTestID;
                    meshParameters.m_debugDrawPickingResultsBuffer = pRenderViewport->m_debugDrawPickingResultsBuffer.GetAppendBufferHandle();
                    memcpy( meshParameters.m_transform, &meshCommand.m_transform, sizeof( Matrix ) );

                    memcpy( pDstMeshArguments_WriteCombined, &meshArgument, sizeof( ShaderTypes::DebugDrawMeshArgument ) );
                    memcpy( pDstMeshParameters_WriteCombined, &meshParameters, sizeof( ShaderTypes::DebugDrawMeshParameters ) );

                    pDstMeshArguments_WriteCombined++;
                    pDstMeshParameters_WriteCombined++;
                    dstParametersDeviceAddress += sizeof( ShaderTypes::DebugDrawMeshParameters );

                    EE_ASSERT( pDstMeshParameters_WriteCombined <= pDstMeshParametersEnd_WriteCombined );
                    EE_ASSERT( pDstMeshArguments_WriteCombined <= pDstMeshArgumentsEnd_WriteCombined );

                    numValidMeshCommands++;
                }

                *pDstMeshArgumentCounter_WriteCombined = numValidMeshCommands;
                numValidMeshCommandsPerBucket[dstBucketIndex] = numValidMeshCommands;

                pDstMeshArgumentCounter_WriteCombined++;
                EE_ASSERT( pDstMeshArgumentCounter_WriteCombined <= pDstMeshArgumentCounterEnd_WriteCombined );

                dstBucketIndex++;
            };

            //-----------------------------------------------------------------

            CopyCommands( m_frameCommandBuffer.m_transparentDepthOnWrite );
            CopyCommands( m_frameCommandBuffer.m_transparentDepthOnNoWrite );
            CopyCommands( m_frameCommandBuffer.m_transparentDepthSeparateWrite );
        }

        //---------------------------------------------------------------------------------------------------

        EE_RHI_COMMAND_BUFFER_PROFILE_SCOPE( pCommandBuffer, "Debug Draw" );

        pRenderViewport->m_debugDrawPickingResultsBuffer.Clear( resourceStates, pCommandBuffer, frameIndex );

        // Setup shader debug drawing
        //---------------------------------------------------------------------------------------------------

        ShaderTypes::ShaderDebugDrawBuffer* pDstShaderDebugDrawBuffer_WriteCombined = reinterpret_cast<ShaderTypes::ShaderDebugDrawBuffer*>( pRenderViewport->m_shaderDebugDrawBuffers[frameIndex]->m_pMappedAddress_WriteCombined );

        pDstShaderDebugDrawBuffer_WriteCombined->m_debugDrawCommandsBuffer_TransparentDepthOn = m_depthBuckets[DEPTH_TEST_ON].m_commandsBuffer.GetAppendBufferHandle();
        pDstShaderDebugDrawBuffer_WriteCombined->m_debugDrawCommandsBuffer_TransparentDepthOff = m_depthBuckets[DEPTH_TEST_SEPARATE_WRITE].m_commandsBuffer.GetAppendBufferHandle();

        ShaderTypes::DebugDrawResourceTableData debugDrawRootConstant = {};
        debugDrawRootConstant.SetDebugDrawCommandsBuffer( RHI::GetBufferHandle( pRenderViewport->m_debugCommandsBuffers[frameIndex].m_buffer, RHI::DescriptorTypeFlags::Buffer ) );
        debugDrawRootConstant.SetRenderViewBuffer( renderViewBuffer );

        debugDrawRootConstant.SetPickingResultsBuffer( pRenderViewport->m_debugDrawPickingResultsBuffer.GetAppendBufferHandle() );
        debugDrawRootConstant.m_mainCameraRenderView = mainCameraViewIndex;

        debugDrawRootConstant.m_commandsOffset = 0;

        // Copy debug draw counts to debug draw arguments
        //---------------------------------------------------------------------------------------------------

        ShaderTypes::DebugDrawResolveResourceTableData debugDrawResolveRootConstant = {};

        EE_ASSERT( !resourceStates.HasPendingBarriers() );
        debugDrawResolveRootConstant.SetCommandsBuffer_TransparentDepthOn( resourceStates, RHI::PipelineStage::ComputeShader, m_depthBuckets[DEPTH_TEST_ON].m_commandsBuffer );
        debugDrawResolveRootConstant.SetCommandsBuffer_TransparentDepthOff( resourceStates, RHI::PipelineStage::ComputeShader, m_depthBuckets[DEPTH_TEST_SEPARATE_WRITE].m_commandsBuffer );
        debugDrawResolveRootConstant.SetArgumentBuffer_TransparentDepthOn( resourceStates, RHI::PipelineStage::ComputeShader, m_depthBuckets[DEPTH_TEST_ON].m_argumentBuffer );
        debugDrawResolveRootConstant.SetArgumentBuffer_TransparentDepthOff( resourceStates, RHI::PipelineStage::ComputeShader, m_depthBuckets[DEPTH_TEST_SEPARATE_WRITE].m_argumentBuffer );
        resourceStates.FlushBarriers( pCommandBuffer );

        debugDrawResolveRootConstant.SetRenderViewBuffer( renderViewBuffer );
        debugDrawResolveRootConstant.SetDebugParametersBufferAddress( pRenderViewport->m_debugParametersBuffers[frameIndex]->m_deviceAddress );

        debugDrawResolveRootConstant.m_mainCameraRenderView = mainCameraViewIndex;

        RHI::CmdSetPipeline( pCommandBuffer, m_pDebugDrawResolveShader->m_pPipeline );
        RHI::CmdSetRootConstants( pCommandBuffer, 0, &debugDrawResolveRootConstant, sizeof( debugDrawResolveRootConstant ) );
        RHI::CmdDispatchCompute( pCommandBuffer, 1, 1, 1 );

        EE_ASSERT( !resourceStates.HasPendingBarriers() );
        for ( DepthTestBucket& bucket : m_depthBuckets )
        {
            resourceStates.ReadOnly( bucket.m_commandsBuffer.m_deviceBuffer, RHI::PipelineStage::NonPixelShader, RHI::ResourceAccess::ShaderResource );
            resourceStates.ReadOnly( bucket.m_argumentBuffer, RHI::PipelineStage::ExecuteIndirect, RHI::ResourceAccess::IndirectArgument );
        }
        resourceStates.FlushBarriers( pCommandBuffer );

        //---------------------------------------------------------------------------------------------------

        // TODO: This part is a bit gnarly and will be refactored once debug meshes are finalized.
        // Lambda MODIFIES debugDrawRootConstant, meshCommandsOffset and meshCounterOffset - it needs to write updated offset for debug commands!
        auto DrawDebugCommands_DepthOnWrite =
            [
                this,
                &resourceStates,
                pCommandBuffer,
                &debugDrawRootConstant,
                pRenderViewport,
                frameIndex
            ] ( char const* pScopeName, uint32_t numCommands, uint32_t numMeshCommands, DepthTestBucket const& bucket, uint32_t& meshCommandsOffset, uint32_t& meshCounterOffset )
        {
            EE_PROFILE_SCOPE_RENDER( "DrawDebugCommands" );
            EE_RHI_COMMAND_BUFFER_PROFILE_SCOPE( pCommandBuffer, pScopeName );

            debugDrawRootConstant.m_numDebugDrawCommands = numCommands;
            debugDrawRootConstant.m_pickingSortPriority = bucket.m_pickingSortPriority;

            // Transparent depth on - 2 passes, depth only to write depth followed by alpha blended pass with depth equals
            //---------------------------------------------------------------------------------------------------
            if ( numMeshCommands )
            {
                EE_ASSERT( !resourceStates.HasPendingBarriers() );
                resourceStates.Writeable( pRenderViewport->m_debugDrawPickingResultsBuffer.m_deviceBuffer, RHI::PipelineStage::NonPixelShader, RHI::ResourceAccess::UnorderedAccess );
                resourceStates.Writeable( pRenderViewport->m_debugDrawPickingResultsBuffer.m_deviceCounterBuffer, RHI::PipelineStage::NonPixelShader, RHI::ResourceAccess::UnorderedAccess );
                resourceStates.FlushBarriers( pCommandBuffer );

                RHI::CmdSetPipeline( pCommandBuffer, m_pTransparentDepthOnDepthPipeline_Mesh );
                RHI::CmdExecuteIndirect
                (
                    pCommandBuffer, m_pDebugDrawMeshShader->m_pCommandSignatureMeshDispatch, numMeshCommands,
                    pRenderViewport->m_meshArgumentBuffers[frameIndex].m_buffer, meshCommandsOffset * sizeof( ShaderTypes::DebugDrawMeshArgument ),
                    pRenderViewport->m_meshArgumentCounterBuffers[frameIndex], meshCounterOffset * sizeof( uint32_t )
                );
            }

            RHI::CmdSetPipeline( pCommandBuffer, m_pTransparentDepthOnDepthPipeline );
            RHI::CmdSetRootConstants( pCommandBuffer, 0, &debugDrawRootConstant, sizeof( debugDrawRootConstant ) );
            RHI::CmdSetRootParameter( pCommandBuffer, 1, pRenderViewport->m_debugParametersBuffers[frameIndex], 0 );

            if ( numCommands )
            {
                EE_ASSERT( !resourceStates.HasPendingBarriers() );
                resourceStates.Writeable( pRenderViewport->m_debugDrawPickingResultsBuffer.m_deviceBuffer, RHI::PipelineStage::NonPixelShader, RHI::ResourceAccess::UnorderedAccess );
                resourceStates.Writeable( pRenderViewport->m_debugDrawPickingResultsBuffer.m_deviceCounterBuffer, RHI::PipelineStage::NonPixelShader, RHI::ResourceAccess::UnorderedAccess );
                resourceStates.FlushBarriers( pCommandBuffer );

                RHI::CmdDispatchMesh( pCommandBuffer, ( numCommands + 63 ) / 64, 1, 1 );
            }

            EE_ASSERT( !resourceStates.HasPendingBarriers() );
            resourceStates.Writeable( pRenderViewport->m_debugDrawPickingResultsBuffer.m_deviceBuffer, RHI::PipelineStage::NonPixelShader, RHI::ResourceAccess::UnorderedAccess );
            resourceStates.Writeable( pRenderViewport->m_debugDrawPickingResultsBuffer.m_deviceCounterBuffer, RHI::PipelineStage::NonPixelShader, RHI::ResourceAccess::UnorderedAccess );
            resourceStates.FlushBarriers( pCommandBuffer );

            RHI::CmdExecuteIndirect( pCommandBuffer, m_pDebugDrawShader->m_pCommandSignatureMeshDispatch, 1, bucket.m_argumentBuffer, 0, nullptr, 0 );

            // Color pass
            //---------------------------------------------------------------------------------------------------
            if ( numMeshCommands )
            {
                EE_ASSERT( !resourceStates.HasPendingBarriers() );
                resourceStates.Writeable( pRenderViewport->m_debugDrawPickingResultsBuffer.m_deviceBuffer, RHI::PipelineStage::NonPixelShader, RHI::ResourceAccess::UnorderedAccess );
                resourceStates.Writeable( pRenderViewport->m_debugDrawPickingResultsBuffer.m_deviceCounterBuffer, RHI::PipelineStage::NonPixelShader, RHI::ResourceAccess::UnorderedAccess );
                resourceStates.FlushBarriers( pCommandBuffer );

                RHI::CmdSetPipeline( pCommandBuffer, m_pTransparentDepthOnColorPipeline_Mesh );
                RHI::CmdExecuteIndirect
                (
                    pCommandBuffer, m_pDebugDrawMeshShader->m_pCommandSignatureMeshDispatch, numMeshCommands,
                    pRenderViewport->m_meshArgumentBuffers[frameIndex].m_buffer, meshCommandsOffset * sizeof( ShaderTypes::DebugDrawMeshArgument ),
                    pRenderViewport->m_meshArgumentCounterBuffers[frameIndex], meshCounterOffset * sizeof( uint32_t )
                );
            }

            RHI::CmdSetPipeline( pCommandBuffer, m_pTransparentDepthOnColorPipeline );
            RHI::CmdSetRootConstants( pCommandBuffer, 0, &debugDrawRootConstant, sizeof( debugDrawRootConstant ) );
            RHI::CmdSetRootParameter( pCommandBuffer, 1, pRenderViewport->m_debugParametersBuffers[frameIndex], 0 );

            if ( numCommands )
            {
                EE_ASSERT( !resourceStates.HasPendingBarriers() );
                resourceStates.Writeable( pRenderViewport->m_debugDrawPickingResultsBuffer.m_deviceBuffer, RHI::PipelineStage::NonPixelShader, RHI::ResourceAccess::UnorderedAccess );
                resourceStates.Writeable( pRenderViewport->m_debugDrawPickingResultsBuffer.m_deviceCounterBuffer, RHI::PipelineStage::NonPixelShader, RHI::ResourceAccess::UnorderedAccess );
                resourceStates.FlushBarriers( pCommandBuffer );

                RHI::CmdDispatchMesh( pCommandBuffer, ( numCommands + 63 ) / 64, 1, 1 );
            }

            EE_ASSERT( !resourceStates.HasPendingBarriers() );
            resourceStates.Writeable( pRenderViewport->m_debugDrawPickingResultsBuffer.m_deviceBuffer, RHI::PipelineStage::NonPixelShader, RHI::ResourceAccess::UnorderedAccess );
            resourceStates.Writeable( pRenderViewport->m_debugDrawPickingResultsBuffer.m_deviceCounterBuffer, RHI::PipelineStage::NonPixelShader, RHI::ResourceAccess::UnorderedAccess );
            resourceStates.FlushBarriers( pCommandBuffer );

            RHI::CmdExecuteIndirect( pCommandBuffer, m_pDebugDrawShader->m_pCommandSignatureMeshDispatch, 1, bucket.m_argumentBuffer, 0, nullptr, 0 );

            // Update offsets
            //---------------------------------------------------------------------------------------------------
            debugDrawRootConstant.m_commandsOffset += numCommands;
            meshCommandsOffset += numMeshCommands;
            meshCounterOffset++;
        };

        // TODO: This part is a bit gnarly and will be refactored once debug meshes are finalized.
        // Lambda MODIFIES debugDrawRootConstant, meshCommandsOffset and meshCounterOffset - it needs to write updated offset for debug commands!
        auto DrawDebugCommands_DepthOnNoWrite =
            [
                this,
                &resourceStates,
                pCommandBuffer,
                &debugDrawRootConstant,
                pRenderViewport,
                frameIndex
            ] ( char const* pScopeName, uint32_t numCommands, uint32_t numMeshCommands, DepthTestBucket const& bucket, uint32_t& meshCommandsOffset, uint32_t& meshCounterOffset )
        {
            EE_PROFILE_SCOPE_RENDER( "DrawDebugCommands" );
            EE_RHI_COMMAND_BUFFER_PROFILE_SCOPE( pCommandBuffer, pScopeName );

            debugDrawRootConstant.m_numDebugDrawCommands = numCommands;

            // Color pass
            //---------------------------------------------------------------------------------------------------
            if ( numMeshCommands )
            {
                EE_ASSERT( !resourceStates.HasPendingBarriers() );
                resourceStates.Writeable( pRenderViewport->m_debugDrawPickingResultsBuffer.m_deviceBuffer, RHI::PipelineStage::NonPixelShader, RHI::ResourceAccess::UnorderedAccess );
                resourceStates.Writeable( pRenderViewport->m_debugDrawPickingResultsBuffer.m_deviceCounterBuffer, RHI::PipelineStage::NonPixelShader, RHI::ResourceAccess::UnorderedAccess );
                resourceStates.FlushBarriers( pCommandBuffer );

                RHI::CmdSetPipeline( pCommandBuffer, m_pTransparentDepthOnNoWriteColorPipeline_Mesh );
                RHI::CmdExecuteIndirect
                (
                    pCommandBuffer, m_pDebugDrawMeshShader->m_pCommandSignatureMeshDispatch, numMeshCommands,
                    pRenderViewport->m_meshArgumentBuffers[frameIndex].m_buffer, meshCommandsOffset * sizeof( ShaderTypes::DebugDrawMeshArgument ),
                    pRenderViewport->m_meshArgumentCounterBuffers[frameIndex], meshCounterOffset * sizeof( uint32_t )
                );
            }

            RHI::CmdSetPipeline( pCommandBuffer, m_pTransparentDepthOnNoWriteColorPipeline );
            RHI::CmdSetRootConstants( pCommandBuffer, 0, &debugDrawRootConstant, sizeof( debugDrawRootConstant ) );
            RHI::CmdSetRootParameter( pCommandBuffer, 1, pRenderViewport->m_debugParametersBuffers[frameIndex], 0 );

            if ( numCommands )
            {
                EE_ASSERT( !resourceStates.HasPendingBarriers() );
                resourceStates.Writeable( pRenderViewport->m_debugDrawPickingResultsBuffer.m_deviceBuffer, RHI::PipelineStage::NonPixelShader, RHI::ResourceAccess::UnorderedAccess );
                resourceStates.Writeable( pRenderViewport->m_debugDrawPickingResultsBuffer.m_deviceCounterBuffer, RHI::PipelineStage::NonPixelShader, RHI::ResourceAccess::UnorderedAccess );
                resourceStates.FlushBarriers( pCommandBuffer );

                RHI::CmdDispatchMesh( pCommandBuffer, ( numCommands + 63 ) / 64, 1, 1 );
            }

            EE_ASSERT( !resourceStates.HasPendingBarriers() );
            resourceStates.Writeable( pRenderViewport->m_debugDrawPickingResultsBuffer.m_deviceBuffer, RHI::PipelineStage::NonPixelShader, RHI::ResourceAccess::UnorderedAccess );
            resourceStates.Writeable( pRenderViewport->m_debugDrawPickingResultsBuffer.m_deviceCounterBuffer, RHI::PipelineStage::NonPixelShader, RHI::ResourceAccess::UnorderedAccess );
            resourceStates.FlushBarriers( pCommandBuffer );

            RHI::CmdExecuteIndirect( pCommandBuffer, m_pDebugDrawShader->m_pCommandSignatureMeshDispatch, 1, bucket.m_argumentBuffer, 0, nullptr, 0 );

            // Update offsets
            //---------------------------------------------------------------------------------------------------
            debugDrawRootConstant.m_commandsOffset += numCommands;
            meshCommandsOffset += numMeshCommands;
            meshCounterOffset++;
        };

        uint32_t meshCommandsOffset = 0;
        uint32_t meshCounterOffset = 0;

        RHI::LoadAction debugDrawLoadAction_Load = {};
        debugDrawLoadAction_Load.m_loadActionsColor[0] = RHI::LoadActionType::Load;
        debugDrawLoadAction_Load.m_loadActionDepth = RHI::LoadActionType::Load;

        RHI::LoadAction debugDrawLoadAction_Clear = debugDrawLoadAction_Load;
        debugDrawLoadAction_Clear.m_loadActionDepth = RHI::LoadActionType::Clear;
        debugDrawLoadAction_Clear.m_depthClearValue = pRenderViewport->m_DebugDraw_DepthTexture->m_clearValue;

        // Regular depth test with depth writes
        EE_ASSERT( !resourceStates.HasPendingBarriers() );
        resourceStates.Writeable( pRenderViewport->m_finalTexture, RHI::PipelineStage::Draw, RHI::ResourceAccess::RenderTarget, RHI::TextureState::RenderTarget );
        resourceStates.Writeable( pRenderViewport->m_ForwardShading_DepthTexture, RHI::PipelineStage::Draw, RHI::ResourceAccess::DepthWrite, RHI::TextureState::DepthWrite );
        resourceStates.FlushBarriers( pCommandBuffer );

        RHI::CmdSetRenderTargets( pCommandBuffer, { &finalRenderTarget.m_pTexture, 1 }, pRenderViewport->m_ForwardShading_DepthTexture, &debugDrawLoadAction_Load );
        DrawDebugCommands_DepthOnWrite
        (
            "Depth=On Write=Yes",
            pRenderViewport->m_numCommands_TransparentDepthOnWrite,
            numValidMeshCommandsPerBucket[DEPTH_TEST_ON_WRITE],
            m_depthBuckets[DEPTH_TEST_ON_WRITE],
            meshCommandsOffset, meshCounterOffset
        );

        // Regular depth test without depth writes
        EE_ASSERT( !resourceStates.HasPendingBarriers() );
        resourceStates.Writeable( pRenderViewport->m_finalTexture, RHI::PipelineStage::Draw, RHI::ResourceAccess::RenderTarget, RHI::TextureState::RenderTarget );
        resourceStates.Writeable( pRenderViewport->m_ForwardShading_DepthTexture, RHI::PipelineStage::Draw, RHI::ResourceAccess::DepthRead, RHI::TextureState::DepthRead );
        resourceStates.FlushBarriers( pCommandBuffer );

        RHI::CmdSetRenderTargets( pCommandBuffer, { &finalRenderTarget.m_pTexture, 1 }, pRenderViewport->m_ForwardShading_DepthTexture, &debugDrawLoadAction_Load );
        DrawDebugCommands_DepthOnNoWrite
        (
            "Depth=On Write=NO",
            pRenderViewport->m_numCommands_TransparentDepthOnNoWrite,
            numValidMeshCommandsPerBucket[DEPTH_TEST_ON],
            m_depthBuckets[DEPTH_TEST_ON],
            meshCommandsOffset, meshCounterOffset
        );

        // Depth test only with other debug draws and with depth writes
        EE_ASSERT( !resourceStates.HasPendingBarriers() );
        resourceStates.Writeable( pRenderViewport->m_finalTexture, RHI::PipelineStage::Draw, RHI::ResourceAccess::RenderTarget, RHI::TextureState::RenderTarget );
        resourceStates.Writeable( pRenderViewport->m_DebugDraw_DepthTexture, RHI::PipelineStage::Draw, RHI::ResourceAccess::DepthWrite, RHI::TextureState::DepthWrite );
        resourceStates.FlushBarriers( pCommandBuffer );

        RHI::CmdSetRenderTargets( pCommandBuffer, { &finalRenderTarget.m_pTexture, 1 }, pRenderViewport->m_DebugDraw_DepthTexture, &debugDrawLoadAction_Clear );
        DrawDebugCommands_DepthOnWrite
        (
            "Depth=Separate Write=Yes",
            pRenderViewport->m_numCommands_TransparentDepthSeparateWrite,
            numValidMeshCommandsPerBucket[DEPTH_TEST_SEPARATE_WRITE],
            m_depthBuckets[DEPTH_TEST_SEPARATE_WRITE],
            meshCommandsOffset, meshCounterOffset
        );

        // Copy picking results
        pRenderViewport->m_debugDrawPickingResultsBuffer.CopyResults( resourceStates, pCommandBuffer, frameIndex );

        // Copy shader debug draw results
        ForEachBucket( [&resourceStates, pCommandBuffer, frameIndex] ( DeviceBufferState& argumentBuffer, DeviceAppendBuffer<void>& commandsBuffer )
        {
            commandsBuffer.CopyResults( resourceStates, pCommandBuffer, frameIndex );
        } );

        // Final barriers
        pRenderViewport->m_debugDrawPickingResultsBuffer.Barrier( resourceStates, pCommandBuffer, frameIndex );

        ForEachBucket( [&resourceStates, pCommandBuffer, frameIndex] ( DeviceBufferState& argumentBuffer, DeviceAppendBuffer<void>& commandsBuffer )
        {
            commandsBuffer.Barrier( resourceStates, pCommandBuffer, frameIndex );
        } );
    }
}
#endif
