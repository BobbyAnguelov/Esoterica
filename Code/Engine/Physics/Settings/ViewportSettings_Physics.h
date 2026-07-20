#pragma once
#include "Engine/Viewport/ViewportSettings.h"
#include "Engine/_Module/API.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::Physics
{
    class EE_ENGINE_API PhysicsViewportSettings : public IViewportSettings
    {
        EE_REFLECT_TYPE( PhysicsViewportSettings );

    public:

        virtual char const* GetFriendlyName() const override { return "Physics"; }
        virtual void DrawMenu( EntityWorld* pWorld ) override;

    public:

        EE_REFLECT();
        bool    m_drawDebug = false;

        // Scale to use when drawing forces
        float   m_forceScale = 1.0f;

        // Global scaling for joint drawing
        float   m_jointScale = 1.0f;

        // Option to draw shapes
        bool    m_drawShapes = true;

        // Option to draw joints
        bool    m_drawJoints = false;

        // Option to draw additional information for joints
        bool    m_drawJointExtras = false;

        // Option to draw the bounding boxes for shapes
        bool    m_drawBounds = false;

        // Option to draw the mass and center of mass of dynamic bodies
        bool    m_drawMass = false;

        // Option to draw the sleep information for dynamic and kinematic bodies
        bool    m_drawSleep = false;

        // Option to draw body names
        bool    m_drawBodyNames = false;

        // Option to draw contact points
        bool    m_drawContacts = false;

        // Draw contact anchor A or B
        bool    m_drawAnchorA;

        // Option to visualize the graph coloring used for contacts and joints
        bool    m_drawGraphColors = false;

        // Option to draw contact features
        bool    m_drawContactFeatures = false;

        // Option to draw contact normals
        bool    m_drawContactNormals = false;

        // Option to draw contact normal forces
        bool    m_drawContactForces = false;

        // Option to draw islands as bounding boxes
        bool    m_drawIslands = false;
    };
}
#endif