#pragma once
#include "Engine/_Module/API.h"
#include "Base/Math/BoundingVolumes.h"
#include "Hitbox_Definition.h"
#include "Base/Math/Intersection.h"

//-------------------------------------------------------------------------
// HitBox
//-------------------------------------------------------------------------
// An AABB that defines the bounds for hit-detection for an object
// Contains a set of internal shapes for more fine grained detection

namespace EE
{
    class DebugDrawContext;
    class SpatialEntityComponent;
    namespace Render { class SkeletalMesh; }
    namespace Animation { class Pose; }

    //-------------------------------------------------------------------------

    class EE_ENGINE_API Hitbox
    {
    public:

        struct Hit
        {
            Hit() = default;
            Hit( HitboxShape const* pShape, float distance ) : m_pShape( pShape ), m_distance( distance ) { EE_ASSERT( m_pShape != nullptr && m_distance >= 0 ); }

            inline operator bool() const { return m_pShape != nullptr; }

            // Sort by severity and then distance
            inline bool operator<( Hit const& rhs ) const 
            {
                if ( m_pShape->m_severity == rhs.m_pShape->m_severity )
                {
                    return m_distance < rhs.m_distance;
                }

                return m_pShape->m_severity < rhs.m_pShape->m_severity;
            }

        public:

            HitboxShape const*      m_pShape = nullptr;
            float                   m_distance = 0.0f;
        };

        //-------------------------------------------------------------------------

        struct Shape
        {
        public:

            int32_t                 m_shapeIdx = InvalidIndex;
            HitboxShape const*      m_pShapeDef = nullptr;
            Transform               m_socketTransform;
            Transform               m_shapeTransform;
            AABB                    m_bounds;
            Vector                  m_velocity = Vector::Zero;
            float                   m_speed = 0.0f;
            bool                    m_isValid = false;
            bool                    m_wasValidLastUpdate = false;
        };

        struct Sphere : public Shape
        {
            inline Math::RaySphereResult CollideRay( Ray const& ray ) const
            {
                return Math::IntersectRaySphere( ray, m_center, m_radius );
            }

        public:

            Vector                  m_center = Vector::Zero;
            float                   m_radius = 0.0f;
        };

        struct Box final : public Shape
        {
            inline Math::RayBoxResult CollideRay( Ray const& ray ) const
            {
                return Math::IntersectRayBox( ray, m_OBB );
            }

        public:

            Vector                  m_halfSize = Vector::Zero;
            OBB                     m_OBB;
        };

        struct Capsule final : public Shape
        {
            inline Math::RayCapsuleResult CollideRay( Ray const& ray ) const
            {
                return Math::IntersectRayCapsule( ray, m_center0, m_center1, m_radius );
            }

        public:

            Vector                  m_center0 = Vector::Zero;
            Vector                  m_center1 = Vector::Zero;
            float                   m_radius = 0.0f;
        };

    public:

        Hitbox( HitboxDefinition const* pDefinition );
        ~Hitbox();

        // Update Transforms
        //-------------------------------------------------------------------------

        void Update( Seconds deltaTime, SpatialEntityComponent const* pComponent );
        void Update( Seconds deltaTime, Transform const& worldTransform, Animation::Pose const* pPose );

        // Queries
        //-------------------------------------------------------------------------

        // Get number of shapes in contained by the hitbox
        int32_t GetNumShapes() const { return (int32_t) m_pDefinition->m_shapes.size(); }

        // Get runtime shape based on meta-data
        TInlineVector<Shape const*, 10> FindMatchingShapes( StringID tag ) const;

        // Get the bounds for the hitbox, this will be invalid before the first update
        AABB const& GetBounds() const { return m_bounds; }

        // Get the center of the bounds for the hitbox, this will be invalid before the first update
        Vector const& GetBoundsCenter() const { return m_bounds.GetCenter(); }

        // Get the bounds for all the shapes of a given damage severity
        AABB GetBoundsForDamageSeverity( HitboxDamageSeverity severity ) const;

        // Get the bounds for all the shapes matching meta data
        inline AABB GetBoundsForMatchingTag( StringID tag ) const { return GetBoundsForMatchingTags( { tag } )[0]; }

        // Get the bounds for all the shapes matching meta data
        TInlineVector<AABB,4> GetBoundsForMatchingTags( TInlineVector<StringID, 4> const& tags ) const;

        // Get the average velocity for all the shapes matching meta data
        inline Vector GeAverageVelocityForMatchingTag( StringID tag ) const { return GeAverageVelocityForMatchingTags( { tag } )[0]; }

        // Get the average velocity for all the shapes matching meta data
        TInlineVector<Vector, 4> GeAverageVelocityForMatchingTags( TInlineVector<StringID, 4> const& tags ) const;

        // Collide a ray with the hitbox shapes
        bool CollideRay( Ray const& ray, TInlineVector<Hit, 10>& outHits ) const;

        // Collide a ray with the hitbox shapes
        inline bool CollideRay( Vector const& fromPoint, Vector const& toPoint, TInlineVector<Hit, 10>& outHits ) const { return CollideRay( Ray( Ray::StartEnd, fromPoint, toPoint ), outHits ); }

        // Debug
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        void DrawDebug( DebugDrawContext& ctx, bool drawTransforms = false ) const;
        #endif

    private:

        void UpdateShapesAndBounds( Seconds deltaTime );

    private:

        HitboxDefinition const*                         m_pDefinition = nullptr;
        uint8_t*                                        m_pShapeMemory = nullptr;
        TInlineVector<Shape*, 40>                       m_shapes;
        TInlineVector<Sphere*, 20>                      m_spheres;
        TInlineVector<Box*, 20>                         m_boxes;
        TInlineVector<Capsule*, 20>                     m_capsules;
        AABB                                            m_bounds;
    };
}