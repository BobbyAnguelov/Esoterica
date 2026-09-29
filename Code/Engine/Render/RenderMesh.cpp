#include "RenderMesh.h"
#include "Base/Drawing/DebugDrawing.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    void MeshStatistics::Accumulate( MeshStatistics const & src )
    {
        if ( src.m_numClusters == 0 )
        {
            return;
        }

        uint32_t const dstNumClusters = m_numClusters;
        uint64_t const totalClusters = uint64_t( dstNumClusters ) + src.m_numClusters;
        float const totalClustersF = float( totalClusters );

        m_averagePositionBitsPerAxisX = ( m_averagePositionBitsPerAxisX * dstNumClusters + src.m_averagePositionBitsPerAxisX * src.m_numClusters ) / totalClustersF;
        m_averagePositionBitsPerAxisY = ( m_averagePositionBitsPerAxisY * dstNumClusters + src.m_averagePositionBitsPerAxisY * src.m_numClusters ) / totalClustersF;
        m_averagePositionBitsPerAxisZ = ( m_averagePositionBitsPerAxisZ * dstNumClusters + src.m_averagePositionBitsPerAxisZ * src.m_numClusters ) / totalClustersF;

        // Medians are not exactly mergeable, approximate with the cluster weighted average
        m_medianPositionBitsPerAxisX = ( m_medianPositionBitsPerAxisX * dstNumClusters + src.m_medianPositionBitsPerAxisX * src.m_numClusters ) / totalClustersF;
        m_medianPositionBitsPerAxisY = ( m_medianPositionBitsPerAxisY * dstNumClusters + src.m_medianPositionBitsPerAxisY * src.m_numClusters ) / totalClustersF;
        m_medianPositionBitsPerAxisZ = ( m_medianPositionBitsPerAxisZ * dstNumClusters + src.m_medianPositionBitsPerAxisZ * src.m_numClusters ) / totalClustersF;

        m_averagePositionBitsPerVertex = ( m_averagePositionBitsPerVertex * dstNumClusters + src.m_averagePositionBitsPerVertex * src.m_numClusters ) / totalClustersF;
        m_averageClusterUtilization = ( m_averageClusterUtilization * dstNumClusters + src.m_averageClusterUtilization * src.m_numClusters ) / totalClustersF;
        m_averageCompressionAccuracy = ( m_averageCompressionAccuracy * dstNumClusters + src.m_averageCompressionAccuracy * src.m_numClusters ) / totalClustersF;

        // Medians are not exactly mergeable, approximate with the cluster weighted average
        m_medianClusterUtilization = ( m_medianClusterUtilization * dstNumClusters + src.m_medianClusterUtilization * src.m_numClusters ) / totalClustersF;

        if ( dstNumClusters > 0 )
        {
            m_minimumPositionBitsPerAxisX = Math::Min( m_minimumPositionBitsPerAxisX, src.m_minimumPositionBitsPerAxisX );
            m_minimumPositionBitsPerAxisY = Math::Min( m_minimumPositionBitsPerAxisY, src.m_minimumPositionBitsPerAxisY );
            m_minimumPositionBitsPerAxisZ = Math::Min( m_minimumPositionBitsPerAxisZ, src.m_minimumPositionBitsPerAxisZ );
            m_maximumPositionBitsPerAxisX = Math::Max( m_maximumPositionBitsPerAxisX, src.m_maximumPositionBitsPerAxisX );
            m_maximumPositionBitsPerAxisY = Math::Max( m_maximumPositionBitsPerAxisY, src.m_maximumPositionBitsPerAxisY );
            m_maximumPositionBitsPerAxisZ = Math::Max( m_maximumPositionBitsPerAxisZ, src.m_maximumPositionBitsPerAxisZ );
            m_minimumPositionBitsPerVertex = Math::Min( m_minimumPositionBitsPerVertex, src.m_minimumPositionBitsPerVertex );
            m_maximumPositionBitsPerVertex = Math::Max( m_maximumPositionBitsPerVertex, src.m_maximumPositionBitsPerVertex );
            m_minimumClusterUtilization = Math::Min( m_minimumClusterUtilization, src.m_minimumClusterUtilization );
            m_maximumClusterUtilization = Math::Max( m_maximumClusterUtilization, src.m_maximumClusterUtilization );
            m_minimumCompressionAccuracy = Math::Min( m_minimumCompressionAccuracy, src.m_minimumCompressionAccuracy );
        }
        else
        {
            m_minimumPositionBitsPerAxisX = src.m_minimumPositionBitsPerAxisX;
            m_minimumPositionBitsPerAxisY = src.m_minimumPositionBitsPerAxisY;
            m_minimumPositionBitsPerAxisZ = src.m_minimumPositionBitsPerAxisZ;
            m_maximumPositionBitsPerAxisX = src.m_maximumPositionBitsPerAxisX;
            m_maximumPositionBitsPerAxisY = src.m_maximumPositionBitsPerAxisY;
            m_maximumPositionBitsPerAxisZ = src.m_maximumPositionBitsPerAxisZ;
            m_minimumPositionBitsPerVertex = src.m_minimumPositionBitsPerVertex;
            m_maximumPositionBitsPerVertex = src.m_maximumPositionBitsPerVertex;
            m_minimumClusterUtilization = src.m_minimumClusterUtilization;
            m_maximumClusterUtilization = src.m_maximumClusterUtilization;
            m_minimumCompressionAccuracy = src.m_minimumCompressionAccuracy;
        }

        m_numClusters += src.m_numClusters;
        m_numVertices += src.m_numVertices;
        m_numTriangles += src.m_numTriangles;
        m_compressedSizeBytes += src.m_compressedSizeBytes;
        m_uncompressedSizeBytes += src.m_uncompressedSizeBytes;
        m_compressionRatio = m_uncompressedSizeBytes > 0 ? float( m_compressedSizeBytes ) / float( m_uncompressedSizeBytes ) : 0.0F;
        m_clusterVertexOverhead = 1.0F - m_averageClusterUtilization;
        m_vertexTriangleReuse = m_numVertices > 0 ? float( m_numTriangles ) * 3.0F / float( m_numVertices ) : 0.0F;
    }

    //-------------------------------------------------------------------------

    Mesh::Socket const* Mesh::GetSocket( StringID socketID ) const
    {
        for ( auto& socket : m_sockets )
        {
            if ( socket.m_ID == socketID )
            {
                return &socket;
            }
        }

        return nullptr;
    }

    //-------------------------------------------------------------------------

    bool SkeletalMesh::IsValid() const
    {
        return Mesh::IsValid() && ( m_boneIDs.size() == m_parentBoneIndices.size() ) && ( m_boneIDs.size() == m_modelSpaceBindPose.size() ) && ( m_boneIDs.size() == m_parentSpaceBindPose.size() ) && ( m_boneIDs.size() == m_modelSpaceInverseBindPose.size() );
    }

    int32_t SkeletalMesh::GetBoneIndex( StringID const& boneID ) const
    {
        size_t const numBones = m_boneIDs.size();
        for ( size_t i = 0; i < numBones; i++ )
        {
            if ( m_boneIDs[i] == boneID )
            {
                return (int32_t) i;
            }
        }

        return InvalidIndex;
    }

    bool SkeletalMesh::IsChildBoneOf( int32_t parentBoneIdx, int32_t childBoneIdx ) const
    {
        EE_ASSERT( IsValidBoneIndex( parentBoneIdx ) );
        EE_ASSERT( IsValidBoneIndex( childBoneIdx ) );

        bool isChild = false;

        int32_t actualParentBoneIdx = GetParentBoneIndex( childBoneIdx );
        while ( actualParentBoneIdx != InvalidIndex )
        {
            if ( actualParentBoneIdx == parentBoneIdx )
            {
                isChild = true;
                break;
            }

            actualParentBoneIdx = GetParentBoneIndex( actualParentBoneIdx );
        }

        return isChild;
    }

    #if EE_DEVELOPMENT_TOOLS
    void SkeletalMesh::DrawBindPose( DebugDrawContext& drawingContext, Transform const& worldTransform, bool drawBoneNames ) const
    {
        auto const numBones = GetNumBones();

        Transform boneWorldTransform = m_modelSpaceBindPose[0] * worldTransform;
        drawingContext.DrawBox( boneWorldTransform, Float3( 0.005f ), Colors::Orange );
        drawingContext.DrawAxis( boneWorldTransform, 0.05f );

        for ( auto i = 1; i < numBones; i++ )
        {
            boneWorldTransform = m_modelSpaceBindPose[i] * worldTransform;

            auto const parentBoneIdx = GetParentBoneIndex( i );
            Transform const parentBoneWorldTransform = m_modelSpaceBindPose[parentBoneIdx] * worldTransform;

            drawingContext.DrawLine( parentBoneWorldTransform.GetTranslation(), boneWorldTransform.GetTranslation(), Colors::Orange );
            drawingContext.DrawBox( boneWorldTransform, Float3( 0.005f ), Colors::Orange );
            drawingContext.DrawAxis( boneWorldTransform, 0.05f, 2.0f );

            if ( drawBoneNames )
            {
                drawingContext.DrawText3D( boneWorldTransform.GetTranslation(), GetBoneID( i ).c_str(), Colors::Orange, DebugFont::Small );
            }
        }
    }
    #endif
}
