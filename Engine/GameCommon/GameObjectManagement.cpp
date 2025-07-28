#include "GameObject.h"
#include "Camera.h"
namespace sca2025 {
	std::vector<GameCommon::GameObject *> colliderObjects;//game objects with colliders
	std::vector<GameCommon::GameObject *> noColliderObjects;//game objects without colliders
	Concurrency::cMutex gameObjectArrayMutex;

	namespace GameCommon {
		void RemoveInactiveGameObjects(std::vector<GameObject *> & o_allGameObjects) {
			for (size_t i = 0; i < o_allGameObjects.size(); i++) {
				if (o_allGameObjects[i]->isActive() == false) {
					delete o_allGameObjects[i];
					o_allGameObjects[i] = o_allGameObjects.back();
					o_allGameObjects.pop_back();
					o_allGameObjects.shrink_to_fit();
					i--;
				}
			}
		}

		void ResetAllGameObjectsVelo(std::vector<GameObject *> & o_gameObjectsWithCollider, std::vector<GameObject *> & o_gameObjectsWithoutCollider, Camera & o_camera) {
			size_t numOfObjects = o_gameObjectsWithCollider.size();
			for (size_t i = 0; i < numOfObjects; i++) {
				o_gameObjectsWithCollider[i]->m_State.velocity = Math::sVector(0, 0, 0);
				o_gameObjectsWithCollider[i]->m_State.angularVelocity = Math::sVector(0, 0, 0);
			}
			numOfObjects = o_gameObjectsWithoutCollider.size();
			for (size_t i = 0; i < numOfObjects; i++) {
				o_gameObjectsWithoutCollider[i]->m_State.velocity = Math::sVector(0, 0, 0);
				o_gameObjectsWithoutCollider[i]->m_State.angularVelocity = Math::sVector(0, 0, 0);
			}

			o_camera.velocity = Math::sVector(0, 0, 0);
			o_camera.axis_X_velocity = 0;
			o_camera.axis_Y_velocity = 0;
		}
	}
}

