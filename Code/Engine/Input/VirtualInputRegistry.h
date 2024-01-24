#pragma once
#include "VirtualInputs.h"
#include "Base/Types/Arrays.h"

//-------------------------------------------------------------------------

namespace EE { class UpdateContext; }

//-------------------------------------------------------------------------

namespace EE::Input
{
    //-------------------------------------------------------------------------
    // Bindings
    //-------------------------------------------------------------------------

    class VirtualInputBinding
    {
        friend class VirtualInputRegistry;

    public:

        VirtualInputBinding( StringID ID ) : m_ID( ID ) { EE_ASSERT( m_ID.IsValid() ); }
        virtual ~VirtualInputBinding() = default;

        inline StringID GetID() const { return m_ID; }
        inline bool IsRegistered() const { return m_isRegistered; }
        inline bool IsBound() const { return m_pInput != nullptr; }
        virtual VirtualInput::Type GetType() const = 0;

    protected:

        StringID        m_ID;
        VirtualInput*   m_pInput = nullptr;
        bool            m_isRegistered = false;
    };

    //-------------------------------------------------------------------------

    class BinaryInputBinding : public VirtualInputBinding
    {
    public:

        virtual VirtualInput::Type GetType() const override { return VirtualInput::Type::Binary; }
        inline bool GetValue() const { return IsBound() ? static_cast<BinaryInput const*>( m_pInput )->GetValue() : false; }
        EE_FORCE_INLINE operator bool() const { return GetValue(); }
    };

    //-------------------------------------------------------------------------

    class AnalogInputBinding : public VirtualInputBinding
    {
    public:

        virtual VirtualInput::Type GetType() const override { return VirtualInput::Type::Analog; }
        inline float GetValue() const { return IsBound() ? static_cast<AnalogInput const*>( m_pInput )->GetValue() : 0.0f; }
        EE_FORCE_INLINE operator float() const { return GetValue(); }
    };

    //-------------------------------------------------------------------------

    class DirectionalInputBinding : public VirtualInputBinding
    {
    public:

        virtual VirtualInput::Type GetType() const override { return VirtualInput::Type::Directional; }
        inline Float2 GetValue() const { return IsBound() ? static_cast<DirectionalInput const*>( m_pInput )->GetValue() : Float2::Zero; }
        EE_FORCE_INLINE operator Float2() const { return GetValue(); }
    };

    //-------------------------------------------------------------------------
    // Virtual Input Registry
    //-------------------------------------------------------------------------

    class InputSystem;

    class VirtualInputRegistry
    {
        friend class InputDebugView;

    public:

        ~VirtualInputRegistry();

        inline bool IsInitialized() const { return m_pInputSystem != nullptr; }
        void Initialize( InputSystem* pInputSystem );
        void Shutdown();
        void Update();

        // Virtual Inputs
        //-------------------------------------------------------------------------

        void CreateInput( StringID ID, BinaryInput::UpdateFunction&& updateFunc );
        void CreateInput( StringID ID, AnalogInput::UpdateFunction&& updateFunc );
        void CreateInput( StringID ID, DirectionalInput::UpdateFunction&& updateFunc );
        void DestroyInput( StringID ID );

        // Bindings
        //-------------------------------------------------------------------------

        void RegisterBinding( VirtualInputBinding& binding );
        void UnregisterBinding( VirtualInputBinding& binding );

    private:

        InputSystem*                    m_pInputSystem = nullptr;
        TVector<VirtualInput*>          m_inputs;
        TVector<VirtualInputBinding*>   m_bindings;
    };
}