#pragma once

#include <vector>

#define GLEW_STATIC
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

enum CameraMovement {
	FORWARD,
	BACKWARD,
	LEFT,
	RIGHT,
	ROTATE_RIGHT,
	ROTATE_LEFT,
	LOOK_UP,
	LOOK_DOWN,
	UP,
	DOWN
};

const GLfloat YAW = -90.0f;
const GLfloat PITCH = 0.0f;
const GLfloat SPEED = 2.5f;
const GLfloat SENSITIVITY = 0.05f;
const GLfloat ZOOM = 45.0f;

class Camera {
public:
	Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, -35.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), GLfloat yaw = YAW,
		GLfloat pitch = PITCH) : front(glm::vec3(0.0f, 0.0f, -1.0f)), movementSpeed(SPEED), mouseSensitivity(SENSITIVITY), zoom(ZOOM)
	{
		this->position = position;
		this->worldUp = up;
		this->yaw = yaw;
		this->pitch = pitch;
		this->updateCameraVectors();
	}

	Camera(GLfloat posX, GLfloat posY, GLfloat posZ, GLfloat upX, GLfloat upY, GLfloat upZ,
		GLfloat yaw, GLfloat pitch) : front(glm::vec3(0.0f, 0.0f, -1.0f)), movementSpeed(SPEED), mouseSensitivity(SENSITIVITY),
		zoom(ZOOM) {
		this->position = glm::vec3(posX, posY, posZ);
		this->worldUp = glm::vec3(upX, upY, upZ);
		this->yaw = yaw;
		this->pitch = pitch;
		this->updateCameraVectors();
	}

	glm::mat4 GetViewMatrix() {
		return glm::lookAt(this->position, this->position + this->front, this->up);
	}

	void ProcessKeyboard(CameraMovement direction, GLfloat deltaTime) {
		GLfloat velocity = this->movementSpeed * deltaTime;
		
		if (FORWARD == direction) {
			this->position += this->front * velocity;
		}
		if (UP == direction) {
			this->position += glm::vec3(0.0f, 0.1f, 0.0f);
		}
		if (DOWN == direction) {
			this->position += glm::vec3(0.0f, -0.1f, 0.0f);
		}
		if (BACKWARD == direction) {
			this->position -= this->front * velocity;
		}
		if (LEFT == direction) {
			this->position -= this->right * velocity;
		}
		if (RIGHT == direction) {
			this->position += this->right * velocity;
		}
		if (ROTATE_LEFT == direction) {
			velocity *= 7;
			this->yaw -= velocity;
		}
		if (ROTATE_RIGHT == direction) {
			velocity *= 7;
			this->yaw += velocity;
		}
		if (LOOK_UP == direction) {
			velocity *= 7;
			this->pitch += velocity;
		}
		if (LOOK_DOWN == direction) {
			velocity *= 7;
			this->pitch -= velocity;
		}
		this->updateCameraVectors();
	}

	void ProcessMouseMovement(GLfloat xOffset, GLfloat yOffset, GLboolean constrainPitch = true) {
		xOffset *= this->mouseSensitivity;
		yOffset *= this->mouseSensitivity;

		int width = glutGet(GLUT_WINDOW_WIDTH);
		int height = glutGet(GLUT_WINDOW_HEIGHT);

		this->yaw += xOffset;
		this->pitch += yOffset;

		if (constrainPitch) {
			if (this->pitch > 89.0f)
				this->pitch = 89.0f;
			if (this->pitch > -89.0f)
				this->pitch = -89.0f;
		}

		//mouseDirectionX -= (x - this->windowWidth / 2) * 0.1f;
		//mouseDirectionY += (y - this->windowHeight / 2) * 0.1f;
		//if (mouseDirectionY > 180) mouseDirectionY = 180;
		//else if (mouseDirectionY < 0) mouseDirectionY = 0;

		this->updateCameraVectors();
		//int cx = (width >> 1);
		//int cy = (height >> 1);
		//glutWarpPointer(cx, cy);
	}

	//MAYBE ADD MOUSE SCROLL FUNCTION...

	glm::vec3 position;
	glm::vec3 front;
	glm::vec3 up;
	glm::vec3 right;
	glm::vec3 worldUp;

	GLfloat yaw, pitch, movementSpeed, mouseSensitivity, zoom;

private:
	void updateCameraVectors() {
		glm::vec3 front;
		front.x = cos(glm::radians(this->yaw)) * cos(glm::radians(this->pitch));
		front.y = sin(glm::radians(this->pitch));
		front.z = sin(glm::radians(this->yaw)) * cos(glm::radians(this->pitch));

		this->front = glm::normalize(front);
		this->right = glm::normalize(glm::cross(this->front, this->worldUp));
		this->up = glm::normalize(glm::cross(this->right, this->front));
	}
};
