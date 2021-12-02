#include "Blocks.h"
#include "Block.h"
#include <memory>
#include <assert.h>
#include <set>
#include <iostream>

#include "Texture.h"
#include "Program.h"
#include "Shape.h"
#include "MatrixStack.h"

using namespace std;
using namespace glm;

Blocks::Blocks() {

	

	std::string resourceDir = "../resources";

	blocks = set<Block>();
	block_materials = map<BlockType, shared_ptr<Shape>>();

	// Initialize the GLSL program that we will use for texture mapping
	texProg = make_shared<Program>();
	texProg->setVerbose(true);
	texProg->setShaderNames(resourceDir + "/tex_vert.vert",
							resourceDir + "/tex_frag0.frag");
	texProg->init();
	texProg->addUniform("P");
	texProg->addUniform("V");
	texProg->addUniform("M");
	texProg->addUniform("flip");
	texProg->addUniform("Texture0");
	texProg->addAttribute("vertPos");
	texProg->addAttribute("vertNor");
	texProg->addAttribute("vertTex");

	vector<tinyobj::shape_t> TOshapes;
	vector<tinyobj::material_t> objMaterials;
	// load in the mesh and make the shape(s)

	string errStr;
	bool rc = tinyobj::LoadObj(TOshapes, objMaterials, errStr,
							(resourceDir + "/cube.obj").c_str());
	if (!rc) {
		cerr << errStr << endl;
	} else {
		cube = make_shared<Shape>();
		cube->createShape(TOshapes[0]);
		cube->measure();
		cube->init();

		//TODO: For each block type, create entry in map with cube with specific coords for material
	}
}

/**
 * Returns vec4 describing the 0.0-1.0 coords in texture for block at x, y
 * (0-indexed) 1 elem is xMin 2 elem is xMax 3 elem is yMin 4 elem is yMax
 */
vec4 Blocks::coords(int x, int y) {
	float blocks = 16;

	assert(x < blocks);
	assert(x >= 0);
	assert(y < blocks);
	assert(y >= 0);

	float block_size = 1.0f / blocks;

	float xMin = x * block_size;
	float xMax = (x + 1) * block_size;

	float yMin = y * block_size;
	float yMax = (y + 1) * block_size;

	return vec4(xMin, xMax, yMin, yMax);
}

/**
 * Helper to return a vector of 6 * 2 elements from the 6 input vec2
 */
vector<float> Blocks::vectorFromCoords(vec4 v1, vec4 v2, vec4 v3, vec4 v4,
									   vec4 v5, vec4 v6) {
				   
	vector<float> vec = {
		v1[0], v1[1], v1[2], v1[3],
		v2[0], v2[1], v2[2], v2[3],
		v3[0], v3[1], v3[2], v3[3],
		v4[0], v4[1], v4[2], v4[3],
		v5[0], v5[1], v5[2], v5[3],
		v6[0], v6[1], v6[2], v6[3],
	};
	return vec;
}

/**
 * Returns texture coordinates in following order, 2*4 coordinates per side:
 * - Back side (NEG Z)
 * - Top side (POS Y)
 * - Front side (POS Z)
 * - Bottom side (NEG Y)
 * - Right side (POS X)
 * - Left side (NEG X)
 */
vector<float> Blocks::textureCoords(BlockType type) {
	switch (type) {
		case grass:
			return vectorFromCoords(coords(3, 15),
									coords(12, 3),	// Top, just grass
									coords(3, 15),
									coords(2, 15),	// Bottom, just dirt
									coords(3, 15), coords(3, 15));

		default:
			break;
	}
}

void Blocks::drawBlocks(shared_ptr<MatrixStack> Model) {
	this->texProg->bind();
	this->texture->bind(this->texProg->getUniform("Texture0"));

	for (auto block : blocks) block.draw(Model, this->texProg, this->cube);

	this->texProg->unbind();
}