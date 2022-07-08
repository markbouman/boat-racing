#include "object.h"
#include <stdio.h>
#include <stdlib.h>

#ifdef __APPLE__
#  include <OpenGL/gl.h>
#  include <OpenGL/glu.h>
#  include <GLUT/glut.h>
#else
#  include <GL/gl.h>
#  include <GL/glu.h>
#  include <GL/freeglut.h>
#endif

#include <iostream>
#include <vector>
#include <fstream>
#include <string>

using namespace std;

vec3::vec3() {
	x = 0;
	y = 0;
	z = 0;
}

vec3::vec3(float nx, float ny, float nz) {
	x = nx;
	y = ny;
	z = nz;
}


void Face::addVertex(int v, int n) {
	vertices.push_back(v);
	normals.push_back(n);
}

Material::Material(string nname, float namb1, float namb2, float namb3, float ndif1, float ndif2, float ndif3, float nspec1, float nspec2, float nspec3) {
	name = nname.c_str();
	amb[0] = namb1;
	amb[1] = namb2;
	amb[2] = namb3;
	amb[3] = 1;
	dif[0] = ndif1;
	dif[1] = ndif2;
	dif[2] = ndif3;
	dif[3] = 1;
	spec[0] = nspec1;
	spec[1] = nspec2;
	spec[2] = nspec3;
	spec[3] = 1;
}

vec3 GetVec3(string line, int i) {
	int j = line.find(" ", i);
	float x = atof((line.substr(i, j-i)).c_str());
	i = j+1;
	j = line.find(" ", i);
	float y = atof((line.substr(i, j-i)).c_str());
	i = j+1;
	j = line.find(" ", i);
	float z = atof((line.substr(i, j-i)).c_str());
	vec3 v(x, y, z);
	return v;
}

Face GetFace(string line, int i, string matName) {
	Face face;
	face.material = matName;
	while (i < line.size()) {
		int j = line.find("/", i);
		int v = atoi((line.substr(i, j-i)).c_str());
		i = j+1;
		j = line.find("/", i);
		// vt not necessary
		i = j+1;
		j = line.find(" ", i);
		int n = atoi((line.substr(i, j-i)).c_str());
		i = j+1;
		face.addVertex(v, n);
		if (j == -1) break;
	}
	return face;
}

Material GetMaterial(const char* file, string line, int nface) {
	string name = line.substr(8);

}


Model::Model(const char* objfile, const char* mtlfile) {
	string line;
	ifstream mtl;
	mtl.open(mtlfile);
	if (mtl.is_open()) {
		string name;
		vec3 amb;
		vec3 dif;
		vec3 spec;
		while (getline (mtl, line)) {
			if (line.compare(0, 7, "newmtl ") == 0) {
				name = line.substr(7);
			} else if (line.compare(0, 3, "Ka ") == 0) {
				amb = GetVec3(line, 3);
			} else if (line.compare(0, 3, "Kd ") == 0) {
				dif = GetVec3(line, 3);
			} else if (line.compare(0, 3, "Ks ") == 0) {
				spec = GetVec3(line, 3);
			} else if (line.compare(0, 6, "illum ") == 0) {
				materials.push_back(Material(name, amb.x, amb.y, amb.z, dif.x, dif.y, dif.z, spec.x, spec.y, spec.z));
			}
		}
		mtl.close();
	} else cout << "File error" << endl; 
	ifstream obj;
	obj.open(objfile);
	if (obj.is_open()) {
		string matName = "";
		while (getline (obj, line)) {
			if (line.compare(0, 2, "v ") == 0) {
				vertices.push_back(GetVec3(line, 2));
			} else if (line.compare(0, 3, "vn ") == 0) {
				normals.push_back(GetVec3(line, 3));
			} else if (line.compare(0, 2, "f ") == 0) {
				faces.push_back(GetFace(line, 2, matName));
				matName = "";
			} else if (line.compare(0, 7, "usemtl ") == 0) {
				matName = line.substr(7, line.size());
			}
		}
		obj.close();
	} else cout << "File error" << endl; 
}

void Model::Draw() {
	for (int i = 0; i < faces.size(); i++) {
		if (faces[i].material.compare("") != 0) {
			for (int j = 0; j < materials.size(); j++) {
				if (faces[i].material.compare(materials[j].name) == 0) {
					glMaterialfv(GL_FRONT, GL_AMBIENT, materials[j].amb);
					glMaterialfv(GL_FRONT, GL_DIFFUSE, materials[j].dif);
					glMaterialfv(GL_FRONT, GL_SPECULAR, materials[j].spec);
				}
			}
		}
		glBegin(GL_POLYGON);
			for (int j = 0; j < faces[i].vertices.size(); j++) {
				int nor = faces[i].normals[j] - 1;
				int ver = faces[i].vertices[j] - 1;
				glNormal3f(normals[nor].x, normals[nor].y, normals[nor].z);
				glVertex3f(vertices[ver].x, vertices[ver].y, vertices[ver].z);
			}
		glEnd();
	}
}