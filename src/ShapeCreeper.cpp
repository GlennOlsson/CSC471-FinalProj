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

#include <chrono>

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

void ShapeCreeper::createPath() {
	// Start and end at same point

	vec3 start_end(-5, 0, -5);
	vec3 c1(10, 0, 1.5);
	vec3 c2(6, 0, 8);

	path = Spline(start_end, c1, c2, start_end, 60);
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
			if(index <= 2)
				max_min(body_min, body_max, bounding_right_lower_leg_min, bounding_right_lower_leg_max);
		} else if(index <= 11) { // Left leg
			max_min(body_min, body_max, bounding_left_leg_min, bounding_left_leg_max);
			if(index <= 8)
				max_min(body_min, body_max, bounding_left_lower_leg_min, bounding_left_lower_leg_max);
		} else if(index >= 15 && index <= 20) { // Right arm
			max_min(body_min, body_max, bounding_right_arm_min, bounding_right_arm_max);
		} else if(index >= 21 && index <= 26) { // Left arm
			max_min(body_min, body_max, bounding_left_arm_min, bounding_left_arm_max);
		}

		index += 1;
	}

	createPath();
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
void ShapeCreeper::drawRightUpperLeg() {
	drawBetween(3, 6);
}
void ShapeCreeper::drawLeftUpperLeg() {
	drawBetween(9, 12);
}
void ShapeCreeper::drawRightLowerLeg() {
	drawBetween(0, 4);
}
void ShapeCreeper::drawLeftLowerLeg() {
	drawBetween(6, 10);
}

void ShapeCreeper::setModel(shared_ptr<MatrixStack> model) {
	float r = 0.2;
	float g = 1;
	float b = 0.4;

	glUniform3f(prog->getUniform("MatDif"), r, g, b);
	glUniform3f(prog->getUniform("MatSpec"), 0.5 * r, 0.5 * g, 0.5 * b);
	glUniform3f(prog->getUniform("MatAmb"), 0.3 * r, 0.3 * g, 0.3 * b);
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

	auto now = std::chrono::high_resolution_clock::now();

	float deltaTime =
		std::chrono::duration_cast<std::chrono::microseconds>(now - last_time)
			.count();

	deltaTime *= 0.000001;

	last_time = now;

	path.update(deltaTime);

	if (path.isDone())
		createPath();

	vec3 dummy_location = path.getPosition();

	// auto dummy_rot_mat = glm::lookAt(last_dummy_loc, dummy_location, vec3(0, 1, 0));


	vec3 gaze = normalize(last_dummy_loc - dummy_location);
	vec3 lookat_pt = dummy_location + gaze;

	vec3 u_vec = cross(vec3(0, 1, 0), gaze);

	float gaze_x = u_vec[0];
	float gaze_y = u_vec[2];

	// cout << "gaze x=" << gaze_x << ", y=" << gaze_y << endl;

	float rotate_angle = acos(gaze_x / sqrt(pow(gaze_x, 2) + pow(gaze_y, 2)));

	// cout << "Gaze : " << gaze[0] << ", " << gaze[1] << ", " << gaze[2] << endl;
	// vec3 lookat_pt = dummy_location + gaze;

	last_dummy_loc = dummy_location;
	model->pushMatrix(); {

		float time = glfwGetTime();
		// float time = 1.0f;

		// model->translate(vec3(2, 0, -5 * (-time / 30.0f)));
		model->translate(dummy_location);

		// cout << "rotate " << rotate_angle << endl;

		// cout << "Dummy loc: " << dummy_location[0] << ", " << dummy_location[1] << ", " << dummy_location[2] << endl;
		// cout << "Dummy lookat: " << lookat_pt[0] << ", " << lookat_pt[1] << ", " << lookat_pt[2] << endl;

		// model->lookAt(dummy_location, lookat_pt, vec3(0, 1, 0));

		// model->multMatrix(dummy_rot_mat);
		model->rotate(-rotate_angle, vec3(0, 1, 0));

		model->rotate(PI / 2.0f, vec3(0, 1, 0));
		model->rotate(-PI / 2, vec3(1, 0, 0));
		model->scale(0.01);

		vec3 center = center_of_bounds(bounding_min, bounding_max);

		// Move right arm like pendulum
		model->pushMatrix(); {
			vec3 arm_center = center_of_bounds(bounding_left_arm_min, bounding_left_arm_max);
			
			model->translate(vec3(0, arm_center[1] * 0.3f, arm_center[2] * 0.35f));

			

			float frac = 0.10f * sin(time) + 0.5f;

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

		// Move left arm like pendulum, opposite sync compared to other arm
		model->pushMatrix(); {
			vec3 arm_center = center_of_bounds(bounding_right_arm_min, bounding_right_arm_max);
			
			model->translate(vec3(0, arm_center[1] * 0.3f, arm_center[2] * 0.35f));

			float frac = -0.10f * sin(time) + 0.5f;

			model->rotate(2 * PI * frac, vec3(0, 1, 0));

			model->rotate(PI * -0.5f, vec3(1, 0, 0));

			// Move to joint (by origin)
			model->translate(vec3(0, arm_center[1] * 0.6f, 0));

			//Move to center
			model->translate(-arm_center);

			setModel(model);
			drawRightArm();

		}
		model->popMatrix();

		// Right leg
		model->pushMatrix(); {
			
			// vec3 right_ass_joint = vec3(ass_center[0] / 2.0f, ass_max[0], ass_max[2] * 0.95f);

			vec3 leg_center = center_of_bounds(bounding_right_leg_min, bounding_right_leg_max);
			
			// model->translate(vec3(0, 0, 0));
			model->translate(vec3(leg_center[0] * -0.7f, leg_center[1] * 0.9f, leg_center[2] * 0.2f));

			float frac = -0.05f * sin(time) + 0.5f;

			model->rotate((2.0f * PI * frac) + PI , vec3(0, 1, 0));

			// model->rotate((2.0f * PI * 0.1) + PI , vec3(0, 1, 0));

			// model->rotate(PI * -0.5f, vec3(1, 0, 0));

			// Move to joint (by origin)
			model->translate(vec3(leg_center[0] * 0.8f, leg_center[1] * -0.0f, leg_center[2] * -1.0f));

			//Move to center
			model->translate(-leg_center);

			// Bend lower leg
			model->pushMatrix(); {
				vec3 lower_leg_center = center_of_bounds(bounding_right_leg_min, bounding_right_leg_max);
			
				// model->translate(vec3(0, 0, 0));
				model->translate(vec3(lower_leg_center[0] * 0.3f, lower_leg_center[1] * 1.0f, lower_leg_center[2] * 1.1f));


				// Between pi/2 and 3pi/2
				float frac = -0.1f * sin(time) + 0.5f;

				float rads = max((2.0f * PI * frac) + PI, 6.3);

				model->rotate(rads , vec3(0, 1, 0));

				// // model->rotate((2.0f * PI * 0.1) + PI , vec3(0, 1, 0));

				// // model->rotate(PI * -0.5f, vec3(1, 0, 0));

				// Move to joint (by origin)
				model->translate(vec3(lower_leg_center[0] * 0.8f, lower_leg_center[1] * -0.0f, lower_leg_center[2] * -0.15f));

				//Move to center
				model->translate(-lower_leg_center);
				
				setModel(model);
				drawRightLowerLeg();
			}
			model->popMatrix();

			setModel(model);
			drawRightUpperLeg();

		}
		model->popMatrix();

		// Left leg
		model->pushMatrix(); {
		
			vec3 leg_center = center_of_bounds(bounding_left_leg_min, bounding_left_leg_max);
			
			// // model->translate(vec3(0, 0, 0));
			model->translate(vec3(leg_center[0] * -0.7f, leg_center[1] * 0.9f, leg_center[2] * 0.2f));

			float frac = 0.05f * sin(time) + 0.5f;

			model->rotate((2.0f * PI * frac) + PI , vec3(0, 1, 0));

			// model->rotate((2.0f * PI * 0.1) + PI , vec3(0, 1, 0));

			// model->rotate(PI * -0.5f, vec3(1, 0, 0));

			// Move to joint (by origin)
			model->translate(vec3(leg_center[0] * 0.8f, leg_center[1] * -0.0f, leg_center[2] * -1.0f));

			//Move to center
			model->translate(-leg_center);

			// Bend lower leg
			model->pushMatrix(); {
				vec3 lower_leg_center = center_of_bounds(bounding_left_leg_min, bounding_left_leg_max);
			
				// model->translate(vec3(0, 0, 0));
				model->translate(vec3(lower_leg_center[0] * 0.3f, lower_leg_center[1] * 1.0f, lower_leg_center[2] * 1.1f));


				// Between pi/2 and 3pi/2
				float frac = 0.1f * sin(time) + 0.5f;

				float rads = max((2.0f * PI * frac) + PI, 6.3);

				model->rotate(rads , vec3(0, 1, 0));

				// // model->rotate((2.0f * PI * 0.1) + PI , vec3(0, 1, 0));

				// // model->rotate(PI * -0.5f, vec3(1, 0, 0));

				// Move to joint (by origin)
				model->translate(vec3(lower_leg_center[0] * 0.8f, lower_leg_center[1] * -0.0f, lower_leg_center[2] * -0.15f));

				//Move to center
				model->translate(-lower_leg_center);
				
				setModel(model);
				drawLeftLowerLeg();
			}
			model->popMatrix();

			setModel(model);
			drawLeftUpperLeg();

		}
		model->popMatrix();

		model->pushMatrix(); {

			model->translate(-center);
			setModel(model);

			drawBetween(12, 15);
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