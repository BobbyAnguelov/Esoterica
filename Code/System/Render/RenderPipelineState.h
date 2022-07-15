#pragma once

#include "System/Render/RenderShader.h"
#include "System/Render/RenderStates.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    struct PipelineState
    {
        inline void Clear()
        {
            m_pVertexShader = nullptr;
            m_pGeometryShader = nullptr;
            m_pHullShader = nullptr;
            m_pComputeShader = nullptr;
            m_pPixelShader = nullptr;
            m_pBlendState = nullptr;
            m_pRasterizerState = nullptr;
        }

    public:

        VertexShader*                   m_pVertexShader = nullptr;
        GeometryShader*                 m_pGeometryShader = nullptr;
        Shader*                         m_pHullShader = nullptr;
        Shader*                         m_pComputeShader = nullptr;
        PixelShader*                    m_pPixelShader = nullptr;
        BlendState*                     m_pBlendState = nullptr;
        RasterizerState*                m_pRasterizerState = nullptr;
    };
}
