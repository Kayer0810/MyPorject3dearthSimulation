#ifndef CAMERA_CLASS_H
#define CAMERA_CLASS_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>

#include "shaderClass.h"

class Camera
{
public:
	// Where, which direction, how tilt, update everytime it moves

	glm::vec3 Position;
	glm::vec3 Orientation = glm::vec3(0.0f, 0.0f, -1.0f);
	glm::vec3 Up = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::mat4 cameraMatrix = glm::mat4(1.0f);

	// GLFWkey first click ---> POV
	bool firstClick = true;

	
	int width;

	int height;



	//AI suggested part 
	// ( 1秒あたりに進む距離(単位/秒)。値を下げるほどWキーでの移動がゆっくりになる。
	// ※このシーンは月軌道半径が約60単位あるので、ある程度大きめの値が必要。
	//   「進みすぎる」と感じたらこの数値を小さくしてください(例: 5.0f など)。
	// Shift押下時の速度倍率 )

	float sprintMultiplier = 4.0f;
	float moveSpeed = 15.0f;
	float sensitivity = 100.0f;

	// Construct Camera function ----> Camera.cpp
	Camera(int width, int height, glm::vec3 position);

	// Updates field of view, when caamera stops and starts
	void updateMatrix(float FOVdeg, float nearPlane, float farPlane);
	// Creates function
	void Matrix(Shader& shader, const char* uniform);
	void Inputs(GLFWwindow* window, float deltaTime);


};
#endif
