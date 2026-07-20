#include "RenderMaterialShaderClusterCapacity.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    void MaterialShaderClusterCapacity::Initialize()
    {
        m_clusterCapacityPerViewPerShader.resize( 3 ); // 3 view layers total
    }

    void MaterialShaderClusterCapacity::Shutdown()
    {
        EE_ASSERT( m_globalClusterCapacity == 1 );
        for ( TVector<uint32_t> const& perShaderCapacity : m_clusterCapacityPerViewPerShader )
        {
            for ( uint32_t capacity : perShaderCapacity )
            {
                EE_ASSERT( capacity == 1 );
            }
        }

        m_clusterCapacityPerViewPerShader.clear();
    }

    void MaterialShaderClusterCapacity::AddGlobalClusters( uint32_t numClusters )
    {
        m_globalClusterCapacity += numClusters;
    }

    void MaterialShaderClusterCapacity::AddViewLayerClusters( size_t viewIndex, size_t shaderIndex, uint32_t numClusters )
    {
        TVector<uint32_t>& capacityPerShader = m_clusterCapacityPerViewPerShader[viewIndex];
        if ( shaderIndex >= capacityPerShader.size() )
        {
            capacityPerShader.resize( shaderIndex + 1, 1 );
        }

        capacityPerShader[shaderIndex] += numClusters;
    }

    void MaterialShaderClusterCapacity::RemoveGlobalClusters( uint32_t numClusters )
    {
        EE_ASSERT( m_globalClusterCapacity > numClusters );
        m_globalClusterCapacity -= numClusters;
    }

    void MaterialShaderClusterCapacity::RemoveViewLayerClusters( size_t viewIndex, size_t shaderIndex, uint32_t numClusters )
    {
        TVector<uint32_t>& capacityPerShader = m_clusterCapacityPerViewPerShader[viewIndex];

        EE_ASSERT( capacityPerShader[shaderIndex] > numClusters );
        capacityPerShader[shaderIndex] -= numClusters;
    }

    void MaterialShaderClusterCapacity::Validate( size_t numShaders )
    {
        for ( TVector<uint32_t> & perShaderCapacity : m_clusterCapacityPerViewPerShader )
        {
            perShaderCapacity.resize( numShaders, 1 );

            for ( uint32_t capacity : perShaderCapacity )
            {
                EE_ASSERT( capacity <= m_globalClusterCapacity );
            }
        }
    }

    uint32_t MaterialShaderClusterCapacity::GetGlobalClusterCapacity() const
    {
        return uint32_t( m_globalClusterCapacity );
    }

    TArrayView<uint32_t const> MaterialShaderClusterCapacity::GetViewLayerClusterCapacity( ViewLayer viewLayer ) const
    {
        return m_clusterCapacityPerViewPerShader[uint32_t( viewLayer )];
    }
}
