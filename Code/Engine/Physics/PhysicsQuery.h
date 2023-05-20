#pragma once
#include "Engine/_Module/API.h"
#include "Engine/Physics/PhysicsSettings.h"
#include "Engine/Entity/EntityIDs.h"
#include "System/Math/Vector.h"

#include <PxQueryFiltering.h>

//-------------------------------------------------------------------------

namespace EE::Physics
{
    namespace PX { class QueryFilter; }

    //-------------------------------------------------------------------------
    // Query Rules
    //-------------------------------------------------------------------------

    class EE_ENGINE_API QueryRules
    {
        friend class PhysicsWorld;
        friend PX::QueryFilter;

    public:

        enum MobilityFilter : uint8_t
        {
            StaticAndDynamic,
            OnlyStatic,
            OnlyDynamic
        };

    public:

        QueryRules()
        {
            UpdateInternals();
        }

        QueryRules( uint32_t collidesWithMask ) : m_collidesWithMask( collidesWithMask )
        {
            UpdateInternals();
        }

        void Reset();

        // Collision Settings
        //-------------------------------------------------------------------------

        EE_FORCE_INLINE void ClearCollidesWithMask()
        {
            m_collidesWithMask = 0;
            UpdateInternals();
        }

        EE_FORCE_INLINE QueryRules& SetCollidesWith( CollisionCategory colliderType, bool value = true )
        {
            m_collidesWithMask |= ( 1u << (uint8_t) colliderType );
            UpdateInternals();
            return *this;
        }

        EE_FORCE_INLINE QueryRules& SetCollidesWith( QueryChannel queryChannel, bool value = true )
        {
            m_collidesWithMask |= ( 1u << (uint8_t) queryChannel );
            UpdateInternals();
            return *this;
        }

        EE_FORCE_INLINE QueryRules& SetCollidesWith( MobilityFilter filter )
        {
            m_mobilityFilter = filter;
            UpdateInternals();
            return *this;
        }

        // Query Settings
        //-------------------------------------------------------------------------

        // Allow all detected hits along the length of the sweep/raycast - sorted by distance
        inline QueryRules& SetAllowMultipleHits( bool allowMultipleHits )
        {
            m_allowMultipleHits = allowMultipleHits;
            UpdateInternals();
            return *this;
        }

        // Should we calculate the depenetration information if we are initially overlapping during a sweep or for overlap queries
        inline QueryRules& SetCalculateDepenetrationInfo( bool calculateDepenetration )
        {
            m_calculateDepenetration = calculateDepenetration;
            UpdateInternals();
            return *this;
        }

        // Ignore Rules
        //-------------------------------------------------------------------------

        inline TInlineVector<EntityID, 5> const& GetIgnoredEntities() const { return m_ignoredEntities; }

        inline bool IsEntityIgnored( EntityID ID ) const { return VectorContains( m_ignoredEntities, ID ); }

        inline QueryRules& AddIgnoredEntity( EntityID const& ID ) { m_ignoredEntities.emplace_back( ID ); return *this; }

        inline QueryRules& RemoveIgnoredEntity( EntityID const& ID ) { m_ignoredEntities.erase_first_unsorted( ID ); return *this; }

        inline QueryRules& ClearIgnoredEntities() { m_ignoredEntities.clear(); return *this; }

        //-------------------------------------------------------------------------

        inline TInlineVector<ComponentID, 5> const& GetIgnoredComponents() const { return m_ignoredComponents; }

        inline bool IsComponentIgnored( ComponentID componentID ) const { return VectorContains( m_ignoredComponents, componentID ); }

        inline QueryRules& AddIgnoredComponent( ComponentID componentID ) { m_ignoredComponents.emplace_back( componentID ); return *this; }

        inline QueryRules& RemoveIgnoredComponent( ComponentID componentID ) { m_ignoredComponents.erase_first_unsorted( componentID ); return *this; }

        inline QueryRules& ClearIgnoredComponents() { m_ignoredComponents.clear(); return *this; }

    private:

        void UpdateInternals();

    private:

        uint32_t                        m_collidesWithMask = 0;
        bool                            m_allowMultipleHits = false;
        bool                            m_calculateDepenetration = true;
        MobilityFilter                  m_mobilityFilter = StaticAndDynamic;
        TInlineVector<EntityID, 5>      m_ignoredEntities;
        TInlineVector<ComponentID, 5>   m_ignoredComponents;

        physx::PxQueryFilterData        m_queryFilterData;
        physx::PxHitFlags               m_hitFlags;
    };

    //-------------------------------------------------------------------------
    // Query Results
    //-------------------------------------------------------------------------

    struct RayCastResults
    {
        struct Hit
        {
            physx::PxActor*             m_pActor = nullptr;
            physx::PxShape*             m_pShape = nullptr;
            Vector                      m_contactPoint; // The contact point on the body we hit
            Vector                      m_normal; // The surface normal on the body we hit
            float                       m_distance; // The distance to the collision
        };

        constexpr static int32_t const s_initialBufferSize = 5;

    public:

        void Reset()
        {
            m_startPosition = m_direction = Vector::Zero;
            m_desiredDistance = m_actualDistance = m_remainingDistance = 0.0f;
            m_hits.clear();
        }

        inline bool HasHits() const { return !m_hits.empty(); }

        // Operators
        //-------------------------------------------------------------------------

        EE_FORCE_INLINE TInlineVector<Hit, s_initialBufferSize>::iterator begin() { return m_hits.begin(); }
        EE_FORCE_INLINE TInlineVector<Hit, s_initialBufferSize>::iterator end() { return m_hits.end(); }
        EE_FORCE_INLINE TInlineVector<Hit, s_initialBufferSize>::const_iterator begin() const { return m_hits.begin(); }
        EE_FORCE_INLINE TInlineVector<Hit, s_initialBufferSize>::const_iterator end() const { return m_hits.end(); }
        EE_FORCE_INLINE Hit& operator[]( uint32_t i ) { EE_ASSERT( i < m_hits.size() ); return m_hits[i]; }
        EE_FORCE_INLINE Hit const& operator[]( uint32_t i ) const { EE_ASSERT( i < m_hits.size() ); return m_hits[i]; }

    public:

        Vector                  m_startPosition = Vector::Zero; // The start point of the ray
        Vector                  m_direction = Vector::Zero; // The direction we cast the ray in
        float                   m_desiredDistance = 0.0f; // How far did we want to ray cast
        float                   m_actualDistance = 0.0f; // How far did the ray cast actually go
        float                   m_remainingDistance = 0.0f; // How much of the original desired distance is left
        TInlineVector<Hit, 5>   m_hits;
    };

    //-------------------------------------------------------------------------

    struct SweepResults
    {
        struct Hit
        {
            physx::PxActor*     m_pActor = nullptr;
            physx::PxShape*     m_pShape = nullptr;
            Vector              m_shapePosition; // The position of the shape at which the hit was hit detected
            Vector              m_contactPoint; // The contact point on the shape we hit, if we are initially penetrating this is relatively meaningless
            Vector              m_normal; // This is often the surface normal for the hit shape but if we are initially penetrating, it is the depenetration normal
            float               m_distance; // The distance to the collision, if we are initially penetrating, it is the depenetration distance
            bool                m_isInitiallyOverlapping; // Did we start the cast in collision
        };

        constexpr static int32_t const s_initialBufferSize = 5;

    public:

        void Reset()
        {
            m_sweepStartPosition = m_sweepDirection = m_sweepEndPosition = Vector::Zero;
            m_desiredDistance = m_actualDistance = m_remainingDistance = 0.0f;
            m_hits.clear();
        }

        // Did this cast start in collision
        inline bool IsInitiallyOverlapping() const { return HasHits() && m_hits[0].m_isInitiallyOverlapping; }

        // Did we hit anything?
        inline bool HasHits() const { return !m_hits.empty(); }

        // Get the depenetration offset if initially penetrating
        inline Vector GetDepenetrationOffset() const 
        {
            EE_ASSERT( IsInitiallyOverlapping() );
            return m_hits[0].m_normal * m_hits[0].m_distance;
        }

        // Get the resulting shape end point
        inline Vector GetResultingEndPosition() const
        {
            return ( m_hits.empty() ) ? m_sweepEndPosition : m_hits[0].m_shapePosition;
        }

        // Operators
        //-------------------------------------------------------------------------

        EE_FORCE_INLINE TInlineVector<Hit, s_initialBufferSize>::iterator begin() { return m_hits.begin(); }
        EE_FORCE_INLINE TInlineVector<Hit, s_initialBufferSize>::iterator end() { return m_hits.end(); }
        EE_FORCE_INLINE TInlineVector<Hit, s_initialBufferSize>::const_iterator begin() const { return m_hits.begin(); }
        EE_FORCE_INLINE TInlineVector<Hit, s_initialBufferSize>::const_iterator end() const { return m_hits.end(); }
        EE_FORCE_INLINE Hit& operator[]( uint32_t i ) { EE_ASSERT( i < m_hits.size() ); return m_hits[i]; }
        EE_FORCE_INLINE Hit const& operator[]( uint32_t i ) const { EE_ASSERT( i < m_hits.size() ); return m_hits[i]; }

    public:

        Vector                  m_sweepStartPosition = Vector::Zero; // The start point for the sweep
        Vector                  m_sweepDirection = Vector::Zero; // The direction we swept in
        Vector                  m_sweepEndPosition = Vector::Zero; // The desired end point for the sweep
        float                   m_desiredDistance = 0.0f; // How far did we want to sweep the shape
        float                   m_actualDistance = 0.0f; // How far did the sweep actually go
        float                   m_remainingDistance = 0.0f; // How much of the original desired distance is left
        bool                    m_hasInitialOverlap = false;
        TInlineVector<Hit, 5>   m_hits;
    };

    //-------------------------------------------------------------------------

    struct OverlapResults
    {
        struct Overlap
        {
            physx::PxActor*     m_pActor = nullptr;
            physx::PxShape*     m_pShape = nullptr;
            Vector              m_normal; // The depenetration normal
            float               m_distance; // The depenetration distance
        };

        constexpr static int32_t const s_initialBufferSize = 5;

    public:

        inline bool HasOverlaps() const { return !m_overlaps.empty(); }

        // Operators
        //-------------------------------------------------------------------------

        EE_FORCE_INLINE TInlineVector<Overlap, s_initialBufferSize>::iterator begin() { return m_overlaps.begin(); }
        EE_FORCE_INLINE TInlineVector<Overlap, s_initialBufferSize>::iterator end() { return m_overlaps.end(); }
        EE_FORCE_INLINE TInlineVector<Overlap, s_initialBufferSize>::const_iterator begin() const { return m_overlaps.begin(); }
        EE_FORCE_INLINE TInlineVector<Overlap, s_initialBufferSize>::const_iterator end() const { return m_overlaps.end(); }
        EE_FORCE_INLINE Overlap& operator[]( uint32_t i ) { EE_ASSERT( i < m_overlaps.size() ); return m_overlaps[i]; }
        EE_FORCE_INLINE Overlap const& operator[]( uint32_t i ) const { EE_ASSERT( i < m_overlaps.size() ); return m_overlaps[i]; }

    public:

        Vector                                          m_overlapPosition = Vector::Zero;
        TInlineVector<Overlap, s_initialBufferSize>     m_overlaps;
    };
}