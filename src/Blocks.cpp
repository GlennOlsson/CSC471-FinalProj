#include "Blocks.h"
#include "Block.h"
#include <memory>
#include <assert.h>
#include <list>
#include <iostream>

#include "Texture.h"
#include "Program.h"
#include "Shape.h"
#include "MatrixStack.h"

#include <math.h>

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

	blocks = list<shared_ptr<Block>>();

	// 100 x-blocks, 100 z-blocks, 20 y blocks
	spatial_lookup = vector<vector<vector<shared_ptr<Block>>>>(100);
	for(int i = 0; i < 100; i++) {
		spatial_lookup[i] = vector<vector<shared_ptr<Block>>>(20);
		for(int j = 0; j < 20; j++) {
			spatial_lookup[i][j] = vector<shared_ptr<Block>>(100);
		}
	}

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
	texProg->addUniform("Texture0");
	texProg->addUniform("lightPos");
	texProg->addAttribute("vertPos");
	texProg->addAttribute("vertNor");
	texProg->addAttribute("vertTex");
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
	For each side, these are the texture coords
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
	};
	return vec;
}

/**
 * Helper to create a vector from block with same side everywhere 
 */
vector<float> Blocks::vectorFromCoords(vec4 side) { 
	return vectorFromCoords(side, side, side, side, side, side);
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
		case grass: {
			vec4 side = coords(3, 15);
			vec4 top = coords(12, 3); // just grass
			vec4 bottom = coords(2, 15); // just dirt
			return vectorFromCoords(side,
									top,
									side,
									bottom,
									side, 
									side
								);
		}
		case stone: {
			// Same texture for all sides
			vec4 side = coords(0, 15);
			return vectorFromCoords(side);
		}
		case wood: {
			// Same texture for all sides
			vec4 side = coords(4, 14);
			vec4 topBottom = coords(5, 14);
			return vectorFromCoords(side,
									topBottom,
									side,
									topBottom,
									side, 
									side
								);
		}
		case leave: {
			// Same texture for all sides
			vec4 side = coords(5, 12);
			return vectorFromCoords(side);
		}
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

// Returns 3 integers of the indexes in spatial_lookup for the point v
void Blocks::spatial_coords(vec3 v, int& x, int& y, int& z) {
	x = round(v[0]);
	y = round(v[1]);
	z = round(v[2]);

	x += 50;
	y += 1;
	z += 50;
}

// Returns true if there is a block at position pos
shared_ptr<Block> Blocks::blockAt(vec3 pos) {
	int x, y, z;
	spatial_coords(pos, x, y, z);

	// If bad coords, there can't be blocks there
	if(x < 0 || x > 100)
		return nullptr;
	if(y < 0 || y > 20)
		return nullptr;
	if(z < 0 || z > 100)
		return nullptr;
	
	return spatial_lookup[x][y][z];
}

void Blocks::addBlock(BlockType type, int x, int y, int z) {
	if(hasBlockAt(vec3(x, y, z)))
		return;

	// Save block type in dict to draw same everytime
	if(block_materials.count(type) == 0) {
		shared_ptr<Shape> cube = loadCube();
		vector<float> txtCoords = textureCoords(type);

		cube->setTexBuf(txtCoords);
		block_materials.insert(pair<BlockType, shared_ptr<Shape>>(type, cube));
	}

	shared_ptr<Block> b = make_shared<Block>(block_materials[type], x, y, z);
	blocks.push_back(b);

	spatial_coords(vec3(x, y, z), x, y, z);

	spatial_lookup[x][y][z] = b;
}

void Blocks::addBlock(BlockType type, vec3 v) {
	addBlock(type, v[0], v[1], v[2]);
}

/**
 * Removes block at x, y, z if there is a block there. Returns true
 * if a block was removed, false if there was not block at position
 */
bool Blocks::removeAt(vec3 lookat) {
	shared_ptr<Block> blockat = blockAt(lookat);

	if(blockat == nullptr) {
		return false;
	}

	blocks.remove(blockat);

	int x, y, z;
	spatial_coords(lookat, x, y, z);

	spatial_lookup[x][y][z] = nullptr;
	return true;
}

void Blocks::drawBlocks(shared_ptr<MatrixStack> Model) {
	this->texProg->bind();
	this->texture->bind(this->texProg->getUniform("Texture0"));

	for (auto block : blocks){
		block->draw(Model, this->texProg);
	}

	this->texProg->unbind();
}

bool Blocks::hasBlockAt(vec3 v) {
	return this->blockAt(v) != nullptr;
}