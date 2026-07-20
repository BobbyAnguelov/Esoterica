#pragma once

#include "Engine/Entity/EntityIDs.h"
#include "Engine/ThirdParty/box3d/include/box3d/id.h"
#include "PhysicsCollisionSettings.h"
#include "Base/Math/Transform.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    class EE_ENGINE_API QueryRules
    {
        friend class PhysicsWorld;

    public:

        QueryRules() = default;

        QueryRules( ObjectCategory category )
            : m_category( 1ull << (uint8_t) category )
        {}

        QueryRules( QueryCategory category )
            : m_category( 1ull << (uint8_t) category )
        {}

        QueryRules( QueryRules const& rhs )
            : m_category( rhs.m_category )
            , m_collisionMask( rhs.m_collisionMask )
            , m_allowMultipleHits( rhs.m_allowMultipleHits )
            , m_calculateDepenetration( rhs.m_calculateDepenetration )
            , m_ignoredEntities( rhs.m_ignoredEntities )
            , m_ignoredComponents( rhs.m_ignoredComponents )
        {}

        void Reset()
        {
            m_category = 0;
            m_collisionMask = 0;
            m_ignoredComponents.clear();
            m_ignoredEntities.clear();
            m_allowMultipleHits = false;
        }

        // Collision Settings
        //-------------------------------------------------------------------------

        EE_FORCE_INLINE uint64_t GetCategory() const { return m_category; }
        EE_FORCE_INLINE void SetCategory( ObjectCategory category ) { m_category = ( 1ull << (uint8_t) category ); }
        EE_FORCE_INLINE void SetCategory( QueryCategory category ) { m_category = ( 1ull << (uint8_t) category ); }

        EE_FORCE_INLINE uint64_t GetCollisionMask() const { return m_collisionMask; }
        EE_FORCE_INLINE void ClearCollisionMask() { m_collisionMask = 0; }
        EE_FORCE_INLINE void SetCollisionMask( uint64_t mask ) { m_collisionMask = mask; }
        EE_FORCE_INLINE void SetCollidesWithAll() { m_collisionMask = ~0ULL; }

        template<typename... Args>
        QueryRules& SetCollidesWith( Args&&... args )
        {
            ( ( *this << static_cast<Args&&>( args ) ), ... );
            return *this;
        }

        template<typename... Args>
        QueryRules& ClearCollidesWith( Args&&... args )
        {
            ( ( *this >> static_cast<Args&&>( args ) ), ... );
            return *this;
        }

        // Query Settings
        //-------------------------------------------------------------------------

        EE_FORCE_INLINE bool IsMultipleHitQuery() const { return m_allowMultipleHits; }

        // Allow all detected hits along the length of the sweep/raycast - sorted by distance
        inline QueryRules& SetAllowMultipleHits( bool allowMultipleHits )
        {
            m_allowMultipleHits = allowMultipleHits;
            return *this;
        }

        // Should we calculate the depenetration information if we are initially overlapping during a sweep or for overlap queries
        inline QueryRules& SetCalculateDepenetrationInfo( bool calculateDepenetration )
        {
            EE_UNIMPLEMENTED_FUNCTION(); // MISSING CODE TO DO THIS IN BOX3D
            m_calculateDepenetration = calculateDepenetration;
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

        inline QueryRules& operator<<( ObjectCategory&& category )
        {
            m_collisionMask = SetCategoryBit( m_collisionMask, category );
            return *this;
        }

        inline QueryRules& operator<<( QueryCategory&& category )
        {
            m_collisionMask = SetCategoryBit( m_collisionMask, category );
            return *this;
        }

        inline QueryRules& operator>>( ObjectCategory&& category )
        {
            m_collisionMask = ClearCategoryBit( m_collisionMask, category );
            return *this;
        }

        inline QueryRules& operator>>( QueryCategory&& category )
        {
            m_collisionMask = ClearCategoryBit( m_collisionMask, category );
            return *this;
        }

    protected:

        uint64_t                        m_category = 0;
        uint64_t                        m_collisionMask = 0;
        bool                            m_allowMultipleHits = false;
        bool                            m_calculateDepenetration = false;
        TInlineVector<EntityID, 5>      m_ignoredEntities;
        TInlineVector<ComponentID, 5>   m_ignoredComponents;
    };

    //-------------------------------------------------------------------------

    struct CastQuery : public QueryRules
    {
        struct Hit
        {
            inline bool operator<( Hit const& rhs )const { return m_distance < rhs.m_distance; }

        public:

            b3BodyId            m_bodyID = {};
            b3ShapeId           m_shapeID = {};
            Vector              m_position; // The position of the shape at which the hit was detected (for rays this is == contact point)
            Vector              m_contactPoint; // The contact point on the shape we hit, if we are initially penetrating this is relatively meaningless
            Vector              m_contactNormal; 
            Vector              m_surfaceNormal; // TODO
            uint64_t            m_materialID; // The material we hit
            float               m_distance; // The distance to the collision, if we are initially penetrating, it is the depenetration distance
            bool                m_isInitiallyOverlapping; // Did we start the cast in collision (only valid for shape casts)
        };

        constexpr static int32_t const s_initialBufferSize = 5;

    public:

        CastQuery() = default;
        CastQuery( QueryRules const& rhs ) : QueryRules( rhs ) {}
        CastQuery( ObjectCategory category ) : QueryRules( category ) {}
        CastQuery( QueryCategory category ) : QueryRules( category ) {}

        // Did this cast start in collision
        inline bool IsInitiallyOverlapping() const { return HasHits() && m_hits[0].m_isInitiallyOverlapping; }

        // Did we hit anything?
        inline bool HasHits() const { return !m_hits.empty(); }

        // Get the first hit position
        EE_FORCE_INLINE Vector const& GetFirstHitPosition() const { EE_ASSERT( HasHits() ); return m_hits[0].m_contactPoint; }

        // Get the depenetration offset if initially penetrating
        inline Vector GetDepenetrationOffset() const
        {
            EE_ASSERT( IsInitiallyOverlapping() );
            return m_hits[0].m_contactNormal * m_hits[0].m_distance;
        }

        // Get the resulting shape end point (only valid for shape casts)
        inline Vector GetResultingEndPosition() const
        {
            return ( m_hits.empty() ) ? m_endPosition : m_hits[0].m_position;
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

        Vector                  m_startPosition = Vector::Zero; // The start point for the sweep
        Vector                  m_endPosition = Vector::Zero; // The desired end point for the sweep
        Vector                  m_direction = Vector::Zero; // The direction we swept in
        float                   m_desiredDistance = 0.0f; // How far did we want to sweep the shape
        float                   m_actualDistance = 0.0f; // How far did the sweep actually go
        float                   m_remainingDistance = 0.0f; // How much of the original desired distance is left
        bool                    m_hasInitialOverlap = false; // Did we start overlapping (only valid for shape casts)
        TInlineVector<Hit, 5>   m_hits;
    };

    //-------------------------------------------------------------------------

    struct OverlapQuery : public QueryRules
    {
        struct Overlap
        {
            b3BodyId            m_bodyID = {};
            b3ShapeId           m_shapeID = {};
            Vector              m_normal; // The depenetration normal
            float               m_distance; // The depenetration distance
        };

        constexpr static int32_t const s_initialBufferSize = 5;

    public:

        OverlapQuery() = default;
        OverlapQuery( QueryRules const& rhs ) : QueryRules( rhs ) {}
        OverlapQuery( ObjectCategory category ) : QueryRules( category ) {}
        OverlapQuery( QueryCategory category ) : QueryRules( category ) {}

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

        Transform                                       m_transform;
        TInlineVector<Overlap, s_initialBufferSize>     m_overlaps;
    };
}