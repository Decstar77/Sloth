#pragma once

#include <dust_item.h>

#include <physics/sloth_physics_world.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace dust {
    struct Entity;

    enum VehicleChassisType {
        VEHICLE_CHASSIS_TYPE_BUGGY,
        VEHICLE_CHASSIS_TYPE_TRUCK,
        VEHICLE_CHASSIS_TYPE_APC,
        VEHICLE_CHASSIS_TYPE_TANK,
        VEHICLE_CHASSIS_TYPE_CRAWLER, // Aircraft carrier but on wheels
    };

    struct VehicleChassisDefinition {
        VehicleChassisType chassisType;
        union {
            struct {
                InventoryItemType engineSlot;
                InventoryItemType tireSlot;
                InventoryItemType turretSlot;
                InventoryItemType powerSlot;
                InventoryItemType generalSlot1;
                InventoryItemType generalSlot2;
            } buggy;
            struct {
                InventoryItemType engineSlot;
                InventoryItemType tireSlot;
                InventoryItemType turretSlot;
                InventoryItemType powerSlot;
                InventoryItemType generalSlot1;
                InventoryItemType generalSlot2;
                InventoryItemType generalSlot3;
                InventoryItemType generalSlot4;
            } truck;
        };
    };

    struct VehicleComputedStats {
        glm::vec3 chassisHalfExtents = { 0.9f, 0.35f, 1.7f };
        glm::vec3 wheelOffsets[4] = { { 0.9f, 0.10f, 1.3f }, { -0.9f, 0.10f, 1.3f }, { 0.9f, 0.10f, -1.3f }, { -0.9f, 0.10f, -1.3f } };
        f32 wheelRadius = 0.45f;
        f32 wheelWidth = 0.3f;
        f32 enginePower = 18000.0f;   // N, forward/back drive force
        f32 turnTorque = 18000.0f;    // steering yaw torque
        f32 maxYawRateRadians = 2.2f; // rad/s, clamps the turn once spun up
        f32 maxSpeed = 10.0f;         // m/s, engine cuts out past this
        f32 gripStrength = 6.0f;      // 1/s, how hard sideways slide is cancelled
        f32 maxSteerAngleDegrees = 30.0f;
        f32 steerAngleDegrees = 0.0f;
        f32 wheelSpinRadians = 0.0f;
    };

    struct VehiclePhysicsObjects {
        RigidBody body;

    };

    struct VehicleData {
        VehicleChassisDefinition    definition;
        VehiclePhysicsObjects       physics;
        VehicleComputedStats        stats;
    };

    void DriveVehicle( PhysicsWorld & physicsWorld, Entity & entity, f32 throttle, f32 steer, f32 deltaTime );
    void DriveVehicleToward( PhysicsWorld & physicsWorld, Entity & entity, glm::vec3 targetPosition, f32 deltaTime );


}
