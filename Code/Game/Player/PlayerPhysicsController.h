#pragma once

#include "Engine/Entity/EntityIDs.h"
#include "Engine/Physics/PhysicsQuery.h"
#include "Engine/Physics/PhysicsLayers.h"
#include "System/Types/Arrays.h"
#include "System/Time/Timers.h"

//-------------------------------------------------------------------------

namespace EE
{
    class EntityWorldUpdateContext;
}

namespace EE::Physics
{
    class CharacterComponent;
    class Scene;
}

//-------------------------------------------------------------------------

namespace EE::Player
{
    class CharacterPhysicsController final
    {
        struct Settings
        {
            float                               m_gravitationalAcceleration = 30.f;
            float                               m_stepHeight = 0.4f;

            Radians                             m_wallSlideAngle = Radians( Degrees( 60 ) );
            Radians                             m_maxNavigableSlopeAngle = Radians( Degrees( 45 ) );

            uint32_t                            m_physicsLayerMask = Physics::CreateLayerMask( Physics::Layers::Environment, Physics::Layers::Characters );
            TInlineVector<EntityID, 5>          m_ignoredActors;
        };

        struct MoveResult
        {
        public:

            MoveResult( Vector const& position )
                : m_initialPosition( position )
                , m_correctedPosition( position )
                , m_finalPosition( position )
                , m_remainingDistance( 0.0f )
            {}

            inline Vector const& GetInitialPosition() const { return m_initialPosition; }
            inline Vector const& GetCorrectedPosition() const { return m_correctedPosition; }
            inline Vector const& GetFinalPosition() const { return m_finalPosition; }
            inline float GetRemainingDistance() const { return m_remainingDistance; }
            inline Physics::SweepResults const& GetSweepResults() const { return m_sweepResults; }

            inline void FinalizePosition( Physics::SweepResults sweepResults, Vector offset = Vector::Zero )
            {
                m_sweepResults = sweepResults;
                if( sweepResults.hasBlock )
                {
                    m_finalPosition = sweepResults.GetShapePosition() + offset;
                    m_remainingDistance = sweepResults.GetRemainingDistance();
                }
                else
                {
                    m_finalPosition = sweepResults.m_sweepEnd + offset;
                    m_remainingDistance = 0.0f;
                }
            }

            inline void ApplyCorrectiveMove( MoveResult const& correctiveMove )
            {
                m_correctedPosition     = correctiveMove.m_correctedPosition;
                m_finalPosition         = correctiveMove.m_finalPosition;
                m_remainingDistance     = correctiveMove.m_remainingDistance;
                m_sweepResults          = correctiveMove.m_sweepResults;
            }

            inline void ApplySubsequentMove( MoveResult const& SubsequentMove, Vector offset = Vector::Zero )
            {
                m_finalPosition         = SubsequentMove.m_finalPosition + offset;
                m_remainingDistance     = SubsequentMove.m_remainingDistance;
                m_sweepResults          = SubsequentMove.m_sweepResults;
            }

        private:

            Vector                  m_initialPosition = Vector::Zero;
            Vector                  m_correctedPosition = Vector::Zero;
            Vector                  m_finalPosition = Vector::Zero;
            float                   m_remainingDistance = 0.0f;
            Physics::SweepResults   m_sweepResults;
        };

        #if EE_DEVELOPMENT_TOOLS
        enum class DebugSweepResultType
        {
            success,
            reprojection,
            collision,
            initialPenetration,
        };

        enum class DebugSweepShapeType
        {
            capsule,
            cylinder,
        };
        #endif

    public:

        enum class FloorType
        {
            Navigable,
            Unnavigable,
            NoFloor,
        };

        CharacterPhysicsController( Physics::CharacterComponent* pCharacterComponent )
            : m_pCharacterComponent( pCharacterComponent )
        {
            EE_ASSERT( m_pCharacterComponent != nullptr );
        }

        // Controller Settings
        //-------------------------------------------------------------------------

        inline Settings& GetSettings() { return m_settings; }
        inline void SetSettings( Settings const& settings ) { m_settings = settings; }

        inline void EnableGravity( float currentVerticalSpeed ) { m_isGravityEnabled = true; m_verticalSpeed = currentVerticalSpeed; }
        inline void DisableGravity() { m_isGravityEnabled = false; m_verticalSpeed = 0.f; }
        inline void ResetGravityMomentum() { m_verticalSpeed = 0.f; }

        inline void EnableStepHeight() { m_isStepHeightEnabled = true; }
        inline void DisableStepHeight() { m_isStepHeightEnabled = false; }

        inline void EnableProjectionOntoFloor() { m_projectOntoFloor = true; }
        inline void DisableProjectionOntoFloor() { m_projectOntoFloor = false; }

        #if EE_DEVELOPMENT_TOOLS
        inline void SetDebugMode( bool isInDebugMode ) { m_isInDebugMode = isInDebugMode; }
        #endif


        // Controller Move
        //-------------------------------------------------------------------------

        float GetInAirTime() const { return m_timeWithoutFloor.GetElapsedTimeSeconds(); }
        bool HasFloor() const { return m_floorType != FloorType::NoFloor; }
        FloorType GetFloorType() const { return m_floorType; }
        bool TryMoveCapsule( EntityWorldUpdateContext const& ctx, Physics::Scene* pPhysicsScene, Vector const& deltaTranslation, Quaternion const& deltaRotation );

    private:

        // Sweep function
        //-------------------------------------------------------------------------

        MoveResult SweepCylinder( EntityWorldUpdateContext const& ctx, Physics::Scene* pPhysicsScene, float cylinderHalfHeight, float cylinderRadius, Vector const& startPosition, Vector const& deltaTranslation, int32_t& idx );
        MoveResult SweepCapsuleVertical( EntityWorldUpdateContext const& ctx, Physics::Scene* pPhysicsScene, float cylinderHalfHeight, float cylinderRadius, Vector const& startPosition, Vector const& deltaTranslation, float stepHeight, int32_t& idx );

        #if EE_DEVELOPMENT_TOOLS
        void DrawDebugSweep( EntityWorldUpdateContext const& ctx, float cylinderHalfHeight, float cylinderRadius, Vector const& startPosition, Vector const& endPosition, DebugSweepResultType resultType, int32_t idx, DebugSweepShapeType shapeType );
        #endif

    private:

        Settings                            m_settings;

        Physics::CharacterComponent*        m_pCharacterComponent = nullptr;

        bool                                m_isStepHeightEnabled = true;
        bool                                m_isGravityEnabled = true;
        bool                                m_projectOntoFloor = true;

        FloorType                           m_floorType = FloorType::NoFloor;
        Vector                              m_floorNormal = Vector::Zero;
        ManualTimer                         m_timeWithoutFloor;
        float                               m_verticalSpeed = 0.0f;

        #if EE_DEVELOPMENT_TOOLS
        bool                                m_isInDebugMode = false;
        #endif
    };
}