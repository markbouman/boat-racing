#ifndef __OBJECT_H__
#define __OBJECT_H__

#include <vector>
#include <string>

using namespace std;

class vec3 {
public:
	float x;
	float y;
	float z;

	vec3();
	vec3(float nx, float ny, float nz);
};

class Face {
public:
	vector<int> vertices;
	vector<int> normals;
	string material;
	
	void addVertex(int v, int n);
};

class Material {
public:
	string name;
	float amb[4];
	float dif[4];
	float spec[4];

	Material(string nname, float namb1, float namb2, float namb3, float ndif1, float ndif2, float ndif3, float nspec1, float nspec2, float nspec3);
};

vec3 GetVec3(string line, int i);

Face GetFace(string line, int i, string matName);

class Model {
public:
	vector<vec3> vertices;
	vector<vec3> normals;
	vector<Face> faces;
	vector<Material> materials;

	Model(const char* objfile, const char* mtlfile);

	void Draw();
};

#endif