// Experiment 1: Create and rotate a triangle about the origin and a fixed point.
//
// Manual requirements covered:
//   - triangle from three non-collinear vertices (defaults or typed in, validated)
//   - rotation angle and direction (clockwise / anticlockwise) accepted at start-up
//     and changeable while running
//   - rotation about the origin using the 2x2 rotation matrix (done by hand)
//   - rotation about a fixed pivot (a,b): T(a,b) R(theta) T(-a,-b)
//   - axes, pivot, angle and a legend are displayed
//   - the ORIGINAL vertices are never overwritten, so transforms are not compounded
//
// Controls (in the graphics window)
//   + / -        angle +5 / -5 degrees
//   3 / 9 / 8    angle = 30 / 90 / 180 degrees (the manual's test cases)
//   d            toggle clockwise / anticlockwise
//   left click   set pivot (a,b)
//   r            reset to start-up values
//   ESC          quit
//
// Build (Linux):  g++ -Wall exp1_rotate_triangle.cpp -o exp1 -lGL -lGLU -lglut
// Build (macOS):  g++ -Wall exp1_rotate_triangle.cpp -o exp1 -framework OpenGL -framework GLUT
// Build (MinGW):  g++ -Wall exp1_rotate_triangle.cpp -o exp1 -lfreeglut -lopengl32 -lglu32

#include <GL/glut.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

struct Point { double x, y; };

const double WORLD = 10.0;                          // world window: [-10,10] x [-10,10]
const Point DEFAULT_TRI[3] = { {2, 1}, {6, 1}, {4, 5} };

Point original[3] = { {2, 1}, {6, 1}, {4, 5} };     // never modified after input
double angleDeg = 30.0;
bool   clockwise = false;
Point  pivot = { 3, 2 };

// start-up values (used by reset)
double startAngle = 30.0;
bool   startCw = false;
Point  startPivot = { 3, 2 };

int winW = 700, winH = 700;

// ---------------------------------------------------------------- maths
// Rotate p about centre c by deg degrees.
//   1) translate c to the origin      (subtract c)
//   2) rotate with the rotation matrix x' = x cos - y sin ; y' = x sin + y cos
//   3) translate back                 (add c)
// With c = (0,0) this is plain rotation about the origin.
Point rotateAbout(Point p, Point c, double deg, bool cw)
{
    double t = deg * M_PI / 180.0;       // convert degrees to radians first
    if (cw) t = -t;                      // clockwise = negative angle
    double dx = p.x - c.x;
    double dy = p.y - c.y;
    double rx = dx * cos(t) - dy * sin(t);
    double ry = dx * sin(t) + dy * cos(t);
    return { rx + c.x, ry + c.y };
}

// ---------------------------------------------------------------- input
static void flushLine()
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {}
}

double askDouble(const char* prompt, double def)
{
    double v;
    printf("%s [default %.2f]: ", prompt, def);
    fflush(stdout);
    if (scanf("%lf", &v) == 1) { flushLine(); return v; }
    flushLine();
    printf("  Invalid input, using default %.2f\n", def);
    return def;
}

bool askYesNo(const char* prompt)
{
    char c = 'n';
    printf("%s (y/n): ", prompt);
    fflush(stdout);
    if (scanf(" %c", &c) != 1) c = 'n';
    flushLine();
    return c == 'y' || c == 'Y';
}

bool isCollinear(const Point v[3])
{
    double area2 = (v[1].x - v[0].x) * (v[2].y - v[0].y)
                 - (v[2].x - v[0].x) * (v[1].y - v[0].y);
    return fabs(area2) < 1e-9;
}

void readUserInput()
{
    printf("=== Experiment 1: triangle rotation ===\n");
    printf("Default triangle: (2,1) (6,1) (4,5)\n");

    if (askYesNo("Enter your own triangle vertices?")) {
        Point v[3];
        for (int i = 0; i < 3; ++i) {
            char label[32];
            snprintf(label, sizeof label, "  Vertex %d x", i + 1);
            v[i].x = askDouble(label, DEFAULT_TRI[i].x);
            snprintf(label, sizeof label, "  Vertex %d y", i + 1);
            v[i].y = askDouble(label, DEFAULT_TRI[i].y);
        }
        bool outside = false;
        for (int i = 0; i < 3; ++i)
            if (fabs(v[i].x) > WORLD || fabs(v[i].y) > WORLD) outside = true;

        if (isCollinear(v))
            printf("Those points are collinear (no triangle). Using the default triangle.\n");
        else if (outside)
            printf("A vertex lies outside [-10,10]. Using the default triangle.\n");
        else
            for (int i = 0; i < 3; ++i) original[i] = v[i];
    }

    angleDeg = askDouble("Rotation angle in degrees", 30.0);
    if (angleDeg < 0) { angleDeg = -angleDeg; clockwise = true; }   // negative = clockwise
    if (!clockwise) clockwise = askYesNo("Rotate clockwise?");
    pivot.x = askDouble("Pivot a", 3.0);
    pivot.y = askDouble("Pivot b", 2.0);

    startAngle = angleDeg; startCw = clockwise; startPivot = pivot;
    printf("\nWindow controls: +/- angle, 3/9/8 = 30/90/180 deg, d = direction, "
           "click = pivot, r = reset, ESC = quit\n");
}

// ---------------------------------------------------------------- drawing
void drawText(double x, double y, const char* s)
{
    glRasterPos2d(x, y);
    for (; *s; ++s) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *s);
}

void drawTriangle(const Point v[3])
{
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < 3; ++i) glVertex2d(v[i].x, v[i].y);
    glEnd();
}

void drawAxes()
{
    glColor3f(0.6f, 0.6f, 0.6f);
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    glVertex2d(-WORLD, 0); glVertex2d(WORLD, 0);       // x axis
    glVertex2d(0, -WORLD); glVertex2d(0, WORLD);       // y axis
    // tick marks every unit
    for (int i = -(int)WORLD; i <= (int)WORLD; ++i) {
        glVertex2d(i, -0.15); glVertex2d(i, 0.15);
        glVertex2d(-0.15, i); glVertex2d(0.15, i);
    }
    glEnd();
    glColor3f(0.4f, 0.4f, 0.4f);
    drawText(WORLD - 0.7, 0.4, "x");
    drawText(0.3, WORLD - 0.7, "y");
    drawText(0.2, -0.8, "O");
}

void display()
{
    glClear(GL_COLOR_BUFFER_BIT);
    drawAxes();

    // Compute transformed vertices from the ORIGINAL ones every frame
    Point rOrigin[3], rPivot[3];
    for (int i = 0; i < 3; ++i) {
        rOrigin[i] = rotateAbout(original[i], { 0, 0 }, angleDeg, clockwise);
        rPivot[i]  = rotateAbout(original[i], pivot,    angleDeg, clockwise);
    }

    glLineWidth(2.0f);

    glColor3f(0.2f, 0.4f, 1.0f);                       // original: blue, solid
    drawTriangle(original);

    glColor3f(1.0f, 0.2f, 0.2f);                       // about origin: red, solid
    drawTriangle(rOrigin);

    glEnable(GL_LINE_STIPPLE);                         // about pivot: green, dashed
    glLineStipple(2, 0x00FF);
    glColor3f(0.0f, 0.65f, 0.2f);
    drawTriangle(rPivot);
    glDisable(GL_LINE_STIPPLE);

    glColor3f(0, 0, 0);                                // pivot marker
    glPointSize(8.0f);
    glBegin(GL_POINTS);
    glVertex2d(pivot.x, pivot.y);
    glEnd();
    drawText(pivot.x + 0.2, pivot.y + 0.3, "pivot");

    // Legend and parameters
    char buf[160];
    glColor3f(0.2f, 0.4f, 1.0f);  drawText(-9.6, 9.2, "Blue (solid): original triangle");
    glColor3f(1.0f, 0.2f, 0.2f);  drawText(-9.6, 8.5, "Red (solid): rotated about origin");
    glColor3f(0.0f, 0.65f, 0.2f); drawText(-9.6, 7.8, "Green (dashed): rotated about pivot");
    glColor3f(0, 0, 0);
    snprintf(buf, sizeof buf, "Angle: %.1f deg, %s    Pivot: (%.2f, %.2f)",
             angleDeg, clockwise ? "clockwise" : "anticlockwise", pivot.x, pivot.y);
    drawText(-9.6, 7.1, buf);

    glutSwapBuffers();
}

void reshape(int w, int h)
{
    if (h == 0) h = 1;
    winW = w; winH = h;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(-WORLD, WORLD, -WORLD, WORLD);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void keyboard(unsigned char key, int, int)
{
    switch (key) {
    case '+': case '=': angleDeg += 5; break;
    case '-': angleDeg -= 5; if (angleDeg < 0) angleDeg += 360; break;
    case '3': angleDeg = 30;  break;
    case '9': angleDeg = 90;  break;
    case '8': angleDeg = 180; break;
    case 'd': clockwise = !clockwise; break;
    case 'r': angleDeg = startAngle; clockwise = startCw; pivot = startPivot; break;
    case 27:  exit(0);
    }
    glutPostRedisplay();
}

void mouse(int button, int state, int mx, int my)
{
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        pivot.x = -WORLD + 2 * WORLD * mx / winW;      // pixels -> world (y is flipped)
        pivot.y =  WORLD - 2 * WORLD * my / winH;
        glutPostRedisplay();
    }
}

int main(int argc, char** argv)
{
    readUserInput();                                   // console input before the window opens
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(winW, winH);
    glutCreateWindow("Exp 1: Triangle rotation");
    glClearColor(1, 1, 1, 1);
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouse);
    glutMainLoop();
    return 0;
}