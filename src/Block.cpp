#include "Block.h"

#include "Texture.h"
#include "Program.h"
#include "Shape.h"
#include "MatrixStack.h"

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
 * Returns wether or not the point intersects with the blocks bounding spehere 
*/
bool Block::intersects(float x, float y, float z) {

}

void Block::draw(shared_ptr<MatrixStack> Model, shared_ptr<Program> textureProgram) {
	Model->pushMatrix();

	Model->translate(vec3(x - 0.5, y - 0.5, z - 0.5));

	glUniformMatrix4fv(textureProgram->getUniform("M"), 1, GL_FALSE,
						   value_ptr(Model->topMatrix()));

	cube->draw(textureProgram);

	Model->popMatrix();
}
