#include "VirtualInputRegistry.h"
#include "Base/Input/InputSystem.h"

//-------------------------------------------------------------------------

namespace EE::Input
{
    VirtualInputRegistry::~VirtualInputRegistry()
    {
        EE_ASSERT( !IsInitialized() );
        EE_ASSERT( m_inputs.empty() );
        EE_ASSERT( m_bindings.empty() );
    }

    void VirtualInputRegistry::Initialize( InputSystem* pInputSystem )
    {
        EE_ASSERT( !IsInitialized() );
        EE_ASSERT( pInputSystem != nullptr );
        m_pInputSystem = pInputSystem;
    }

    void VirtualInputRegistry::Shutdown()
    {
        EE_ASSERT( IsInitialized() );
        EE_ASSERT( m_inputs.empty() );
        EE_ASSERT( m_bindings.empty() );
        m_pInputSystem = nullptr;
    }

    void VirtualInputRegistry::Update()
    {
        EE_ASSERT( IsInitialized() );

        //-------------------------------------------------------------------------

        TInlineVector<InputDevice const*, 5> connectedInputDevices;
        for ( auto pDevice : m_pInputSystem->GetInputDevices() )
        {
            if ( pDevice->IsConnected() )
            {
                connectedInputDevices.emplace_back( pDevice );
            }
        }

        //-------------------------------------------------------------------------

        for ( auto pInput : m_inputs )
        {
            pInput->Update( connectedInputDevices );
        }
    }

    //-------------------------------------------------------------------------

    void VirtualInputRegistry::CreateInput( StringID ID, BinaryInput::UpdateFunction&& updateFunc )
    {
        EE_ASSERT( IsInitialized() );
        m_inputs.emplace_back( EE::New<BinaryInput>( ID, eastl::forward<BinaryInput::UpdateFunction&&>( updateFunc ) ) );
    }

    void VirtualInputRegistry::CreateInput( StringID ID, AnalogInput::UpdateFunction&& updateFunc )
    {
        EE_ASSERT( IsInitialized() );
        m_inputs.emplace_back( EE::New<AnalogInput>( ID, eastl::forward<AnalogInput::UpdateFunction&&>( updateFunc ) ) );
    }

    void VirtualInputRegistry::CreateInput( StringID ID, DirectionalInput::UpdateFunction&& updateFunc )
    {
        EE_ASSERT( IsInitialized() );
        m_inputs.emplace_back( EE::New<DirectionalInput>( ID, eastl::forward<DirectionalInput::UpdateFunction&&>( updateFunc ) ) );
    }

    void VirtualInputRegistry::DestroyInput( StringID ID )
    {
        EE_ASSERT( IsInitialized() );

        // Invalidate any bindings
        //-------------------------------------------------------------------------

        for ( auto pBinding : m_bindings )
        {
            if ( pBinding->m_ID == ID )
            {
                pBinding->m_pInput = nullptr;
            }
        }

        // Remove Input
        //-------------------------------------------------------------------------

        for ( auto iter = m_inputs.begin(); iter != m_inputs.end(); ++iter )
        {
            if ( ( *iter )->GetID() == ID )
            {
                EE::Delete( *iter );
                m_inputs.erase( iter );
                break;
            }
        }
    }

    //-------------------------------------------------------------------------

    void VirtualInputRegistry::RegisterBinding( VirtualInputBinding& binding )
    {
        EE_ASSERT( IsInitialized() );
        EE_ASSERT( binding.GetID().IsValid() );

        for ( auto pInput : m_inputs )
        {
            if ( pInput->GetID() != binding.GetID() )
            {
                EE_LOG_WARNING( "Input", "Input Binding", "Input binding failed, unknown ID (%s)", pInput->GetID().c_str() );
                continue;
            }

            if ( pInput->GetType() != binding.GetType() )
            {
                EE_LOG_WARNING( "Input", "Input Binding", "Input binding (%) failed due to type mismatch!", pInput->GetID().c_str() );
                continue;
            }

            binding.m_pInput = pInput;
            binding.m_isRegistered = true;
            m_bindings.emplace_back( &binding );
            break;
        }
    }

    void VirtualInputRegistry::UnregisterBinding( VirtualInputBinding& binding )
    {
        EE_ASSERT( IsInitialized() );
        EE_ASSERT( binding.IsRegistered() );
        EE_ASSERT( VectorContains( m_bindings, &binding ) );
        binding.m_pInput = nullptr;
        binding.m_isRegistered = false;
        m_bindings.erase_first( &binding );
    }
}