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

/* Explanations
 * p	 : passiveFaulting = continuously do 1 fault every update
 * f	 : do 200 faults now
 * F	 : do 2000 faults now
 * w	 : toggle between filled polygons, wireframe, and both
 * 
*/

/* Terrain constants */ 
const int terrainLength = 100;
const int terrainWidth = 100;
const float maxHeight = 50;
const float minHeight = 0;
const float displacement = 0.5;
float terrain[terrainLength][terrainWidth] = { 0.5 }; // sets all heights to 0.5
const enum PolygonMode { Fill, Wireframe, FilledWire };

/* Camera */
Vector3 camPos = { -40, 38, 30 };

bool mouseCursorEnabled = true;
float mouseAngleX = 0;
float mouseAngleY = -45;
Vector3 mouse;

/* Lighting */
Vector3 light0 = { (terrainWidth / 2), maxHeight + 1, (terrainLength / 2) };
Vector3 light1 = { -(terrainWidth / 2), maxHeight + 1, -(terrainLength / 2) };
Vector3 normals[terrainLength][terrainWidth][4];

/* States */
int totalTime = 0;
PolygonMode polygonMode = Fill;
bool lightingEnabled = true;
bool passiveFaulting = false;
bool needNormals = true;
bool drawSurfaceNormals = false; //debug


/* Fault terrain algorithm from:
 *  http://www.lighthouse3d.com/opengl/terrain/index.php?fault
 */
void faultTerrain(void)
{
	float v = (float) rand();
	float a = sin(v);
	float b = cos(v);
	float d = sqrt(terrainWidth*terrainWidth + terrainLength*terrainLength);
	float c = ((float)rand() / RAND_MAX) * d - d / 2;
	for (int i = 0; i < terrainLength; i++) {
		for (int j = 0; j < terrainWidth; j++) {
			if (a*i + b*j - c  > 0) {
				terrain[i][j] += (terrain[i][j] + 0.1 > maxHeight) ? 0 : displacement;
			}
			else
				terrain[i][j] -= (terrain[i][j] + displacement < minHeight) ? 0 : displacement;
		}
	}
}

void resetTerrain(void) {
	for (int i = 0; i < terrainLength; i++) {
		for (int j = 0; j < terrainWidth; j++) {
			terrain[i][j] = 0.5; // initial terrain height
		}
	}
}

void setNormals(Vector3 quad[4])
{
	for (int q = 0; q < 4; q++) {
		normals[(int)quad[q].x][(int)quad[q].z][0] = quad[0].cross(quad[1]);
		normals[(int)quad[q].x][(int)quad[q].z][1] = quad[1].cross(quad[2]);
		normals[(int)quad[q].x][(int)quad[q].z][2] = quad[2].cross(quad[3]);
		normals[(int)quad[q].x][(int)quad[q].z][3] = quad[3].cross(quad[0]);
	}
}

void drawTerrain(void)
{
	Vector3 color = { 0.0f, 0.0f, 0.5f };
	for (int i = 0; i < terrainLength-1; i++) {
		for (int j = 0; j < terrainWidth - 1; j++) {
			Vector3 quad[4] = { { i + 0.f, terrain[i][j], j + 0.f }, { i + 0.f, terrain[i][j + 1], j + 1.f },
			{ i + 1.f, terrain[i + 1][j + 1], j + 1.f }, { i + 1.f, terrain[i + 1][j], j + 0.f } };

			if (lightingEnabled && needNormals) setNormals(quad);

			Vector3 averagedNormal = { (normals[i][j][0].x + normals[i][j][1].x + normals[i][j][2].x + normals[i][j][3].x) / 4.f,
				(normals[i][j][0].y + normals[i][j][1].y + normals[i][j][2].y + normals[i][j][3].y) / 4.f,
				(normals[i][j][0].z + normals[i][j][1].z + normals[i][j][2].z + normals[i][j][3].z) / 4.f };
			
			if (drawSurfaceNormals) { //debug
				glBegin(GL_LINES);
				glColor3f(1, 0, 0);
				glVertex3f(i + 0.5f, terrain[i][j], j + 0.5f);
				glVertex3fv(averagedNormal.add({ i + 0.5f, terrain[i][j], j + 0.5f }).v);
				glEnd();
			}
			
			glBegin(GL_QUADS);
				for (int q = 0; q < 4; q++) {
					//glColor3fv(quad[q].scale(1.f/maxHeight).v); // wrong colors but cool
					glColor3f(quad[q].y / maxHeight, quad[q].y / maxHeight, quad[q].y / maxHeight);
					if (lightingEnabled) glNormal3fv(normals[i][j][q].v);
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
	needNormals = false;
}


void display(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
		gluLookAt(camPos.x, camPos.y, camPos.z, 0, 0, 0, 0, 1, 0);
/*		glRotatef(-mouseAngleY, 0.0f, 1.0, 0.0f);
		glRotatef(-mouseAngleX, 1.0f, 0.0f, 0.0f);
		glTranslatef(-camPos.x, -camPos.y, -camPos.z);
		std::cout << mouseAngleX << " .. " << mouseAngleY << "\n";
*/	glColor3f(1, 1, 1);

	glPushMatrix();
		glTranslatef(-terrainWidth / 2, 0, -terrainLength / 2); // center the terrain onto origin
		drawTerrain();
	glPopMatrix();
//	drawBox({ 0, -1, 0 }, { 100, 2, 100 });

	glutSwapBuffers();
}

void update(int value)
{
	int elapsedTime = glutGet(GLUT_ELAPSED_TIME);
	int deltaTime = elapsedTime - totalTime;
	totalTime = elapsedTime;

	if (passiveFaulting) faultTerrain();

	glutPostRedisplay();
	glutTimerFunc(16, update, 0);
}

void passiveMouse(int x, int y)
{
	if (!mouseCursorEnabled) {
		mouseAngleX += (float)(x - glutGet(GLUT_WINDOW_WIDTH) / 2);
		mouseAngleY += (float)(y - glutGet(GLUT_WINDOW_HEIGHT) / 2);
		if (mouseAngleY > 180) mouseAngleY = 180;
		else if (mouseAngleY < -180) mouseAngleY = -180;
		if (mouseAngleX > 360) mouseAngleX -= 360;
		else if (mouseAngleX < 0) mouseAngleX += 360;

		glutWarpPointer((glutGet(GLUT_WINDOW_X) + glutGet(GLUT_WINDOW_WIDTH)) / 2, (glutGet(GLUT_WINDOW_Y) + glutGet(GLUT_WINDOW_HEIGHT)) / 2);
	}
}

void keyboard(unsigned char key, int x, int y)
{
	switch (key) {

	case 'm':
		if (!mouseCursorEnabled) {
				glutSetCursor(GLUT_CURSOR_CROSSHAIR);
				mouseCursorEnabled = true;
		}
		else {
			glutSetCursor(GLUT_CURSOR_NONE);
			mouseCursorEnabled = false; 
		}
		break;
	case 'n':
		needNormals = true;
		break;
	case 'r':
		resetTerrain();
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

		std::cout << polygonMode << std::endl;
		break;
	case 'f':
		for (int i = 0; i < 800; i++)
			faultTerrain();
		break;
	case ' ':
		for (int i = 0; i < 100; i++)
			faultTerrain();
		break;
	case 'q':
		exit(0);
		break;
	}
}

void special(int key, int x, int y)
{
	switch (key) {
	case GLUT_KEY_LEFT:
		camPos.y -= 1;
		break;
	case GLUT_KEY_RIGHT:
		camPos.y += 1;
		break;
	case GLUT_KEY_DOWN:
		camPos.x -= 1;
		break;
	case GLUT_KEY_UP:
		camPos.x += 1;
		break;
	case GLUT_KEY_PAGE_UP:
		camPos.z += 1;
		break;
	case GLUT_KEY_PAGE_DOWN:
		camPos.z -= 1;
		break;
	}
}

void reshape(int w, int h)
{
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60, (float)w / (float)h, 1, 500);
}

void init(void)
{
	glutSetCursor(GLUT_CURSOR_CROSSHAIR);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);
	glCullFace(GL_BACK);
	glClearColor(0, 0, 0, 0);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHT1);

	float pureWhite[4] = { 1, 1, 1, 1 };
	float lightYellow[4] = { 1, 1, 0.7, 1 };
	float lightBlue[4] = { 0.5, 0.9, 0.97, 1 };

	float light0ambient[4] = { 0.804f, 1.f, 0.98f, 1.f }; // light blue
	float light0diffuse[4] = { 0.804f, 1.f, 0.98f, 1.f };
	float light0spec[4] = { 1.f, 1.f, 1.f, 1.f };

	float light1ambient[4] = { 0, 1.f, 0.4f, 1.f }; // green
	float light1diffuse[4] = { 0, 1.f, 0.4f, 1.f };
	float light1spec[4] = { 1.f, 1.f, 1.f, 1.f };

	glLightfv(GL_LIGHT0, GL_POSITION, light0.v);
	glLightfv(GL_LIGHT0, GL_AMBIENT, light0ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light0diffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, light0spec);

	glLightfv(GL_LIGHT1, GL_POSITION, light1.v);
	glLightfv(GL_LIGHT1, GL_AMBIENT, light1ambient);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, light1diffuse);
	glLightfv(GL_LIGHT1, GL_SPECULAR, light1spec);
	
	glEnable(GL_COLOR_MATERIAL);
	glColorMaterial ( GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE ) ; // use glColor for materials
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, pureWhite);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 20);
	
	//resetTerrain();

	//srand(time(NULL));
}

int main(int argc, char* argv[])
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);

	glutInitWindowSize(800, 800);
	glutInitWindowPosition(0, 0);
	glutCreateWindow("Fault Terrain Modeling");

	glutReshapeFunc(reshape);
	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);
	glutSpecialFunc(special);
	//glutPassiveMotionFunc(passiveMouse);
	glutTimerFunc(16, update, 0);
	init();

	glutMainLoop();
	return(0);
}
