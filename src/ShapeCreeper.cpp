#include "ShapeCreeper.h"

#include <tiny_obj_loader/tiny_obj_loader.h>

#include "MatrixStack.h"
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "Program.h"

#include <math.h>
#include <glad/glad.h>
#include "GLSL.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GLFW/glfw3.h>

#define PI 3.1415

using std::cout;
using std::endl;
using std::shared_ptr;
using std::make_shared;
using std::vector;

using namespace glm;

void max_min(vec3& body_min, vec3& body_max, vec3& curr_min, vec3& curr_max) {

	float minX = min(body_min[0], curr_min[0]);
	float minY = min(body_min[1], curr_min[1]);
	float minZ = min(body_min[2], curr_min[2]);

	curr_min = vec3(minX, minY, minZ);

	float maxX = max(body_max[0], curr_max[0]);
	float maxY = max(body_max[1], curr_max[1]);
	float maxZ = max(body_max[2], curr_max[2]);

	curr_max = vec3(maxX, maxY, maxZ);
}

ShapeCreeper::ShapeCreeper(vector<tinyobj::shape_t> s, shared_ptr<Program> prog) {

	this->prog = prog;

	bounding_max = vec3(-10000, -10000, -10000);
	bounding_min = vec3(10000, 10000, 10000);

	bounding_right_arm_max = vec3(-10000, -10000, -10000);
	bounding_right_arm_min = vec3(10000, 10000, 10000);

	bounding_left_arm_max = vec3(-10000, -10000, -10000);
	bounding_left_arm_min = vec3(10000, 10000, 10000);

	bounding_right_leg_max = vec3(-10000, -10000, -10000);
	bounding_right_leg_min = vec3(10000, 10000, 10000);

	bounding_left_leg_max = vec3(-10000, -10000, -10000);
	bounding_left_leg_min = vec3(10000, 10000, 10000);

	int index = 0;
	for (auto shape : s) {
		auto body_part = make_shared<Shape>();
		body_part->createShape(shape);
		body_part->measure();
		body_part->init();
		shapes.push_back(body_part);

		float body_max_x = body_part->max[0];
		float body_max_y = body_part->max[1];
		float body_max_z = body_part->max[2];

		float body_min_x = body_part->min[0];
		float body_min_y = body_part->min[1];
		float body_min_z = body_part->min[2];

		vec3 body_min = vec3(body_min_x, body_min_y, body_min_z);
		vec3 body_max = vec3(body_max_x, body_max_y, body_max_z);

		max_min(body_min, body_max, bounding_min, bounding_max);

		if(index <= 5) { // Right leg
			max_min(body_min, body_max, bounding_right_leg_min, bounding_right_leg_max);
		} else if(index <= 11) { // Left leg
			max_min(body_min, body_max, bounding_left_leg_min, bounding_left_leg_max);
		} else if(index >= 15 && index <= 20) { // Right arm
			max_min(body_min, body_max, bounding_right_arm_min, bounding_right_arm_max);
		} else if(index >= 21 && index <= 26) { // Left arm
			max_min(body_min, body_max, bounding_left_arm_min, bounding_left_arm_max);
		}

		index += 1;
	}
}

void ShapeCreeper::drawBetween(int i, int j) {
	for(; i < j; ++i) {
		shapes[i]->draw(prog);
	}
}

void ShapeCreeper::drawAll() {
	drawBetween(0, shapes.size());
}

void ShapeCreeper::drawRightArm() {
	drawBetween(15, 21);
}
void ShapeCreeper::drawLeftArm() {
	drawBetween(21, 27);
}
void ShapeCreeper::drawRightLeg() {
	drawBetween(0, 6);
}
void ShapeCreeper::drawLeftLeg() {
	drawBetween(6, 12);
}

void ShapeCreeper::setModel(shared_ptr<MatrixStack> model) {
	glUniformMatrix4fv(this->prog->getUniform("M"), 1, GL_FALSE,
						   value_ptr(model->topMatrix()));
}

vec3 size_of_bounds(vec3 min, vec3 max) {
	return max - min;
}

vec3 center_of_bounds(vec3 min, vec3 max) {
	return 0.5f * size_of_bounds(min, max) + min;
}

void ShapeCreeper::initPlacement(shared_ptr<MatrixStack> model) {
	model->pushMatrix(); {

		model->translate(vec3(2, 0, -5));

		model->rotate(-PI / 2, vec3(0, 1, 0));
		model->rotate(-PI / 2, vec3(1, 0, 0));
		model->scale(0.01);

		vec3 center = center_of_bounds(bounding_min, bounding_max);

		// model->translate(-center);

		model->pushMatrix(); {

			vec3 left_shoulder_max = shapes[14]->max;
			vec3 left_shoulder_min = shapes[14]->min;
			vec3 left_shoulder_center = center_of_bounds(left_shoulder_min, left_shoulder_max);
			
			vec3 left_shoulder_joint = vec3(left_shoulder_center[1] / 2.0f, left_shoulder_max[0], left_shoulder_max[2] * 0.95f);

			vec3 arm_center = center_of_bounds(bounding_left_arm_min, bounding_left_arm_max);
			
			model->translate(vec3(0, arm_center[1] * 0.3f, arm_center[2] * 0.35f));

			// model->translate(left_shoulder_joint);

			
			// model->translate(vec3(arm_center[0] * 0.5f, 0, arm_center[1] * 0.5f));
			// model->translate(arm_center * 0.5f);

			// float time = fmodf((float) glfwGetTime(), 10.0f);
			float time = glfwGetTime();

			float frac = 0.15f * sin(time) + 0.5f;

			model->rotate(2 * PI * frac, vec3(0, 1, 0));

			model->rotate(PI * 0.5f, vec3(1, 0, 0));

			// Move to joint (by origin)
			model->translate(vec3(0, arm_center[1] * 0.6f, 0));

			//Move to center
			model->translate(-arm_center);

			setModel(model);
			drawLeftArm();

		}
		model->popMatrix();

		model->pushMatrix(); {

			model->translate(-center);
			setModel(model);

			drawBetween(0, 20);
			drawBetween(27, 29);
		}
		model->popMatrix();
	}
	model->popMatrix();
}

void ShapeCreeper::move(float t, shared_ptr<MatrixStack> model) {
	model->pushMatrix(); {

	}
	model->popMatrix();
}