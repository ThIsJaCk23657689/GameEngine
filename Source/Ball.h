#pragma once
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>
#include "Utill.h"
#include "Obstacle.h"

constexpr float BALL_MAX_SPEED = 5.0f;



class Ball {
public:
	Ball(glm::vec3 positon, glm::vec3 velocity, float mass = 1.0f) {
		this->Mass = mass;
		this->Position = positon;
		this->Velocity = velocity;
		this->Acceleration = glm::vec3(0.0f);

		this->Radius = this->Mass * 0.05;
		
		glm::mat4 model(1.0f);
		model = glm::translate(model, positon);
		model = glm::scale(model, glm::vec3(this->Radius));
		this->Model = model;

		this->Ambient = glm::vec4(0.2f, 0.2f, 0.2f, 1.0);
		this->Diffuse = glm::vec4(1.0f, 0.25f, 0.25f, 1.0);
		this->Specular = glm::vec4(0.55f, 0.45f, 0.45f, 1.0);
	}

	void Update(float delta_time, float gravity, float dragforce) {

		this->Acceleration = glm::vec3(0.0f);
		this->Force = dragforce * -this->Velocity;
		this->Acceleration = this->Force / this->Mass;
		this->Acceleration += glm::vec3(0.0f, -gravity, 0.0f);
		
		this->Position = this->Position + this->Velocity * delta_time;
		this->Velocity = this->Velocity + this->Acceleration * delta_time;
		this->Velocity = Nexus::Utill::clamp(this->Velocity, 0.0f, BALL_MAX_SPEED);

		glm::mat4 model(1.0f);
		model = glm::translate(model, this->Position);
		model = glm::scale(model, glm::vec3(this->Radius));
		this->Model = model;
	}

	void ViewVolumeIncludingTest(Nexus::ViewVolume* view_volume) {
		std::vector<int> sum;
		// Front
		sum.push_back(ContainerTestWithAPlane(glm::vec3(view_volume->NearPlaneVertex[0]), view_volume->ViewVolumeNormal[0]));
		// Top
		sum.push_back(ContainerTestWithAPlane(glm::vec3(view_volume->NearPlaneVertex[0]), view_volume->ViewVolumeNormal[1]));
		// Right
		sum.push_back(ContainerTestWithAPlane(glm::vec3(view_volume->NearPlaneVertex[0]), view_volume->ViewVolumeNormal[2]));
		// Back
		sum.push_back(ContainerTestWithAPlane(glm::vec3(view_volume->FarPlaneVertex[1]), view_volume->ViewVolumeNormal[3]));
		// Bottom
		sum.push_back(ContainerTestWithAPlane(glm::vec3(view_volume->FarPlaneVertex[1]), view_volume->ViewVolumeNormal[4]));
		// Left
		sum.push_back(ContainerTestWithAPlane(glm::vec3(view_volume->FarPlaneVertex[1]), view_volume->ViewVolumeNormal[5]));

		bool IsOutSide = false;
		bool IsIntersection = false;
		for (unsigned int i = 0; i < sum.size(); i++) {
			if (sum[i] == -1) {
				// Outside
				IsOutSide = true;
				IsIntersection = false;
				break;
			}
			
			if(sum[i] == 0) {
				// Intersection
				IsIntersection = true;
			}
		}

		if (IsOutSide) {
			// 6�����u�n���@�ӥ����O�P�w OutSide �N�OOutSide
			this->Ambient = glm::vec4(0.2f, 0.2f, 0.2f, 1.0);
			this->Diffuse = glm::vec4(0.25f, 0.25f, 1.0f, 1.0);
			this->Specular = glm::vec4(0.55f, 0.45f, 0.45f, 1.0);
		} else if (IsIntersection) {
			// 6�����u�n���@�ӥ����O�P�w IsIntersection (�B�S�����@�����OOutSide)
			this->Ambient = glm::vec4(0.2f, 0.2f, 0.2f, 1.0);
			this->Diffuse = glm::vec4(0.25f, 1.0f, 0.25f, 1.0);
			this->Specular = glm::vec4(0.55f, 0.45f, 0.45f, 1.0);
		} else {
			// Inside
			this->Ambient = glm::vec4(0.2f, 0.2f, 0.2f, 1.0);
			this->Diffuse = glm::vec4(1.0f, 0.25f, 0.25f, 1.0);
			this->Specular = glm::vec4(0.55f, 0.45f, 0.45f, 1.0);
		}
	}

	void Edge(float elasticities) {
		if (this->Position.x + this->Radius > 10.0f) {
			this->Position.x = 10.0f - this->Radius;
			this->Velocity = elasticities * glm::vec3(-this->Velocity.x, this->Velocity.y, this->Velocity.z);
		} else if (this->Position.x - Radius < -10.0f) {
			this->Position.x = -10.0f + this->Radius;
			this->Velocity = elasticities * glm::vec3(-this->Velocity.x, this->Velocity.y, this->Velocity.z);
		}
		
		if (this->Position.y + this->Radius > 20.0f) {
			this->Position.y = 20.0f - this->Radius;
			this->Velocity = elasticities * glm::vec3(this->Velocity.x, -this->Velocity.y, this->Velocity.z);
		} else if (this->Position.y - Radius < 0.0f) {
			this->Position.y = 0.0f + this->Radius;
			this->Velocity = elasticities * glm::vec3(this->Velocity.x, -this->Velocity.y, this->Velocity.z);
		}
		
		if (this->Position.z + this->Radius > 10.0f) {
			this->Position.z = 10.0f - this->Radius;
			this->Velocity = elasticities * glm::vec3(this->Velocity.x, this->Velocity.y, -this->Velocity.z);
		} else if (this->Position.z - Radius < -10.0f) {
			this->Position.z = -10.0f + this->Radius;
			this->Velocity = elasticities * glm::vec3(this->Velocity.x, this->Velocity.y, -this->Velocity.z);
		}
	}

	void CollisionWithBall(Ball ball, float elasticities) {
		float distance = glm::length(this->Position - ball.Position);
		if (distance < (this->Radius + ball.Radius + 0.01f)) {
			// Collision
			glm::vec3 temp = glm::normalize(ball.Position - this->Position) * (this->Radius * 4.01f);
			ball.Position = this->Position + temp;
			glm::vec3 temp_v = ball.Velocity;
			ball.Velocity = elasticities * -this->Velocity;
			this->Velocity = elasticities * -temp_v;
		}
	}

	void CollisionWithObstacle(Obstacle obstacle, float elasticities) {
		glm::vec3 diff = this->Position - obstacle.GetPosition();
		glm::vec3 aabb_half_extents = obstacle.GetSize() / 2.0f;
		glm::vec3 clamped = glm::clamp(diff, -aabb_half_extents, aabb_half_extents);
		glm::vec3 closest = obstacle.GetPosition() + clamped;
		diff = this->Position - closest;

		if (glm::length(diff) < this->Radius) {
			// Collision
			this->Position = closest + glm::normalize(diff) * (this->Radius + 0.01f);
			this->Velocity = elasticities * -this->Velocity;
		}
	}

	void SetMass(float mass) {
		this->Mass = mass;
	}
	void SetModel(glm::mat4 model) {
		this->Model = model;
	}

	glm::mat4 GetModel() const {
		return this->Model;
	}
	glm::vec3 GetPosition() const {
		return this->Position;
	}
	glm::vec4 GetAmbient() const {
		return this->Ambient;
	}
	glm::vec4 GetDiffuse() const {
		return this->Diffuse;
	}
	glm::vec4 GetSpecular() const {
		return this->Specular;
	}
	
private:
	float Radius = 0.2f;
	float Mass = 1.0f;
	glm::vec3 Position;
	glm::vec3 Velocity;
	glm::vec3 Acceleration;
	glm::vec4 Ambient;
	glm::vec4 Diffuse;
	glm::vec4 Specular;
	glm::mat4 Model;

	glm::vec3 Force = glm::vec3(0.0f);

	int ContainerTestWithAPlane(glm::vec3 position, glm::vec3 normal) {
		// ���w�@�ӹL�������IQ(x, y ,z)�M�����󥭭����k�V�qN(A, B, C)�A�i�H�ɥX������{���G
		// Ax + By + Cz + D = d�A�䤤 D �O�@�ӱ`�ơF�� d �N���O�I�P�����������Z���C
		// �]���O������{���]���@�I�P�������Z�����O0�^�A�ҥH�O d = 0�A�i�H�D�X�`�� D ����C
		// ���U�ӭn�D�IP(x0, y0, z0)�쥭���̪�Z���A�i�H�M�Τ��� d = | Ax0 + By0 + Cz0 + D | / (A^2 + B^2 + C^2)^0.5
		// �]���k�V�qN�ƥ������W�Ʀ����V�q�A�ҥH���׬� 1�A�ҥH�i�H�������IP�a�J�����h��
		// �]���o��n�h�����IP�O�b�������~���٬O�����A�ҥH�ڭ̥i�H�⵴��ȥh��
		// �ҥH�p�G��X�ӬO���ȥN��O�~���]�P�k�V�q�����p��90�ס^�F�Ϥ���X�ӬO�t�ȥN��O�����]�P�k�V�q�����j��90�ס^
		// �Ӧ]���ڭ̬O�p��y��P�������Z���A�o�n�Ҷq��y�骺�b�|���סA�ëD�u�Ҽ{�y�骺��m�A���L�n�a�b���O�y��
		// �ҥH�u�n�Z���O���� r ~ -r ��������O�ۥ�I
		// PS: �O��@���[���A���]�y���m��P�A�ӧڭ̭n�p�⪺�Z��d�A���N�O�V�qQP��v��k�V�q�W�A�ҥH���� ||QP|| * cos(theta) �������C
		
		glm::vec3 temp_1 = glm::normalize(normal);
		glm::vec3 temp_2 = this->Position - position;
		float distance = glm::dot(temp_1, temp_2);
		if(distance >= this->Radius) {
			// outside
			return -1;
		}

		if (distance <= -this->Radius) {
			// inside
			return 1;
		}

		// intersection
		return 0;
	}
};