/*
2015110758 류영석 2021-2 컴퓨터 그래픽스 과제 2
추가 기능: 모델 별 질감은 glMaterialfv만 써서 다르게 설정, 0 누르면 모든 회전 초기 위치로 복귀
*/

#include <stdlib.h>
#include <stdio.h>
#include <cmath>
#include "..\include\GL\freeglut.h"
#include <vector>
#include <iostream>

using namespace std;

#pragma region 신규 클래스, enum등

enum JointType
{
	NotJoint, Vertical, Horizontal, Slide
};

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

/// <summary>
/// 각 블록을 나타내는 클래스
/// </summary>
class Block {
public:
	vector<AttachData>* attached = new vector<AttachData>();
	Vector3* size = new Vector3();
	Vector3* position = new Vector3();
	Vector3* rotation = new Vector3();
	float rotateAngle;
	/// <summary>
	/// 1~6까지 존재
	/// </summary>
	int textureType;
	/// <summary>
	/// 1: 큐브 2: 스피어
	/// </summary>
	int shapeType;
};

/// <summary>
/// 블록의 관절 정보
/// </summary>
class AttachData {
public:
	/// <summary>
	/// slider 관절의 이동 범위 제한. x: 최댓값 y: 최솟값 z: Reserved
	/// </summary>
	Vector3* jointLimits = new Vector3();
	Block* block;
	JointType jointType;

	AttachData(Vector3* pos, Block* blk, JointType jt) {
		jointLimits = pos;
		block = blk;
		jointType = jt;
	}
};
#pragma endregion

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
float Zoom = -50;
float Pan[3] = { 0.0, -3.0, 0.0 };

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

#pragma region 추가 구현 선언

//추가 구현 함수들

void InitializeBlocks();
void GiveMaterial(Block* blk);
void DrawBlocksRecursive(Block* block);
void DrawBlocksRecursive(Block* block, Block* ancestor);

//추가 구현 변수들

vector<Block*> blocks;
vector<AttachData*> joints;

#pragma endregion

int main(int argc, char **argv)
{
	// GLUT 초기화(더블 칼라버퍼, RBGA, 깊이버퍼 사용)
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);

	// 윈도우 생성
	glutInitWindowSize(Width, Height);
	glutCreateWindow("HW2_2015110758");

	cout << "1 : 바닥 시계 회전 2: 바닥 반시계 회전 3: 팔 시계 회전 4: 팔 반시계 회전 5: 집게 좁히기 6: 집게 벌리기 0 : 초기화" << endl;

	/// 블록 목록 초기화
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

#pragma region 추가 구현 함수 본문


/// <summary>
/// 로봇팔 모양으로 블록 초기화. 별도 데이터 파일 읽는 구조로도 변형 가능 + 세로로 쌓는 모형이라면 범용 사용 가능
/// </summary>
void InitializeBlocks()
{
	Block* base = new Block();
	base->position = new Vector3(0, 0.5, 0);
	base->size = new Vector3(10, 1, 10);
	base->shapeType = 1;
	base->textureType = 1;
	base->rotation = new Vector3(0, 1, 0);
	blocks.push_back(base);

	AttachData* data0 = new AttachData(new Vector3(), base, Horizontal);

	Block* lower = new Block();
	lower->position = new Vector3(0, 3, 0);
	lower->size = new Vector3(1, 5, 1);
	lower->shapeType = 1;
	lower->textureType = 2;
	lower->rotation = new Vector3(0, 1, 0);

	AttachData* data1 = new AttachData(base->position, lower, NotJoint);

	Block* center = new Block();
	center->shapeType = 2;
	center->position = new Vector3(0, 0.5, 0);
	center->size = new Vector3(1, 1, 1);
	center->textureType = 3;
	center->rotation = new Vector3(0, 0, 1);

	AttachData* data2 = new AttachData(lower->position, center, Vertical);

	Block* upper = new Block();
	upper->position = new Vector3(0, 3, 0);
	upper->size = new Vector3(1, 5, 1);
	upper->shapeType = 1;
	upper->textureType = 4;
	AttachData* data3 = new AttachData(center->position, upper, NotJoint);

	Block* clawL = new Block();
	clawL->position = new Vector3(-0.5, 0.7, 0);
	clawL->size = new Vector3(0.1, 2, 1.0);
	clawL->shapeType = 1;
	clawL->textureType = 5;

	AttachData* data4A = new AttachData(upper->position, clawL, Slide);
	data4A->jointLimits = new Vector3(-0.5, -0.05, 0);

	Block* clawR = new Block();
	clawR->position = new Vector3(0.5, 0.7, 0);
	clawR->size = new Vector3(0.1, 2, 1.0);
	clawR->shapeType = 1;
	clawR->textureType = 6;

	AttachData* data4B = new AttachData(upper->position, clawR, Slide);
	data4B->jointLimits = new Vector3(0.5, 0.05, 0);

	upper->attached->push_back(*data4A); upper->attached->push_back(*data4B);
	center->attached->push_back(*data3);
	lower->attached->push_back(*data2);
	base->attached->push_back(*data1);

	blocks.push_back(base);

	joints.push_back(data4A);joints.push_back(data4B); joints.push_back(data0); joints.push_back(data2);
}

/// <summary>
/// 정해진 색상별로 GL_DIFFUSE 부여
/// </summary>
/// <param name="blk"></param>
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
			float mat0_diffuse[] = { 0.1, 0.1, 0.1 };
			glMaterialfv(GL_FRONT, GL_DIFFUSE, mat0_diffuse);
			break;
		}
		default: {
			float mat0_diffuse[] = { 0, 0, 0 };
			glMaterialfv(GL_FRONT, GL_DIFFUSE, mat0_diffuse);
			break;
		}
	}
}

/// <summary>
/// 재귀적으로 모델링 생성하여 상위 Matrix의 자식으로 넣음. 최초 호출용
/// </summary>
/// <param name="block"></param>
void DrawBlocksRecursive(Block* block) {
	glPushMatrix();
	{
		GiveMaterial(block);
		glTranslatef(block->position->x, block->position->y, block->position->z);
		glScalef(block->size->x, block->size->y, block->size->z);
		if (block->shapeType == 1)
			glutSolidCube(1.0);
		else
			glutSolidSphere(1.0, 50, 50);
		glRotatef(block->rotateAngle, block->rotation->x, block->rotation->y, block->rotation->z);
		vector<AttachData> atch = *(block->attached);
		for (auto a : atch)
			DrawBlocksRecursive(a.block, block);
	}
	glPopMatrix();
}

/// <summary>
/// 재귀적으로 모델링 생성하여 상위 Matrix의 자식으로 넣음. 
/// </summary>
/// <param name="block"></param>
/// <param name="ancestor"></param>
void DrawBlocksRecursive(Block* block, Block* ancestor) {
	glPushMatrix();
	{
		GiveMaterial(block);
		GLfloat nx = block->position->x;
		GLfloat ny = block->position->y;
		GLfloat nz = block->position->z;
		glTranslatef(nx, ny, nz);
		glScalef(block->size->x / ancestor->size->x, block->size->y / ancestor->size->y, block->size->z / ancestor->size->z);
		if (block->shapeType == 1)
			glutSolidCube(1.0);
		else
			glutSolidSphere(1.0, 50, 50);
		glRotatef(block->rotateAngle, block->rotation->x, block->rotation->y, block->rotation->z);
		vector<AttachData> atch = *(block->attached);
		for (auto a : atch)
			DrawBlocksRecursive(a.block, block);
	}
	glPopMatrix();
}
#pragma endregion


void RenderRobot()
{
	for (auto blk : blocks) {
		DrawBlocksRecursive(blk);
	}
}

void InitOpenGL()
{
	// 깊이 테스트를 활성화
	glEnable(GL_DEPTH_TEST);

	// 조명 모델 활성화
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);	
	float mat0_ambient[] = { 0.3, 0.5, 0.3 };
	float mat0_shininess = 40;
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, mat0_ambient);
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

/// <summary>
/// 123456 + 0 키보드 입력 받아서 처리
/// </summary>
/// <param name="key"></param>
/// <param name="x"></param>
/// <param name="y"></param>
void Keyboard(unsigned char key, int x, int y)
{
	vector<AttachData*> targets;
	float value = 0;
	switch (key)
	{
		case '1':
		case '2':
			for (auto j : joints)
				if (j->jointType == Horizontal)
					targets.push_back(j);
			value = key == '1' ? -1 : 1;
			break;
		case '3':
		case '4':
			for (auto j : joints)
				if (j->jointType == Vertical)
					targets.push_back(j);
			value = key == '3' ? -1 : 1;
			break;
		case '5':
		case '6':
			for (auto j : joints)
				if (j->jointType == Slide)
					targets.push_back(j);
			value = key == '5' ? -0.05 : 0.05;
			break;
		case '0':
			for (auto j : joints) {
				if (j->jointType == Vertical || j->jointType == Horizontal)
					j->block->rotateAngle = 0;
				else if (j->jointType == Slide)
					j->block->position->x = j->jointLimits->x;
			}
		default:
			break;
	}

	if (!targets.empty()) {
		for (auto j : targets) {
			if (j->jointType == Vertical || j->jointType == Horizontal)
				j->block->rotateAngle += value;
			else if (j->jointType == Slide) {
				auto curPos = j->block->position->x;
				auto maxPos = j->jointLimits->x;
				auto minPos = j->jointLimits->y;
				float inverse = maxPos>0 ? 1 : -1;

				//좁히기
				if (value < 0) {
					if (abs(curPos - minPos) > 0.05) 
						j->block->position->x -= value * -inverse;
					else 
						j->block->position->x = minPos;
				}
				else {
					if (abs(curPos - maxPos) > 0.05)
						j->block->position->x += value * inverse;
					else 
						j->block->position->x = maxPos;
				}
			}
		}
	}
	glutPostRedisplay();
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
