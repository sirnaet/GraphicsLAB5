// Experiment 3: Colour cube and perspective viewing by moving the camera.
//
// The cube is NEVER transformed: it stays at the world origin. Only the camera
// changes. A floor grid and coloured world axes are drawn so you can see that
// the whole WORLD shifts relative to the camera, not that the cube moves.
//
// Camera model: eye position + yaw/pitch (viewing direction) + roll (up vector).
//   target = eye + forward      -> eye and target move together when you move
//
// Controls
//   LEFT / RIGHT arrows   move camera left / right   (along the camera's right vector)
//   UP / DOWN arrows      move camera up / down      (along world Y)
//   w / s                 move camera forward / backward (along the view direction)
//   j / l                 turn view left / right  (yaw)
//   i / k                 look up / down          (pitch)
//   q / e                 roll: tilts the UP vector
//   c                     re-aim the camera at the cube (without moving it)
//   f / F                 narrower / wider field of view (orthographic: zoom in / out)
//   n / N                 near plane closer / farther
//   m / M                 far plane closer / farther
//   p                     toggle perspective / orthographic
//   r                     reset everything
//   ESC                   quit
//
// Build (Linux):  g++ -Wall exp3_camera_cube.cpp -o exp3 -lGL -lGLU -lglut
// Build (macOS):  g++ -Wall exp3_camera_cube.cpp -o exp3 -framework OpenGL -framework GLUT
// Build (MinGW):  g++ -Wall exp3_camera_cube.cpp -o exp3 -lfreeglut -lopengl32 -lglu32

#ifdef __APPLE__
#include <GLUT/glut.h>      // FIX: macOS has no <GL/glut.h>
#else
#include <GL/glut.h>
#endif
#include <cmath>
#include <math.h>
#include <cstdio>
#include <cstdlib>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ------------------------------------------------------------ small vector helpers
struct Vec3 { double x, y, z; };
Vec3   operator+(Vec3 a, Vec3 b)   { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
Vec3   operator*(Vec3 a, double s) { return { a.x * s, a.y * s, a.z * s }; }
double dot(Vec3 a, Vec3 b)         { return a.x * b.x + a.y * b.y + a.z * b.z; }
Vec3   cross(Vec3 a, Vec3 b)
{
    return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
}
Vec3 normalize(Vec3 a)
{
    double len = sqrt(dot(a, a));
    return { a.x / len, a.y / len, a.z / len };
}
double deg2rad(double d) { return d * M_PI / 180.0; }

// ------------------------------------------------------------ cube data
const GLfloat V[8][3] = {
    {-1,-1,-1}, { 1,-1,-1}, { 1, 1,-1}, {-1, 1,-1},
    {-1,-1, 1}, { 1,-1, 1}, { 1, 1, 1}, {-1, 1, 1}
};
const int F[6][4] = {
    {4,5,6,7}, {1,0,3,2}, {0,4,7,3}, {5,1,2,6}, {3,7,6,2}, {0,1,5,4}
};
const GLfloat C[6][3] = {
    {1,0,0}, {0,1,0}, {0,0,1}, {1,1,0}, {1,0,1}, {0,1,1}
};

// ------------------------------------------------------------ camera state
Vec3   eye;                 // camera position in world coordinates
double yaw, pitch, roll;    // degrees: viewing direction (yaw, pitch) and up-vector tilt (roll)

// derived each frame
Vec3 fwd, upVec, rightVec;

// ------------------------------------------------------------ projection state
double fov, nearP, farP;    // perspective parameters
double orthoHalf;           // orthographic half-height
bool   perspective;
int    winW = 800, winH = 700;

void aimAtOrigin()
{
    // FIX 1: if the eye is (almost) at the origin there is no direction to aim along;
    // normalising a zero vector gave NaN and corrupted the whole camera.
    double len = sqrt(dot(eye, eye));
    if (len < 1e-6) return;

    Vec3 d = { -eye.x / len, -eye.y / len, -eye.z / len };
    pitch = asin(d.y) * 180.0 / M_PI;
    yaw   = atan2(d.x, -d.z) * 180.0 / M_PI;

    // FIX 2: updateBasis() relies on |pitch| <= 85 so fwd is never parallel to the
    // world up vector. With the eye straight above/below the cube, pitch became
    // +-90, cross(fwd, worldUp) was zero and normalize() produced NaN (black screen).
    if (pitch >  85) pitch =  85;
    if (pitch < -85) pitch = -85;
}

void resetCamera()
{
    eye = { 4, 3, 7 };
    roll = 0;
    fov = 45.0; nearP = 1.0; farP = 100.0;
    orthoHalf = 4.0;
    perspective = true;
    aimAtOrigin();
}

void updateBasis()
{
    // viewing direction from yaw/pitch (yaw = 0 looks down -Z)
    double y = deg2rad(yaw), p = deg2rad(pitch);
    fwd = { cos(p) * sin(y), sin(p), -cos(p) * cos(y) };

    // Pitch is limited to +-85 deg, so fwd is never parallel to world up and
    // this cross product never becomes zero.
    Vec3 worldUp = { 0, 1, 0 };
    Vec3 right0  = normalize(cross(fwd, worldUp));
    Vec3 up0     = cross(right0, fwd);

    // roll tilts the up vector around the viewing direction
    double r = deg2rad(roll);
    upVec    = normalize(up0 * cos(r) + right0 * sin(r));
    rightVec = cross(fwd, upVec);
}

// ------------------------------------------------------------ drawing
void drawCube()
{
    // FIX 3: the black outline is drawn at exactly the same depth as the coloured
    // faces, so depth testing made the edges flicker / break up (z-fighting).
    // Push the filled faces slightly back so the outlines always win.
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.0f, 1.0f);
    glBegin(GL_QUADS);
    for (int f = 0; f < 6; ++f) {
        glColor3fv(C[f]);
        for (int k = 0; k < 4; ++k) glVertex3fv(V[F[f][k]]);
    }
    glEnd();
    glDisable(GL_POLYGON_OFFSET_FILL);

    glColor3f(0, 0, 0);
    glLineWidth(2.0f);
    for (int f = 0; f < 6; ++f) {
        glBegin(GL_LINE_LOOP);
        for (int k = 0; k < 4; ++k) glVertex3fv(V[F[f][k]]);
        glEnd();
    }
}

void drawFloorAndAxes()
{
    // reference grid on the plane just below the cube: it makes camera motion visible
    glColor3f(0.7f, 0.7f, 0.7f);
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    for (int i = -20; i <= 20; ++i) {
        glVertex3f((float)i, -1.01f, -20); glVertex3f((float)i, -1.01f, 20);
        glVertex3f(-20, -1.01f, (float)i); glVertex3f(20, -1.01f, (float)i);
    }
    glEnd();
    // (coloured world axes removed)
}

void drawText(int x, int y, const char* s)
{
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    gluOrtho2D(0, winW, 0, winH);
    glMatrixMode(GL_MODELVIEW);  glPushMatrix(); glLoadIdentity();
    glDisable(GL_DEPTH_TEST);

    glRasterPos2i(x, y);
    for (; *s; ++s) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *s);

    glEnable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW);  glPopMatrix();
}

// Is the cube centre inside the view volume? Used to explain "where did it go?"
const char* visibilityMessage()
{
    Vec3 d = { -eye.x, -eye.y, -eye.z };          // eye -> cube centre
    double depth = dot(d, fwd);
    double aspect = (double)winW / winH;
    const double R = 1.8;                          // roughly the cube's radius

    if (depth + R < nearP)  return "Cube is behind the camera / closer than the near plane";
    if (depth - R > farP)   return "Cube is beyond the FAR plane (clipped). Press M, or c / r";
    if (perspective) {
        double halfH = depth * tan(deg2rad(fov) / 2.0);
        if (fabs(dot(d, upVec)) > halfH + R || fabs(dot(d, rightVec)) > halfH * aspect + R)
            return "Cube is outside the field of view. Press c to re-aim at it";
    } else {
        if (fabs(dot(d, upVec)) > orthoHalf + R || fabs(dot(d, rightVec)) > orthoHalf * aspect + R)
            return "Cube is outside the view box. Press c to re-aim at it";
    }
    return nullptr;
}

void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    updateBasis();

    // ---- projection ----
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    double aspect = (double)winW / winH;
    if (perspective) gluPerspective(fov, aspect, nearP, farP);
    else             glOrtho(-orthoHalf * aspect, orthoHalf * aspect,
                             -orthoHalf, orthoHalf, nearP, farP);

    // ---- view: look-at built from eye, target = eye + forward, and the up vector ----
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    Vec3 target = eye + fwd;
    gluLookAt(eye.x, eye.y, eye.z,
              target.x, target.y, target.z,
              upVec.x, upVec.y, upVec.z);

    // ---- model: nothing! The cube is drawn at the origin with no transform ----
    drawFloorAndAxes();
    drawCube();

    // ---- on-screen information ----
    char buf[200];
    glColor3f(0, 0, 0);
    snprintf(buf, sizeof buf, "Eye: (%.2f, %.2f, %.2f)   Target: (%.2f, %.2f, %.2f)",
             eye.x, eye.y, eye.z, target.x, target.y, target.z);
    drawText(10, winH - 20, buf);
    snprintf(buf, sizeof buf, "Up vector: (%.2f, %.2f, %.2f)   yaw %.0f  pitch %.0f  roll %.0f",
             upVec.x, upVec.y, upVec.z, yaw, pitch, roll);
    drawText(10, winH - 38, buf);
    if (perspective)
        snprintf(buf, sizeof buf, "PERSPECTIVE   FOV: %.0f deg   aspect: %.2f   near: %.1f   far: %.0f",
                 fov, aspect, nearP, farP);
    else
        snprintf(buf, sizeof buf, "ORTHOGRAPHIC   half-height: %.1f   near: %.1f   far: %.0f",
                 orthoHalf, nearP, farP);
    drawText(10, winH - 56, buf);
    drawText(10, 12, "The cube is fixed at (0,0,0); only the camera changes.  Press c to re-aim, r to reset.");

    const char* msg = visibilityMessage();
    if (msg) { glColor3f(0.8f, 0, 0); drawText(10, winH - 78, msg); }

    glutSwapBuffers();
}

void reshape(int w, int h)
{
    if (h == 0) h = 1;
    winW = w; winH = h;
    glViewport(0, 0, w, h);
}

// ------------------------------------------------------------ input
double clampValue(double v, double lim)
{
    if (v >  lim) return  lim;
    if (v < -lim) return -lim;
    return v;
}

void clampEye()
{
    const double L = 60.0;      // keep the camera from drifting away forever
    eye.x = clampValue(eye.x, L);
    eye.y = clampValue(eye.y, L);
    eye.z = clampValue(eye.z, L);
}

void special(int key, int, int)
{
    const double step = 0.4;
    updateBasis();
    switch (key) {
    case GLUT_KEY_LEFT:  eye = eye + rightVec * (-step); break;     // strafe left
    case GLUT_KEY_RIGHT: eye = eye + rightVec * ( step); break;     // strafe right
    case GLUT_KEY_UP:    eye = eye + Vec3{ 0, step, 0 };  break;    // up
    case GLUT_KEY_DOWN:  eye = eye + Vec3{ 0, -step, 0 }; break;    // down
    }
    clampEye();
    glutPostRedisplay();
}

void keyboard(unsigned char key, int, int)
{
    const double step = 0.4;
    updateBasis();
    switch (key) {
    case 'w': eye = eye + fwd * step;    break;                      // forward
    case 's': eye = eye + fwd * (-step); break;                      // backward
    case 'j': yaw   -= 3; break;
    case 'l': yaw   += 3; break;
    case 'i': pitch += 3; if (pitch >  85) pitch =  85; break;
    case 'k': pitch -= 3; if (pitch < -85) pitch = -85; break;
    case 'q': roll  -= 5; break;
    case 'e': roll  += 5; break;
    case 'c': aimAtOrigin(); break;
    case 'f':
        if (perspective) { fov -= 5; if (fov < 10) fov = 10; }
        else             { orthoHalf -= 0.5; if (orthoHalf < 1) orthoHalf = 1; }
        break;
    case 'F':
        if (perspective) { fov += 5; if (fov > 120) fov = 120; }
        else             { orthoHalf += 0.5; if (orthoHalf > 20) orthoHalf = 20; }
        break;
    case 'n': nearP -= 0.1; if (nearP < 0.1) nearP = 0.1; break;
    case 'N': nearP += 0.1; if (nearP > farP - 1) nearP = farP - 1; break;
    case 'm': farP  -= 5;   if (farP < nearP + 1) farP = nearP + 1; break;
    case 'M': farP  += 5;   if (farP > 300) farP = 300; break;
    case 'p': perspective = !perspective; break;
    case 'r': resetCamera(); break;
    case 27:  exit(0);
    }
    clampEye();
    glutPostRedisplay();
}

int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(winW, winH);
    glutCreateWindow("Exp 3: Camera and perspective viewing");
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.97f, 0.97f, 0.97f, 1);
    resetCamera();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(special);
    glutMainLoop();
    return 0;
}