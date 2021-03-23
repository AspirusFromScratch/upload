#include "tigr/tigr.h"
#include <cmath>
#include <iostream>

using namespace std;

Tigr *screen = tigrWindow(320, 200, "C++ 3d Rendering", 0);

int frame_count;

int z_buffer[64000];

const float PI = 3.14;

Tigr *atlas = tigrLoadImage("atlas.png");

float divide(float a, float b)
{
	if (b == 0)
	{
		cout << "Tried to divide " << a << " by " << b << "\n";
		return 0;
	}
	else
	{
		return (a/b);
	}
}

class Camera
{
public:
	int x = 0;
	int y = 0;
	int z = 0;
	int xrot = 0;
	int yrot = 0;
	int  fov = 200;
	int znear = 10;
	float xsin;
	float xcos;
	float ysin;
	float ycos;
	void calcTrigValues()
	{
		xsin = sin(((float)xrot/180)*PI);
		xcos = cos(((float)xrot/180)*PI);
		ysin = sin(((float)yrot/180)*PI);
		ycos = cos(((float)yrot/180)*PI);
	}
};

Camera camera;

namespace render
{
// Returns the smallers number.
int min3x(int a, int b, int c)
{
	if      (a <= b && a <= c) {return a;}
	else if (b <= c)           {return b;}
	else                       {return c;}
}
// Returns the largest number.
int max3x(int a, int b, int c)
{
	if      (a >= b && a >= c) {return a;}
	else if (b >= c)           {return b;}
	else                       {return c;}
}
struct Vertex2d
{
	int x, y;
};
struct Vertex3d
{
	int x, y, z;
};
struct Texture
{
	Vertex2d t;
	Vertex2d ts;
	Vertex2d size;
	bool invert = false;
	Vertex2d getTexture(float u, float v)
	{
		Vertex2d tx;
		u = u * size.x;
		v = v * size.y;
		if (invert)
		{
			u = size.x - u;
			v = size.y - v;	
		}
		u = (int)u % ts.x + t.x;
		v = (int)v % ts.y + t.y;
		tx.x = u;
		tx.y = v;
		return tx;
	}
};
// Returns true if the Point C is to the Right Side of or on the line passing from Point A to Point B.
bool isToRightSide(Vertex3d a, Vertex3d b, Vertex2d c){
	return ((b.x - a.x)*(c.y - a.y) - (b.y - a.y)*(c.x - a.x)) >= 0;
}
// Plot a single pixel at (x, y) with the given color. (0, 0) is translated to the middle of the screen.
void plot(int x, int y, TPixel color)
{
	x =  x + 160;
	y = -y + 100;
	tigrPlot(screen, x, y, color);
}
// Plot a single pixel at (x, y) with the color from the atlas bitmap at (u, v). (0, 0) is translated to the middle of the screen.
void plotFromAtlas(int x, int y, int u, int v)
{
	x =  x + 160;
	y = -y + 100;
	tigrBlit(screen, atlas, x, y, u, v, 1, 1);
}
// Flush the Z-Buffer.
void flush()
{
	for (int i = 0; i < 320*200; i++)
	{
		z_buffer[i] = 100000;
	}
}
// Rasterize and Render a 2d Triangle with Affine Texture Mapping.
void render2dTriangle(Vertex3d v1, Vertex3d v2, Vertex3d v3, Texture texture, int z)
{
	float u, v;
	int zindex;
	Vertex2d uv;
	// Find out the bounding box made by the triangle.
	Vertex2d b1 {min3x(v1.x, v2.x, v3.x), min3x(v1.y, v2.y, v3.y)};
	Vertex2d b2 {max3x(v1.x, v2.x, v3.x), max3x(v1.y, v2.y, v3.y)};
	if (b1.x < -160) {b1.x = -160;}
	if (b1.x >  160) {b1.x =  160;}
	if (b1.y < -100) {b1.y = -100;}
	if (b1.y >  100) {b1.y =  100;}
	if (b2.x < -160) {b2.x = -160;}
	if (b2.x >  160) {b2.x =  160;}
	if (b2.y < -100) {b2.y = -100;}
	if (b2.y >  100) {b2.y =  100;}
	float d = (v2.x-v1.x) * (v3.y-v1.y)-(v3.x-v1.x)* (v2.y-v1.y);
	// For each pixel in the bounding box.
	for (int x = b1.x; x <= b2.x; x++)
	{
		for (int y = b1.y; y < b2.y; y++)
		{
			// Check if pixel is inside or on triangle.
			if (isToRightSide(v1, v2, {x-1, y}) && isToRightSide(v2, v3, {x-1, y}) && isToRightSide(v3, v1, {x-1, y}))
			{
				zindex = (160+x)+(100+y)*320;
				if (z_buffer[zindex] >= z)
				{				
					u = ((v2.x-v1.x) * (y-v1.y) - (x-v1.x) * (v2.y-v1.y)) / d;
					v = ((v3.x-v2.x) * (y-v2.y) - (x-v2.x) * (v3.y-v2.y)) / d;
					uv = texture.getTexture(u, v);
					plotFromAtlas(x, y, uv.x, uv.y);
					z_buffer[zindex] = z;
				}
			}
		}
	}
}
// Returns a Vertex2d rotated and translated to the Camera then perspective projected on screen.
Vertex3d toCamera(Vertex3d p)
{
	// Translate Point to Camera.
	p.x -= camera.x;
	p.y -= camera.y;
	p.z -= camera.z;
	// Rotate Point around Y axis.
	float sin, cos;
	Vertex3d yr;
	sin = camera.ysin;
	cos = camera.ycos;
	yr.x = (p.x*cos)+(p.z*sin);
	yr.y = p.y;
	yr.z = (p.x*sin)-(p.z*cos);
	// Rotate Point around X axis.
	Vertex3d xr;
	sin = camera.xsin;
	cos = camera.xcos;
	xr.x = yr.x;
	xr.y = (yr.y*cos)+(yr.z*sin);
	xr.z = (yr.y*sin)-(yr.z*cos);
	return xr;
}
// Apply Perspective Projection to a point.
Vertex3d toScreen(Vertex3d p)
{
	Vertex3d s;
	float sx = divide(p.x, p.z) * camera.fov;
	float sy = divide(p.y, p.z) * camera.fov;
	s.x = (int)sx;
	s.y = (int)sy;
	s.z = p.z;
	return s;
}
// Render a 3d Triangle.
void render3dTriangle(Vertex3d v1, Vertex3d v2, Vertex3d v3, Texture texture)
{
	Vertex3d sv1 = toCamera(v1);
	Vertex3d sv2 = toCamera(v2);
	Vertex3d sv3 = toCamera(v3);
	if (sv1.z < camera.znear && sv2.z < camera.znear && sv3.z < camera.znear)
		return;
	if (sv1.z < camera.znear)
		sv1.z = camera.znear;
	if (sv2.z < camera.znear)
		sv2.z = camera.znear;
	if (sv3.z < camera.znear)
		sv3.z = camera.znear;
	render2dTriangle(toScreen(sv1), toScreen(sv2), toScreen(sv3), texture, (sv1.z + sv2.z + sv3.z));
}
// Render a Quad from two Triangles.
void render3dQuad(Vertex3d v1, Vertex3d v2, Vertex3d v3, Vertex3d v4, Texture texture, bool flip_winding)
{
	if (flip_winding)
	{
		render3dTriangle(v3, v2, v1, texture);
		texture.invert = true;
		render3dTriangle(v1, v4, v3, texture);
	}
	else
	{
		render3dTriangle(v1, v2, v3, texture);
		texture.invert = true;
		render3dTriangle(v3, v4, v1, texture);
	}
}
// Render a cuboid.
void renderCube(Vertex3d p, Vertex3d s, Texture texture)
{
	// Code Snippet : render3dQuad({p.x, p.y, p.z}, {p.x, p.y, p.z}, {p.x, p.y, p.z}, {p.x, p.y, p.z}, texture,  false);
	render3dQuad({p.x, p.y, p.z}, {p.x+s.x, p.y, p.z}, {p.x+s.x, p.y+s.y, p.z}, {p.x, p.y+s.y, p.z}, texture,  false);
	render3dQuad({p.x, p.y, p.z+s.z}, {p.x+s.x, p.y, p.z+s.z}, {p.x+s.x, p.y+s.y, p.z+s.z}, {p.x, p.y+s.y, p.z+s.z}, texture,  true);
	render3dQuad({p.x, p.y, p.z}, {p.x, p.y+s.y, p.z}, {p.x, p.y+s.y, p.z+s.z}, {p.x, p.y, p.z+s.z}, texture,  false);
	render3dQuad({p.x+s.x, p.y, p.z}, {p.x+s.x, p.y+s.y, p.z}, {p.x+s.x, p.y+s.y, p.z+s.z}, {p.x+s.x, p.y, p.z+s.z}, texture,  true);
}
// Runs once before GameLoop starts.
void setup()
{
	frame_count  = 0;
	camera.x = 0;
	camera.y = 0;
	camera.z = -100;
	camera.xrot = 0;
	camera.yrot = 0;
}
// Handle Controls
void controls()
{
	if (0 != tigrKeyHeld(screen, TK_UP))
	{
		camera.xrot++;
	}
	if (0 != tigrKeyHeld(screen, TK_DOWN))
	{
		camera.xrot--;
	}
	if (0 != tigrKeyHeld(screen, TK_RIGHT))
	{
		camera.yrot--;
	}
	if (0 != tigrKeyHeld(screen, TK_LEFT))
	{
		camera.yrot++;
	}
	if (0 != tigrKeyHeld(screen, 'W'))
	{
		camera.x -= camera.ysin * 5;
		camera.z += camera.ycos * 5;
	}
	if (0 != tigrKeyHeld(screen, 'S'))
	{
		camera.x -= camera.ysin * -5;
		camera.z += camera.ycos * -5;
	}
	if (0 != tigrKeyHeld(screen, 'D'))
	{
		camera.x += camera.ycos * 5;
		camera.z += camera.ysin * 5;
	}
	if (0 != tigrKeyHeld(screen, 'A'))
	{
		camera.x += camera.ycos * -5;
		camera.z += camera.ysin * -5;
	}
	if (0 != tigrKeyHeld(screen, 'E'))
	{
		camera.y++;
	}
	if (0 != tigrKeyHeld(screen, 'Q'))
	{
		camera.y--;
	}
}
// Main GameLoop.
void main()
{
	flush();
	// Calculate Trignometry Values once every frame.
	camera.calcTrigValues();
	// Write your 3d model here.
	Texture cube_texture {{0, 0}, {128, 128}, {128, 128}};
	for (int x = 0; x < 5; x++)
		renderCube({x*128, 0, 0}, {128, 128, 128}, cube_texture);
	for (int x = 0; x < 5; x++)
		renderCube({0, 0, x*128}, {128, 128, 128}, cube_texture);
	// Handle Player Controls
	controls();
}

}

int main(int argc, char *argv[])
{	
	render::setup();
	while (!tigrClosed(screen))
	{
		tigrClear(screen, tigrRGB(98, 98, 82));
		render::main();
		frame_count++;
		//cout << "Frame Updated\n";
		tigrUpdate(screen);
	}
	tigrFree(screen);
	return 0;
}