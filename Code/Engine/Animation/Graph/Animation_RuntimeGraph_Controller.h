#pragma once
#include "Engine/_Module/API.h"
#include "Engine/Animation/Components/Component_AnimationGraph.h"
#include "Engine/Render/Components/Component_SkeletalMesh.h"
#include "System/Algorithm/Hash.h"
#include "System/Types/StringID.h"
#include "System/Log.h"

//-------------------------------------------------------------------------
// Animation Graph Controller
//-------------------------------------------------------------------------
// Each graph can have a main graph controller that manages general/global state as well a multiple subgraph-controllers
// The subgraph controllers will be responsible for driving a specific portion of the graph i.e. locomotion/combat/climbing/etc...

namespace EE::Animation
{
    namespace Internal
    {
        class EE_ENGINE_API GraphControllerBase
        {

        public:

            template<typename ParameterType>
            class ControlParameter final
            {

            public:

                ControlParameter( char const* parameterName ) : m_ID( parameterName ) {}

                inline bool IsBound() const { return m_index != InvalidIndex; }

                // Bind a control Parameter to a graph instance
                bool TryBind( GraphControllerBase * pController )
                {
                    EE_ASSERT( m_ID.IsValid() );
                    EE_ASSERT( pController != nullptr && pController->m_pGraphComponent != nullptr );

                    // Try to find parameter
                    m_index = pController->m_pGraphComponent->GetControlParameterIndex( m_ID );
                    if ( m_index == InvalidIndex )
                    {
                        #if EE_DEVELOPMENT_TOOLS
                        EE_LOG_WARNING( "Animation", "Failed to bind to control parameter (%s): parameter not found. Controller (%s) and graph (%s)", m_ID.c_str(), pController->GetName(), pController->m_pGraphComponent->GetGraphVariationID().c_str() );
                        #endif

                        return false;
                    }

                    // Validate parameter type
                    if ( pController->m_pGraphComponent->GetControlParameterValueType( m_index ) != ValueTypeValidation<ParameterType>::Type )
                    {
                        #if EE_DEVELOPMENT_TOOLS
                        EE_LOG_WARNING( "Animation", "Failed to bind to control parameter (%s): type mismatch. Controller (%s) and graph (%s)", m_ID.c_str(), pController->GetName(), pController->m_pGraphComponent->GetGraphVariationID().c_str() );
                        #endif

                        m_index = InvalidIndex;
                        return false;
                    }

                    return true;
                }

                // Set Control Parameter Value
                void Set( GraphControllerBase* pController, ParameterType const& value )
                {
                    if ( m_index != InvalidIndex )
                    {
                        pController->m_pGraphComponent->SetControlParameterValue<ParameterType>( m_index, value );
                    }
                    else
                    {
                        #if EE_DEVELOPMENT_TOOLS
                        EE_LOG_WARNING( "Animation", "Trying to unbound control parameter %s in controller %s", m_ID.c_str(), pController->GetName() );
                        #endif
                    }
                }

            private:

                StringID                        m_ID;
                int16_t                  m_index = InvalidIndex;
            };

        protected:

            GraphControllerBase( AnimationGraphComponent* pGraphComponent, Render::SkeletalMeshComponent* pMeshComponent );
            virtual ~GraphControllerBase() = default;

            // Optional update that runs before the graph evaluation allowing you to set more parameters, etc...
            virtual void PreGraphUpdate( Seconds deltaTime ) {}

            // Optional update that runs after the graph evaluation allowing you to read events and clear transient state
            virtual void PostGraphUpdate( Seconds deltaTime ) {}

            //-------------------------------------------------------------------------

            EE_FORCE_INLINE Vector ConvertWorldSpacePointToCharacterSpace( Vector const& worldPoint ) const
            {
                return m_pAnimatedMeshComponent->GetWorldTransform().GetInverse().TransformPoint( worldPoint );
            }

            EE_FORCE_INLINE Vector ConvertWorldSpaceVectorToCharacterSpace( Vector const& worldVector ) const
            {
                return m_pAnimatedMeshComponent->GetWorldTransform().GetInverse().RotateVector( worldVector );
            }

            //-------------------------------------------------------------------------

            #if EE_DEVELOPMENT_TOOLS
            // Returns a friendly name for this controller
            virtual char const* GetName() const = 0;
            #endif

        private:

            AnimationGraphComponent*            m_pGraphComponent = nullptr;
            Render::SkeletalMeshComponent*      m_pAnimatedMeshComponent = nullptr;
        };
    }

    //-------------------------------------------------------------------------
    // Sub Graph Controller
    //-------------------------------------------------------------------------

    class EE_ENGINE_API SubGraphController : public Internal::GraphControllerBase
    {
        friend class GraphController;

    public:

        template<typename T> using ControlParameter = Internal::GraphControllerBase::ControlParameter<T>;
        using GraphControllerBase::GraphControllerBase;

    protected:

        // Get the ID for this controller
        virtual uint32_t GetSubGraphControllerID() const = 0;
    };

    //-------------------------------------------------------------------------
    // Main Graph Controller
    //-------------------------------------------------------------------------

    class EE_ENGINE_API GraphController : public Internal::GraphControllerBase
    {
    public:

        template<typename T> using ControlParameter = Internal::GraphControllerBase::ControlParameter<T>;

    public:

        using GraphControllerBase::GraphControllerBase;
        virtual ~GraphController();

        inline bool HasSubGraphControllers() const { return !m_subGraphControllers.empty(); }

        // Get a specific graph controller
        template<typename T>
        inline T* GetSubGraphController() const
        {
            static_assert( std::is_base_of<EE::Animation::SubGraphController, T>::value, "T is not derived from SubGraphController" );

            for ( auto pController : m_subGraphControllers )
            {
                if ( pController->GetSubGraphControllerID() == T::s_subGraphControllerID )
                {
                    return static_cast<T*>( pController );
                }
            }
            return nullptr;
        }

        // Update that runs before the graph evaluation allowing you to set more parameters, etc...
        virtual void PreGraphUpdate( Seconds deltaTime );

        // Update that runs after the graph evaluation allowing you to read events and clear transient state
        virtual void PostGraphUpdate( Seconds deltaTime );

    protected:

        TInlineVector<SubGraphController*, 6>      m_subGraphControllers;
    };
}

//-------------------------------------------------------------------------

#define EE_ANIMATION_SUBGRAPH_CONTROLLER_ID( TypeName ) \
constexpr static uint32_t const s_subGraphControllerID = Hash::FNV1a::GetHash32( #TypeName ); \
virtual uint32_t GetSubGraphControllerID() const override final { return TypeName::s_subGraphControllerID; }\
EE_DEVELOPMENT_TOOLS_LINE_IN_MACRO( char const* GetName() const override final { return #TypeName; } )