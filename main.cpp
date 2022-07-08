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
#include <cmath>
#include <vector>
#include <fstream>
#include <string>
#include <ctime>

#include "object.h"
#include "PPM.h"

using namespace std;

float terrainX = 200;
float terrainZ = 5000;

float renderD = 1000; // render distance

float boatStartX = terrainX / 2;
float boatStartZ = 5;

// course constants
float gateHalfSize = 10;
float gateSpacing = 50;


int currentGate = 0;

float gravity = 0.1; // for particles

// boat controls
bool leftpressed;
bool rightpressed;

time_t startTime;
time_t finishTime;
int bonusTime;

bool started;

Model* boatModel;
GLubyte* waterTexture;

class Boat {
public:
	float posx;
	float posz;
	float angle;
	float speed;

	float maxX;
	float minX;
	float maxZ;
	float minZ;

	const static float maxspeed = 3;
	const static float turnspeed = 0.03;
	const static float acceleration = 0.05;

	void movex(float dir) {
		posx += dir;
	}
	void movez(float dir) {
		posz += dir;
	}
	void rotate(float a) {
		angle += a;
	}
	void accelerate() {
		speed += acceleration;
		if (speed >= maxspeed) speed = maxspeed;
	}

	void draw(){
	    glPushMatrix();
	    	glTranslatef(posx, 0, posz); //move boat horizontally
			glRotatef(angle * 180/3.14, 0, 1, 0); //rotate boat according to movement
	    	boatModel -> Draw();
	    glPopMatrix();
	}

	bool collision(float x, float z, float r) { // check collisions on the XZ plane with a circular object
		// check that the object is close
		if ((abs(x - posx) > r + 3) || (abs(z - posz) > r + 3)) {
			return false;
		}
		float v[4][2] = { // x, z coords of bounding plane
			{maxX, maxZ},
			{maxX, minZ},
			{minX, minZ},
			{minX, maxZ},
		};
		int e[4][2] = { // edges
			{0, 1}, {1, 2}, {2, 3}, {3, 0}
		};
		float c = cos(angle);
		float s = sin(angle);
		for (int i = 0; i < 4; i++) { // transform v
			float vx = v[i][0];
			float vz = v[i][1];
			v[i][0] = vx * c - vz * s + posx - x;
			v[i][1] = vz * c + vx * s + posz - z;
		}
		// check if the center of the circle is in the square
		int a = 0;
		for (int i = 0; i < 4; i++) {
			float v1x = v[e[i][0]][0];
			float v1y = v[e[i][0]][1];
			float v2x = v[e[i][1]][0];
			float v2y = v[e[i][1]][1];
			if (v1y == v2y) {
				continue;
			}
			float t = v1y / (v1y-v2y);
			float xi = v1x + (v2x-v1x) * t; 
			if (xi == 0) return true;
			if (t >= 0 && t < 1 && xi > 0) {
				a++;
			}
		}
		if (a == 1) return true;
		// check if the circle intersects with each edge
		for (int i = 0; i < 4; i++) {
			float v1x = v[e[i][0]][0];
			float v1y = v[e[i][0]][1];
			float v2x = v[e[i][1]][0];
			float v2y = v[e[i][1]][1];
			float a = pow(v2x-v1x, 2) + pow(v2y-v1y, 2);
			float b = 2*v1x*(v2x-v1x) + 2*v2y*(v2y-v1y);
			float c = pow(v1x, 2) + pow(v1y, 2) - pow(r, 2);
			float d = pow(b, 2) - 4*a*c;
			if (d < 0) {
				continue;
			}
			float t1 = (-b + sqrt(d))/(2*a);
			float t2 = (-b - sqrt(d))/(2*a);
			if ((t1 > 0 && t1 < 1) || (t2 > 0 && t2 < 1)) {
				return true;
			}
		}
		return false;
	}

};

Boat b;

class Buoy {
public:
	float x;
	float z;
	float r;

	Buoy() {
		x = 0;
		z = 0;
		r = 0.5;
	}

	Buoy(float nx, float nz) {
		x = nx;
		z = nz;
		r = 0.5;
	}

	void draw(bool selected) {
    	if (b.collision(x, z, r)) {
    		b.speed = 0.5;
    	}
		if (selected) {
	    	float amb[4] = {0, 1, 0, 1};
	    	glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, amb);	
		} else {
	    	float amb[4] = {1, 0.3, 0, 1};
	    	glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, amb);
		}
		glPushMatrix();
			glTranslatef(x, 0, z);
	    	glutSolidSphere(r, 20, 20);
		glPopMatrix();
	}
};

class Gate {
public:
	Buoy left;
	Buoy right;

	Gate(float x, float z) {
		left.x = x - gateHalfSize;
		left.z = z;
		right.x = x + gateHalfSize;
		right.z = z;
	}

	void draw(int i) {
		if (i == currentGate) {
			left.draw(true);
			right.draw(true);
		} else {
			left.draw(false);
			right.draw(false);
		}
	}
};

vector<Gate> Gates;

class Bonus {
public:
	float x;
	float z;
	float r;
	bool hit;

	Bonus() {
		x = 0;
		z = 0;
		r = 0.5;
	}

	Bonus(float nx, float nz) {
		x = nx;
		z = nz;
		r = 0.5;
	}

	void draw() {
		if (!hit && currentGate < Gates.size()) {
	    	if (b.collision(x, z, r)) {
	    		bonusTime += 2;
	    		hit = true;
	    	}
	    	float amb[4] = {1, 0, 1, 1};
	    	glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, amb);
			glPushMatrix();
				glTranslatef(x, 0, z);
		    	glutSolidSphere(r, 20, 20);
			glPopMatrix();
		}
	}
};
vector<Bonus> Bonuses;

// get the first gate within a certain distance of the boat
int gateAtDistance(float dist) {
	int i = 0;
	int j = Gates.size() - 1;
	while (i < j - 1) {
		int half = (i + j) / 2;
		if (Gates[half].left.z > b.posz - renderD) {
			j = half;
		} else {
			i = half;
		}
	}
	return i;
}

float weight(float i, float j, float weight) {
	float s = pow(sin(weight * 1.52), 2);
	return (i * s + j * (1 - s));
}

void generateBuoys() {
	float gatex = boatStartX;
	float gatez = boatStartZ + 50;

	float newx = 0;
	float newz = 0;

	while (gatez < terrainZ - 5) {
		// position every fifth gate in a random position and smoothly fill the gap
		newx = rand() % ((int) (terrainX - gateHalfSize*2)) + gateHalfSize;
		newz = gatez + 5 * gateSpacing;

		for (int i = 0; i < 5; i++) {
			float zpos = newz * i*0.2 + gatez * (1-i*0.2);
			float xpos = weight(newx, gatex, i*0.2);
			Gates.push_back(Gate(xpos, zpos));
			// add bonuses between random gates
			if (rand() % 5 == 0) {
				Bonuses.push_back(Bonus(xpos + ((float) (rand() % 10) - 5) * gateHalfSize * 0.1, zpos));
			}
		}

		gatex = newx;
		gatez = newz;
	}
	Gates.push_back(Gate(gatex, gatez));

}

struct particle {
	bool active;
	float x;
	float y;
	float z;
	float vx;
	float vy;
	float vz;

	void draw() {
		float amb[4] = {1, 1, 1, 0.7};
    	float dif[4] = {0, 0, 0, 0.7};
    	glMaterialfv(GL_FRONT, GL_AMBIENT, amb);
    	glMaterialfv(GL_FRONT, GL_DIFFUSE, dif);
    	glPushMatrix();
    		glTranslatef(b.posx, 0, b.posz);
			glRotatef(b.angle * 180/3.14, 0, 1, 0);
			glTranslatef(-x, y, -z-2.5);
			glBegin(GL_QUADS);
			float s = 0.02;
    		glVertex3f(s, -s, -0.0);
    		glVertex3f(-s, -s, -0.0);
    		glVertex3f(-s, s, 0.0);
    		glVertex3f(s, s, 0.0);
			glEnd();
    	glPopMatrix();
	}

	void update() {
		vy -= gravity;
		x += vx;
		y += vy;
		z += vz;
	}

	void start() {
		active = true;
		x = 0;
		y = ((rand() % 20) * 0.01 + 0.1);
		z = 0;
		vx = b.speed * 0.03 * ((rand() % 20) * 0.1 - 1);
		vy = b.speed * (rand() % 20) * 0.01;
		vz = b.speed * (rand() % 20) * 0.02;
	}
};

particle ps[900];

void setPerspective() {
	glEnable(GL_LIGHTING);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45, 1.5, 1, 500);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void setOrthographic() {
	glDisable(GL_LIGHTING);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, 1200, 0, 800);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void display() {
	glClearColor(0.7, 0.8, 1, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    setPerspective();

	// camera
    gluLookAt(
        b.posx - 15 * sin(b.angle), 5, b.posz - 15 * cos(b.angle),
        b.posx, 0, b.posz,
        0, 1, 0
    );

    // Boat
    b.draw();
    // Obstacles
    for (int i = gateAtDistance(renderD); i < Gates.size(); i++) {
    	if (Gates[i].left.z > b.posz + renderD) {
    		break;
    	}
    	Gates[i].draw(i);
    }
    // bonuses
    for (int i = 0; i < Bonuses.size(); i++) {
    	Bonuses[i].draw();
    }
    // Water
    glBindTexture(GL_TEXTURE_2D, 0);
    glBegin(GL_QUADS);
    	float amb[4] = {0.8, 0.8, 1, 0.8};
    	float dif[4] = {0, 0, 0, 0.8};
    	glMaterialfv(GL_FRONT, GL_AMBIENT, amb);
    	glMaterialfv(GL_FRONT, GL_DIFFUSE, dif);
    	int n = 100;
    	glTexCoord2f((b.posx - renderD) / n, (b.posz - renderD) / n);
    	glVertex3f(b.posx - renderD, 0, b.posz - renderD);
    	glTexCoord2f((b.posx - renderD) / n, (b.posz + renderD) / n);
    	glVertex3f(b.posx - renderD, 0, b.posz + renderD);
    	glTexCoord2f((b.posx + renderD) / n, (b.posz + renderD) / n);
    	glVertex3f(b.posx + renderD, 0, b.posz + renderD);
    	glTexCoord2f((b.posx + renderD) / n, (b.posz - renderD) / n);
    	glVertex3f(b.posx + renderD, 0, b.posz - renderD);
    glEnd();
    glBindTexture(GL_TEXTURE_2D, 1);
    // Particles
    int nParticles = 0;
    for (int i = 0; i < 900; i++) {
    	if (ps[i].active && ps[i].y >= 0) {
    		ps[i].update();
    		ps[i].draw();
    	} else if (nParticles < 120) {
    		ps[i].start();
    		nParticles++;
    	}
    }
    // score
    setOrthographic();

	glColor3f(1, 1, 1);
	glLineWidth(3);
    glPushMatrix();
	    glTranslatef(0, 750, 0);
	    glScalef(0.2, 0.2, 0.2);
	    char text[12];
	    int seconds = time(NULL) - startTime;
	    if (!started) {
	    	seconds = 0;
	    }
	    if (currentGate == Gates.size()) {
	    	seconds = finishTime - startTime;
	    }
	    int le = sprintf(text, "Time: %i", seconds - bonusTime);
	    for (int i = 0; i < le; i++) {
	    	glutStrokeCharacter(GLUT_STROKE_ROMAN, text[i]);
	    }
	glPopMatrix();
    if (!started) {
    	glPushMatrix();
		    glTranslatef(320, 500, 0);
		    glScalef(0.4, 0.4, 0.4);
		    string text = "Press SPACE to start";
		    for (int i = 0; i < text.size(); i++) {
		    	glutStrokeCharacter(GLUT_STROKE_ROMAN, text[i]);
		    }
		glPopMatrix();
    }
    if (currentGate == Gates.size()) {
    	glPushMatrix();
		    glTranslatef(500, 500, 0);
		    glScalef(0.4, 0.4, 0.4);
		    string text = "Finished";
		    for (int i = 0; i < text.size(); i++) {
		    	glutStrokeCharacter(GLUT_STROKE_ROMAN, text[i]);
		    }
		glPopMatrix();
    }
    glutPostRedisplay();
    glFlush();
}

void keyboard(unsigned char key, int x, int y) {
	int mod = glutGetModifiers();
	
	switch(key) {
		case 'q':
		case 'Q':
			exit(0);
			break;
		case ' ':
			if (!started) {
				started = true;
				startTime = time(NULL);
			}
	}
}

void specialKeyboard(int key, int x, int y) {
	switch (key) {
		case GLUT_KEY_LEFT:
			leftpressed = true;
			break;
		case GLUT_KEY_RIGHT:
			rightpressed = true;
			break;
	}
}

void specialKeyboardUp(int key, int x, int y) {
	switch (key) {
		case GLUT_KEY_LEFT:
			leftpressed = false;
			break;
  
		case GLUT_KEY_RIGHT:
			rightpressed = false;
			break;
	}
}

void timer(int value) {
	glutTimerFunc(30, timer, 0);
	if (started) {
		if (leftpressed) {
			b.rotate(b.turnspeed);
		}
		else if (rightpressed) {
			b.rotate(-b.turnspeed);
		}
		// check if through gate
		Gate * g = &Gates[currentGate];
		bool checkx = b.posx > g->left.x && b.posx < g->right.x;
		if (checkx) {
			bool checkz1 = b.posz <= g->left.z && b.posz + b.speed*cos(b.angle) > g->left.z;
			bool checkz2 = b.posz >= g->left.z && b.posz + b.speed*cos(b.angle) < g->left.z;
			if (checkz1 || checkz2) {
				currentGate++;
				if (currentGate == Gates.size()) {
					finishTime = time(NULL);
				}
			}
		}
		b.movex(b.speed * sin(b.angle));
		b.movez(b.speed * cos(b.angle));
		b.accelerate();
	}	
}

void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    gluPerspective(45, 1.5, 1, 500);

    glMatrixMode(GL_MODELVIEW);
}

void init() {
	// load boat
	boatModel = new Model("boat.obj", "boat.mtl");
	// boat bounds
	b.maxX = 0.9;
	b.minX = -0.9;
	b.maxZ = 2;
	b.minZ = -2;

	b.posx = boatStartX;
	b.posz = boatStartZ;

	glutInitWindowSize(1200, 800);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH);
	glutCreateWindow("Boat Racing");

	glEnable(GL_LIGHT0);

	float pos[4] = {1, 1, 1, 0};
	float amb[4] = {0.8, 0.8, 0.8, 1};
	float dif[4] = {0.2, 0.2, 0.2, 1};
	float spec[4] = {1, 1, 1, 1};

	glLightfv(GL_LIGHT0, GL_POSITION, pos);
	glLightfv(GL_LIGHT0, GL_AMBIENT, amb);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, dif);
	glLightfv(GL_LIGHT0, GL_SPECULAR, spec);

	glutTimerFunc(30, timer, 0); //smooth held inputs
	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard); //currently just has q to quit
	glutSpecialFunc(specialKeyboard); //arrow keys pressed controls
	glutSpecialUpFunc(specialKeyboardUp); //arrow keys released controls
	glutReshapeFunc(reshape);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glEnable (GL_DEPTH_TEST); 

	generateBuoys();

	// load water texture
	glEnable(GL_TEXTURE_2D);
    GLuint textures[2];
    glGenTextures(2, textures);
    int waterW;
    int waterH;
	waterTexture = LoadPPM("Water.ppm", &waterW, &waterH);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, waterW, waterH, 0, GL_RGB, GL_UNSIGNED_BYTE, waterTexture);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, textures[0]);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

int main(int argc, char** argv) {

	glutInit(&argc, argv);		//starts up GLUT
	init();

	glutMainLoop();

	return (0);

}