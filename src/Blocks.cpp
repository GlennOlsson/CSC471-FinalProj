#include "Blocks.h"
#include "Block.h"
#include <memory>
#include <assert.h>
#include <vector>
#include <iostream>

#include "Texture.h"
#include "Program.h"
#include "Shape.h"
#include "MatrixStack.h"

using namespace std;
using namespace glm;

Blocks::Blocks() {

	// setup texture file
	string file = "minecraft.jpg";
	texture = make_shared<Texture>();
	texture->setFilename(resourceDir + "/" + file);
	texture->init();
	texture->setUnit(0);
	texture->setWrapModes(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

	blocks = vector<Block>();
	block_materials = map<BlockType, shared_ptr<Shape>>();

	// Initialize the GLSL program that we will use for texture mapping
	texProg = make_shared<Program>();
	texProg->setVerbose(true);
	texProg->setShaderNames(resourceDir + "/tex_vert.vert",
							resourceDir + "/tex_frag0.frag");
	// texProg->setShaderNames(resourceDir + "/simple_vert.vert",
	// 						 resourceDir + "/simple_frag.frag");
	texProg->init();
	texProg->addUniform("P");
	texProg->addUniform("V");
	texProg->addUniform("M");
	texProg->addUniform("flip");
	texProg->addUniform("Texture0");
	texProg->addAttribute("vertPos");
	texProg->addAttribute("vertNor");
	texProg->addAttribute("vertTex");

		texProg->addUniform("MatDif");
		texProg->addUniform("MatSpec");
		texProg->addUniform("MatAmb");
		texProg->addUniform("P");
		texProg->addUniform("V");
		texProg->addUniform("M");
		texProg->addUniform("MatAmb");
		texProg->addUniform("lightPos");
		texProg->addAttribute("vertPos");
		texProg->addAttribute("vertNor");

		texProg->addUniform("MatDif");
		texProg->addUniform("MatSpec");
		texProg->addUniform("MatShine");
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
	
	/*
		minx, miny,
		maxx, miny,
		minx, maxy,
		maxx, maxy
	*/
	vector<float> vec = {
		v1[0], v1[2],
		v1[1], v1[2],
		v1[0], v1[3],
		v1[1], v1[3],

		v2[0], v2[2],
		v2[1], v2[2],
		v2[0], v2[3],
		v2[1], v2[3],

		// Flip upside down (for some reason?)
		v3[0], v3[3],
		v3[1], v3[3],
		v3[0], v3[2],
		v3[1], v3[2],

		v4[0], v4[2],
		v4[1], v4[2],
		v4[0], v4[3],
		v4[1], v4[3],

		v5[0], v5[2],
		v5[1], v5[2],
		v5[0], v5[3],
		v5[1], v5[3],

		v6[0], v6[2],
		v6[1], v6[2],
		v6[0], v6[3],
		v6[1], v6[3],

		// v1[0], v1[1], v1[2], v1[3],
		// v2[0], v2[1], v2[2], v2[3],
		// v3[0], v3[1], v3[2], v3[3],
		// v4[0], v4[1], v4[2], v4[3],
		// v5[0], v5[1], v5[2], v5[3],
		// v6[0], v6[1], v6[2], v6[3],
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
	// float blocks = 16;

	// float block_size = 1.0/blocks;
	// vector<float> v = {
	// 	0.5, 0.5,
	// 	0.5f + block_size, 0.5,
	// 	0.5, 0.5f + block_size,
	// 	0.5f + block_size, 0.5f + block_size,

	// 	// TOP (?), POS Y
	// 	0.5f + block_size, 0.5f + block_size,
	// 	0.5f + 2*block_size, 0.5f + block_size,
	// 	0.5f + block_size, 0.5f + 2*block_size,
	// 	0.5f + 2*block_size, 0.5f + 2*block_size,

	// 	// POS Z
	// 	0.5f + 2*block_size, 0.5f + 2*block_size,
	// 	0.5f + 3*block_size, 0.5f + 2*block_size,
	// 	0.5f + 2*block_size, 0.5f + 3*block_size,
	// 	0.5f + 3*block_size, 0.5f + 3*block_size,


	// 	// BOTTOM, NEG Y
	// 	0.5f + 3*block_size, 0.5f + 3*block_size,
	// 	0.5f + 4*block_size, 0.5f + 3*block_size,
	// 	0.5f + 3*block_size, 0.5f + 4*block_size,
	// 	0.5f + 4*block_size, 0.5f + 4*block_size,

	// 	// POS X
	// 	0.5f + 4*block_size, 0.5f + 4*block_size,
	// 	0.5f + 5*block_size, 0.5f + 4*block_size,
	// 	0.5f + 4*block_size, 0.5f + 5*block_size,
	// 	0.5f + 5*block_size, 0.5f + 5*block_size,

	// 	// 0, 0,
	// 	// block_size, 0,
	// 	// 0, block_size,
	// 	// block_size, block_size,

	// 	// NEG X
	// 	0.5f + 5*block_size, 0.5f + 5*block_size,
	// 	0.5f + 6*block_size, 0.5f + 5*block_size,
	// 	0.5f + 5*block_size, 0.5f + 6*block_size,
	// 	0.5f + 6*block_size, 0.5f + 6*block_size,
	// };
	// return v;
	switch (type) {
		case grass:
			return vectorFromCoords(coords(3, 15),
									coords(12, 3),	// Top, just grass
									coords(3, 15),
									coords(2, 15),	// Bottom, just dirt
									coords(3, 15), 
									coords(3, 15)
								);

		default:
			break;
	}
}

shared_ptr<Shape> Blocks::loadCube() {
	vector<tinyobj::shape_t> TOshapes;
	vector<tinyobj::material_t> objMaterials;
	// load in the mesh and make the shape(s)

	string errStr;
	bool rc = tinyobj::LoadObj(TOshapes, objMaterials, errStr,
							(resourceDir + "/cube.obj").c_str());
	
	if (!rc) 
		cerr << errStr << endl;
	shared_ptr<Shape> cube = make_shared<Shape>();
	cube->createShape(TOshapes[0]);
	cube->measure();
	cube->init();
	return cube;
}

void Blocks::addBlock(BlockType type, int x, int y, int z) {
	if(block_materials.count(type) == 0) {
		shared_ptr<Shape> cube = loadCube();
		vector<float> txtCoords = textureCoords(type);

		cout << "txtCoords: " << endl;
		for(int i = 0; i < txtCoords.size(); i += 2) {
			cout << "\t" << txtCoords[i] << ", " << txtCoords[i + 1] << endl;
		}

		cube->setTexBuf(txtCoords);
		block_materials.insert(pair<BlockType, shared_ptr<Shape>>(type, cube));
	}

	Block b = Block(block_materials[type], x, y, z);
	blocks.push_back(b);
}

void Blocks::drawBlocks(shared_ptr<MatrixStack> Model) {
	this->texProg->bind();
	this->texture->bind(this->texProg->getUniform("Texture0"));

	float r = 0.1;
	float g = 0.9;
	float b = 0.2;

	// Assign reasonable rgb values
	glUniform3f(texProg->getUniform("MatDif"), r, g, b);
	glUniform3f(texProg->getUniform("MatSpec"), 0.5 * r, 0.5 * g, 0.5 * b);
	glUniform3f(texProg->getUniform("MatAmb"), 0.3 * r, 0.3 * g, 0.3 * b);

	for (auto block : blocks){
		// cout << "Draw (" << block.x << ", " << block.y << ", " << block.z << ")" << endl;
		block.draw(Model, this->texProg);
	}

	// cout << "Drawn all" << endl;

	this->texProg->unbind();
}