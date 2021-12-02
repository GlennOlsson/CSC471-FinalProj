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
	// Mid of block
	int x;
	int y;
	int z;

	Block(int x, int y, int z);

	/**
	 * Returns wether or not the point intersects with the blocks bounding spehere 
	*/
	bool intersects(float x, float y, float z);

	void draw(shared_ptr<MatrixStack> Model, shared_ptr<Program> textureProgram, shared_ptr<Shape> cube);
};

#endif