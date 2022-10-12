#pragma once

#include "System/Math/BoundingVolumes.h"
#include "System/Types/Arrays.h"

//-------------------------------------------------------------------------

namespace EE::Drawing { class DrawContext; }

//-------------------------------------------------------------------------

namespace EE::Math
{
    class EE_SYSTEM_API AABBTree
    {
        struct Node
        {
        public:

            Node() = default;
            Node( AABB&& bounds, uint64_t userData = 0 ) : m_bounds( bounds ), m_volume( bounds.GetVolume() ), m_userData( userData ) {}
            Node( AABB const& bounds, uint64_t userData = 0 ) : m_bounds( bounds ), m_volume( bounds.GetVolume() ), m_userData( userData ) {}

            inline bool IsLeafNode() const { return m_rightNodeIdx == InvalidIndex; }

        public:

            AABB            m_bounds = AABB( Vector::Zero );

            int32_t         m_leftNodeIdx = InvalidIndex;
            int32_t         m_rightNodeIdx = InvalidIndex;
            int32_t         m_parentNodeIdx = InvalidIndex;
            float           m_volume = 0;

            uint64_t        m_userData = 0xFFFFFFFFFFFFFFFF;
            bool            m_isFree = true;
        };

    public:

        AABBTree();

        inline bool IsEmpty() const { return m_rootNodeIdx == InvalidIndex; }

        void InsertBox( AABB const& aabb, uint64_t userData );
        void RemoveBox( uint64_t userData );

        EE_FORCE_INLINE void InsertBox( AABB const& aabb, void* pUserData ) { InsertBox( aabb, reinterpret_cast<uint64_t>( pUserData ) ); }
        EE_FORCE_INLINE void RemoveBox( void* pUserData ) { RemoveBox( reinterpret_cast<uint64_t>( pUserData ) ); }

        bool FindOverlaps( AABB const& queryBox, TVector<uint64_t>& outResults ) const;

        template<typename T>
        bool FindOverlaps( AABB const& queryBox, TVector<T*>& outResults ) const
        {
            return FindOverlaps( queryBox, reinterpret_cast<TVector<uint64_t>&>( outResults ) );
        }

        #if EE_DEVELOPMENT_TOOLS
        void DrawDebug( Drawing::DrawContext& drawingContext ) const;
        #endif

    private:

        void InsertNode( int32_t leafNodeIdx, AABB const& newSiblingBox, uint64_t userData );
        void RemoveNode( int32_t nodeToRemoveIdx );
        void UpdateBranchNodeBounds( int32_t nodeIdx );

        int32_t RequestNode( AABB const& box, uint64_t userData = 0 );
        void ReleaseNode( int32_t nodeIdx );

        int32_t FindBestLeafNodeToCreateSiblingFor( int32_t startNodeIdx, AABB const& newBox ) const;
        void FindAllOverlappingLeafNodes( int32_t currentNodeIdx, AABB const& queryBox, TVector<uint64_t>& outResults ) const;
        void FindAllOverlappingLeafNodes( int32_t currentNodeIdx, OBB const& queryBox, TVector<uint64_t>& outResults ) const;

        #if EE_DEVELOPMENT_TOOLS
        void DrawBranch( Drawing::DrawContext& drawingContext, int32_t nodeIdx ) const;
        void DrawLeaf( Drawing::DrawContext& drawingContext, int32_t nodeIdx ) const;
        #endif

    private:

        TVector<Node>       m_nodes;
        int32_t               m_rootNodeIdx = InvalidIndex;
        int32_t               m_freeNodeIdx = 0;
    };
}

