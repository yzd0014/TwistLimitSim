#pragma once
#include "Engine/GameCommon/GameObject.h"
#include "Engine/Application/cbApplication.h"
#include "External/EigenLibrary/Eigen/Dense"
#include "External/EigenLibrary/Eigen/Geometry"
#include "MultiBodyTypeDefine.h"
#include "Engine/Math/DataTypeDefine.h"
#include "Engine/Math/3DMathHelpers.h"

namespace sca2025
{
	class MultiBody : public sca2025::GameCommon::GameObject
	{
	public:
		MultiBody(Effect * i_pEffect, Assets::cHandle<Mesh> i_Mesh, Physics::sRigidBodyState i_State, Application::cbApplication* i_application);
		void Tick(const double i_secondCountToIntegrate) override;
		void UpdateGameObjectBasedOnInput() override;

		_Scalar damping = 1.0;
		int constraintSolverMode = IMPULSE;
		int constraintType = SWING_C;//only used for testing
		int twistMode = EULER_V2;
		int integrationMethod = EXPLICIT;
		bool gravity = false ;
		bool enablePositionSolve = true;
		bool adaptiveTimestep = false;
		Application::cbApplication* pApp = nullptr;
	private:
		void MultiBodyInitialization();
		void ConfigurateBallJoint(_Vector3& xAxis, _Vector3& yAxis, _Vector3& zAxis, _Scalar swingAngle, _Scalar twistAngle);
		void ConfigureSingleBallJoint(int bodyNum, _Vector3& xAxis, _Vector3& zAxis, _Scalar swingAngle, _Scalar twistAngle);
		void AddRigidBody(int parent, int i_jointType, _Vector3 jointPositionParent, _Vector3 jointPositionChild, Assets::cHandle<Mesh> i_mesh, Vector3d i_meshScale, _Matrix3& i_localInertiaTensor);
		void SetHingeJoint(int jointNum, _Vector3 hingeDirLocal, _Scalar hingeLength);

		void ComputeMr();
		void ComputeHt(_Vector& i_q, std::vector<_Quat>& i_quat);
		_Vector ComputeQr(_Vector i_qdot);
		_Vector ComputeQr_SikpVelocityUpdate(_Vector& i_qdot);
		void ComputeGamma_t(std::vector<_Vector>& o_gamma_t, _Vector& i_qdot);
		
		void ForwardAngularAndTranslationalVelocity(_Vector& i_qdot);
		void ResetExternalForces();
		
		void EulerIntegration(const _Scalar h);
		void RK4Integration(const _Scalar h);
		void Integrate_q(_Vector& o_q, std::vector<_Quat>& o_quat, _Vector& i_q, std::vector<_Quat>& i_quat, _Vector& i_qdot, _Scalar h);

		void ForwardKinematics(_Vector& i_q, std::vector<_Quat>& i_quat);
		void Forward();
		void UpdateBodyRotation(_Vector& i_q, std::vector<_Quat>& i_quat);
		
		void ClampRotationVector();
		_Scalar ComputeKineticEnergy();
		_Scalar ComputePotentialEnergy();
		_Scalar ComputeTotalEnergy();
		_Vector3 ComputeTranslationalMomentum();
		_Vector3 ComputeAngularMomentum();
		
		void BallJointLimitCheck();
		void SolveVelocityJointLimit(const _Scalar h);
		void SolvePositionJointLimit();
		_Scalar ComputeSwingError(int jointNum);
		_Scalar ComputeTwistEulerError(int jointNum);
		void ComputeTwistEulerJacobian(int jointNum, _Matrix& o_J);
		void ComputeTwistEulerJacobian(int i, bool isUpperBound, _Matrix& o_J);
		void ComputeTwistDirectJacobian(int jointNum, int i_limitType, _Matrix& o_J);
		void ComputeSwingJacobian(int jointNum, _Matrix& o_J);
		void SwitchConstraint(int i);
		void UpdateInitialPosition();//call this function whenever poistion is updated
		
		//unit tests
		void UnitTest5_1();//section 5.1 in the paper
		void UnitTest5_2();//section 5.2 in the paper
		void UnitTest5_3a();//section 5.3 in the paper
		void UnitTest5_3b();//section 5.3 in the paper
		void UnitTest5_4a();//section 5.4 in the paper
		void UnitTest5_4b();//section 5.4 in the paper
		void UnitTest5_5();//section 5.5 in the paper
		void UnitTest5_6();//section 5.6 in the paper
		void UnitTest5_7();//section 5.7 in the paper
		void UnitTest5_8a();//section 5.8 in the paper
		void UnitTest5_8b();//section 5.8 in the paper
		void UnitTest0();
		void RunUnitTest();

		void SaveDataToMatlab(_Scalar totalDuration);
		void SaveDataToHoudini(_Scalar totalDuration, _Scalar logInterval, int numOfFrames);

		_Vector q;
		_Vector qdot;
		std::vector<int> jointType;
		std::vector<int> posDOF;
		std::vector<int> velDOF;
		std::vector<int> posStartIndex;
		std::vector<int> velStartIndex;
		std::vector<int> parentArr;
		_Matrix Mr;
		_Matrix MrInverse;
		std::vector<_Matrix> Mbody;
		std::vector<_Matrix3> localInertiaTensors;
		std::vector<_Vector3> w_abs_world;//absolute 
		std::vector<_Vector3> w_rel_world;//relative
		std::vector<_Vector3> w_rel_local;
		std::vector<_Vector3> vel;
		std::vector<_Vector3> pos;//rigid body center of mass
		std::vector<_Vector3> jointPos;
		std::vector<_Vector3> uLocalsChild;
		std::vector<_Vector3> uGlobalsChild;
		std::vector<_Vector3> uLocalsParent;
		std::vector<_Vector3> uGlobalsParent;
		std::vector<_Vector3> hingeDirLocals;
		std::vector<_Vector3> hingeDirGlobals;
		std::vector<_Scalar> hingeMagnitude;//distance between the point from each body that defines the position of the hinge joint
		std::vector<_Vector> externalForces;//extern force in maximal coordinate
		
		std::vector<_Matrix3> R_global;//rigidbody rotation
		std::vector<_Matrix3> R_local;
		std::vector<_Matrix3> J_rotation;//rotation jabobian matrix
		std::vector<_Matrix> D;
		std::vector<_Matrix> Ht;
		std::vector<_Matrix> H;
		
		std::vector<_Quat> obs_ori;
		std::vector<_Quat> rel_ori;//relative rotation to parent for each body
		std::vector<GameCommon::GameObject *> m_linkBodys;
		_Scalar rigidBodyMass = 1.0f;

		std::vector<_Scalar> g;
		std::vector<_Scalar> constraintValue;
		std::vector<int> jointsID;
		std::vector<int> limitType;
		size_t constraintNum = 0;
		
		std::vector<_Scalar> jointLimit;
		std::vector<std::pair<_Scalar, _Scalar>> jointRange;//first is swing, second is twist
		std::vector<_Vector3> twistAxis;
		std::vector<_Vector3> eulerX;
		std::vector<_Vector3> eulerY;
		std::vector<_Vector3> eulerZ;
		std::vector<_Quat> lastValidOri;
		std::vector<_Scalar> mAlpha;
		std::vector<_Scalar> mBeta;
		std::vector<_Scalar> mGamma;
		std::vector<uint16_t> vectorFieldNum;
		std::vector<_Quat> eulerDecompositionOffset;
		std::vector<_Matrix3> eulerDecompositionOffsetMat;
		_Matrix J_constraint;
		_Matrix effectiveMass0;
		_Matrix effectiveMass1;
		_Scalar swingEpsilon = 1e-6;//0.000001;

		std::vector<_Scalar> totalTwist;

		_Scalar kineticEnergy0 = 0;
		_Scalar totalEnergy0 = 0;
		_Vector3 angularMomentum0;
		_Vector3 linearMomentum0;
		//_Vector conservedQuantity;

		int tickCountSimulated = 0;
		int numOfLinks = 0;
		int totalPosDOF = 0;
		int totalVelDOF = 0;
		int totalXDOF = 0;//used for position solve
		int geometry = BOX;
		/*int frameNum = 12;
		_Scalar animationDuration = 5;*/
		int frameNum = 120;
		_Scalar animationDuration = 5;
		_Scalar dt;
		_Scalar totalJointError = 0;

		std::function<void()> m_control;
		std::function<void()> m_MatlabSave;
		std::function<void(int frames_number)> m_HoudiniSave;
		std::function<void(FILE * i_pFile)> m_keyPressSave;

		//debug related parameters
		GameObject* swingArrow = nullptr;
		GameObject* twistArrow = nullptr;
		GameObject* xArrow = nullptr;
		GameObject* yArrow = nullptr;
		GameObject* zArrow = nullptr;
/*******************************************************************************************/
		void GetEulerAngles(int jointNum, _Quat i_quat, _Scalar o_eulerAngles[])
		{
			_Quat inputQuat = eulerDecompositionOffset[jointNum] * i_quat * eulerDecompositionOffset[jointNum].inverse();
			Math::quaternion2Euler(inputQuat, o_eulerAngles, Math::RotSeq::yzx);
		}
		
		inline _Scalar Compute_a(_Scalar theta)
		{
			_Scalar alpha;
			if (theta < 0.0001) alpha = 1.0f - theta * theta / 6.0f;
			else alpha = sin(theta) / theta;

			return alpha;
		}
		inline _Scalar Compute_a_dot(_Scalar c, _Scalar b, _Vector3& r, _Vector3& rdot)
		{
			_Scalar out;
			out = (c - b) * r.dot(rdot);
			return out;
		}
		inline _Scalar Compute_b(_Scalar theta)
		{
			_Scalar beta;
			if (theta < 0.0001) beta = 0.5f - theta * theta / 24.0f;
			else beta = (1.0f - cos(theta)) / (theta * theta);

			return beta;
		}
		inline _Scalar Compute_b_dot(_Scalar theta, _Scalar a, _Scalar b, _Vector3 r, _Vector3 rdot)
		{
			_Scalar out;
			if (theta < 0.0001) out = (-1.0f / 12.0f + 1.0f / 180.0f * theta * theta) * r.dot(rdot);
			else out = (a - 2.0f * b) / (theta * theta) * r.dot(rdot);
			
			return out;
		}

		inline _Scalar Compute_c(_Scalar theta, _Scalar i_a)
		{
			_Scalar gamma;
			if (theta < 0.0001) gamma = 1.0f / 6.0f - theta * theta / 120.0f;
			else gamma = (1.0f - i_a) / (theta * theta);

			return gamma;
		}

		inline _Scalar Compute_c_dot(_Scalar theta, _Scalar b, _Scalar c, _Vector3 r, _Vector3 rdot)
		{
			_Scalar out;
			if (theta < 0.0001) out = (-1.0f / 60.0f + 1.0f / 1260.0f * theta * theta) * r.dot(rdot);
			else out = (b - 3.0f * c) / (theta * theta) * r.dot(rdot);

			return out;
		}

		inline _Scalar Compute_s(_Scalar theta, _Scalar i_a, _Scalar i_b)
		{
			_Scalar zeta;
			if (theta < 0.0001) zeta = 1.0f / 12.0f + theta * theta / 720.0f;
			else zeta = (1 - i_a / (2 * i_b)) / (theta * theta);

			return zeta;
		}
	};
}