#pragma once
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>
#include "Utill.h"
#include "Obstacle.h"

constexpr float BALL_MAX_SPEED = 5.0f;



class Ball {
public:
	Ball(glm::vec3 positon, glm::vec3 velocity) {
		this->Position = positon;
		this->Velocity = velocity;
		this->Acceleration = glm::vec3(0.0f);
		
		glm::mat4 model(1.0f);
		model = glm::translate(model, positon);
		model = glm::scale(model, glm::vec3(this->Radius));
		this->Model = model;
	}

	void Update(float delta_time) {
		this->Position = this->Position + this->Velocity * delta_time;
		this->Velocity = this->Velocity + this->Acceleration * delta_time;
		this->Velocity = Nexus::Utill::clamp(this->Velocity, 1.0f, BALL_MAX_SPEED);

		glm::mat4 model(1.0f);
		model = glm::translate(model, this->Position);
		model = glm::scale(model, glm::vec3(this->Radius));
		this->Model = model;
	}

	void Edge() {
		if (this->Position.x + this->Radius > 10.0f) {
			this->Position.x = 10.0f - this->Radius;
			this->Velocity = glm::vec3(-this->Velocity.x, this->Velocity.y, this->Velocity.z);
		} else if (this->Position.x - Radius < -10.0f) {
			this->Position.x = -10.0f + this->Radius;
			this->Velocity = glm::vec3(-this->Velocity.x, this->Velocity.y, this->Velocity.z);
		}
		
		if (this->Position.y + this->Radius > 20.0f) {
			this->Position.y = 20.0f - this->Radius;
			this->Velocity = glm::vec3(this->Velocity.x, -this->Velocity.y, this->Velocity.z);
		} else if (this->Position.y - Radius < 0.0f) {
			this->Position.y = 0.0f + this->Radius;
			this->Velocity = glm::vec3(this->Velocity.x, -this->Velocity.y, this->Velocity.z);
		}
		
		if (this->Position.z + this->Radius > 10.0f) {
			this->Position.z = 10.0f - this->Radius;
			this->Velocity = glm::vec3(this->Velocity.x, this->Velocity.y, -this->Velocity.z);
		} else if (this->Position.z - Radius < -10.0f) {
			this->Position.z = -10.0f + this->Radius;
			this->Velocity = glm::vec3(this->Velocity.x, this->Velocity.y, -this->Velocity.z);
		}
	}

	void CollisionWithBall(Ball ball) {
		float distance = glm::length(this->Position - ball.Position);
		if (distance < this->Radius * 2.01f) {
			// Collision
			glm::vec3 temp = glm::normalize(ball.Position - this->Position) * (this->Radius * 4.01f);
			ball.Position = this->Position + temp;
			glm::vec3 temp_v = ball.Velocity;
			ball.Velocity = -this->Velocity;
			this->Velocity = -temp_v;
		}
	}

	void CollisionWithObstacle(Obstacle obstacle) {
		glm::vec3 diff = this->Position - obstacle.GetPosition();
		glm::vec3 aabb_half_extents = obstacle.GetSize() / 2.0f;
		glm::vec3 clamped = glm::clamp(diff, -aabb_half_extents, aabb_half_extents);
		glm::vec3 closest = obstacle.GetPosition() + clamped;
		diff = this->Position - closest;

		if (glm::length(diff) < this->Radius) {
			// Collision
			this->Position = closest + glm::normalize(diff) * (this->Radius + 0.01f);
			this->Velocity = -this->Velocity;
		}
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
private:
	float Radius = 0.2f;
	glm::vec3 Position;
	glm::vec3 Velocity;
	glm::vec3 Acceleration;
	glm::mat4 Model;
};