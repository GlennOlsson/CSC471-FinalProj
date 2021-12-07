
#pragma once

#ifndef LAB471_SHAPE_CREEPER_H_INCLUDED
#define LAB471_SHAPE_CREEPER_H_INCLUDED

#include "Shape.h"

#include "MatrixStack.h"
#include <string>
#include <vector>
#include <memory>
#include <glm/gtc/type_ptr.hpp>
#include <tiny_obj_loader/tiny_obj_loader.h>
#include <glad/glad.h>
#include "GLSL.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GLFW/glfw3.h>

#include <chrono>

#include "Spline.h"

class Program;

using std::shared_ptr;
using std::vector;

using namespace glm;

class ShapeCreeper {

	vector<shared_ptr<Shape>> shapes;

	vec3 bounding_max;
	vec3 bounding_min;

	void drawBetween(int i, int j);
	void drawAll();

	void drawRightArm();
	void drawLeftArm();

	void drawRightUpperLeg();
	void drawLeftUpperLeg();
	
	void drawRightLowerLeg();
	void drawLeftLowerLeg();

	vec3 bounding_right_arm_max;
	vec3 bounding_right_arm_min;

	vec3 bounding_left_arm_max;
	vec3 bounding_left_arm_min;

	vec3 bounding_right_leg_max;
	vec3 bounding_right_leg_min;

	vec3 bounding_right_lower_leg_max;
	vec3 bounding_right_lower_leg_min;

	vec3 bounding_left_leg_max;
	vec3 bounding_left_leg_min;

	vec3 bounding_left_lower_leg_max;
	vec3 bounding_left_lower_leg_min;

	void setModel(shared_ptr<MatrixStack> model);

	shared_ptr<Program> prog;

	void createPath();
	Spline path;

	vec3 last_dummy_loc;

	std::chrono::steady_clock::time_point last_time = std::chrono::high_resolution_clock::now();

public:

	ShapeCreeper(vector<tinyobj::shape_t> s, shared_ptr<Program> prog);

	void initPlacement(shared_ptr<MatrixStack> model);

	void move(float t, shared_ptr<MatrixStack> model);
};

#endif // LAB471_SHAPE_H_INCLUDED
