#ifdef _WIN32
#include <windows.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef __APPLE__
#include <GL/gl.h>
#include <GL/glut.h>
#else
#include <OpenGL/gl.h>
#include <GLUT/glut.h>
#endif

#include "GLMetaseq.h"
#include <time.h>

// テクスチャの定義
#define imageWidth 256
#define imageHeight 256
unsigned char tailTexImage[imageHeight][imageWidth][3];
unsigned char screenTexImage[imageHeight][imageWidth][3];
GLuint tailTexName;
GLuint screenTexName;

const float roomSize = 100.0f; //部屋のサイズ

// グローバル変数
int g_mouseX = 0;
int g_mouseY = 0;
int g_rotX = 0;
int g_rotY = 0;
float step_angle = 0.0; // 足の振り角度
float step_angle_change = 0.1;  // 初期の角度変化量
int walking = 0;        // 歩行中かどうかのフラグ
float g_fovy = 50.0;    // 視野角
float g_posX = 0.0;     // X座標の位置
float g_posY = -100.0;     // Y座標の位置
float g_posZ = 0;
float g_camera2_rotation = 0.0; // camera2 の回転角度

// パトランプの移動に関するグローバル変数
float redLightColor[] = { 1.0, 0.0, 0.0, 1.0 };         // 全強度の赤色光、他の色はなし
float redLightPosition[] = { 40.0, 0.0, -400.0, 1.0 };  // 初期位置、これは再計算されます
float g_patrol_light_posX = 40.0;                       // 初期位置
float g_patrol_light_posZ = -400.0;                     // 固定のZ座標
float g_patrol_light_move_speed = 0.5;                  // 移動速度
int g_patrol_light_moving = 0;                          // 移動中フラグ（0: 停止, 1: 移動中）
float g_patrol_light_direction = 1.0;                   // 移動方向（1: 右へ, -1: 左へ）
float g_patrol_light_range = 200.0;                     // 移動範囲（初期位置からの最大距離）

int g_camera2_rotation_target = 0; // 目標角度
int g_camera2_rotation_direction = 0; // 方向フラグ
int g_patrol_light_start_after_menu1 = 0; // メニュー1の動作が終わった後にパトランプを動かすフラグ



// MQOモデル
MQO_MODEL g_camera, g_camera2, g_head, g_right_hand, g_left_hand, g_right_leg, g_left_leg, g_patrol_light, g_light;

// プロトタイプ宣言
void Draw(void);
void myReshape(int w, int h);
void Keyboard(unsigned char key, int x, int y);
void SpecialKeys(int key, int x, int y);
void Mouse(int button, int state, int x, int y);
void Motion(int x, int y);
void Quit(void);
void Menu(int value);

// PPM画像の読み込み
void readPPMImage(char* filename, unsigned char image[imageHeight][imageWidth][3])
{
    FILE* fp;
    if (fopen_s(&fp, filename, "rb") != 0) {
        fprintf(stderr, "Cannot open ppm file %s\n", filename);
        exit(1);
    }

    char header[100];
    fgets(header, sizeof(header), fp);
    fgets(header, sizeof(header), fp);
    fgets(header, sizeof(header), fp);
    fread(image, 1, imageWidth * imageHeight * 3, fp);
    fclose(fp);
}

// テクスチャの設定
void setUpTexture(void)
{
    readPPMImage("floor.ppm", tailTexImage);
    readPPMImage("screen.ppm", screenTexImage);

    // 床のテクスチャ設定
    glGenTextures(1, &tailTexName);
    glBindTexture(GL_TEXTURE_2D, tailTexName);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); // 変更: GL_CLAMP から GL_REPEAT へ
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // 変更: GL_NEAREST から GL_LINEAR へ
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, imageWidth, imageHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, tailTexImage);

    // スクリーンのテクスチャ設定
    glGenTextures(1, &screenTexName);
    glBindTexture(GL_TEXTURE_2D, screenTexName);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, imageWidth, imageHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, screenTexImage);
}

void drawRoom() {
    glEnable(GL_TEXTURE_2D);

    // 床
    glBindTexture(GL_TEXTURE_2D, tailTexName);
    glColor3f(1.0, 1.0, 1.0);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0, 0.0); glVertex3f(-roomSize, -5.0, -roomSize);
    glTexCoord2f(1.0, 0.0); glVertex3f(roomSize, -5.0, -roomSize);
    glTexCoord2f(1.0, 1.0); glVertex3f(roomSize, -5.0, roomSize);
    glTexCoord2f(0.0, 1.0); glVertex3f(-roomSize, -5.0, roomSize);
    glEnd();

    // スクリーン
    glBindTexture(GL_TEXTURE_2D, screenTexName);
    glColor3f(1.0, 1.0, 1.0);
    float screenHeight = 45.0f;
    glBegin(GL_QUADS);
    glTexCoord2f(0.0, 0.0); glVertex3f(-roomSize, -5.0, -roomSize);
    glTexCoord2f(1.0, 0.0); glVertex3f(roomSize, -5.0, -roomSize);
    glTexCoord2f(1.0, 1.0); glVertex3f(roomSize, screenHeight, -roomSize);
    glTexCoord2f(0.0, 1.0); glVertex3f(-roomSize, screenHeight, -roomSize);
    glEnd();

    glEnd();

}

void DrawPatrolLight() {
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glTranslatef(g_patrol_light_posX, 0.0, g_patrol_light_posZ);
    glScalef(0.3f, 0.3f, 0.3f);
    mqoCallModel(g_patrol_light);

    redLightPosition[0] = g_patrol_light_posX;
    redLightPosition[1] = 1000.0;
    redLightPosition[2] = g_patrol_light_posZ;
    glLightfv(GL_LIGHT1, GL_POSITION, redLightPosition);

    glPopMatrix();
}



void Draw(void)
{
    float light_position[] = { 100.0, 500.0, 200.0, 1.0 };      // 光源の位置
    float white[] = { 1.0, 1.0, 1.0, 1.0 };                 // 光の色(RGB)
    GLfloat ambient[] = { 1.0, 1.0, 1.0, 1.0 };             // 環境光の色
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, white);                //拡散光
    glLightfv(GL_LIGHT0, GL_SPECULAR, white);               //鏡面光
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);        //環境光モデル
    glEnable(GL_LIGHT0);                                    // 光源を有効にする

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.f, 0.f, 0.2f, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // ルームを描画する部分でライティングを無効に
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    drawRoom();

    // オブジェクト描画前にライティングを有効化
    
    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);  

    glTranslatef(g_posX, g_posY, -400.0 + g_posZ);
    glRotatef(g_rotX, 1, 0, 0);
    glRotatef(g_rotY, 0, 1, 0);
    glEnable(GL_DEPTH_TEST);

    // ライティングの影響を受けるための設定
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, white);
    glMaterialfv(GL_FRONT, GL_SPECULAR, white);
    glMaterialf(GL_FRONT, GL_SHININESS, 100.0f);

    mqoCallModel(g_camera);

    glPushMatrix();
    glTranslatef(43.08, 202.4, 37.86);
    glRotatef(g_camera2_rotation, 0, 1, 0);
    glTranslatef(-43.08, -202.4, -37.86);
    mqoCallModel(g_camera2);
    glPopMatrix();

    mqoCallModel(g_head);

    if (walking) {
        step_angle += step_angle_change;
        if (step_angle >= 45 || step_angle <= -45) {
            step_angle_change = -step_angle_change;
        }
    }

    // 右手の描画
    glPushMatrix();
    glTranslatef(-65.0, 60.0, 20.0);
    glTranslatef(0, 10, 0);
    glRotatef(step_angle, 1, 0, 0);
    glTranslatef(0, -10, 0);
    mqoCallModel(g_right_hand);
    glPopMatrix();

    // 左足の描画
    glPushMatrix();
    glTranslatef(-20.0, -40.5, 15.0);
    glTranslatef(0, 20, 0);
    glRotatef(-step_angle, 1, 0, 0);
    glTranslatef(0, -20, 0);
    mqoCallModel(g_left_leg);
    glPopMatrix();

    // 左手の描画
    glPushMatrix();
    glTranslatef(62.0, 60.0, 20.0);
    glTranslatef(0, 10, 0);
    glRotatef(-step_angle, 1, 0, 0);
    glTranslatef(0, -10, 0);
    mqoCallModel(g_left_hand);
    glPopMatrix();

    // 右足の描画
    glPushMatrix();
    glTranslatef(20.0, -40.5, 12.0);
    glTranslatef(0, 20, 0);
    glRotatef(step_angle, 1, 0, 0);
    glTranslatef(0, -20, 0);
    mqoCallModel(g_right_leg);
    glPopMatrix();

    // パトランプの位置更新
    if (g_patrol_light_moving) {
        g_patrol_light_posX += g_patrol_light_move_speed * g_patrol_light_direction;
        if (g_patrol_light_posX > g_patrol_light_range || g_patrol_light_posX < -g_patrol_light_range) {
            g_patrol_light_direction *= -1;
        }
    }
    glEnable(GL_LIGHTING);
    DrawPatrolLight();

    // メニューでcamera2.mqoを動かす
    if (g_camera2_rotation_direction != 0) {
        g_camera2_rotation += 0.05 * g_camera2_rotation_direction;
        if ((g_camera2_rotation_direction == 1 && g_camera2_rotation >= g_camera2_rotation_target) ||
            (g_camera2_rotation_direction == -1 && g_camera2_rotation <= g_camera2_rotation_target)) {
            g_camera2_rotation = g_camera2_rotation_target;
            g_camera2_rotation_direction = 0;

            // メニュー1の動作が完了したらパトランプを動かす
            if (g_patrol_light_start_after_menu1) {
                g_patrol_light_moving = 1;
                g_patrol_light_start_after_menu1 = 0;
            }
        }
    }
    glutSwapBuffers();
}

void myReshape(int w, int h)
{
    double znear = 10;
    double zfar = 10000;

    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(g_fovy, (double)w / h, znear, zfar);
}

void Keyboard(unsigned char key, int x, int y)
{
    switch (key) {
    case 'w':
    case 'W':
        walking = !walking;
        if (walking) step_angle = 0;
        break;
    case 'r':
        if (g_camera2_rotation > -90.0) {
            g_camera2_rotation -= 10.0;
            if (g_camera2_rotation < -90.0) {
                g_camera2_rotation = -90.0;
            }
        }
        break;
    case 'R':
        if (g_camera2_rotation < 0.0) {
            g_camera2_rotation += 10.0;
            if (g_camera2_rotation > 0.0) {
                g_camera2_rotation = 0.0;
            }
        }
        break;
    case 'p':
        g_patrol_light_moving = !g_patrol_light_moving;
        break;
    case 'q':
    case 'Q':
    case 0x1b:
        exit(0);
    default:
        break;
    }
    glutPostRedisplay();
}

void SpecialKeys(int key, int x, int y)
{
    switch (key) {
    case GLUT_KEY_UP:
        if (g_posZ + 50.0 <= -100) {
            g_posZ += 10.0;
            printf("Moving forward, g_posZ: %f\n", g_posZ);
        }
        break;
    case GLUT_KEY_DOWN:
        if (g_posZ - 50.0 >= -1000) {
            g_posZ -= 10.0;
            printf("Moving backward, g_posZ: %f\n", g_posZ);
        }
        break;
    case GLUT_KEY_LEFT:
        g_posX -= 10.0;
        break;
    case GLUT_KEY_RIGHT:
        g_posX += 10.0;
        break;
    }
    glutPostRedisplay();
}

void Mouse(int button, int state, int x, int y)
{
    if (state == GLUT_DOWN) {
        g_mouseX = x;
        g_mouseY = y;
    }
}

void Motion(int x, int y)
{
    int xd;

    xd = x - g_mouseX;

    g_rotY += xd;

    g_mouseX = x;
    g_mouseY = y;
}


void Quit(void)
{
    mqoDeleteModel(g_camera);
    mqoDeleteModel(g_head);
    mqoDeleteModel(g_right_hand);
    mqoDeleteModel(g_left_hand);
    mqoDeleteModel(g_right_leg);
    mqoDeleteModel(g_left_leg);
    mqoCleanup();
}

void Menu(int value)
{
    switch (value)
    {
    case 1:
        g_camera2_rotation_target = -90;
        g_camera2_rotation_direction = -1;
        g_patrol_light_start_after_menu1 = 1;
        break;
    case 2:
        g_camera2_rotation_target = 0;
        g_camera2_rotation_direction = 1;
        g_patrol_light_moving = 0;
        break;
    case 3:
        exit(0);
        break;
    }
    glutPostRedisplay();
}

int main(int argc, char* argv[])
{
    srand(time(NULL));
    glutInit(&argc, argv);
    glutInitWindowPosition(100, 50);
    glutInitWindowSize(640, 480);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutCreateWindow("MQO Loader for OpenGL");

    mqoInit();
    g_camera = mqoCreateModel("camera1.mqo", 1.0);
    g_camera2 = mqoCreateModel("camera2.mqo", 1.0);
    g_head = mqoCreateModel("head.mqo", 1.0);
    g_right_hand = mqoCreateModel("righthand.mqo", 1.0);
    g_left_hand = mqoCreateModel("lefthand.mqo", 1.0);
    g_right_leg = mqoCreateModel("rightleg.mqo", 1.0);
    g_left_leg = mqoCreateModel("leftleg.mqo", 1.0);
    g_patrol_light = mqoCreateModel("patrol_light.mqo", 1.0);
    g_light = mqoCreateModel("g_light.mqo", 1.0);

    atexit(Quit);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);

    glutDisplayFunc(Draw);
    glutIdleFunc(Draw);
    glutReshapeFunc(myReshape);
    setUpTexture();
    glutKeyboardFunc(Keyboard);
    glutSpecialFunc(SpecialKeys);
    glutMouseFunc(Mouse);
    glutMotionFunc(Motion);

    // メニュー
    int submenu = glutCreateMenu(Menu);
    glutAddMenuEntry("Start videotaping", 1);
    glutAddMenuEntry("Finish videotaping", 2);

    glutCreateMenu(Menu);
    glutAddSubMenu("videocamera", submenu);
    glutAddMenuEntry("Finish program", 3);
    glutAttachMenu(GLUT_RIGHT_BUTTON);

    glutMainLoop();

    return 0;
}