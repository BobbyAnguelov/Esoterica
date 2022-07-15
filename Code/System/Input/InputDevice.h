#pragma once

#include "System/Esoterica.h"
#include "System/Time/Time.h"

//-------------------------------------------------------------------------

namespace EE
{
    namespace Input
    {
        // Generic message data for various input messages (e.g. keyboard/mouse messages )
        struct GenericMessage
        {
            uint64_t m_data0;
            uint64_t m_data1;
            uint64_t m_data2;
            uint64_t m_data3;
        };

        //-------------------------------------------------------------------------

        enum class DeviceCategory
        {
            KeyboardMouse,
            Controller
        };

        //-------------------------------------------------------------------------

        enum class ResetType
        {
            Partial,    // Regular frame to frame reset
            Full        // Special full reset of all input state
        };

        class InputDevice
        {

        public:

            virtual void Initialize() = 0;
            virtual void Shutdown() = 0;

            // Get the category for this device (controller/mice), this is necessary since we may have multiple controller device types (e.g. XBox, PS, 3rdParty)
            virtual DeviceCategory GetDeviceCategory() const = 0;

            // Called by the OS message pump to forward system input events, occurs after the frame end but before the start of the new frame
            virtual void ProcessMessage( GenericMessage const& message ) {}

            // Called at the start of the frame to update the current device state
            virtual void UpdateState( Seconds deltaTime ) = 0;

            // Called at the end of the frame to reset any tracked device state
            virtual void ClearFrameState( ResetType resetType = ResetType::Partial ) {};
        };
    }
}