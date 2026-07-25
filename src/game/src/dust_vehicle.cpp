#include <dust_vehicle.h>

#include <dust_entity.h>

namespace dust {
    const char * ToString( VehicleChassisType type ) {
        switch ( type ) {
            case VEHICLE_CHASSIS_TYPE_BUGGY:
                return "Buggy";
            case VEHICLE_CHASSIS_TYPE_TRUCK:
                return "Truck";
            case VEHICLE_CHASSIS_TYPE_APC:
                return "APC";
            case VEHICLE_CHASSIS_TYPE_TANK:
                return "Tank";
            case VEHICLE_CHASSIS_TYPE_CRAWLER:
                return "Crawler";
        }

        return "Unknown";
    }

    
    void DriveVehicle( PhysicsWorld & physicsWorld, Entity & entity, f32 throttle, f32 steer, f32 deltaTime ) {
        if ( entity.type != ENTITY_TYPE_VEHICLE || !entity.rigidBody.IsValid() ) {
            return;
        }

        VehicleData & vehicle = entity.vehicle;

        glm::vec3 forward = entity.rotation * glm::vec3( 0.0f, 0.0f, 1.0f );
        glm::vec3 right = entity.rotation * glm::vec3( 1.0f, 0.0f, 0.0f );
        glm::vec3 up = entity.rotation * glm::vec3( 0.0f, 1.0f, 0.0f );

        glm::vec3 velocity = physicsWorld.GetLinearVelocity( entity.rigidBody );
        f32 forwardSpeed = glm::dot( velocity, forward );

        if ( throttle != 0.0f && glm::abs( forwardSpeed ) < vehicle.maxSpeed ) {
            physicsWorld.AddForce( entity.rigidBody, forward * throttle * vehicle.enginePower );
        }

        // Steering torque; flips sign in reverse like a real car backing up.
        // Not scaled down at low speed: the chassis is a flat box resting
        // directly on the ground (no wheels), so its own yaw friction
        // already resists spinning in place — a speed-scaled torque on top
        // of that was enough to fully cancel out at low speed and left
        // steering doing nothing. Turn rate is capped below instead.
        if ( steer != 0.0f ) {
            f32 reverseSign = forwardSpeed < 0.0f ? -1.0f : 1.0f;
            physicsWorld.AddTorque( entity.rigidBody, up * steer * reverseSign * vehicle.turnTorque );
        }

        // Clamp yaw spin rate so the steering torque above (sized to
        // overcome ground friction) doesn't turn the vehicle into a
        // spinning top once it gets going.
        {
            glm::vec3 angularVelocity = physicsWorld.GetAngularVelocity( entity.rigidBody );
            f32 yawRate = glm::dot( angularVelocity, up );
            f32 clampedYawRate = glm::clamp( yawRate, -vehicle.maxYawRateRadians, vehicle.maxYawRateRadians );
            if ( clampedYawRate != yawRate ) {
                physicsWorld.SetAngularVelocity( entity.rigidBody, angularVelocity + up * ( clampedYawRate - yawRate ) );
            }
        }

        // Arcade tire grip: cancel most sideways velocity each frame so the
        // vehicle corners instead of sliding around like a hockey puck.
        // Stands in for real wheel friction until there's an actual wheel
        // model.
        f32 lateralSpeed = glm::dot( velocity, right );
        f32 gripFactor = glm::clamp( vehicle.gripStrength * deltaTime, 0.0f, 1.0f );
        physicsWorld.SetLinearVelocity( entity.rigidBody, velocity - right * lateralSpeed * gripFactor );

        // Visual-only wheel state, consumed by DustGame::DrawVehicle().
        vehicle.steerAngleDegrees = glm::mix( vehicle.steerAngleDegrees, steer * vehicle.maxSteerAngleDegrees, glm::clamp( 8.0f * deltaTime, 0.0f, 1.0f ) );
        vehicle.wheelSpinRadians += ( forwardSpeed / glm::max( vehicle.wheelRadius, 0.01f ) ) * deltaTime;
    }

    void DriveVehicleToward( PhysicsWorld & physicsWorld, Entity & entity, glm::vec3 targetPosition, f32 deltaTime ) {
        // Distance at which throttle starts ramping down instead of driving
        // in at full speed and overshooting/orbiting the target.
        constexpr f32 slowRadius = 15.0f;
        // Full steering lock is used once misaligned by this much or more;
        // scales down smoothly below it so the vehicle doesn't twitch once
        // it's basically already facing the target.
        constexpr f32 fullSteerAngle = 30.0f * 3.14159265f / 180.0f;

        glm::vec3 toTarget = targetPosition - entity.position;
        toTarget.y = 0.0f;
        f32 distance = glm::length( toTarget );
        if ( distance < 0.01f ) {
            return;
        }
        glm::vec3 toTargetDir = toTarget / distance;

        glm::vec3 forward = entity.rotation * glm::vec3( 0.0f, 0.0f, 1.0f );
        forward.y = 0.0f;
        f32 forwardLength = glm::length( forward );
        if ( forwardLength < 0.01f ) {
            return;
        }
        forward /= forwardLength;

        glm::vec3 up = entity.rotation * glm::vec3( 0.0f, 1.0f, 0.0f );

        // Signed angle from `forward` to `toTargetDir` about `up`, in the
        // same sign convention DriveVehicle's steering torque uses (positive
        // steer rotates `forward` toward `up × forward`) - so a positive
        // angle here always means "steer positive to close it", regardless
        // of which way the vehicle happens to be facing.
        f32 angle = glm::atan( glm::dot( up, glm::cross( forward, toTargetDir ) ), glm::dot( forward, toTargetDir ) );

        f32 steer = glm::clamp( angle / fullSteerAngle, -1.0f, 1.0f );
        f32 throttle = glm::clamp( distance / slowRadius, 0.25f, 1.0f );

        DriveVehicle( physicsWorld, entity, throttle, steer, deltaTime );
    }
}
