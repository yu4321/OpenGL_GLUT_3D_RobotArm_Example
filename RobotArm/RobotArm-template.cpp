#include <stdlib.h>
#include <stdio.h>
#include <cmath>
#include "..\include\GL\freeglut.h"
#include <vector>
#include <iostream>

using namespace std;

enum JointType
{
	NotJoint, Ball, OneSide, Slide
};

//class Joint;
class Block;
class Vector3;
class AttachData;

class Vector3 {
public:
	float x, y, z;

	Vector3(float nx = 0, float ny = 0, float nz = 0) {
		x = nx;
		y = ny;
		z = nz;
	}
};

class Block {
public:
	vector<AttachData>* attached=new vector<AttachData>();
	Vector3* size = new Vector3();
	Vector3* position = new Vector3();
	Vector3* rotation = new Vector3();
	float rotateAngle;
	int textureType;
	int shapeType;
};

class AttachData {
public:
	Vector3 *attachedPosition = new Vector3();
	Block *block;
	JointType jointType;

	AttachData(Vector3* pos, Block* blk, JointType jt) {
		attachedPosition = pos;
		block = blk;
		jointType = jt;
	}
};





// 윈도우 크기
int Width = 800, Height = 800;

// 장면 조작을 위한 변수들
int ManipulateMode = 0; // 1: 회전, 2: 이동
int StartPt[2];
float Axis[3] = { 1.0, 0.0, 0.0 };
float Angle = 0.0;
float RotMat[16] = { 1, 0, 0, 0,
					0, 1, 0, 0,
					0, 0, 1, 0,
					0, 0, 0, 1 };
float Zoom = -30.0;
float Pan[3] = { 0.0, 0.0, 0.0 };

// 콜백 함수들
void Reshape(int w, int h);
void Mouse(int button, int state, int x, int y);
void Motion(int x, int y);
void MouseWheel(int button, int dir, int x, int y);
void Keyboard(unsigned char key, int x, int y);
void Render();

// 사용자 정의 함수들
void InitOpenGL();
void GetSphereCoord(int x, int y, float *px, float *py, float *pz);
void RenderFloor();
void RenderRobot();
void drawCube(float sx, float sy, float sz);
void SetupViewVolume();
void SetupViewTransform();
void Sub(double out[3], double a[3], double b[3]);
void Add(double out[3], double a[3], double b[3]);
void Cross(double out[3], double a[3], double b[3]);

//추가 구현 함수들
void InitializeBlocks();
void GiveMaterial(Block* blk);
void DrawBlocksRecursive(Block* block);
void DrawBlocksRecursive(Block* block, Block* ancestor,GLfloat over);

//추가 구현 변수들
vector<Block*> blocks;


void Cross(double out[3], double a[3], double b[3])
{
	out[0] = a[1] * b[2] - a[2] * b[1];
	out[1] = a[2] * b[0] - a[0] * b[2];
	out[2] = a[0] * b[1] - a[1] * b[0];
}

void Sub(double out[3], double a[3], double b[3])
{
	out[0] = a[0] - b[0];
	out[1] = a[1] - b[1];
	out[2] = a[2] - b[2];
}

void Add(double out[3], double a[3], double b[3])
{
	out[0] = a[0] + b[0];
	out[1] = a[1] + b[1];
	out[2] = a[2] + b[2];
}

int main(int argc, char **argv)
{
	// GLUT 초기화(더블 칼라버퍼, RBGA, 깊이버퍼 사용)
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);

	// 윈도우 생성
	glutInitWindowSize(Width, Height);
	glutCreateWindow("3DViewer");

	InitializeBlocks();

	// OpenGL 초기화
	InitOpenGL();

	// 콜백함수 등록
	glutReshapeFunc(Reshape);
	glutMouseFunc(Mouse);
	glutKeyboardFunc(Keyboard);
	glutMotionFunc(Motion); // 마우스 버튼 누리고 움직일 때, 자동으로 호출되는 함수
	glutMouseWheelFunc(MouseWheel);
	glutDisplayFunc(Render);

	// 메시지 루프 진입
	glutMainLoop();
	return 0;
}

void InitializeBlocks() 
{
	// 구현 하세요.
	Block* base = new Block();
	base->position = new Vector3(0, 0.5, 0);
	base->size = new Vector3(10, 1, 10);
	base->shapeType = 1;
	base->textureType = 1;
	blocks.push_back(base);

	Block* lower = new Block();
	lower->position = new Vector3(0, 3, 0);
	lower->size = new Vector3(1, 5, 1);
	lower->shapeType = 1;
	lower->textureType = 2;

	AttachData data1(base->position, lower, NotJoint);
	//blocks.push_back(lower);

	Block* center = new Block();
	center->shapeType = 2;
	center->position = new Vector3(0, 0.5, 0);
	center->size = new Vector3(1, 1, 1);
	center->textureType = 3;
	center->rotateAngle = 30;

	center->rotation = new Vector3(0,0,1);
	AttachData data2(lower->position, center, Ball);
	//blocks.push_back(center);

	Block* upper = new Block();
	upper->position = new Vector3(0, 3, 0);
	upper->size = new Vector3(1, 5, 1);
	upper->shapeType = 1;
	upper->textureType = 4;
	AttachData data3(center->position, upper, NotJoint);
	//blocks.push_back(upper);

	Block* clawL = new Block();
	clawL->position = new Vector3(-0.5, 0.7, 0);
	clawL->size = new Vector3(0.1, 2, 1.0);
	clawL->shapeType = 1;
	clawL->textureType = 5;
	AttachData data4A(upper->position, clawL, Slide);
	//blocks.push_back(clawL);


	Block* clawR = new Block();
	clawR->position = new Vector3(0.5, 0.7, 0);
	clawR->size = new Vector3(0.1, 2, 1.0);
	clawR->shapeType = 1;
	clawR->textureType = 6;
	AttachData data4B(upper->position, clawR, Slide);

	upper->attached->push_back(data4A); upper->attached->push_back(data4B);
	center->attached->push_back(data3);
	lower->attached->push_back(data2);
	base->attached->push_back(data1);
	blocks.push_back(base);
	//blocks.push_back(clawR);
}

void GiveMaterial(Block* blk) 
{
	switch (blk->textureType) {
		case 1: {
			float mat0_diffuse[] = { 0.6, 0.6, 0.0 };
			glMaterialfv(GL_FRONT, GL_DIFFUSE, mat0_diffuse);
			break;
		}
		case 2: {
			float mat0_diffuse[] = { 0.5, 0.6, 0.5 };
			glMaterialfv(GL_FRONT, GL_DIFFUSE, mat0_diffuse);
			break;
		}
		case 3: {
			float mat0_diffuse[] = { 0.1, 0.1, 0.7 };
			glMaterialfv(GL_FRONT, GL_DIFFUSE, mat0_diffuse);
			break;
		}
		case 4: {
			float mat0_diffuse[] = { 0.7, 0.0, 1.0 };
			glMaterialfv(GL_FRONT, GL_DIFFUSE, mat0_diffuse);
			break;
		}
		case 5: {
			float mat0_diffuse[] = { 1.0, 1.0, 1.0 };
			glMaterialfv(GL_FRONT, GL_DIFFUSE, mat0_diffuse);
			break;
		}
		case 6: {
			float mat0_diffuse[] = { 0, 0, 0 };
			glMaterialfv(GL_FRONT, GL_DIFFUSE, mat0_diffuse);
			break;
		}
	}
}


void InitOpenGL()
{
	// 깊이 테스트를 활성화
	glEnable(GL_DEPTH_TEST);

	// 조명 모델 활성화
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);	
	float mat0_ambient[] = { 0.3, 0.0, 0.0 };
	float mat0_specular[] = { 0.9, 0.9, 0.9 };
	float mat0_shininess = 20;
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, mat0_ambient);
	//glMaterialfv(GL_FRONT, GL_SPECULAR, mat0_specular);
	glMaterialfv(GL_FRONT, GL_SHININESS, &mat0_shininess);
}

void Render()
{
	// Clear color buffer
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Set up viewing volume
	SetupViewVolume();

	// Set up viewing transformation
	SetupViewTransform();

	glMatrixMode(GL_MODELVIEW);
	RenderFloor();
	RenderRobot();

	// Swap buffers for double buffering.
	glutSwapBuffers();
}

void RenderRobot()
{
	//glMatrixMode(GL_MODELVIEW);
	//glLoadIdentity();
	for (auto blk : blocks) {
		DrawBlocksRecursive(blk);
		////GiveMaterial(blk);
		//if (blk.shapeType == 1)
		//	DrawBlock(blk);
		//else
		//	DrawSphere(blk);
	}
}

void DrawBlocksRecursive(Block* block, Block* ancestor, GLfloat over) {
	cout << "r - block generate push matrix : " << block->textureType << endl;
	glPushMatrix();
	{
		GiveMaterial(block);
		//glTranslatef(block->position->x, block->position->y , block->position->z);
		GLfloat nx = block->position->x;//(block->size->x + ancestor->size->x) / 2 + block->position->x;
		GLfloat ny =  block->position->y;
		//GLfloat ny = (block->size->y + over) / 2 + block->position->y;
		GLfloat nz = block->position->z;//(block->size->z + ancestor->size->z) / 2 + block->position->z;
		glTranslatef(nx, ny, nz);
		/*glTranslatef((block->size->x + ancestor->size->x) / 2 + block->position->x,
			(block->size->y + ancestor->size->y) / 2 + block->position->y,
			(block->size->z + ancestor->size->z) / 2 + block->position->z);*/
		glScalef(block->size->x/ancestor->size->x, block->size->y/ancestor->size->y, block->size->z/ancestor->size->z);
		if (block->shapeType == 1)
			glutSolidCube(1.0);
		else
			glutSolidSphere(1.0, 50, 50);
		glRotatef(block->rotateAngle, block->rotation->x, block->rotation->y, block->rotation->z);
		vector<AttachData> atch = *(block->attached);
		//block->size->y = ny - block->position->y;
		for (auto a : atch)
			DrawBlocksRecursive(a.block,block, block->size->y +over);
	}
	glPopMatrix();
	cout << "r - block generate pop matrix : " << block->textureType << endl;
}

void DrawBlocksRecursive(Block* block) {
	cout << "block generate push matrix : " << block->textureType << endl;
	glPushMatrix();
	{
		GiveMaterial(block);
		glTranslatef(block->position->x, block->position->y, block->position->z);
		glRotatef(block->rotateAngle, block->rotation->x, block->rotation->y, block->rotation->z);
		glScalef(block->size->x, block->size->y, block->size->z);
		if (block->shapeType == 1)
			glutSolidCube(1.0);
		else
			glutSolidSphere(1.0, 50, 50);

		vector<AttachData> atch = *(block->attached);
		for (auto a : atch)
			DrawBlocksRecursive(a.block,block,  block->size->y);
	}
	glPopMatrix();
	cout << "block generate pop matrix : " << block->textureType << endl;
}

//void DrawBlock(Block block) {
//	glPushMatrix();
//	GiveMaterial(block);
//	glTranslatef(block.position.x, block.position.y, block.position.z);
//	glRotatef(block.rotateAngle, block.rotation.x, block.rotation.y, block.rotation.z);
//	glScalef(block.size.x, block.size.y, block.size.z);
//	glutSolidCube(1.0);
//	glPopMatrix();
//}
//
//void DrawSphere(Block block) {
//	glPushMatrix();
//	GiveMaterial(block);
//	glTranslatef(block.position.x, block.position.y, block.position.z);
//	glRotatef(block.rotateAngle, block.rotation.x, block.rotation.y, block.rotation.z);
//	glScalef(block.size.x, block.size.y, block.size.z);
//	glutSolidSphere(1.0,50,50);
//	glPopMatrix();
//}

void Keyboard(unsigned char key, int x, int y)
{
	// 구현 하세요.
}

void drawCube(float sx, float sy, float sz)
{
	glPushMatrix();
	glTranslatef(0.0, sy * 0.5, 0.0);
	glScalef(sx, sy, sz);
	glutSolidCube(1.0);
	glPopMatrix();
}



void Reshape(int w, int h)
{
	// 뷰포트 변환
	glViewport(0, 0, w, h);
	Width = w;
	Height = h;
}

void SetupViewVolume()
{
	// 관측 공간 지정
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(30.0, (double)Width / (double)Height, 1.0, 10000.0);
}

void SetupViewTransform()
{
	// 모델 뷰 행렬을 단위 행렬로 초기화, M = I
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// 줌 인/아웃을 위한 변환, M = I * T_zoom
	glTranslatef(0.0, 0.0, Zoom);

	// 새로운 회전을 적용, M = I * T_zoom * R_new
	glRotatef(Angle, Axis[0], Axis[1], Axis[2]);

	// 기존 회전을 적용, M = I * T_zoom * R_new * R_old	//   R_n .... * R3 * R2 * R1
	glMultMatrixf(RotMat);

	// 회전 행렬 추출, R_old = R_new * R_old
	glGetFloatv(GL_MODELVIEW_MATRIX, RotMat);
	RotMat[12] = RotMat[13] = RotMat[14] = 0.0;

	// 이동 변환을 적용, M = I * T_zoom * R_new * R_old * T_pan
	glTranslatef(Pan[0], Pan[1], Pan[2]);
}

void RenderFloor()
{
	glDisable(GL_LIGHTING);
	glColor3f(0.7f, 0.7f, 0.7f);
	for (int x = -10; x <= 10; x++)
	{
		if (x == 0)
			continue;
		glBegin(GL_LINES);
		glVertex3f((float)x, 0.0, -10.0f);
		glVertex3f((float)x, 0.0, 10.0f);
		glVertex3f(-10.0f, 0.0, (float)x);
		glVertex3f(10.0f, 0.0, (float)x);
		glEnd();
	}

	glLineWidth(2.0f);
	glColor3f(0.3f, 0.3f, 0.3f);
	glBegin(GL_LINES);
	glVertex3f(-10.0f, 0.0f, 0.0f);
	glVertex3f(10.0f, 0.0, 0.0f);
	glEnd();

	glBegin(GL_LINES);
	glVertex3f(0.0f, 0.0f, -10.0f);
	glVertex3f(0.0f, 0.0, 10.0f);
	glEnd();
	glLineWidth(1.0f);
	glEnable(GL_LIGHTING);
}

void Mouse(int button, int state, int x, int y)
{
	if (state == GLUT_DOWN)
	{
		StartPt[0] = x;
		StartPt[1] = y;
		if (button == GLUT_LEFT_BUTTON)
			ManipulateMode = 1;	// 회전

		if (button == GLUT_RIGHT_BUTTON)
			ManipulateMode = 2;	// 이동
	}
	if (state == GLUT_UP)
	{
		ManipulateMode = 0;	// 리셋
		StartPt[0] = StartPt[1] = 0;
		Angle = 0.0;
	}
}

void Motion(int x, int y)
{
	// 회전축과 회전 각도 계산
	if (ManipulateMode == 1)
	{
		// 단위 구 위의 좌표 계산
		float px, py, pz, qx, qy, qz;
		GetSphereCoord(StartPt[0], StartPt[1], &px, &py, &pz);
		GetSphereCoord(x, y, &qx, &qy, &qz);

		// 회전 축과 각도 계산
		Axis[0] = py * qz - pz * qy;
		Axis[1] = pz * qx - px * qz;
		Axis[2] = px * qy - py * qx;
		Angle = 0.0;
		float len = Axis[0] * Axis[0] + Axis[1] * Axis[1] + Axis[2] * Axis[2];
		if (len > 0.0001) // 일정 변위 이상만 처리
			Angle = acos(px * qx + py * qy + pz * qz) * 180.0f / 3.141592f;
	}

	// 이동 변위 계산
	if (ManipulateMode == 2)
	{
		float dx = (float)(x - StartPt[0]) * 0.01f;
		float dy = (float)(StartPt[1] - y) * 0.01f;
		// 회전 행렬 및 역행렬
		// R = 0 4 8   invR = 0 1 2
		//     1 5 9          4 5 6    
		//     2 6 10         8 9 10
		// invR * (dx, dy, 0)
		Pan[0] += RotMat[0] * dx + RotMat[1] * dy;
		Pan[1] += RotMat[4] * dx + RotMat[5] * dy;
		Pan[2] += RotMat[8] * dx + RotMat[9] * dy;
	}

	StartPt[0] = x;	// Update startpt as current position
	StartPt[1] = y;
	glutPostRedisplay();
}

/*!
*	\brief 마우스 스크롤을 처리하는 콜백 함수
*
*	\param button[in]	마우스 버튼 정보(GLUT_LEFT_BUTTON, GLUT_MIDDLE_BUTTON, GLUT_RIGHT_BUTTON)
*	\param dir[in]		스크롤의 방향
*	\param x[in]		좌측 상단을 (0, 0) 기준으로 마우스 포인터의 가로 위치
*	\param y[in]		좌측 상단을 (0, 0) 기준으로 마우스 포인터의 세로 위치
*/
void MouseWheel(int button, int dir, int x, int y)
{
	if (dir > 0)
		Zoom += 1.0;
	else
		Zoom -= 1.0;
	glutPostRedisplay();
}

void GetSphereCoord(int x, int y, float *px, float *py, float *pz)
{
	*px = (2.0f * x - Width) / Width;
	*py = (-2.0f * y + Height) / Height;

	float r = (*px) * (*px) + (*py) * (*py); // 원점으로부터의 거리 계산
	if (r >= 1.0f)
	{
		*px = *px / sqrt(r);
		*py = *py / sqrt(r);
		*pz = 0.0f;
	}
	else
		*pz = sqrt(1.0f - r);  // 일반적인 경우
}
