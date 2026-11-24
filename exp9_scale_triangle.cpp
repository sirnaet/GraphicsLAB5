// Experiment 9: Create a triangle and implement scaling WITHOUT built-in scaling functions.
//
// Manual requirements covered:
//   - triangle from three explicit vertices, original stored separately and never changed
//   - X-axis scaling (sx,1), Y-axis scaling (1,sy) and uniform XY scaling (s,s)
//   - factors > 1 zoom in, factors between 0 and 1 zoom out
//   - scaling about the origin AND about a chosen reference point (fixed point / centroid)
//   - keyboard controls to increase/decrease factors and to reset
//   - original (blue) and scaled (red) triangles shown together
//   - factors validated: they must be positive (console input and while running)
//   - NO glScalef / glScaled anywhere: every new vertex is computed by hand.
//     (gluOrtho2D only defines the visible window; it does not scale the triangle.)
//
// Maths
//   About the origin:       x' = sx * x          y' = sy * y
//   About a point (a,b):    translate (a,b) to the origin, scale, translate back
//                           x' = a + sx * (x - a)    y' = b + sy * (y - b)
//   Centroid of a triangle: ((x1+x2+x3)/3, (y1+y2+y3)/3)
//
// Controls (graphics window)
//   1 / 2 / 3     mode: X-axis only / Y-axis only / uniform XY
//   + / -         increase / decrease the factor of the current mode (step 0.1)
//   o             cycle reference point: origin -> fixed point -> centroid
//   left click    set the fixed point (and switch to it)
//   r             reset factors to 1, mode to uniform, reference to origin
//   ESC           quit
//
// Build (Linux):  g++ -Wall exp9_scale_triangle.cpp -o exp9 -lGL -lGLU -lglut
// Build (macOS):  g++ -Wall exp9_scale_triangle.cpp -o exp9 -framework OpenGL -framework GLUT
// Build (MinGW):  g++ -Wall exp9_scale_triangle.cpp -o exp9 -lfreeglut -lopengl32 -lglu32

#include <GL/glut.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>

struct Point { double x, y; };

const double WORLD = 12.0;                       // world window [-12,12] x [-12,12]
const double MIN_FACTOR = 0.1, MAX_FACTOR = 5.0;

const Point original[3] = { {1, 1}, {5, 1}, {3, 4} };   // stored separately, never modified

enum Mode { MODE_X, MODE_Y, MODE_XY };
enum Ref  { REF_ORIGIN, REF_FIXED, REF_CENTROID };

Mode   mode = MODE_XY;
Ref    refMode = REF_ORIGIN;
double sx = 1.5, sy = 1.5, su = 1.5;             // factor for X-mode, Y-mode, uniform mode
Point  fixedPt = { 2, 2 };
int    winW = 700, winH = 700;

// ---------------------------------------------------------------- scaling by hand
// Scale p about the reference point c with factors (fx, fy):
//   1) translate so that c is the origin   : (p.x - c.x, p.y - c.y)
//   2) scale                                : multiply by fx and fy
//   3) translate back                       : add c
// For c = (0,0) this is exactly x' = fx*x, y' = fy*y.
Point scaleAbout(Point p, Point c, double fx, double fy)
{
    double tx = p.x - c.x;
    double ty = p.y - c.y;
    tx = fx * tx;
    ty = fy * ty;
    return { tx + c.x, ty + c.y };
}

Point centroid()
{
    return { (original[0].x + original[1].x + original[2].x) / 3.0,
             (original[0].y + original[1].y + original[2].y) / 3.0 };
}

Point referencePoint()
{
    switch (refMode) {
    case REF_FIXED:    return fixedPt;
    case REF_CENTROID: return centroid();
    default:           return { 0, 0 };
    }
}

// effective factors for the current mode: (sx,1), (1,sy) or (s,s)
void effectiveFactors(double& fx, double& fy)
{
    switch (mode) {
    case MODE_X:  fx = sx; fy = 1.0; break;
    case MODE_Y:  fx = 1.0; fy = sy; break;
    default:      fx = su; fy = su;  break;
    }
}

// ---------------------------------------------------------------- input validation
double askFactor(const char* prompt, double def)
{
    double v;
    printf("%s [default %.2f, allowed %.1f to %.1f]: ", prompt, def, MIN_FACTOR, MAX_FACTOR);
    fflush(stdout);
    int ok = scanf("%lf", &v);
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {}      // discard the rest of the line
    if (ok != 1) {
        printf("  Not a number. Using default %.2f\n", def);
        return def;
    }
    if (v <= 0.0) {
        printf("  Scale factors must be positive. Using default %.2f\n", def);
        return def;
    }
    if (v < MIN_FACTOR || v > MAX_FACTOR) {
        printf("  Out of range. Using default %.2f\n", def);
        return def;
    }
    return v;
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
    glVertex2d(-WORLD, 0); glVertex2d(WORLD, 0);
    glVertex2d(0, -WORLD); glVertex2d(0, WORLD);
    for (int i = -(int)WORLD; i <= (int)WORLD; ++i) {
        glVertex2d(i, -0.15); glVertex2d(i, 0.15);
        glVertex2d(-0.15, i); glVertex2d(0.15, i);
    }
    glEnd();
    glColor3f(0.4f, 0.4f, 0.4f);
    drawText(WORLD - 0.8, 0.4, "x");
    drawText(0.3, WORLD - 0.8, "y");
}

void display()
{
    glClear(GL_COLOR_BUFFER_BIT);
    drawAxes();

    double fx, fy;
    effectiveFactors(fx, fy);
    Point ref = referencePoint();

    Point scaled[3];
    for (int i = 0; i < 3; ++i) scaled[i] = scaleAbout(original[i], ref, fx, fy);

    glLineWidth(2.0f);
    glColor3f(0.2f, 0.4f, 1.0f);  drawTriangle(original);   // original: blue
    glColor3f(0.9f, 0.15f, 0.15f); drawTriangle(scaled);    // scaled:   red

    // reference point marker
    glColor3f(0, 0, 0);
    glPointSize(8.0f);
    glBegin(GL_POINTS);
    glVertex2d(ref.x, ref.y);
    glEnd();
    drawText(ref.x + 0.2, ref.y + 0.3, "ref");

    // text
    char buf[200];
    const char* modeName = (mode == MODE_X) ? "X-axis only (sx,1)"
                         : (mode == MODE_Y) ? "Y-axis only (1,sy)" : "Uniform XY (s,s)";
    const char* refName  = (refMode == REF_ORIGIN) ? "origin"
                         : (refMode == REF_FIXED)  ? "fixed point" : "centroid";
    double ty = WORLD - 1.0;
    glColor3f(0.2f, 0.4f, 1.0f);  drawText(-WORLD + 0.5, ty, "Blue: original triangle");        ty -= 0.8;
    glColor3f(0.9f, 0.15f, 0.15f); drawText(-WORLD + 0.5, ty, "Red: scaled triangle");          ty -= 0.8;
    glColor3f(0, 0, 0);
    snprintf(buf, sizeof buf, "Mode: %s    factors used: sx = %.2f, sy = %.2f", modeName, fx, fy);
    drawText(-WORLD + 0.5, ty, buf);                                                             ty -= 0.8;
    snprintf(buf, sizeof buf, "Reference: %s (%.2f, %.2f)   %s", refName, ref.x, ref.y,
             (fx > 1.0 || fy > 1.0) ? "zoom in" : ((fx < 1.0 || fy < 1.0) ? "zoom out" : ""));
    drawText(-WORLD + 0.5, ty, buf);                                                             ty -= 0.8;
    for (int i = 0; i < 3; ++i) {
        snprintf(buf, sizeof buf, "V%d (%.2f, %.2f)  ->  (%.2f, %.2f)", i + 1,
                 original[i].x, original[i].y, scaled[i].x, scaled[i].y);
        drawText(-WORLD + 0.5, ty, buf);
        ty -= 0.8;
    }

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

// ---------------------------------------------------------------- interaction
double clampFactor(double v)
{
    // factors must stay positive (no reflection in this experiment)
    if (v < MIN_FACTOR) return MIN_FACTOR;
    if (v > MAX_FACTOR) return MAX_FACTOR;
    return v;
}

void keyboard(unsigned char key, int, int)
{
    switch (key) {
    case '1': mode = MODE_X;  break;
    case '2': mode = MODE_Y;  break;
    case '3': mode = MODE_XY; break;
    case '+': case '=':
        if (mode == MODE_X)      sx = clampFactor(sx + 0.1);
        else if (mode == MODE_Y) sy = clampFactor(sy + 0.1);
        else                     su = clampFactor(su + 0.1);
        break;
    case '-':
        if (mode == MODE_X)      sx = clampFactor(sx - 0.1);
        else if (mode == MODE_Y) sy = clampFactor(sy - 0.1);
        else                     su = clampFactor(su - 0.1);
        break;
    case 'o': refMode = (Ref)((refMode + 1) % 3); break;
    case 'r': sx = sy = su = 1.0; mode = MODE_XY; refMode = REF_ORIGIN; break;
    case 27:  exit(0);
    }
    glutPostRedisplay();
}

void mouse(int button, int state, int mx, int my)
{
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        fixedPt.x = -WORLD + 2 * WORLD * mx / winW;      // pixels -> world coordinates
        fixedPt.y =  WORLD - 2 * WORLD * my / winH;
        refMode = REF_FIXED;
        glutPostRedisplay();
    }
}

int main(int argc, char** argv)
{
    printf("=== Experiment 9: triangle scaling (no built-in scaling) ===\n");
    sx = askFactor("Initial X-axis factor sx", 1.5);
    sy = askFactor("Initial Y-axis factor sy", 1.5);
    su = askFactor("Initial uniform factor s ", 1.5);
    printf("\nWindow keys: 1/2/3 mode, +/- factor, o reference point, click = fixed point, r reset\n");

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(winW, winH);
    glutCreateWindow("Exp 9: Triangle scaling");
    glClearColor(1, 1, 1, 1);
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouse);
    glutMainLoop();
    return 0;
}