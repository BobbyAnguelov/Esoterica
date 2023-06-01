#pragma once

#include "Engine/Animation/TaskSystem/Animation_Task.h"
//-------------------------------------------------------------------------

namespace EE::Animation::Tasks
{
    class LookAtIKTask : public Task
    {
        EE_REFLECT_TYPE( LookAtIKTask );

    public:

        LookAtIKTask( TaskSourceID sourceID, TaskIndex sourceTaskIdx, Vector const& worldSpaceTarget );
        virtual void Execute( TaskContext const& context ) override;

        virtual bool AllowsSerialization() const override { return true; }
        virtual void Serialize( TaskSerializer& serializer ) const override;
        virtual void Deserialize( TaskSerializer& serializer ) override;

        #if EE_DEVELOPMENT_TOOLS
        virtual String GetDebugText() const override { return "Look At IK"; }
        virtual Color GetDebugColor() const override { return Colors::Cyan; }
        #endif

    private:

        LookAtIKTask() : Task( 0xFF ) {}

    private:

        Vector  m_worldSpaceTarget;
        Vector  m_characterSpaceTarget;
        bool    m_generateEffectorData = true; // Whether we should calculate effector data or assume it was deserialized
    };
}