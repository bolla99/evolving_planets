//
// Created by Giovanni Bollati on 06/03/26.
//

#ifndef EVOLVING_PLANETS_PHYSICSSYSTEM_HPP
#define EVOLVING_PLANETS_PHYSICSSYSTEM_HPP

#include <btBulletDynamicsCommon.h>
#include <unordered_map>
#include "Engine/ECS/Components.hpp"
#include "Engine/ECS/World.hpp"
#include "Engine/ECS/Systems.hpp"


struct PhysicsSystem : public ISystem
{
    PhysicsSystem()
    {
        // INIT BULLET
        collisionConfiguration = new btDefaultCollisionConfiguration();
        dispatcher = new btCollisionDispatcher(collisionConfiguration);
        overlappingPairCache = new btDbvtBroadphase();
        solver = new btSequentialImpulseConstraintSolver();

        dynamicWorld = new btDiscreteDynamicsWorld(dispatcher, overlappingPairCache, solver, collisionConfiguration);
        dynamicWorld->setGravity(btVector3(0, -9.81f, 0));
    }

    void update(World& world, const Context& ctx, float dt) override
    {
        // CLEAN DELETED RIGID BODIES
        for (auto it = activeBodies.begin(); it != activeBodies.end(); ) {
            if (!world.hasEntity(it->first) || !world.hasComponent<RigidBodyComponent>(it->first)) {
                dynamicWorld->removeRigidBody(it->second);
                delete it->second; // Dealloca il rigid body
                it = activeBodies.erase(it); // Rimuove dalla mappa
            } else {
                ++it;
            }
        }
        // GET ENTITIES
        auto entities = world.query<RigidBodyComponent, Transform>();

        // INITIALIZE NEW RIGID BODIES
        for (auto& entity : entities)
        {
            auto& rb = world.getComponent<RigidBodyComponent>(entity);
            auto& transform = world.getComponent<Transform>(entity);

            // initialize rigid body if not already done
            if (!rb.body)
            {
                // set collider
                switch (rb.colliderType) {
                case ColliderType::SPHERE:
                    rb.btCollider = new btSphereShape(rb.radius);
                    break;
                case ColliderType::BOX:
                    rb.btCollider = new btBoxShape(btVector3(rb.boxHalfExtents.x, rb.boxHalfExtents.y, rb.boxHalfExtents.z));
                    break;
                default: ;
                }
                rb.btCollider->setMargin(0.05f);
                if (rb.mass > 0.0f) {
                    rb.btCollider->calculateLocalInertia(rb.mass, rb.localIntertia);
                }
                // set motion state
                rb.motionState = new btDefaultMotionState(btTransform(
                    btQuaternion(transform.rotation.x, transform.rotation.y, transform.rotation.z, transform.rotation.w),
                    btVector3(transform.position.x, transform.position.y, transform.position.z)
                ));
                // set rigidbody
                auto rigidBodyInfo = btRigidBody::btRigidBodyConstructionInfo(rb.mass, rb.motionState, rb.btCollider, rb.localIntertia);
                rb.body = new btRigidBody(rigidBodyInfo);
                rb.body->setRestitution(rb.bounciness);
                rb.body->setFriction(rb.friction);

                if (rb.isKinematic)
                {
                    rb.body->setCollisionFlags(rb.body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
                    rb.body->setActivationState(DISABLE_DEACTIVATION);
                }

                dynamicWorld->addRigidBody(rb.body);
                activeBodies[entity] = rb.body;
            }
        }

        // UPDATE DIRTIES
        for (auto& entity : entities)
        {
            auto& rb = world.getComponent<RigidBodyComponent>(entity);
            auto& tf = world.getComponent<Transform>(entity);

            // update active
            if (rb.dirty)
            {
                if (rb.active) {
                rb.body->setCollisionFlags(rb.body->getCollisionFlags() & ~btCollisionObject::CF_NO_CONTACT_RESPONSE);
                rb.body->forceActivationState(ACTIVE_TAG);
                rb.body->activate(true);
                }
                else
                {
                    rb.body->setCollisionFlags(rb.body->getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE);
                    rb.body->forceActivationState(DISABLE_SIMULATION);
                    rb.body->setLinearVelocity({0.0f, 0.0f, 0.0f});
                    rb.body->setAngularVelocity({0.0f, 0.0f, 0.0f});
                    rb.body->clearForces();
                }
            }
        }

        // TELEPORT AND UPDATE KINEMATICS
        for (auto& entity : entities) {
            auto& rb = world.getComponent<RigidBodyComponent>(entity);
            auto& tf = world.getComponent<Transform>(entity);

            if (rb.teleportRequested)
            {
                btTransform target;
                target.setOrigin(btVector3(rb.teleportPos.x, rb.teleportPos.y, rb.teleportPos.z));
                target.setRotation(btQuaternion(rb.teleportRot.x, rb.teleportRot.y, rb.teleportRot.z, rb.teleportRot.w));

                // 1. Sposta il corpo fisico
                rb.body->setWorldTransform(target);

                // 2. Fondamentale: aggiorna anche il MotionState
                rb.motionState->setWorldTransform(target);

                // 3. Azzera le forze residue (altrimenti l'oggetto "esplode" via)
                rb.body->setLinearVelocity(btVector3(0, 0, 0));
                rb.body->setAngularVelocity(btVector3(0, 0, 0));
                rb.body->clearForces();
                rb.body->activate(true);

                rb.teleportRequested = false;

                // Aggiorna subito anche il transform grafico per evitare lag di 1 frame
                tf.position = rb.teleportPos;
                tf.rotation = rb.teleportRot;
            }

            if (rb.isKinematic) {
                // SINCRONIZZA IL KINEMATICO: Transform -> Bullet
                btTransform target;
                target.setOrigin(btVector3(tf.position.x, tf.position.y, tf.position.z));
                target.setRotation(btQuaternion(tf.rotation.x, tf.rotation.y, tf.rotation.z, tf.rotation.w));

                // Per i kinematici si aggiorna il MotionState per calcolare le velocità di attrito
                rb.motionState->setWorldTransform(target);
            }
        }

        // update dynamic world
        if (dt > 0.0f) {
            dynamicWorld->stepSimulation(dt, 10);
        }

        for (auto& entity : entities) {
            auto& rb = world.getComponent<RigidBodyComponent>(entity);
            auto& tf = world.getComponent<Transform>(entity);

            // update transform from rigid body
            if (not rb.isKinematic and rb.mass > 0.0f)
            {
                btTransform btTrans;
                rb.body->getMotionState()->getWorldTransform(btTrans);
                tf.position = glm::vec3(btTrans.getOrigin().getX(), btTrans.getOrigin().getY(), btTrans.getOrigin().getZ());
                auto btRot = btTrans.getRotation();
                tf.rotation = glm::quat(btRot.getW(), btRot.getX(), btRot.getY(), btRot.getZ());
            }
        }
    }
    [[nodiscard]] std::string name() const override { return "PhysicsSystem"; }

    btCollisionConfiguration* collisionConfiguration;
    btCollisionDispatcher* dispatcher;
    btBroadphaseInterface* overlappingPairCache;
    btConstraintSolver* solver;
    btDynamicsWorld* dynamicWorld = nullptr;
    std::unordered_map<uint64_t, btRigidBody*> activeBodies;
};

#endif //EVOLVING_PLANETS_PHYSICSSYSTEM_HPP