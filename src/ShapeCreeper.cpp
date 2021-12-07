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

ShapeCreeper::ShapeCreeper(vector<tinyobj::shape_t> s, shared_ptr<Program> prog) {

	this->prog = prog;

	float maxX = -10000;
	float maxY = -10000;
	float maxZ = -10000;

	float minX = 10000;
	float minY = 10000;
	float minZ = 10000;

	for (auto shape : s) {
		auto body_part = make_shared<Shape>();
		body_part->createShape(shape);
		body_part->measure();
		body_part->init();
		shapes.push_back(body_part);

		maxX = max((float) body_part->max[0], maxX);
		maxY = max((float) body_part->max[1], maxY); 
		maxZ = max((float) body_part->max[2], maxZ);

		minX = min((float) body_part->min[0], minX);
		minY = min((float) body_part->min[1], minY); 
		minZ = min((float) body_part->min[2], minZ);
	}

	this->bounding_max = vec3(maxX, maxY, maxZ);
	this->bounding_min = vec3(minX, minY, minZ);
}

void ShapeCreeper::drawBetween(int i, int j) {
	for(; i < j; ++i) {
		shapes[i]->draw(prog);
	}
}

void ShapeCreeper::drawAll() {
	drawBetween(0, shapes.size());
}

void ShapeCreeper::setModel(shared_ptr<MatrixStack> model) {
	glUniformMatrix4fv(this->prog->getUniform("M"), 1, GL_FALSE,
						   value_ptr(model->topMatrix()));
}

void ShapeCreeper::initPlacement(shared_ptr<MatrixStack> model) {
	model->pushMatrix(); {

		model->translate(vec3(2, 0, -5));

		model->rotate(-PI / 2, vec3(0, 1, 0));
		model->rotate(-PI / 2, vec3(1, 0, 0));
		model->scale(0.01);

		model->pushMatrix(); {
			model->translate(vec3(2, 0, -5));

			model->rotate(-PI / 2, vec3(1, 0, 0));

			setModel(model);
			drawBetween(21, 27);

		}
		model->popMatrix();

		vec3 center = 0.5f * (bounding_max - bounding_min) + bounding_min;

		model->translate(-center);
		
		setModel(model);

		drawBetween(0, 20);
		drawBetween(27, 29);
	}
	model->popMatrix();
}

void ShapeCreeper::move(float t, shared_ptr<MatrixStack> model) {
	model->pushMatrix(); {

	}
	model->popMatrix();
}