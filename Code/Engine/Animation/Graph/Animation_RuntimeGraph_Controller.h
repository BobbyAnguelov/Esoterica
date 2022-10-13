#pragma once
#include "Engine/_Module/API.h"
#include "Engine/Animation/Components/Component_AnimationGraph.h"
#include "Engine/Render/Components/Component_SkeletalMesh.h"
#include "Engine/Entity/EntityLog.h"
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
    class GraphController;

    //-------------------------------------------------------------------------

    namespace Internal
    {
        class EE_ENGINE_API GraphControllerBase
        {
            friend class GraphController;

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
                    EE_ASSERT( pController != nullptr && pController->m_pGraphInstance != nullptr );

                    // Try to find parameter
                    m_index = pController->m_pGraphInstance->GetControlParameterIndex( m_ID );
                    if ( m_index == InvalidIndex )
                    {
                        #if EE_DEVELOPMENT_TOOLS
                        EE_LOG_WARNING( "Animation", "Failed to bind to control parameter (%s): parameter not found. Controller (%s) and graph (%s)", m_ID.c_str(), pController->GetName(), pController->m_pGraphInstance->GetResourceID().c_str() );
                        #endif

                        return false;
                    }

                    // Validate parameter type
                    if ( pController->m_pGraphInstance->GetControlParameterType( m_index ) != ValueTypeValidation<ParameterType>::Type )
                    {
                        #if EE_DEVELOPMENT_TOOLS
                        EE_LOG_WARNING( "Animation", "Failed to bind to control parameter (%s): type mismatch. Controller (%s) and graph (%s)", m_ID.c_str(), pController->GetName(), pController->m_pGraphInstance->GetResourceID().c_str() );
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
                        pController->m_pGraphInstance->SetControlParameterValue<ParameterType>( m_index, value );
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
                int16_t                         m_index = InvalidIndex;
            };

        protected:

            GraphControllerBase( GraphInstance* pGraphInstance, Render::SkeletalMeshComponent* pMeshComponent );
            virtual ~GraphControllerBase() = default;

            // Optional update that runs before the graph evaluation allowing you to set more parameters, etc...
            virtual void PreGraphUpdate( Seconds deltaTime ) {}

            // Optional update that runs after the graph evaluation allowing you to read events and clear transient state
            virtual void PostGraphUpdate( Seconds deltaTime ) {}

            // Character Info
            //-------------------------------------------------------------------------

            EE_FORCE_INLINE Vector ConvertWorldSpacePointToCharacterSpace( Vector const& worldPoint ) const
            {
                return m_pAnimatedMeshComponent->GetWorldTransform().InverseTransformPoint( worldPoint );
            }

            EE_FORCE_INLINE Vector ConvertWorldSpaceVectorToCharacterSpace( Vector const& worldVector ) const
            {
                return m_pAnimatedMeshComponent->GetWorldTransform().InverseTransformVector( worldVector );
            }

            // Graph Info
            //-------------------------------------------------------------------------

            inline SampledEventsBuffer const& GetSampledEvents() const { return m_pGraphInstance->GetSampledEvents(); }

            //-------------------------------------------------------------------------

            #if EE_DEVELOPMENT_TOOLS
            // Returns a friendly name for this controller
            virtual char const* GetName() const = 0;
            #endif

        private:

            GraphInstance*                      m_pGraphInstance = nullptr;
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
    // External Graph Controller
    //-------------------------------------------------------------------------

    class EE_ENGINE_API ExternalGraphController : public Internal::GraphControllerBase
    {
        friend class GraphController;

    public:

        ExternalGraphController( StringID slotID, GraphInstance* pGraphInstance, Render::SkeletalMeshComponent* pMeshComponent, bool shouldAutoDestroy );
        using GraphControllerBase::GraphControllerBase;

    private:

        StringID    m_slotID;
        bool        m_requiresAutoDestruction = false;
        bool        m_wasActivated = false;
    };

    //-------------------------------------------------------------------------
    // Main Graph Controller
    //-------------------------------------------------------------------------

    class EE_ENGINE_API GraphController : public Internal::GraphControllerBase
    {
    public:

        template<typename T> using ControlParameter = Internal::GraphControllerBase::ControlParameter<T>;

    public:

        GraphController( AnimationGraphComponent* pGraphComponent, Render::SkeletalMeshComponent* pMeshComponent );
        virtual ~GraphController();

        // Sub-graph Controllers
        //-------------------------------------------------------------------------

        inline bool HasSubGraphControllers() const { return !m_subGraphControllers.empty(); }

        // Create subgraph controller
        template<typename T>
        void CreateSubGraphController()
        {
            m_subGraphControllers.emplace_back( EE::New<T>( m_pGraphInstance, m_pAnimatedMeshComponent ) );
        }

        // Get a specific graph controller
        template<typename T>
        inline T* GetSubGraphController() const
        {
            static_assert( std::is_base_of<EE::Animation::SubGraphController, T>::value, "T is not derived from SubGraphController" );

            for ( auto pController : m_subGraphControllers )
            {
                if ( pController->GetSubGraphControllerID() == T::s_subGraphControllerID )
                {
                    return reinterpret_cast<T*>( pController );
                }
            }
            return nullptr;
        }

        // External Graph Controllers
        //-------------------------------------------------------------------------

        inline bool IsValidExternalGraphSlotID( StringID slotID ) const { return m_pGraphInstance->IsValidExternalGraphSlotID( slotID ); }

        // Create a new external controller that will attach and drive a specified graph
        template<typename T>
        T* TryCreateExternalGraphController( StringID slotID, GraphVariation const* pGraph, bool autoDestroy = false )
        {
            static_assert( std::is_base_of<EE::Animation::ExternalGraphController, T>::value, "T is not derived from ExternalGraphController" );
            EE_ASSERT( slotID.IsValid() && pGraph != nullptr );

            if ( !m_pGraphInstance->IsValidExternalGraphSlotID( slotID ) )
            {
                EE_LOG_ENTITY_ERROR( m_pGraphComponent, "Animation", "Invalid slot ID (%s) - Component (%s) on Entity (%ul)!", slotID.c_str(), m_pGraphComponent->GetNameID().c_str(), m_pGraphComponent->GetEntityID().m_value );
                return nullptr;
            }

            if ( m_pGraphInstance->IsExternalGraphSlotFilled( slotID ) )
            {
                EE_LOG_ENTITY_ERROR( m_pGraphComponent, "Animation", "External graph slot (%s) is filled! - Component (%s) on Entity (%ul)", slotID.c_str(), m_pGraphComponent->GetNameID().c_str(), m_pGraphComponent->GetEntityID().m_value );
                return nullptr;
            }

            GraphInstance* pExternalGraphInstance = m_pGraphInstance->ConnectExternalGraph( slotID, pGraph );
            EE_ASSERT( pExternalGraphInstance != nullptr );

            T* pNewExternalController = EE::New<T>( slotID, pExternalGraphInstance, m_pAnimatedMeshComponent, autoDestroy );
            m_externalGraphControllers.emplace_back( pNewExternalController );
            return pNewExternalController;
        }

        // Destroy External Graph Controller
        void DestroyExternalGraphController( StringID slotID )
        {
            for ( size_t i = 0u; i < m_externalGraphControllers.size(); i++ )
            {
                if ( m_externalGraphControllers[i]->m_slotID == slotID )
                {
                    DestroyExternalGraphController( m_externalGraphControllers[i] );
                    m_externalGraphControllers.erase_unsorted( m_externalGraphControllers.begin() + i );
                    return;
                }
            }

            EE_UNREACHABLE_CODE();
        }

        // Update
        //-------------------------------------------------------------------------

        // Update that runs before the graph evaluation allowing you to set more parameters, etc...
        virtual void PreGraphUpdate( Seconds deltaTime );

        // Update that runs after the graph evaluation allowing you to read events and clear transient state
        virtual void PostGraphUpdate( Seconds deltaTime );

    private:

        void DestroyExternalGraphController( ExternalGraphController*& pController );

    private:

        AnimationGraphComponent*                        m_pGraphComponent = nullptr;
        TInlineVector<SubGraphController*, 6>           m_subGraphControllers;
        TInlineVector<ExternalGraphController*, 6>      m_externalGraphControllers;
    };
}

//-------------------------------------------------------------------------

#define EE_ANIMATION_SUBGRAPH_CONTROLLER_ID( TypeName ) \
constexpr static uint32_t const s_subGraphControllerID = Hash::FNV1a::GetHash32( #TypeName ); \
virtual uint32_t GetSubGraphControllerID() const override final { return TypeName::s_subGraphControllerID; }\
EE_DEVELOPMENT_TOOLS_LINE_IN_MACRO( char const* GetName() const override final { return #TypeName; } )