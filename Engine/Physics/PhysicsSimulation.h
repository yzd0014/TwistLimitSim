#pragma once
#include "Engine/GameCommon/GameObject.h"
#include "PointJoint.h"
#include "HingeJoint.h"
#include <vector>
#define constraintMaxNum 50
namespace sca2025 {
	struct CollisionPair;

	namespace Physics {
		extern double totalSimulationTime;
		
		extern bool simPause;
		extern bool nextSimStep;
		extern bool simPlay;

		extern std::vector<ContactManifold3D> allManifolds;
		extern std::vector<PointJoint> allPointJoints;
		extern std::vector<HingeJoint> allHingeJoints;
		
		void InitializePhysics(std::vector<GameCommon::GameObject *> & i_colliderObjects, std::vector<GameCommon::GameObject *> & i_noColliderObjects);
		//void ConstraintResolver(std::vector<ContactManifold3D>& o_allManifolds, float i_dt);
		//void RunPhysics(std::vector<GameCommon::GameObject *> & i_allGameObjects, std::vector<GameCommon::GameObject *> & i_debugGraphics, Assets::cHandle<Mesh> i_debugMesh, Effect* i_pDebugEffect, float i_dt);
		void RunPhysics(std::vector<GameCommon::GameObject *> & i_colliderObjects, std::vector<GameCommon::GameObject *> & i_noColliderObjects, float i_dt);
		
		//*************following functions ared used for continuious collision detection************************//
		void MoveObjectsForward(std::vector<GameCommon::GameObject *> & o_allGameObjects, float timeSpan);
		void PhysicsUpdate(std::vector<GameCommon::GameObject *> & o_allGameObjects, float i_dt);
		void ResolveCollision(CollisionPair & o_earliestCollision);
	}
}
