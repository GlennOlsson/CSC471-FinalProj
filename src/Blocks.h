#include "Block.h"
#include <memory>
#include <assert.h>
#include <vector>
#include <map>

#include "Texture.h"
#include "Program.h"
#include "Shape.h"
#include "MatrixStack.h"

#ifndef MINECRAFT_BLOCKS_H
#define MINECRAFT_BLOCKS_H

using namespace std;
using namespace glm;

enum BlockType {
	grass,
	wood,
	leave,
	stone
};

class Blocks {
private:
	string resourceDir = "../resources";

	vector<Block> blocks;
	map<BlockType, shared_ptr<Shape>> block_materials;

	/**
	 * Returns vec4 describing the 0.0-1.0 coords in texture for block at x, y (0-indexed)
	 * 1 elem is xMin
	 * 2 elem is xMax
	 * 3 elem is yMin
	 * 4 elem is yMax
	 */
	vec4 coords(int x, int y);

	vector<float> vectorFromCoords(vec4 v1, vec4 v2, vec4 v3, vec4 v4, vec4 v5, vec4 v6);
	vector<float> vectorFromCoords(vec4 side);

	shared_ptr<Shape> loadCube();

public:
	shared_ptr<Program> texProg;
	shared_ptr<Texture> texture;

	Blocks();

	/** 
	* Returns texture coordinates in following order, 2*4 coordinates per side:
	* - Back side (NEG Z)
	* - Top side (POS Y)
	* - Front side (POS Z)
	* - Bottom side (NEG Y)
	* - Right side (POS X)
	* - Left side (NEG X)
	*/
	vector<float> textureCoords(BlockType type);

	void addBlock(BlockType type, int x, int y, int z);

	void drawBlocks(shared_ptr<MatrixStack> Model);
};

#endif