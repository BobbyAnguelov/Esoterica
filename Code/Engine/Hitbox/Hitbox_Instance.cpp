#include "Hitbox_Instance.h"
#include "Engine/Animation/AnimationPose.h"
#include "Engine/Entity/EntitySpatialComponent.h"
#include "Base/Drawing/DebugDrawing.h"

#include "EASTL/sort.h"

//-------------------------------------------------------------------------

namespace EE
{
    Hitbox::Hitbox( HitboxDefinition const* pDefinition )
        : m_pDefinition( pDefinition )
    {
        EE_ASSERT( pDefinition != nullptr );
        EE_ASSERT( pDefinition->m_instanceShapesStartOffsets.size() == GetNumShapes() );

        m_pShapeMemory = reinterpret_cast<uint8_t*>( EE::Alloc( pDefinition->m_instanceRequiredMemory, pDefinition->m_instanceRequiredAlignment ) );

        int32_t const numShapes = GetNumShapes();
        for ( int32_t i = 0; i < numShapes; i++ )
        {
            switch ( m_pDefinition->m_shapes[i].m_type )
            {
                case HitboxShape::Type::Box:
                {
                    auto pBox = new ( m_pShapeMemory + m_pDefinition->m_instanceShapesStartOffsets[i] ) Box();
                    pBox->m_shapeIdx = i;
                    pBox->m_pShapeDef = &m_pDefinition->m_shapes[i];

                    m_shapes.emplace_back( pBox );
                    m_boxes.emplace_back( pBox );
                }
                break;

                case HitboxShape::Type::Sphere:
                {
                    auto pSphere = new ( m_pShapeMemory + m_pDefinition->m_instanceShapesStartOffsets[i] ) Sphere();
                    pSphere->m_shapeIdx = i;
                    pSphere->m_pShapeDef = &m_pDefinition->m_shapes[i];
                    pSphere->m_radius = m_pDefinition->m_shapes[i].m_radius;

                    m_shapes.emplace_back( pSphere );
                    m_spheres.emplace_back( pSphere );
                }
                break;

                case HitboxShape::Type::Capsule:
                {
                    auto pCapsule = new ( m_pShapeMemory + m_pDefinition->m_instanceShapesStartOffsets[i] ) Capsule();
                    pCapsule->m_shapeIdx = i;
                    pCapsule->m_pShapeDef = &m_pDefinition->m_shapes[i];
                    pCapsule->m_radius = m_pDefinition->m_shapes[i].m_radius;

                    m_shapes.emplace_back( pCapsule );
                    m_capsules.emplace_back( pCapsule );
                }
                break;
            }
        }
    }

    Hitbox::~Hitbox()
    {
        for ( auto const& pShape : m_boxes )
        {
            pShape->~Box();
        }

        //-------------------------------------------------------------------------

        for ( auto const& pShape : m_spheres )
        {
            pShape->~Sphere();
        }

        //-------------------------------------------------------------------------

        for ( auto const& pShape : m_capsules )
        {
            pShape->~Capsule();
        }

        EE::Free( m_pShapeMemory );
    }

    // Update
    //-------------------------------------------------------------------------

    void Hitbox::Update( Seconds deltaTime, SpatialEntityComponent const* pComponent )
    {
        EE_ASSERT( pComponent != nullptr && pComponent->IsInitialized() );

        int32_t const numShapes = GetNumShapes();
        for ( int32_t i = 0; i < numShapes; i++ )
        {
            Transform socketTransform;
            bool const wasSocketFound = pComponent->TryGetAttachmentSocketTransform( m_pDefinition->m_shapes[i].m_socketID, m_shapes[i]->m_socketTransform );
            if ( wasSocketFound )
            {
                m_shapes[i]->m_wasValidLastUpdate = m_shapes[i]->m_isValid;
                m_shapes[i]->m_isValid = true;
            }
            else
            {
                m_shapes[i]->m_isValid = false;
                m_shapes[i]->m_wasValidLastUpdate = false;
            }
        }

        UpdateShapesAndBounds( deltaTime );
    }

    void Hitbox::Update( Seconds deltaTime, Transform const& worldTransform, Animation::Pose const* pPose )
    {
        EE_ASSERT( pPose != nullptr && pPose->IsPoseSet() );
        EE_ASSERT( pPose->HasModelSpaceTransforms() );

        //-------------------------------------------------------------------------

        int32_t const numShapes = GetNumShapes();
        for ( int32_t i = 0; i < numShapes; i++ )
        {
            auto pSkeleton = pPose->GetSkeleton();
            int32_t const boneIndex = pSkeleton->GetBoneIndex( m_pDefinition->m_shapes[i].m_socketID );
            if ( boneIndex != InvalidIndex )
            {
                m_shapes[i]->m_wasValidLastUpdate = m_shapes[i]->m_isValid;
                m_shapes[i]->m_isValid = true;
                m_shapes[i]->m_socketTransform = pPose->GetModelSpaceTransform( boneIndex ) * worldTransform;
            }
            else
            {
                m_shapes[i]->m_isValid = false;
                m_shapes[i]->m_wasValidLastUpdate = false;
                m_shapes[i]->m_socketTransform = worldTransform;
            }
        }

        UpdateShapesAndBounds( deltaTime );
    }

    TInlineVector<Hitbox::Shape const*, 10> Hitbox::FindMatchingShapes( StringID tag ) const
    {
        TInlineVector<Hitbox::Shape const*, 10> matchingShapes;

        for ( auto const& pShape : m_shapes )
        {
            if ( pShape->m_pShapeDef->HasMatchingTag( tag ) )
            {
                matchingShapes.emplace_back( pShape );
            }
        }

        //-------------------------------------------------------------------------

        return matchingShapes;
    }

    void Hitbox::UpdateShapesAndBounds( Seconds deltaTime )
    {
        Vector hitboxBoundsMin( FLT_MAX );
        Vector hitboxBoundsMax( -FLT_MAX );

        int32_t const numShapes = GetNumShapes();
        for ( int32_t i = 0; i < numShapes; i++ )
        {
            auto pShape = m_shapes[i];
            if ( !pShape->m_isValid )
            {
                continue;
            }

            auto const& shapeDef = m_pDefinition->m_shapes[i];

            // Calculate new shape transform and velocity
            //-------------------------------------------------------------------------

            Vector const vDeltaTime( deltaTime.ToFloat() );
            Transform const newShapeTransform = shapeDef.m_offset * pShape->m_socketTransform;

            if ( pShape->m_wasValidLastUpdate )
            {
                pShape->m_velocity = ( newShapeTransform.GetTranslation() - pShape->m_shapeTransform.GetTranslation() ) / vDeltaTime;
                pShape->m_speed = pShape->m_velocity.GetLength3();
            }
            else
            {
                pShape->m_velocity = Vector::Zero;
                pShape->m_speed = 0.0f;
            }

            // Update bounds
            //-------------------------------------------------------------------------

            switch ( shapeDef.m_type )
            {
                case HitboxShape::Type::Box:
                {
                    auto pBox = static_cast<Box*>( pShape );
                    pBox->m_shapeTransform = newShapeTransform;
                    pBox->m_halfSize = shapeDef.m_boxExtents;
                    pBox->m_OBB = OBB( pBox->m_shapeTransform.GetTranslation(), shapeDef.m_boxExtents, pBox->m_shapeTransform.GetRotation() );
                    pBox->m_bounds = AABB( pBox->m_OBB );

                    hitboxBoundsMin = Vector::Min( hitboxBoundsMin, pBox->m_bounds.GetMin() );
                    hitboxBoundsMax = Vector::Max( hitboxBoundsMax, pBox->m_bounds.GetMax() );
                }
                break;

                case HitboxShape::Type::Sphere:
                {
                    auto pSphere = static_cast<Sphere*>( pShape );
                    pSphere->m_shapeTransform = newShapeTransform;
                    pSphere->m_center = pSphere->m_shapeTransform.GetTranslation();
                    pSphere->m_bounds = AABB( pSphere->m_center, Vector( pSphere->m_radius ) );

                    hitboxBoundsMin = Vector::Min( hitboxBoundsMin, pSphere->m_bounds.GetMin() );
                    hitboxBoundsMax = Vector::Max( hitboxBoundsMax, pSphere->m_bounds.GetMax() );
                }
                break;

                case HitboxShape::Type::Capsule:
                {
                    auto pCapsule = static_cast<Capsule*>( pShape );
                    pCapsule->m_shapeTransform = newShapeTransform;
                    pCapsule->m_center0 = pCapsule->m_shapeTransform.TransformPoint( Vector( 0, 0, shapeDef.m_halfHeight ) );
                    pCapsule->m_center1 = pCapsule->m_shapeTransform.TransformPoint( Vector( 0, 0, -shapeDef.m_halfHeight ) );

                    Vector const vRadius( pCapsule->m_radius );
                    Vector const capsuleBoundsMin = Vector::Min( pCapsule->m_center0 - vRadius, pCapsule->m_center1 - vRadius ).GetWithW0();
                    Vector const capsuleBoundsMax = Vector::Max( pCapsule->m_center0 + vRadius, pCapsule->m_center1 + vRadius ).GetWithW0();
                    pCapsule->m_bounds = AABB::FromMinMax( capsuleBoundsMin, capsuleBoundsMax );

                    hitboxBoundsMin = Vector::Min( hitboxBoundsMin, capsuleBoundsMin );
                    hitboxBoundsMax = Vector::Max( hitboxBoundsMax, capsuleBoundsMax );
                }
                break;
            }
        }

        // Overall bounds
        //-------------------------------------------------------------------------

        if ( hitboxBoundsMax.IsGreaterThan3( hitboxBoundsMin ) )
        {
            m_bounds = AABB::FromMinMax( hitboxBoundsMin, hitboxBoundsMax );
        }
        else
        {
            m_bounds.Reset();
        }
    }

    AABB Hitbox::GetBoundsForDamageSeverity( HitboxDamageSeverity severity ) const
    {
        Vector severityBoundsMin( FLT_MAX );
        Vector severityBoundsMax( -FLT_MAX );

        for ( auto const& pShape : m_shapes )
        {
            if ( pShape->m_pShapeDef->m_severity != severity )
            {
                continue;
            }

            severityBoundsMin = Vector::Min( severityBoundsMin, pShape->m_bounds.GetMin() );
            severityBoundsMax = Vector::Max( severityBoundsMax, pShape->m_bounds.GetMax() );
        }

        // Overall bounds
        //-------------------------------------------------------------------------

        AABB bounds;

        if ( severityBoundsMax.IsGreaterThan3( severityBoundsMin ) )
        {
            bounds = AABB::FromMinMax( severityBoundsMin, severityBoundsMax );
        }

        return bounds;
    }

    TInlineVector<EE::AABB, 4> Hitbox::GetBoundsForMatchingTags( TInlineVector<StringID, 4> const& tags ) const
    {
        struct Bounds
        {
            Vector m_min = Vector( FLT_MAX );
            Vector m_max = Vector( -FLT_MAX );
        };

        size_t const numTags = tags.size();
        EE_ASSERT( numTags > 0 );
        TInlineVector<Bounds, 4> bounds( numTags );

        for ( auto const& pShape : m_shapes )
        {
            for ( size_t t = 0; t < numTags; t++ )
            {
                if ( pShape->m_pShapeDef->HasMatchingTag( tags[t] ) )
                {
                    bounds[t].m_min = Vector::Min( bounds[t].m_min, pShape->m_bounds.GetMin() );
                    bounds[t].m_max = Vector::Max( bounds[t].m_max, pShape->m_bounds.GetMax() );
                }
            }
        }

        // Overall bounds
        //-------------------------------------------------------------------------

        TInlineVector<EE::AABB, 4> boxes( numTags );

        for ( size_t t = 0; t < numTags; t++ )
        {
            if ( bounds[t].m_max.IsGreaterThan3( bounds[t].m_min ) )
            {
                boxes[t] = AABB::FromMinMax( bounds[t].m_min, bounds[t].m_max );
            }
        }

        return boxes;
    }

    TInlineVector<Vector, 4> Hitbox::GeAverageVelocityForMatchingTags( TInlineVector<StringID, 4> const& tags ) const
    {
        size_t const numTags = tags.size();
        EE_ASSERT( numTags > 0 );

        TInlineVector<Vector, 4> velocities;
        velocities.resize( numTags, Vector::Zero );

        for ( auto const& pShape : m_shapes )
        {
            for ( size_t t = 0; t < numTags; t++ )
            {
                if ( pShape->m_pShapeDef->HasMatchingTag( tags[t] ) )
                {
                    velocities[t] += pShape->m_velocity;
                }
            }
        }

        // Overall bounds
        //-------------------------------------------------------------------------

        for ( size_t t = 0; t < numTags; t++ )
        {
            velocities[t] /= (float) numTags;
        }

        return velocities;
    }

    // Queries
    //-------------------------------------------------------------------------

    bool Hitbox::CollideRay( Ray const& ray, TInlineVector<Hit, 10>& outHits ) const
    {
        outHits.clear();

        if ( !m_bounds.IsValid() )
        {
            return false;
        }

        if ( !Math::IntersectRayBox( ray, m_bounds ) )
        {
            return false;
        }

        // Check each individual shape
        //-------------------------------------------------------------------------

        for ( auto const& pShape : m_boxes )
        {
            if ( !pShape->m_isValid )
            {
                continue;
            }

            auto const &shapeDef = m_pDefinition->m_shapes[pShape->m_shapeIdx];

            Math::RayBoxResult const result = Math::IntersectRayBox( ray, pShape->m_OBB );
            if ( result )
            {
                outHits.emplace_back( Hit( &shapeDef, result.m_T ) );
            }
        }

        //-------------------------------------------------------------------------

        for ( auto const& pShape : m_spheres )
        {
            if ( !pShape->m_isValid )
            {
                continue;
            }

            auto const &shapeDef = m_pDefinition->m_shapes[pShape->m_shapeIdx];

            Math::RaySphereResult const result = Math::IntersectRaySphere( ray, pShape->m_center, pShape->m_radius );
            if ( result )
            {
                outHits.emplace_back( &shapeDef, result.m_intersectionDistance0 );
            }
        }

        //-------------------------------------------------------------------------

        for ( auto const& pShape : m_capsules )
        {
            if ( !pShape->m_isValid )
            {
                continue;
            }

            auto const &shapeDef = m_pDefinition->m_shapes[pShape->m_shapeIdx];

            Math::RayCapsuleResult const result = Math::IntersectRayCapsule( ray, pShape->m_center0, pShape->m_center1, pShape->m_radius );
            if ( result )
            {
                outHits.emplace_back( &shapeDef, result.m_T );
            }
        }

        // Sort hits by distance and severity
        //-------------------------------------------------------------------------

        eastl::sort( outHits.begin(), outHits.end() );

        return !outHits.empty();
    }

    // Debug
    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    void Hitbox::DrawDebug( DebugDrawContext& ctx, bool drawTransforms ) const
    {
        if ( m_bounds.IsValid() )
        {
            ctx.DrawWireBox( m_bounds, Colors::Cyan );
        }

        //-------------------------------------------------------------------------

        for ( auto const& pShape : m_boxes )
        {
            if ( !pShape->m_isValid )
            {
                continue;
            }

            auto const &shapeDef = m_pDefinition->m_shapes[pShape->m_shapeIdx];

            if ( drawTransforms )
            {
                Transform offsetTransform = shapeDef.m_offset * pShape->m_socketTransform;
                InlineString str( InlineString::CtorSprintf(), "%d", pShape->m_shapeIdx );

                ctx.DrawLine( offsetTransform.GetTranslation(), pShape->m_socketTransform.GetTranslation(), Colors::White );
                ctx.DrawText3D( offsetTransform.GetTranslation(), str.c_str(), Colors::Cyan );
                ctx.DrawAxis( offsetTransform );
            }

            ctx.DrawWireBox( pShape->m_OBB, HitboxShape::GetSeverityColor( shapeDef.m_severity ).GetAlphaVersion( 0.55f ) );
        }

        //-------------------------------------------------------------------------

        for ( auto const& pShape : m_spheres )
        {
            if ( !pShape->m_isValid )
            {
                continue;
            }

            auto const &shapeDef = m_pDefinition->m_shapes[pShape->m_shapeIdx];

            if ( drawTransforms )
            {
                Transform offsetTransform = shapeDef.m_offset * pShape->m_socketTransform;
                InlineString str( InlineString::CtorSprintf(), "%d", pShape->m_shapeIdx );

                ctx.DrawLine( offsetTransform.GetTranslation(), pShape->m_socketTransform.GetTranslation(), Colors::White );
                ctx.DrawText3D( offsetTransform.GetTranslation(), str.c_str(), Colors::Cyan );
                ctx.DrawAxis( offsetTransform );
            }

            ctx.DrawWireSphere( pShape->m_center, pShape->m_radius, HitboxShape::GetSeverityColor( shapeDef.m_severity ).GetAlphaVersion( 0.55f ) );
        }

        //-------------------------------------------------------------------------

        for ( auto const& pShape : m_capsules )
        {
            if ( !pShape->m_isValid )
            {
                continue;
            }

            auto const &shapeDef = m_pDefinition->m_shapes[pShape->m_shapeIdx];

            if ( drawTransforms )
            {
                Transform offsetTransform = shapeDef.m_offset * pShape->m_socketTransform;
                InlineString str( InlineString::CtorSprintf(), "%d", pShape->m_shapeIdx );

                ctx.DrawLine( offsetTransform.GetTranslation(), pShape->m_socketTransform.GetTranslation(), Colors::White );
                ctx.DrawText3D( offsetTransform.GetTranslation(), str.c_str(), Colors::Cyan );
                ctx.DrawAxis( offsetTransform );
            }

            ctx.DrawWireCapsule( pShape->m_center0, pShape->m_center1, pShape->m_radius, HitboxShape::GetSeverityColor( shapeDef.m_severity ).GetAlphaVersion( 0.55f ) );
        }
    }
    #endif

    

}