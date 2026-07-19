#include "sloth_physics_world.h"

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>

namespace sloth {
    // ------------------------------------------------------------------------
    // Minimal two-layer setup (static / moving), the same boilerplate Jolt's
    // own HelloWorld sample uses. Only moving objects need broadphase
    // queries against each other and against statics; statics never need to
    // query anything.
    // ------------------------------------------------------------------------

    namespace Layers {
        static constexpr JPH::ObjectLayer NonMoving = 0;
        static constexpr JPH::ObjectLayer Moving = 1;
        static constexpr JPH::ObjectLayer NumLayers = 2;
    } // namespace Layers

    namespace BroadPhaseLayers {
        static constexpr JPH::BroadPhaseLayer NonMoving( 0 );
        static constexpr JPH::BroadPhaseLayer Moving( 1 );
        static constexpr u32 NumLayers = 2;
    } // namespace BroadPhaseLayers

    class BroadPhaseLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface {
      public:
        BroadPhaseLayerInterfaceImpl() {
            objectToBroadPhase[Layers::NonMoving] = BroadPhaseLayers::NonMoving;
            objectToBroadPhase[Layers::Moving] = BroadPhaseLayers::Moving;
        }

        JPH::uint GetNumBroadPhaseLayers() const override { return BroadPhaseLayers::NumLayers; }

        JPH::BroadPhaseLayer GetBroadPhaseLayer( JPH::ObjectLayer layer ) const override {
            return objectToBroadPhase[layer];
        }

      private:
        JPH::BroadPhaseLayer objectToBroadPhase[Layers::NumLayers];
    };

    class ObjectVsBroadPhaseLayerFilterImpl final : public JPH::ObjectVsBroadPhaseLayerFilter {
      public:
        bool ShouldCollide( JPH::ObjectLayer layer1, JPH::BroadPhaseLayer layer2 ) const override {
            switch ( layer1 ) {
                case Layers::NonMoving:
                    return layer2 == BroadPhaseLayers::Moving;
                case Layers::Moving:
                    return true;
                default:
                    SL_ASSERT( false );
                    return false;
            }
        }
    };

    class ObjectLayerPairFilterImpl final : public JPH::ObjectLayerPairFilter {
      public:
        bool ShouldCollide( JPH::ObjectLayer object1, JPH::ObjectLayer object2 ) const override {
            switch ( object1 ) {
                case Layers::NonMoving:
                    return object2 == Layers::Moving;
                case Layers::Moving:
                    return true;
                default:
                    SL_ASSERT( false );
                    return false;
            }
        }
    };

    // ------------------------------------------------------------------------
    // glm <-> Jolt conversions. Kept private to this file; the public API
    // only ever speaks glm.
    // ------------------------------------------------------------------------

    static JPH::Vec3 ToJolt( const glm::vec3 & v ) { return JPH::Vec3( v.x, v.y, v.z ); }
    static JPH::Quat ToJolt( const glm::quat & q ) { return JPH::Quat( q.x, q.y, q.z, q.w ); }
    static glm::vec3 FromJolt( JPH::Vec3Arg v ) { return glm::vec3( v.GetX(), v.GetY(), v.GetZ() ); }
    static glm::quat FromJolt( JPH::QuatArg q ) { return glm::quat( q.GetW(), q.GetX(), q.GetY(), q.GetZ() ); }

    static JPH::EMotionType ToJolt( BodyMotionType motionType ) {
        switch ( motionType ) {
            case BodyMotionType::Static:
                return JPH::EMotionType::Static;
            case BodyMotionType::Kinematic:
                return JPH::EMotionType::Kinematic;
            case BodyMotionType::Dynamic:
                return JPH::EMotionType::Dynamic;
        }

        SL_ASSERT_MSG( false, "Unknown BodyMotionType" );
        return JPH::EMotionType::Static;
    }

    // ------------------------------------------------------------------------
    // PhysicsWorld::Impl
    // ------------------------------------------------------------------------

    static constexpr u32 MaxBodies = 1024;
    static constexpr u32 NumBodyMutexes = 0; // 0 = Jolt picks a sensible default
    static constexpr u32 MaxBodyPairs = 1024;
    static constexpr u32 MaxContactConstraints = 1024;
    static constexpr usize TempAllocatorSize = 10 * 1024 * 1024; // 10 MB, per Jolt's own samples
    static constexpr f32 FixedTimeStep = 1.0f / 60.0f;
    static constexpr i32 MaxSubSteps = 4; // caps the accumulator so a stall doesn't cause a spiral of death

    struct PhysicsWorld::Impl {
        JPH::Factory * factory = nullptr;

        BroadPhaseLayerInterfaceImpl broadPhaseLayerInterface;
        ObjectVsBroadPhaseLayerFilterImpl objectVsBroadPhaseLayerFilter;
        ObjectLayerPairFilterImpl objectLayerPairFilter;

        // Constructed in PhysicsWorld's constructor body, not as an inline
        // member initializer: TempAllocatorImpl's constructor calls
        // JPH::Allocate() immediately, which is a null function pointer
        // until JPH::RegisterDefaultAllocator() runs. Member initializers
        // on Impl would run during make_unique<Impl>() in PhysicsWorld's
        // mem-initializer list, before that registration happens.
        std::unique_ptr<JPH::TempAllocatorImpl> tempAllocator;
        std::unique_ptr<JPH::JobSystemThreadPool> jobSystem;
        JPH::PhysicsSystem physicsSystem;
        JPH::BodyInterface * bodyInterface = nullptr;

        f32 accumulator = 0.0f;

        RigidBody CreateBodyFromShape( const JPH::Shape * shape, const RigidBodyDesc & desc );
    };

    PhysicsWorld::PhysicsWorld()
        : impl( std::make_unique<Impl>() ) {
        JPH::RegisterDefaultAllocator();

        impl->factory = new JPH::Factory();
        JPH::Factory::sInstance = impl->factory;
        JPH::RegisterTypes();

        impl->tempAllocator = std::make_unique<JPH::TempAllocatorImpl>( TempAllocatorSize );
        impl->jobSystem = std::make_unique<JPH::JobSystemThreadPool>( JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, -1 );

        impl->physicsSystem.Init( MaxBodies, NumBodyMutexes, MaxBodyPairs, MaxContactConstraints,
            impl->broadPhaseLayerInterface, impl->objectVsBroadPhaseLayerFilter,
            impl->objectLayerPairFilter );

        impl->bodyInterface = &impl->physicsSystem.GetBodyInterface();
    }

    PhysicsWorld::~PhysicsWorld() {
        // Tear down everything that touches registered shape types (bodies,
        // the physics system, the temp allocator) before unregistering
        // those types and destroying the factory they were registered with.
        impl.reset();

        JPH::UnregisterTypes();

        delete JPH::Factory::sInstance;
        JPH::Factory::sInstance = nullptr;
    }

    void PhysicsWorld::Update( f32 deltaTime ) {
        impl->accumulator += deltaTime;
        impl->accumulator = impl->accumulator < FixedTimeStep * MaxSubSteps ? impl->accumulator : FixedTimeStep * MaxSubSteps;

        while ( impl->accumulator >= FixedTimeStep ) {
            impl->physicsSystem.Update( FixedTimeStep, 1, impl->tempAllocator.get(), impl->jobSystem.get() );
            impl->accumulator -= FixedTimeStep;
        }
    }

    RigidBody PhysicsWorld::Impl::CreateBodyFromShape( const JPH::Shape * shape, const RigidBodyDesc & desc ) {
        JPH::ObjectLayer layer = desc.motionType == BodyMotionType::Static ? Layers::NonMoving : Layers::Moving;

        JPH::BodyCreationSettings settings( shape, ToJolt( desc.position ), ToJolt( desc.rotation ),
            ToJolt( desc.motionType ), layer );
        settings.mFriction = desc.friction;
        settings.mRestitution = desc.restitution;

        JPH::Body * body = bodyInterface->CreateBody( settings );
        SL_ASSERT_MSG( body != nullptr, "PhysicsWorld: failed to create body (max body count reached?)" );

        bodyInterface->AddBody( body->GetID(), JPH::EActivation::Activate );

        return RigidBody { body->GetID().GetIndexAndSequenceNumber() };
    }

    RigidBody PhysicsWorld::CreateBoxBody( const glm::vec3 & halfExtents, const RigidBodyDesc & desc ) {
        JPH::ShapeSettings::ShapeResult shapeResult = JPH::BoxShapeSettings( ToJolt( halfExtents ) ).Create();
        SL_ASSERT_MSG( !shapeResult.HasError(), "PhysicsWorld: failed to create box shape: %s", shapeResult.GetError().c_str() );

        return impl->CreateBodyFromShape( shapeResult.Get(), desc );
    }

    RigidBody PhysicsWorld::CreateSphereBody( f32 radius, const RigidBodyDesc & desc ) {
        JPH::ShapeSettings::ShapeResult shapeResult = JPH::SphereShapeSettings( radius ).Create();
        SL_ASSERT_MSG( !shapeResult.HasError(), "PhysicsWorld: failed to create sphere shape: %s", shapeResult.GetError().c_str() );

        return impl->CreateBodyFromShape( shapeResult.Get(), desc );
    }

    void PhysicsWorld::DestroyBody( RigidBody body ) {
        JPH::BodyID id( body.Id );
        impl->bodyInterface->RemoveBody( id );
        impl->bodyInterface->DestroyBody( id );
    }

    glm::vec3 PhysicsWorld::GetPosition( RigidBody body ) const {
        return FromJolt( impl->bodyInterface->GetPosition( JPH::BodyID( body.Id ) ) );
    }

    glm::quat PhysicsWorld::GetRotation( RigidBody body ) const {
        return FromJolt( impl->bodyInterface->GetRotation( JPH::BodyID( body.Id ) ) );
    }

    void PhysicsWorld::SetLinearVelocity( RigidBody body, const glm::vec3 & velocity ) {
        impl->bodyInterface->SetLinearVelocity( JPH::BodyID( body.Id ), ToJolt( velocity ) );
    }

    glm::vec3 PhysicsWorld::GetLinearVelocity( RigidBody body ) const {
        return FromJolt( impl->bodyInterface->GetLinearVelocity( JPH::BodyID( body.Id ) ) );
    }

    void PhysicsWorld::SetAngularVelocity( RigidBody body, const glm::vec3 & angularVelocity ) {
        impl->bodyInterface->SetAngularVelocity( JPH::BodyID( body.Id ), ToJolt( angularVelocity ) );
    }

    glm::vec3 PhysicsWorld::GetAngularVelocity( RigidBody body ) const {
        return FromJolt( impl->bodyInterface->GetAngularVelocity( JPH::BodyID( body.Id ) ) );
    }

    void PhysicsWorld::AddForce( RigidBody body, const glm::vec3 & force ) {
        impl->bodyInterface->AddForce( JPH::BodyID( body.Id ), ToJolt( force ) );
    }

    void PhysicsWorld::AddTorque( RigidBody body, const glm::vec3 & torque ) {
        impl->bodyInterface->AddTorque( JPH::BodyID( body.Id ), ToJolt( torque ) );
    }

    bool PhysicsWorld::Raycast( const glm::vec3 & origin, const glm::vec3 & direction, f32 maxDistance, RayCastHit & outHit ) const {
        JPH::RRayCast ray( ToJolt( origin ), ToJolt( glm::normalize( direction ) ) * maxDistance );

        JPH::RayCastResult result;
        bool hit = impl->physicsSystem.GetNarrowPhaseQuery().CastRay( ray, result );
        if ( !hit ) {
            return false;
        }

        outHit.body = RigidBody { result.mBodyID.GetIndexAndSequenceNumber() };
        outHit.point = FromJolt( ray.GetPointOnRay( result.mFraction ) );
        return true;
    }

    f32 PhysicsWorld::GetInterpolationAlpha() const {
        return impl->accumulator / FixedTimeStep;
    }

} // namespace sloth
