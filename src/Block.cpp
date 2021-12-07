#include "Block.h"

#include "Texture.h"
#include "Program.h"
#include "Shape.h"
#include "MatrixStack.h"

#include <iostream>

#include <glad/glad.h>
#include "GLSL.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace std;
using namespace glm;

Block::Block(shared_ptr<Shape> cube, int x, int y, int z) {
	this->cube = cube;

	this->x = x;
	this->y = y;
	this->z = z;
}

/**
 * Returns absolute distance from blocks center to point
*/
float Block::intersects(vec3 point) {
	float radius = 1;

	vec3 dist_vec = vec3(this->x, this->y, this->z) - point;
	float dist = length(dist_vec);

	return dist;
}

void Block::draw(shared_ptr<MatrixStack> Model, shared_ptr<Program> textureProgram) {
	Model->pushMatrix();

	Model->translate(vec3(x, y-0.5, z));

	glUniformMatrix4fv(textureProgram->getUniform("M"), 1, GL_FALSE,
						   value_ptr(Model->topMatrix()));

	cube->draw(textureProgram);

	Model->popMatrix();
}
