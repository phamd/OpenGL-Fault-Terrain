#ifdef __APPLE__
#include <GLUT/glut.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/glut.h>
#endif
#include <list>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <iostream> // cout
#include "Vector3.h"

/* Controls
 * p		: passiveFaulting : continuously do 1 fault every update
 * <space>	: do 100 faults now
 * f		: do 800 faults now
 * w		: toggle between filled polygons, wireframe, and both
 * i,j,k,l	: rotate the camera around its own axis
 * mouse	: rotate the camera on its y-axis only (click and drag)
 * h,n		: move the camera forwards and backwards
 * Up,Down	: move the camera forwards and backwards (alternate buttons)
 * r		: reset the scene and camera position
 * t		: toggle lighting
 * y		: toggle shaders
*/

/* //debug
std::vector<std::vector<float>> terrain(terrainZ, std::vector<float>(terrainX, 0.5));
std::vector<std::vector<Vector3>> vertexNormals(terrainZ, std::vector<Vector3>(terrainX, Vector3()));
std::vector<std::vector<Vector3>> faceNormals(terrainZ, std::vector<Vector3>(terrainX, Vector3()));
*/

/* Terrain variables */ 
const int terrainZ = 200;
const int terrainX = 200;
const float maxHeight = 50;
const float minHeight = 0;
const float displacement = 0.5;
float terrain[terrainZ][terrainX] = { 0.5 }; // sets all heights to 0.5
enum PolygonMode { Fill, Wireframe, FilledWire };
float terrainAngle = 0;

/* Camera */
float theta = 300;
Vector3 camPos = { 0, 106.0f, 79.3f };
Vector3 camLook = { 300.f * (float)cos(theta) + camPos.x, -249.f, 300.f * (float)sin(theta) + camPos.z };
Vector3 mover;

/* Mouse controls */
bool mouseCursorEnabled = true;
float mouseAngleX = 0;
float mouseAngleY = -45;
bool lastState = GLUT_UP;
bool currentState = GLUT_UP;
Vector3 mouse;

/* Lighting */
Vector3 light0 = { (terrainX / 2.0f), maxHeight + 1.0f, (terrainZ / 2.0f) };
Vector3 light1 = { -(terrainX / 2.0f), maxHeight + 1.0f, -(terrainZ / 2.0f) };

/* Normals */
Vector3 vertexNormals[terrainZ][terrainX];
Vector3 faceNormals[terrainZ][terrainX];

/* States */
int totalTime = 0;
PolygonMode polygonMode = Fill;
bool lightingEnabled = false;
bool passiveFaulting = false;
bool needFaceNormals = true;
bool needVertexNormals = false;
bool flatShading = true; // Flat vs Smooth
bool faultedAfterSmooth = false; // If toggling between shading modes, don't calculate vertex normals again unless a fault occurs.
bool keyStates[256] = { false };

/* Fault terrain algorithm from:
 *  http://www.lighthouse3d.com/opengl/terrain/index.php?fault
 */
void faultTerrain(int times)
{
	for (int t = 0; t < times; t++) {
		float v = (float)rand();
		float a = sin(v);
		float b = cos(v);
		float d = sqrt(terrainX*terrainX + terrainZ*terrainZ);
		float c = ((float)rand() / RAND_MAX) * d - d / 2.0f;
		for (int z = 0; z < terrainZ; z++) {
			for (int x = 0; x < terrainX; x++) {
				if (a*z + b*x - c  > 0) {
					terrain[z][x] += (terrain[z][x] + 0.1f > maxHeight) ? 0 : displacement;
				}
				else
					terrain[z][x] -= (terrain[z][x] + displacement < minHeight) ? 0 : displacement;
			}
		}
	}
	needFaceNormals = true;
	if (!flatShading) needVertexNormals = true;
	faultedAfterSmooth = true;
}

void resetTerrain(void) {
	for (int z = 0; z < terrainZ; z++) {
		for (int x = 0; x < terrainX; x++) {
			terrain[z][x] = 0.5; // initial terrain height
		}
	}
	needFaceNormals = true;
	if (!flatShading) needVertexNormals = true;
}

void resetCamera(void) {
	theta = 300;
	camPos = { 0, 106.0f, 79.3f };
	camLook = { 300.f * (float)cos(theta) + camPos.x, -249.f, 300.f * (float)sin(theta) + camPos.z };
}

Vector3 topographicColoring(Vector3 in){
	if(in.y < maxHeight * 0.2f){
		return {0,0.7,0};
	}
	if (in.y < maxHeight * 0.4f){
		return {1,0.8,0};
	}
	if (in.y < maxHeight * 0.6f){
		return {1,0.6,0};
	}
	if (in.y < maxHeight * 0.8f){
		return {1,0,0};
	}
	else{
		return {0.5,0.5,0.5};
	}
}

void setFaceNormals(void)
{
	Vector3 v1, v2, v3, v4;
	for (int z = 0; z < terrainZ - 1; z++) {
		for (int x = 0; x < terrainX - 1; x++) {
			Vector3 quad[4] = { { x + 0.f, terrain[z][x], z + 0.f }, { x + 0.f, terrain[ z+ 1][x], z + 1.f },
			{ x + 1.f, terrain[ z+ 1][x + 1], z + 1.f }, { x + 1.f, terrain[z][x + 1], z + 0.f } };
			v1 = quad[0].cross(quad[1]);
			v2 = quad[1].cross(quad[2]);
			v3 = quad[2].cross(quad[3]);
			v4 = quad[3].cross(quad[0]);
			Vector3 faceNormal = { (v1.x + v2.x + v3.x + v4.x) / 4.f,
				(v1.y + v2.y + v3.y + v4.y) / 4.f, (v1.z + v2.z + v3.z + v4.z) / 4.f };
			faceNormals[z][x] = faceNormal;
		}
	}
	needFaceNormals = false;
}

void setVertexNormals(void)
{
	Vector3 v1, v2, v3, v4;
	
	for (int z = 1; z < terrainZ - 1; z++) { // Excludes outside perimeter of vertices
		for (int x = 1; x < terrainX - 1; x++) {
			Vector3 quad[4] = { { x + 0.f, terrain[z][x], z + 0.f }, { x + 0.f, terrain[z + 1][x], z + 1.f },
			{ x + 1.f, terrain[z + 1][x + 1], z + 1.f }, { x + 1.f, terrain[z][x + 1], z + 0.f } };

			v1 = faceNormals[z-1][x-1];
			v2 = faceNormals[z][x-1];
			v3 = faceNormals[z][x];
			v4 = faceNormals[z-1][x];
			Vector3 vertexNormal = { (v1.x + v2.x + v3.x + v4.x) / 4.f,
				(v1.y + v2.y + v3.y + v4.y) / 4.f, (v1.z + v2.z + v3.z + v4.z) / 4.f };
			printf("%f %f %f\n", vertexNormal.x, vertexNormal.y, vertexNormal.z);
			vertexNormals[z][x] = vertexNormal;
		}
	}
	needVertexNormals = false;
}

/* (0,0)    (x,0)
 *	  ------
 *	  |    |
 *	  |    |
 *	  ------
 * (0,z)    (x,z)
 */
void drawTerrain(void)
{
	Vector3 color = { 0.0f, 0.0f, 0.5f };
	for (int z = 0; z < terrainZ - 1; z++) {
		for (int x = 0; x < terrainX - 1; x++) {
			Vector3 quad[4] = { { x + 0.f, terrain[z][x], z + 0.f }, { x + 0.f, terrain[z + 1][x], z + 1.f },
			{ x + 1.f, terrain[z + 1][x + 1], z + 1.f }, { x + 1.f, terrain[z][x + 1], z + 0.f } };

			if (false) { //debug: draw vertex normals
				glBegin(GL_LINES);
				glColor3f(1, 0, 0);
				glVertex3f(x, terrain[z][x], z);
				glVertex3fv(vertexNormals[z][x].add({ x + 0.f, terrain[z][x], z + 0.f }).v);
				glEnd();
			}
			
			if (flatShading) glNormal3fv(faceNormals[z][x].v); // define face normal for whole quad
			glBegin(GL_QUADS);
				for (int q = 0; q < 4; q++) {
					glColor3fv(topographicColoring(quad[q]).v);
					if (x < 10 && z < 5) glColor3f(1, 0, 0); //DEBUG
					if (!flatShading) glNormal3fv(vertexNormals[z][x].v);
					glVertex3fv(quad[q].v);
				}
			glEnd();
			
			if (polygonMode == FilledWire) {
				glBegin(GL_LINE_LOOP);
				for (int q = 0; q < 4; q++) {
					glColor3f(1, 0, 1); // wireframe color in combined mode
					glVertex3fv(quad[q].v);
				}
				glEnd();
			}
		}
	}
}


void display(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
		gluLookAt(camPos.x, camPos.y, camPos.z, camLook.x, camLook.y, camLook.z, 0, 1, 0);
		glRotatef(terrainAngle, 0, 1, 0);
/*		glRotatef(-mouseAngleY, 0.0f, 1.0, 0.0f);
		glRotatef(-mouseAngleX, 1.0f, 0.0f, 0.0f);
		glTranslatef(-camPos.x, -camPos.y, -camPos.z);
		std::cout << mouseAngleX << " .. " << mouseAngleY << "\n";
*/	glColor3f(1, 1, 1);

	glPushMatrix();
		glTranslatef(-terrainX / 2, 0, -terrainZ / 2); // center the terrain onto origin
		drawTerrain();
	glPopMatrix();
//	drawBox({ 0, -1, 0 }, { 100, 2, 100 });

	glutSwapBuffers();
}

void updateCamera(void) {
	mover = Vector3(camLook.x - camPos.x, camLook.y - camPos.y, camLook.z - camPos.z).normalize();

	if (keyStates['h']) {
		camPos = camPos.add(mover.scale(+2));
	}
	else if (keyStates['n']) {
		camPos = camPos.add(mover.scale(-2));
	}
	if (keyStates['i']) {
		if (camLook.y <= 300){
			camLook.y += 10;
		}
	}
	else if (keyStates['k']) {
		if (camLook.y >= -300){
			camLook.y -= 10;
		}
	}
	if (keyStates['j']) {
		theta -= 0.1;
		camLook.x = 300 * cos(theta) + camPos.x;
		camLook.z = 300 * sin(theta) + camPos.z;
	}
	else if (keyStates['l']) {
		theta += 0.1;
		camLook.x = 300 * cos(theta) + camPos.x;
		camLook.z = 300 * sin(theta) + camPos.z;
		std::cout << camLook.x << std::endl;
	}
}

void update(int value)
{
	int elapsedTime = glutGet(GLUT_ELAPSED_TIME);
	int deltaTime = elapsedTime - totalTime;
	totalTime = elapsedTime;

	updateCamera();

	// Normals
	if (needFaceNormals)
		setFaceNormals();
	if (needVertexNormals)
		setVertexNormals();

	// Fault algorithm
	if (passiveFaulting) faultTerrain(1);

	glutPostRedisplay();
	glutTimerFunc(16, update, 0);
}

void toggleLighting(bool lit) {
	if (lit)
		glEnable(GL_LIGHTING);
	else
		glDisable(GL_LIGHTING);
	lightingEnabled = lit;
}

void toggleShading(bool flat) {
	if (flat) {
		glShadeModel(GL_FLAT);
	}
	else {
		glShadeModel(GL_SMOOTH);
		if (faultedAfterSmooth) {
			needVertexNormals = true;
			faultedAfterSmooth = false;
		}
	}
	flatShading = flat;
}

void activeMouse(int button, int state, int x, int y)
{
	currentState = state;
	if (lastState == GLUT_UP) {
		if (currentState == GLUT_DOWN) {
			mouse.x = x;
			mouse.y = y;
		}
	}
	lastState = state;
}

void passiveMouse(int x, int y)
{
	if (lastState == GLUT_DOWN) {
		if (currentState == GLUT_DOWN) {
			camLook.y += mouse.y - y;
			mouse.y = y;
			mouse.x = x;
			//std::cout << camLook.y << std::endl;
		}
	}
}

void keyboardUp(unsigned char key, int x, int y)
{
	keyStates[key] = false;
}

void keyboard(unsigned char key, int x, int y)
{
	keyStates[key] = true;

	switch (key) {
	case 't': // Toggle Lighting
		if (lightingEnabled)
			toggleLighting(false);
		else
			toggleLighting(true);
		break;
	case 'y': // Toggle Shading mode
		if (flatShading)
			toggleShading(false);
		else
			toggleShading(true);
		break;
	case 'g':
		needFaceNormals = true;
		needVertexNormals = true;
		break;
	case 'r': // Reset terrain
		resetTerrain();
		resetCamera();
		break;
	case 'p':
		passiveFaulting = !passiveFaulting;
		break;
	case 'w':
		switch (polygonMode) {
		case(Fill):
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			polygonMode = Wireframe;
			break;
		case(Wireframe):
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			polygonMode = FilledWire;
			break;
		case(FilledWire):
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			polygonMode = Fill;
		}
		break;
	case 'f':
		faultTerrain(800);
		break;
	case ' ':
		faultTerrain(100);
		break;
	case 'q':
		exit(0);
		break;
	}
}

/* Alternate controls for moving camera position */
void specialUp(int key, int x, int y)
{
	switch (key) {
	case GLUT_KEY_UP:
		keyStates['h'] = false;
		break;
	case GLUT_KEY_DOWN:
		keyStates['n'] = false;
		break;
	}
}

void special(int key, int x, int y)
{
	switch (key) {
	case GLUT_KEY_UP:
		keyStates['h'] = true; 
		break;
	case GLUT_KEY_DOWN:
		keyStates['n'] = true;
		break;
	case GLUT_KEY_LEFT:
		terrainAngle += (terrainAngle == 350) ? -350 : 10;
		break;
	case GLUT_KEY_RIGHT:
		terrainAngle += (terrainAngle == 0) ? +350 : -10;
		break;
    }
}

void reshape(int w, int h)
{
	glViewport(0, 0, w , h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60, (float)w / (float)h, 1, 500);
}

void init(void)
{
	// Background Color
	glClearColor(0, 0, 0, 0);

	// Culling
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);
	glCullFace(GL_BACK);

	// Shading
	toggleShading(flatShading);

	// Lighting
	toggleLighting(lightingEnabled);
	float pureWhite[4] = { 1, 1, 1, 1 };
	float light0ambient[4] = { 0.804f, 1.f, 0.98f, 1.f }; // light blue
	float light0diffuse[4] = { 0.804f, 1.f, 0.98f, 1.f };
	float light1ambient[4] = { 0, 1.f, 0.4f, 1.f }; // green
	float light1diffuse[4] = { 0, 1.f, 0.4f, 1.f };
	glEnable(GL_LIGHT0);
	glLightfv(GL_LIGHT0, GL_POSITION, light0.v);
	glLightfv(GL_LIGHT0, GL_AMBIENT, light0ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light0diffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, pureWhite);
	glEnable(GL_LIGHT1);
	glLightfv(GL_LIGHT1, GL_POSITION, light1.v);
	glLightfv(GL_LIGHT1, GL_AMBIENT, light1ambient);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, light1diffuse);
	glLightfv(GL_LIGHT1, GL_SPECULAR, pureWhite);

	// Meterials
	glEnable(GL_COLOR_MATERIAL);
	glColorMaterial ( GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE ) ; // use glColor for material
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, pureWhite);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 20);
	
	//resetTerrain();

	//srand(time(NULL));
}

int main(int argc, char* argv[])
{
//	printf("Enter a terrain size: ");
//	scanf("%f", &terrainSize);

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);

	glutInitWindowSize(800, 800);
	glutInitWindowPosition(0, 0);
	glutCreateWindow("Fault Terrain Modeling");

	glutReshapeFunc(reshape);
	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);
	glutKeyboardUpFunc(keyboardUp);
	glutSpecialFunc(special);
	glutSpecialUpFunc(specialUp);
	glutMouseFunc(activeMouse);
	glutMotionFunc(passiveMouse);
	//glutPassiveMotionFunc(passiveMouse);
	glutTimerFunc(16, update, 0);
	init();

	glutMainLoop();
	return(0);
}
