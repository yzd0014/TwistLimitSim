#include "MultiBody.h"
#include "Engine/Physics/PhysicsSimulation.h"
#include "Engine/Math/sVector.h"
#include "Engine/Math/EigenHelper.h"
#include "Engine/UserInput/UserInput.h"
#include "Engine/GameCommon/GameplayUtility.h"
#define _USE_MATH_DEFINES
#include <math.h>
#include <iomanip>

sca2025::MultiBody::MultiBody(Effect * i_pEffect, Assets::cHandle<Mesh> i_Mesh, Physics::sRigidBodyState i_State, Application::cbApplication* i_application):
	GameCommon::GameObject(i_pEffect, i_Mesh, i_State)
{
	RunUnitTest();
	
	UpdateInitialPosition();
	pApp = i_application;
}

void sca2025::MultiBody::MultiBodyInitialization()
{
	w_abs_world.resize(numOfLinks);
	w_rel_world.resize(numOfLinks);
	w_rel_local.resize(numOfLinks);
	vel.resize(numOfLinks);
	pos.resize(numOfLinks);
	jointPos.resize(numOfLinks);
	obs_ori.resize(numOfLinks);
	rel_ori.resize(numOfLinks);
	R_global.resize(numOfLinks);
	R_local.resize(numOfLinks);
	J_rotation.resize(numOfLinks);
	D.resize(numOfLinks);
	Ht.resize(numOfLinks);
	H.resize(numOfLinks);
	Mbody.resize(numOfLinks);
	localInertiaTensors.resize(numOfLinks);
	g.resize(numOfLinks);
	jointLimit.resize(numOfLinks);
	jointRange.resize(numOfLinks);
	hingeDirLocals.resize(numOfLinks);
	hingeDirGlobals.resize(numOfLinks);
	hingeMagnitude.resize(numOfLinks);
	twistAxis.resize(numOfLinks);
	eulerX.resize(numOfLinks);
	eulerY.resize(numOfLinks);
	eulerZ.resize(numOfLinks);
	mAlpha.resize(numOfLinks);
	mBeta.resize(numOfLinks);
	mGamma.resize(numOfLinks);
	vectorFieldNum.resize(numOfLinks);
	eulerDecompositionOffset.resize(numOfLinks);
	lastValidOri.resize(numOfLinks);
	eulerDecompositionOffsetMat.resize(numOfLinks);
	totalTwist.resize(numOfLinks);
	externalForces.resize(numOfLinks);
	for (int i = 0; i < numOfLinks; i++)
	{
		w_abs_world[i].setZero();
		w_rel_world[i].setZero();
		w_rel_local[i].setZero();
		vel[i].setZero();
		jointPos[i].setZero();
		pos[i].setZero();
		obs_ori[i].setIdentity();
		rel_ori[i].setIdentity();
		R_global[i].setIdentity();
		R_local[i].setIdentity();

		D[i].resize(6, 6);
		
		jointLimit[i] = -1;
		std::pair<_Scalar, _Scalar> defaultRange(-1, -1);
		jointRange[i] = defaultRange;

		externalForces[i].resize(6);
		externalForces[i].setZero();

		totalTwist[i] = 0;
		lastValidOri[i].setIdentity();

		mAlpha[i] = 0;
		mBeta[i] = 0;
		mGamma[i] = 0;

		vectorFieldNum[i] = 0;
		twistAxis[i] = _Vector3(0, -1, 0);
		eulerX[i] = _Vector3(0, -1, 0);
		eulerY[i] = _Vector3(0, 0, 1);
		eulerZ[i] = _Vector3(-1, 0, 0);

		_Matrix3 deformationGradient;
		Math::ComputeDeformationGradient(eulerY[i], eulerZ[i], eulerX[i], _Vector3(0, 1, 0), _Vector3(0, 0, 1), _Vector3(1, 0, 0), deformationGradient);
		eulerDecompositionOffsetMat[i] = deformationGradient;
		eulerDecompositionOffset[i] = Math::RotationConversion_MatToQuat(deformationGradient);
	}
	Math::NativeVector2EigenVector(m_State.position, jointPos[0]);
	Mr.resize(totalVelDOF, totalVelDOF);
	q.resize(totalPosDOF);
	q.setZero();
	qdot.resize(totalVelDOF);
	qdot.setZero();
}

void sca2025::MultiBody::ConfigurateBallJoint(_Vector3& xAxis, _Vector3& yAxis, _Vector3& zAxis, _Scalar swingAngle, _Scalar twistAngle)
{
	for (int i = 0; i < numOfLinks; i++)
	{
		ConfigureSingleBallJoint(i, xAxis, zAxis, swingAngle, twistAngle);
	}
}

void sca2025::MultiBody::ConfigureSingleBallJoint(int bodyNum, _Vector3& xAxis, _Vector3& zAxis, _Scalar swingAngle, _Scalar twistAngle)
{
	int i = bodyNum;
	eulerX[i] = xAxis;//axis in parent frame
	eulerZ[i] = zAxis;//axis in parent frame
	_Vector3 yAxis;
	yAxis = zAxis.cross(xAxis);
	yAxis.normalize();
	eulerY[i] = yAxis;
	twistAxis[i] = xAxis;

	jointRange[i].first = swingAngle;//swing
	jointRange[i].second = twistAngle;//twist

	_Matrix3 deformationGradient;
	Math::ComputeDeformationGradient(eulerY[i], eulerZ[i], eulerX[i], _Vector3(0, 1, 0), _Vector3(0, 0, 1), _Vector3(1, 0, 0), deformationGradient);
	eulerDecompositionOffsetMat[i] = deformationGradient;
	eulerDecompositionOffset[i] = Math::RotationConversion_MatToQuat(deformationGradient);
}

void sca2025::MultiBody::Tick(const double i_secondCountToIntegrate)
{	
	if (adaptiveTimestep) pApp->UpdateDeltaTime(pApp->GetSimulationUpdatePeriod_inSeconds());
	dt = (_Scalar)i_secondCountToIntegrate;
	tickCountSimulated++;
	/*{
		SaveDataToMatlab(10);
	}*/
	{
		frameNum = 250;
		animationDuration = (frameNum - 1) * (1.0 / 24.0);;
		//SaveDataToHoudini(0, 0.1, 690);
		//SaveDataToHoudini(6, -1, 150);
	}
	
	ResetExternalForces();
	if(m_control) m_control();
	if (integrationMethod == EXPLICIT)
	{
		EulerIntegration(dt);
	}
	else if (integrationMethod == RK4)
	{
		RK4Integration(dt);
	}
}

void sca2025::MultiBody::ClampRotationVector()
{
	for (int i = 0; i < numOfLinks; i++)
	{
		if (jointType[i] == BALL_JOINT_3D)
		{
			_Vector3 r = q.segment(posStartIndex[i], 3);
			_Scalar theta = r.norm();
			if (theta > M_PI)
			{
				_Scalar eta = (_Scalar)(1.0f - 2.0f * M_PI / theta);

				//reparameterize position
				std::cout << "rotation vector clamped" << std::endl;
				q.segment(posStartIndex[i], 3) = eta * r;

				//reparameterize velocity
				_Vector3 r_dot = qdot.segment(velStartIndex[i], 3);
				_Vector3 r_dot_new = eta * r_dot + 2 * M_PI * (r.dot(r_dot) / pow(theta, 3)) * r;
				qdot.segment(velStartIndex[i], 3) = r_dot_new;
			}
		}
	}
}

void sca2025::MultiBody::Integrate_q(_Vector& o_q, std::vector<_Quat>& o_quat, _Vector& i_q, std::vector<_Quat>& i_quat, _Vector& i_qdot, _Scalar h)
{
	for (int i = 0; i < numOfLinks; i++)
	{
		if (jointType[i] == BALL_JOINT_3D)
		{
			o_q.segment(posStartIndex[i], 3) = i_q.segment(posStartIndex[i], 3) + i_qdot.segment(velStartIndex[i], 3) * h;
		}
		else if (jointType[i] == BALL_JOINT_4D)
		{
			Math::QuatIntegrate(o_quat[i], i_quat[i], i_qdot.segment(velStartIndex[i], 3), h);
		}
		else if (jointType[i] == FREE_JOINT)
		{
			o_q.segment(posStartIndex[i], 3) = i_q.segment(posStartIndex[i], 3) + i_qdot.segment(velStartIndex[i], 3) * h;

			w_rel_local[i] = i_qdot.segment(velStartIndex[i] + 3, 3);
			Math::QuatIntegrate(o_quat[i], i_quat[i], w_rel_local[i], h);
		}
		else if (jointType[i] == HINGE_JOINT)
		{
			o_q(posStartIndex[i]) = i_q(posStartIndex[i]) + i_qdot(velStartIndex[i]) * h;
		}
	}
}

void sca2025::MultiBody::EulerIntegration(const _Scalar h)
{
	_Vector Qr = ComputeQr_SikpVelocityUpdate(qdot);
	_Vector qddot = MrInverse * Qr;

	qdot = qdot + qddot * h;
	qdot = damping * qdot;
	if (constraintSolverMode == IMPULSE)
	{
		BallJointLimitCheck();
		SolveVelocityJointLimit(h);
	}
	
	if (enablePositionSolve)
	{
		SolvePositionJointLimit();
	}
	Integrate_q(q, rel_ori, q, rel_ori, qdot, h);
	ClampRotationVector();
	Forward();
}

void sca2025::MultiBody::RK4Integration(const _Scalar h)
{
	_Vector k1 = MrInverse * ComputeQr_SikpVelocityUpdate(qdot);
	_Vector k2 = MrInverse * ComputeQr(qdot + 0.5 * h * k1);
	_Vector k3 = MrInverse * ComputeQr(qdot + 0.5 * h * k2);
	_Vector k4 = MrInverse * ComputeQr(qdot + h * k3);

	_Vector qddot = (1.0f / 6.0f) * (k1 + 2 * k2 + 2 * k3 + k4);
	qdot = qdot + h * qddot;

	if (constraintSolverMode == IMPULSE)
	{
		BallJointLimitCheck();
		SolveVelocityJointLimit(h);
	}

	if (enablePositionSolve)
	{
		SolvePositionJointLimit();
	}
	Integrate_q(q, rel_ori, q, rel_ori, qdot, h);
	ClampRotationVector();
	Forward();
}

void sca2025::MultiBody::ComputeHt(_Vector& i_q, std::vector<_Quat>& i_quat)
{
	ForwardKinematics(i_q, i_quat);
	for (int i = 0; i < numOfLinks; i++)
	{
		int j = parentArr[i];
		if (jointType[i] == BALL_JOINT_4D)
		{
			//compute H
			H[i].resize(6, 3);
			H[i].setZero();
			
			if (i == 0)
			{
				H[i].block<3, 3>(0, 0) = Math::ToSkewSymmetricMatrix(uGlobalsChild[i]);
				H[i].block<3, 3>(3, 0) = _Matrix::Identity(3, 3);
			}
			else
			{
				H[i].block<3, 3>(0, 0) = Math::ToSkewSymmetricMatrix(uGlobalsChild[i]) * R_global[j];
				H[i].block<3, 3>(3, 0) = R_global[j];
			}
			//compute D
			if (i > 0)
			{
				D[i].setIdentity();
				D[i].block<3, 3>(0, 3) = Math::ToSkewSymmetricMatrix(uGlobalsChild[i]) - Math::ToSkewSymmetricMatrix(uGlobalsParent[i]);
			}
		}
		else if (jointType[i] == BALL_JOINT_3D)
		{
			//compute H
			_Vector3 r = i_q.segment(posStartIndex[i], 3);
			_Scalar theta = r.norm();
			_Scalar b = Compute_b(theta);
			_Scalar a = Compute_a(theta);
			_Scalar c = Compute_c(theta, a);
			J_rotation[i] = _Matrix::Identity(3, 3) + b * Math::ToSkewSymmetricMatrix(r) + c * Math::ToSkewSymmetricMatrix(r) * Math::ToSkewSymmetricMatrix(r);
			_Matrix3 A;
			if (i == 0) A = J_rotation[i];
			else A = R_global[j] * J_rotation[i];
			H[i].resize(6, 3);
			H[i].setZero();
			H[i].block<3, 3>(0, 0) = Math::ToSkewSymmetricMatrix(uGlobalsChild[i]) * A;
			H[i].block<3, 3>(3, 0) = A;
			//compute D
			if (i > 0)
			{
				D[i].setIdentity();
				D[i].block<3, 3>(0, 3) = Math::ToSkewSymmetricMatrix(uGlobalsChild[i]) - Math::ToSkewSymmetricMatrix(uGlobalsParent[i]);
			}
		}
		else if (jointType[i] == FREE_JOINT)
		{
			//compute H
			H[i].resize(6, 6);
			H[i].setIdentity();
			//compute D
			D[i].setZero();
		}
		else if (jointType[i] == HINGE_JOINT)
		{
			//compute H
			H[i].resize(6, 1);
			H[i].block<3, 1>(0, 0) = Math::ToSkewSymmetricMatrix(uGlobalsChild[i]) * hingeDirGlobals[i];
			H[i].block<3, 1>(3, 0) = hingeDirGlobals[i];
			//compute D
			D[i].setIdentity();
			if (i > 0)
			{
				_Vector3 hingeVec = hingeMagnitude[i] * hingeDirGlobals[i];
				_Vector3 iVec = uGlobalsChild[i] - uGlobalsParent[i] - hingeVec;
				D[i].block<3, 3>(0, 3) = Math::ToSkewSymmetricMatrix(iVec);
			}
		}
		//compose Ht
		Ht[i].resize(6, totalVelDOF);
		Ht[i].setZero();
		int k = i;
		while (k != -1)
		{
			_Matrix D_temp;
			D_temp.resize(6, 6);
			D_temp.setIdentity();
			int j = i;
			while (j > k)
			{
				D_temp = D_temp * D[j];
				j = parentArr[j];
			}
			Ht[i].block(0, velStartIndex[k], 6, velDOF[k]) = D_temp * H[k];
			k = parentArr[k];
		}
	}
}

void sca2025::MultiBody::ComputeMr()
{
	Mr.setZero();
	for (int i = 0; i < numOfLinks; i++)
	{
		_Matrix M_temp = Ht[i].transpose() * Mbody[i] * Ht[i];
		Mr = Mr + M_temp;
	}
	if (Mr.determinant() < 0.0000001)
	{
		EAE6320_ASSERTF(false, "mass matrix singluarity reached!");
	}
}

_Vector sca2025::MultiBody::ComputeQr_SikpVelocityUpdate(_Vector& i_qdot)
{
	std::vector<_Vector> gamma_t;
	ComputeGamma_t(gamma_t, i_qdot);

	_Vector Qr;
	Qr.resize(totalVelDOF);
	Qr.setZero();
	for (int i = 0; i < numOfLinks; i++)
	{
		if (gravity)
		{
			_Scalar g = -9.8;
			externalForces[i].block<3, 1>(0, 0) = externalForces[i].block<3, 1>(0, 0) + _Vector3(0.0f, g, 0.0f);
		}
		_Vector Fv;
		Fv.resize(6);
		Fv.setZero();
		Fv.block<3, 1>(3, 0) = -w_abs_world[i].cross(Mbody[i].block<3, 3>(3, 3) * w_abs_world[i]);
		_Vector Q_temp;
		Q_temp.resize(totalVelDOF);
		Q_temp.setZero();
		Q_temp = Ht[i].transpose() * (externalForces[i] + Fv - Mbody[i] * gamma_t[i]);
		Qr = Qr + Q_temp;
	}

	return Qr;
}

_Vector sca2025::MultiBody::ComputeQr(_Vector i_qdot)
{
	ForwardAngularAndTranslationalVelocity(i_qdot);
	return ComputeQr_SikpVelocityUpdate(i_qdot);
}

void sca2025::MultiBody::ComputeGamma_t(std::vector<_Vector>& o_gamma_t, _Vector& i_qdot)
{	
	std::vector<_Vector> gamma;
	gamma.resize(numOfLinks);
	for (int i = 0; i < numOfLinks; i++)
	{
		int j = parentArr[i];
		if (jointType[i] == BALL_JOINT_4D)
		{
			_Vector3 r_dot = i_qdot.segment(velStartIndex[i], 3);
			_Vector3 gamma_theta;
			gamma_theta.setZero();
			if (i > 0)
			{
				gamma_theta = Math::ToSkewSymmetricMatrix(w_abs_world[j]) * R_global[j] * r_dot;
			}
			
			gamma[i].resize(6);
			gamma[i].setZero();
			if (i == 0)
			{
				gamma[i].block<3, 1>(0, 0) = Math::ToSkewSymmetricMatrix(uGlobalsChild[i]) * gamma_theta - w_abs_world[i].cross(w_abs_world[i].cross(uGlobalsChild[i]));
			}
			else
			{
				gamma[i].block<3, 1>(0, 0) = Math::ToSkewSymmetricMatrix(uGlobalsChild[i]) * gamma_theta - w_abs_world[i].cross(w_abs_world[i].cross(uGlobalsChild[i])) + w_abs_world[j].cross(w_abs_world[j].cross(uGlobalsParent[i]));
			}
			gamma[i].block<3, 1>(3, 0) = gamma_theta;
		}
		else if (jointType[i] == BALL_JOINT_3D)
		{
			_Vector3 r = q.segment(posStartIndex[i], 3);
			_Vector3 r_dot = i_qdot.segment(velStartIndex[i], 3);
			_Scalar theta = r.norm();
			_Scalar b = Compute_b(theta);
			_Scalar a = Compute_a(theta);
			_Scalar c = Compute_c(theta, a);
			_Scalar a_dot = Compute_a_dot(c, b, r, r_dot);
			_Scalar b_dot = Compute_b_dot(theta, a, b, r, r_dot);
			_Scalar c_dot = Compute_c_dot(theta, b, c, r, r_dot);

			_Vector3 Jdot_rdot;
			Jdot_rdot = (c * r.dot(r_dot) + a_dot) * r_dot - (b_dot * r_dot).cross(r) + (c_dot * r.dot(r_dot) + c * r_dot.dot(r_dot)) * r;
			_Vector3 gamma_theta;
			if (i == 0)
			{
				gamma_theta = Jdot_rdot;
			}
			else
			{
				gamma_theta = Math::ToSkewSymmetricMatrix(w_abs_world[j]) * R_global[j] * J_rotation[i] * r_dot + R_global[j] * Jdot_rdot;
			}
			gamma[i].resize(6);
			gamma[i].setZero();
			if (i == 0)
			{
				gamma[i].block<3, 1>(0, 0) = Math::ToSkewSymmetricMatrix(uGlobalsChild[i]) * gamma_theta - w_abs_world[i].cross(w_abs_world[i].cross(uGlobalsChild[i]));
			}
			else
			{
				gamma[i].block<3, 1>(0, 0) = Math::ToSkewSymmetricMatrix(uGlobalsChild[i]) * gamma_theta - w_abs_world[i].cross(w_abs_world[i].cross(uGlobalsChild[i])) + w_abs_world[j].cross(w_abs_world[j].cross(uGlobalsParent[i]));
			}
			gamma[i].block<3, 1>(3, 0) = gamma_theta;
		}
		else if (jointType[i] == FREE_JOINT)
		{
			gamma[i].resize(6);
			gamma[i].setZero();
		}
		else if (jointType[i] == HINGE_JOINT)
		{
			_Vector3 gamma_theta;
			gamma_theta.setZero();
			if (i > 0)
			{
				gamma_theta += Math::ToSkewSymmetricMatrix(w_abs_world[j]) * hingeDirGlobals[i] * i_qdot(velStartIndex[i]);
			}
			_Vector3 gamma_r;
			gamma_r = -w_abs_world[i].cross(w_abs_world[i].cross(uGlobalsChild[i]));
			_Vector3 hingeVec = hingeMagnitude[i] * hingeDirGlobals[i];
			if (i > 0)
			{
				gamma_r += w_abs_world[j].cross(w_abs_world[j].cross(uGlobalsParent[i] + hingeVec));
			}
			gamma[i].resize(6);
			gamma[i].block<3, 1>(0, 0) = Math::ToSkewSymmetricMatrix(uGlobalsChild[i]) * gamma_theta + gamma_r;
			gamma[i].block<3, 1>(3, 0) = gamma_theta;
		}
	}

	o_gamma_t.resize(numOfLinks);
	for (int i = 0; i < numOfLinks; i++)
	{
		o_gamma_t[i].resize(6);
		o_gamma_t[i].setZero();
		int j = i;
		while (j != -1)
		{
			_Matrix D_temp;
			D_temp.resize(6, 6);
			D_temp.setIdentity();
			int k = i;
			while (k >= j + 1)
			{
				D_temp = D_temp * D[k];
				k = parentArr[k];
			}
			o_gamma_t[i] = o_gamma_t[i] + D_temp * gamma[j];
			j = parentArr[j];
		}
	}
}

void sca2025::MultiBody::ForwardAngularAndTranslationalVelocity(_Vector& i_qdot)
{
	for (int i = 0; i < numOfLinks; i++)
	{
		_Vector tran_rot_velocity;
		tran_rot_velocity = Ht[i] * i_qdot;
		vel[i] = tran_rot_velocity.segment(0, 3);
		w_abs_world[i] = tran_rot_velocity.segment(3, 3);
	}
}

void sca2025::MultiBody::UpdateBodyRotation(_Vector& i_q, std::vector<_Quat>& i_quat)
{
	for (int i = 0; i < numOfLinks; i++)
	{
		int j = parentArr[i];
		//update orientation
		if (jointType[i] == BALL_JOINT_4D)
		{
			if (i == 0)
			{
				obs_ori[i] = i_quat[i];
			}
			else
			{
				obs_ori[i] = obs_ori[j] * i_quat[i];
			}
			R_local[i] = i_quat[i].toRotationMatrix();
			R_global[i] = obs_ori[i].toRotationMatrix();
			m_linkBodys[i]->m_State.orientation = Math::ConvertEigenQuatToNativeQuat(obs_ori[i]);
		}
		else if (jointType[i] == BALL_JOINT_3D)
		{
			_Vector3 r = i_q.segment(posStartIndex[i], 3);

#if defined (HIGH_PRECISION_MODE)
			R_local[i] = AngleAxisd(r.norm(), r.normalized());
#else
			R_local[i] = AngleAxisf(r.norm(), r.normalized());
#endif
			if (i == 0)
			{
				R_global[i] = R_local[i];
			}
			else
			{
				int j = parentArr[i];
				R_global[i] = R_global[j] * R_local[i];
			}

#if defined (HIGH_PRECISION_MODE)
			AngleAxisd angleAxis_global(R_global[i]);
#else
			AngleAxisf angleAxis_global(R_global[i]);
#endif

			m_linkBodys[i]->m_State.orientation = Math::cQuaternion((float)angleAxis_global.angle(), Math::EigenVector2nativeVector(angleAxis_global.axis()));
			m_linkBodys[i]->m_State.orientation.Normalize();
		}
		else if (jointType[i] == FREE_JOINT)
		{
			obs_ori[i] = i_quat[i];
			R_local[i] = obs_ori[i].toRotationMatrix();
			R_global[i] = obs_ori[i].toRotationMatrix();
			m_linkBodys[i]->m_State.orientation = Math::ConvertEigenQuatToNativeQuat(obs_ori[i]);
		}
		else if (jointType[i] == HINGE_JOINT)
		{
			int j = parentArr[i];
			
			_Scalar angle = i_q(posStartIndex[i]);
#if defined (HIGH_PRECISION_MODE)
			R_local[i] = AngleAxisd(angle, hingeDirLocals[i]);
#else
			R_local[i] = AngleAxisf(angle, axis);
#endif
			if (i == 0)
			{
				R_global[i] = R_local[i];
			}
			else
			{
				R_global[i] = R_global[j] * R_local[i];
			}
			
			if (i > 0) hingeDirGlobals[i] = R_global[i] * hingeDirLocals[i];

#if defined (HIGH_PRECISION_MODE)
			AngleAxisd angleAxis_global(R_global[i]);
#else
			AngleAxisf angleAxis_global(R_global[i]);
#endif
			m_linkBodys[i]->m_State.orientation = Math::cQuaternion((float)angleAxis_global.angle(), Math::EigenVector2nativeVector(angleAxis_global.axis()));
			m_linkBodys[i]->m_State.orientation.Normalize();
		}
	}
}

void sca2025::MultiBody::ForwardKinematics(_Vector& i_q, std::vector<_Quat>& i_quat)
{
	UpdateBodyRotation(i_q, i_quat);
	for (int i = 0; i < numOfLinks; i++)
	{
		int j = parentArr[i];
		//update position
		uGlobalsChild[i] = R_global[i] * uLocalsChild[i];
		if (i > 0)
		{
			uGlobalsParent[i] = R_global[j] * uLocalsParent[i];
			jointPos[i] = pos[j] + uGlobalsParent[i];
		}
		if (jointType[i] == BALL_JOINT_3D || jointType[i] == BALL_JOINT_4D)
		{
			pos[i] = jointPos[i] - uGlobalsChild[i];
		}
		else if (jointType[i] == FREE_JOINT)
		{
			pos[i] = i_q.segment(posStartIndex[i], 3);
		}
		else if (jointType[i] == HINGE_JOINT)
		{
			pos[i] = jointPos[i] + hingeMagnitude[i] * hingeDirGlobals[i] - uGlobalsChild[i];
		}

		//update Euler angles
		_Scalar eulerAngles[3];
		GetEulerAngles(i, i_quat[i], eulerAngles);
		mAlpha[i] = eulerAngles[2];
		mBeta[i] = eulerAngles[1];
		mGamma[i] = eulerAngles[0];
				
		//update render position
		m_linkBodys[i]->m_State.position = Math::sVector((float)pos[i](0), (float)pos[i](1), (float)pos[i](2));
		
		//update inertia tensor
		if (geometry != BOX && geometry != BALL)
		{
			_Matrix3 globalInertiaTensor;
			globalInertiaTensor = R_global[i] * localInertiaTensors[i] * R_global[i].transpose();
			Mbody[i].block<3, 3>(3, 3) = globalInertiaTensor;
		}	
	}
}

void sca2025::MultiBody::ResetExternalForces()
{
	for (int i = 0; i < numOfLinks; i++)
	{
		externalForces[i].setZero();
	}
}

void sca2025::MultiBody::Forward()
{
	ComputeHt(q, rel_ori);
	ComputeMr();
	MrInverse = Mr.inverse();
	ForwardAngularAndTranslationalVelocity(qdot);
}

_Vector3 sca2025::MultiBody::ComputeTranslationalMomentum()
{
	_Vector3 translationalMomentum;
	translationalMomentum.setZero();
	for (int i = 0; i < numOfLinks; i++)
	{
		translationalMomentum += Mbody[i].block<3, 3>(0, 0) * vel[i];
	}
	return translationalMomentum;
}

_Vector3 sca2025::MultiBody::ComputeAngularMomentum()
{
	_Vector3 angularMomentum;
	angularMomentum.setZero();
	for (int i = 0; i < numOfLinks; i++)
	{
		angularMomentum += Mbody[i].block<3, 3>(3, 3) * w_abs_world[i] + rigidBodyMass * pos[i].cross(vel[i]);
	}
	return angularMomentum;
}

_Scalar sca2025::MultiBody::ComputeKineticEnergy()
{
	_Scalar out = 0;
	for (int i = 0; i < numOfLinks; i++)
	{
		_Scalar kineticEnergyRotation = 0;
		kineticEnergyRotation = 0.5 * w_abs_world[i].transpose() * Mbody[i].block<3, 3>(3, 3) * w_abs_world[i];

		_Scalar kineticEnergyTranslaion = 0;
		kineticEnergyTranslaion = 0.5 * vel[i].transpose() * Mbody[i].block<3, 3>(0, 0) * vel[i];

		out += kineticEnergyRotation + kineticEnergyTranslaion;
	}
	return out;
}

_Scalar sca2025::MultiBody::ComputePotentialEnergy()
{
	_Scalar out = 0;
	for (int i = 0; i < numOfLinks; i++)
	{
		_Scalar potentialEnergy = 0;
		_Vector3 g(0.0f, 9.81f, 0.0f);
		_Vector3 x;
		Math::NativeVector2EigenVector(m_linkBodys[i]->m_State.position, x);
		potentialEnergy = g.transpose() * Mbody[i].block<3, 3>(0, 0) * x;

		out += potentialEnergy;
	}
	return out;
}

_Scalar sca2025::MultiBody::ComputeTotalEnergy()
{
	_Scalar energy = ComputeKineticEnergy();
	if (gravity) energy += ComputePotentialEnergy();
	return energy;
}

void sca2025::MultiBody::SetHingeJoint(int jointNum, _Vector3 hingeDirLocal, _Scalar hingeLength)
{
	hingeDirLocals[jointNum] = hingeDirLocal.normalized();
	hingeDirGlobals[jointNum] = hingeDirLocals[jointNum];
	hingeMagnitude[jointNum] = hingeLength;
}

void sca2025::MultiBody::AddRigidBody(int parent, int i_jointType, _Vector3 jointPositionChild, _Vector3 jointPositionParent, Assets::cHandle<Mesh> i_mesh, Vector3d i_meshScale, _Matrix3& i_localInertiaTensor)
{
	//initialize body
	GameCommon::GameObject *pGameObject = new GameCommon::GameObject(defaultEffect, i_mesh, Physics::sRigidBodyState());
	pGameObject->scale = i_meshScale;
	m_linkBodys.push_back(pGameObject);
	localInertiaTensors.push_back(i_localInertiaTensor);
	_Matrix M_d;
	M_d.resize(6, 6);
	M_d.setZero();
	M_d(0, 0) = rigidBodyMass;
	M_d(1, 1) = rigidBodyMass;
	M_d(2, 2) = rigidBodyMass;
	M_d.block<3, 3>(3, 3) = i_localInertiaTensor;
	Mbody.push_back(M_d);

	parentArr.push_back(-1);
	parentArr[numOfLinks] = parent;
	
	//initialize joint
	jointType.push_back(i_jointType);
	if (i_jointType == BALL_JOINT_4D)
	{
		velDOF.push_back(3);
		posDOF.push_back(4);
		totalVelDOF += 3;
		totalPosDOF += 4;
	}
	else if (i_jointType == FREE_JOINT)
	{
		velDOF.push_back(6);
		posDOF.push_back(7);
		totalVelDOF += 6;
		totalPosDOF += 7;
	}
	else if (i_jointType == HINGE_JOINT)
	{
		velDOF.push_back(1);
		posDOF.push_back(1);
		totalVelDOF += 1;
		totalPosDOF += 1;
	}
	else
	{
		std::cout << "TODO for other type of joints" << std::endl;//TODO
	}
	if (parent == -1)
	{
		velStartIndex.push_back(0);
		posStartIndex.push_back(0);
	}
	else
	{
		velStartIndex.push_back(velStartIndex[numOfLinks - 1] + velDOF[numOfLinks - 1]);
		posStartIndex.push_back(posStartIndex[numOfLinks - 1] + posDOF[numOfLinks - 1]);
	}
	uLocalsChild.push_back(jointPositionChild);
	uGlobalsChild.push_back(jointPositionChild);
	uLocalsParent.push_back(jointPositionParent);
	uGlobalsParent.push_back(jointPositionParent);

	//hinge joint specific
	hingeDirGlobals.push_back(_Vector3(0, 0, 0));
	hingeDirLocals.push_back(_Vector3(0, 0, 0));
	hingeMagnitude.push_back(0);
	
	numOfLinks++;
}

void sca2025::MultiBody::UpdateGameObjectBasedOnInput()
{
	if (UserInput::IsKeyFromReleasedToPressed('R'))
	{
		//save binary data to file
		FILE * pFile;
		const char* filePath = "key_press_save.txt";
		pFile = fopen(filePath, "wb");
		if (m_keyPressSave)
		{
			m_keyPressSave(pFile);
		};
		fclose(pFile);
		std::cout << "data saved to file" << std::endl;
	}
	if (UserInput::IsKeyFromReleasedToPressed('K'))
	{
		//Save data to Houdini
		static int frames_saved = 1;
		LOG_TO_FILE << frames_saved << ",";
		for (int i = 0; i < numOfLinks; i++)
		{
			_Vector3 vecRot = Math::RotationConversion_QuatToVec(obs_ori[i]);
			_Scalar rotAngle = vecRot.norm();
			if (abs(rotAngle) < 1e-8)
			{
				vecRot = _Vector3(1, 0, 0);
			}
			else
			{
				vecRot = vecRot / rotAngle;
			}
			LOG_TO_FILE << pos[i](0) << "," << pos[i](1) << "," << pos[i](2) << "," << vecRot(0) << "," << vecRot(1) << "," << vecRot(2) << "," << rotAngle;
			if (i != numOfLinks - 1)
			{
				LOG_TO_FILE << ",";
			}
			else
			{
				LOG_TO_FILE << std::endl;
			}
		}
		frames_saved++;
		std::cout << "a frame saved!" << std::endl;
	}
}

void sca2025::MultiBody::SaveDataToMatlab(_Scalar totalDuration)
{
	static _Scalar targetTime = 1;
	static int frames_saved = 0;
	_Scalar t = (_Scalar)sca2025::Physics::totalSimulationTime;
	_Scalar logInterval = 0.0001;
	if (t <= totalDuration)
	{	
		if (t >= targetTime || t == 0)
		{
			frames_saved++;
			if (m_MatlabSave)
			{
				m_MatlabSave();
			}
			targetTime = logInterval * frames_saved;
		}
	}
	else
	{
		sca2025::Physics::simPause = true;
	}
}

void sca2025::MultiBody::SaveDataToHoudini(_Scalar totalDuration, _Scalar logInterval, int numOfFrames)
{
	_Scalar interval;
	if (logInterval < 0)
	{
		interval = totalDuration / (numOfFrames - 1);
	}
	else
	{
		interval = logInterval;
	}
	static _Scalar targetTime = 1;
	static int frames_saved = 0;
	_Scalar t = (_Scalar)sca2025::Physics::totalSimulationTime;
	if (frames_saved < numOfFrames)
	{
		if (t == 0 || t >= targetTime)
		{
			frames_saved++;
			if (m_HoudiniSave)
			{
				m_HoudiniSave(frames_saved);
			}
			targetTime = interval * frames_saved;
		}
	}
	else
	{
		sca2025::Physics::simPause = true;
	}
}