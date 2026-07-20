#pragma once

#include "Engine/Render/RenderViewLayer.h"
#include "Base/Types/BitFlags.h"
#include "Base/TypeSystem/ReflectedType.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    class EE_ENGINE_API MaterialShaderClusterCapacity
    {
    public:

        void Initialize();
        void Shutdown();

        void AddGlobalClusters( uint32_t numClusters );
        void AddViewLayerClusters( size_t viewLayerIndex, size_t shaderIndex, uint32_t numClusters );

        void RemoveGlobalClusters( uint32_t numClusters );
        void RemoveViewLayerClusters( size_t viewLayerIndex, size_t shaderIndex, uint32_t numClusters );

        void Validate( size_t numShaders );

        uint32_t GetGlobalClusterCapacity() const;
        TArrayView<uint32_t const> GetViewLayerClusterCapacity( ViewLayer viewLayer ) const;

    private:

        size_t                          m_globalClusterCapacity = 1;
        TVector<TVector<uint32_t>>      m_clusterCapacityPerViewPerShader;
    };
}
