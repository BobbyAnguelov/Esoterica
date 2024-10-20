#pragma once
#include "Engine/Animation/AnimationSkeleton.h"
#include "Base/Resource/ResourcePtr.h"

//-------------------------------------------------------------------------

class RkSolver;
struct RkTransform;
struct RkIkEffector;

template <typename T>
class RkArray;

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class EE_ENGINE_API IKRigDefinition : public Resource::IResource
    {
        EE_RESOURCE( 'ik', "IK Rig", 4, false );

        friend class IKRigCompiler;
        friend class IKRigLoader;
        friend class IKRig;

        EE_SERIALIZE( m_skeleton, m_links, m_iterations );

    public:

        struct EE_ENGINE_API Link : public IReflectedType
        {
            EE_SERIALIZE( m_boneID, m_mass, m_radius, m_isEffector, m_swingLimit, m_minTwistLimit, m_maxTwistLimit );
            EE_REFLECT_TYPE( Link );

            inline bool IsValid() const { return m_boneID.IsValid() && m_mass > 0 && m_radius.m_x > 0 && m_radius.m_y > 0 && m_radius.m_z > 0; }

        public:

            EE_REFLECT( Category = "Body" ); // Bone ID for this link
            StringID    m_boneID;

            EE_REFLECT( Category = "Body" );
            float       m_mass = 0.0f;

            EE_REFLECT( Category = "Body" );
            Float3      m_radius = Float3::Zero;

            EE_REFLECT( Category = "Body" );
            bool        m_isEffector = false;

            //-------------------------------------------------------------------------

            EE_REFLECT( Category = "Joint" );
            Radians     m_swingLimit = Math::TwoPi;

            EE_REFLECT( Category = "Joint" );
            Radians     m_minTwistLimit = -Math::TwoPi;

            EE_REFLECT( Category = "Joint" );
            Radians     m_maxTwistLimit = Math::TwoPi;
        };

    public:

        virtual bool IsValid() const override;

        Skeleton const* GetSkeleton() const { return m_skeleton.GetPtr(); }
        int32_t GetNumLinks() const { return (int32_t) m_links.size(); }
        int32_t GetNumJoints() const { return (int32_t) m_links.size() - 1; }
        int32_t GetNumEffectors() const { return m_numEffectors; }

        int32_t GetParentLinkIndex( int32_t linkIdx ) const;
        int32_t GetLinkIndexForBoneID( StringID boneID ) const;
        int32_t GetParentLinkIndexForBoneID( StringID boneID ) const;
        int32_t GetLinkIndexForEffector( int32_t effectorIdx ) const;

        Vector CalculateCOM( int32_t linkIdx ) const;

    private:

        // Creates all the necessary additional runtime data needed to instantiate this definition (i.e. bone mappings, etc...)
        void CreateRuntimeData();

    private:

        TResourcePtr<Animation::Skeleton>       m_skeleton;
        TVector<Link>                           m_links;
        int32_t                                 m_iterations = 32;

        // Runtime Data
        TVector<int32_t>                        m_boneToBodyMap;
        TVector<int32_t>                        m_bodyToBoneMap;
        int32_t                                 m_numEffectors = 0;
    };

    //-------------------------------------------------------------------------

    class Pose;

    //-------------------------------------------------------------------------

    class EE_ENGINE_API IKRig final
    {

    public:

        IKRig( IKRigDefinition const* pDefinition );
        ~IKRig();

        void Solve( Pose* pPose );

        void SetEffectorTarget( int32_t effectorIdx, Transform target );

        // Debug
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        void DrawDebug( Drawing::DrawContext& ctx, Transform const& worldTransform ) const;
        #endif

    private:

        IKRigDefinition const*      m_pDefinition = nullptr;
        TVector<int32_t>            m_boneToBodyMap;
        TVector<int32_t>            m_bodyToBoneMap;

        RkSolver*                   m_pSolver = nullptr;
        RkArray<RkTransform>*       m_pBodyTransforms = nullptr;
        RkArray<RkIkEffector>*      m_pEffectors = nullptr;

        TVector<Transform>          m_modelSpacePoseTransforms;
    };
}