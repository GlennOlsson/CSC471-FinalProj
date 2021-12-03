#include "Texture.h"
#include "Program.h"
#include "Shape.h"
#include "MatrixStack.h"

#include <glad/glad.h>
#include "GLSL.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <memory>

#ifndef MINE_BLOCK_H
#define MINE_BLOCK_H

using namespace std;
using namespace glm;

class Block {

public:

	shared_ptr<Shape> cube;

	// Mid of block
	int x;
	int y;
	int z;

	Block(shared_ptr<Shape> cube, int x, int y, int z);

	/**
	 * Returns wether or not the point intersects with the blocks bounding spehere 
	*/
	float intersects(vec3 point);

	void draw(shared_ptr<MatrixStack> Model, shared_ptr<Program> textureProgram);
};

#endif