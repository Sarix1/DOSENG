#include "TYPES.H"
#include "VIDEO.H"
#include "MATH.H"
#include "GFX.H"

// Coverage spans between two edges for triangle/shape drawing
static struct Coverage coverage;

/*************************************************************/
/* CAUTION: no boundary checking in most of below functions! */
/*************************************************************/

// Write a pixel to off-screen buffer or VGA buffer, depending where drawTarget points
void setPixel(int x, int y, byte color)
{
	if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT)
		drawTarget[(y<<8) + (y<<6) + x] = color;
}

// Fill the screen
void setPixelsAll(byte color)
{
	_fmemset(drawTarget, color, SCREEN_SIZE);
}

// Horizontal line
void setPixelsHorizontally(int x, int y, int len, byte color)
{
	byte far* p = drawTarget + (y<<8) + (y<<6) + x;
	_fmemset(p, color, abs(len));
}

// Vertical line
void setPixelsVertically(int x, int y, int len, byte color)
{
	byte far* p = drawTarget + (y<<8) + (y<<6) + x;

	while (len--)
	{
		*p = color;
		p += SCREEN_WIDTH;
	}
}

// 45 degree line
void setPixelsDiagonally(int x, int y, int len, int horz_dir, byte color)
{
	int offset = SCREEN_WIDTH;
	byte far* p = drawTarget + (y<<8) + (y<<6) + x;
	
	// len determines upwards/downards direction (len > 0 == down)
	// horz_dir determines left/right (horz_dir > 0 == right)
	if (len > 0) 
	{		
		offset += horz_dir;
		
		while (len--)
		{
			*p = color;
			p += offset;
		}
	}
	else
	{
		offset -= horz_dir;
		
		while (len++)
		{
			*p = color;
			p -= offset;
		}
	}
}

// Unorthodox line
void setPixelsSlope(int ax, int ay, int bx, int by, byte color)
{
	int i, x, y, px, py;
	
	const int dx = bx - ax;
	const int dy = by - ay;
	const int dx_abs = abs(dx);
	const int dy_abs = abs(dy);
	const int x_sign = sign(dx);
	const int y_sign = sign(dy);
	
	x = dy_abs >> 1;
	y = dx_abs >> 1;
	
	// Starting point
	px = ax;
	py = ay;

	if (dx_abs > dy_abs)
	{
		for (i = 0; i <= dx_abs; i++)
		{
			setPixel(px, py, color);
			y += dy_abs;
			
			if (y >= dx_abs)
			{
				y -= dx_abs;
				py += y_sign;
			}
			
			px += x_sign;
		}
	}
	else
	{
		for (i = 0; i <= dy_abs; i++)
		{
			setPixel(px, py, color);
			x += dx_abs;
			
			if (x >= dy_abs)
			{
				x -= dy_abs;
				px += x_sign;
			}
			
			py += y_sign;
		}
	}
}

// Put a single pixel
void drawPixel( int x, int y,
				byte color)
{
	// boundary check ?
	setPixel(x, y, color);
}

// Fill the screen
void drawFill(byte color)
{
	setPixelsAll(color);
}

// Draw a line between two points
void drawLine(struct Point2D* p1, struct Point2D* p2, byte color)
{
	const int dx = p2->x - p1->x;
	const int dy = p2->y - p1->y;
	
	// Use the specific functions for vertical, horizontal, diagonal, and slope
	if (!dx)
		setPixelsVertically(p1->x, 							// x start pos
							(p1->y < p2->y ? p1->y : p2->y),// y start pos
							abs(dy)+1, 						// length
							color);
	else if (!dy)
		setPixelsHorizontally(	(p1->x < p2->x ? p1->x : p2->x),// x start pos
								p1->y,                          // y start pos
								abs(dx)+1,                      // length
								color);
	else if (abs(dx) == abs(dy))
		setPixelsDiagonally(p1->x,			// x start pos
							p1->y,			// y start pos
							dy + sign(dy),	// length
							sign(dx),		// horizontal direction
							color);
	else
		setPixelsSlope( p1->x, p1->y,
						p2->x, p2->y,
						color);
}

// Draw multiple lines between points
void drawLines(struct Point2D* points[], byte color)
{
	int i = 0;
	while (points[++i] != NULL)
	{
		drawLine(points[i-1], points[i], color);
	}
}

// Draw a box
void drawBoxFill(int x, int y, int w, int h, byte color)
{
	// Needs boundary checking here
	while (h--)
		setPixelsHorizontally(x, y++, w, color);
}

// Draw a box centered at x and y
void drawBoxFillCenter(int x, int y, int w, int h, byte color)
{
	drawBoxFill((x+1 - w/2 - w%2), (y+1 - h/2 - h%2), w, h, color);
}

// Draw a rectangular frame
void drawBoxFrame(int x, int y,
				  int w, int h,
				  byte color)
{
	// Needs boundary checking here
	setPixelsHorizontally	(x,		y,		w,	color);
	setPixelsHorizontally	(x,		y+h-1,	w,	color);
	setPixelsVertically		(x,		y,		h,	color);
	setPixelsVertically		(x+w-1,	y,		h,	color);
}

// Draw a rectangular frame centered at x and y
void drawBoxFrameCenter(int x, int y, int w, int h, byte color)
{
	drawBoxFrame((x+1 - w/2 - w%2), (y+1 - h/2 - h%2), w, h, color);
}

// Circle drawing
void drawCircleFrame(int x, int y, int diameter, byte color)
{
	// r is for inverse of remainder
	// where FRACTION is involved, fixed point values are used
	const int	radius			= diameter / 2;
	const long	inverseRadius	= ((1 / (float)radius) * (1 << FRACTION)) + 0.5;
	const int	r				= (diameter % 2) ^ 1;
	
	int py			= radius;
	int px			= 0;
	long distance	= 0;
	long heightRatio;
	
	while (px <= py)
	{
		setPixel(x+px,		y-py+r,	color);
		setPixel(x+px,		y+py,	color);
		setPixel(x-px+r,	y-py+r,	color);
		setPixel(x-px+r,	y+py,	color);
		setPixel(x+py,		y-px+r,	color);
		setPixel(x+py,		y+px,	color);
		setPixel(x-py+r,	y-px+r,	color);
		setPixel(x-py+r,	y+px,	color);

		distance   += inverseRadius;
		heightRatio	= SinAcosTable[distance >> FRAC_TO_TRIG];
		py			= (int)(((radius * (long long)heightRatio) >> FRACTION)) + 1;
		px++;
	}
}

// Filled circle
void drawCircleFill(int x, int y, int diameter, byte color)
{
	// r is for inverse of remainder
	// where FRACTION is involved, fixed point values are used
	const int	radius			= diameter / 2;
	const long	inverseRadius	= ((1 / (float)radius) * (1 << FRACTION)) + 0.5;
	const int	r				= (diameter % 2) ^ 1;

	int w;
	int py			= 1;
	int px			= radius;
	long distance	= 0x1F;
	long widthRatio;
	
	if (!r)
		setPixelsHorizontally(x-px,	y,	diameter,	color);
	
	while (py <= radius)
	{
		w = px * 2 + (r ^ 1);
		setPixelsHorizontally(x-px+r,	y-py+r,	w,	color);
		setPixelsHorizontally(x-px+r,	y+py,	w,	color);
		
		distance	+= inverseRadius;
		widthRatio	 = SinAcosTable[distance >> FRAC_TO_TRIG];
		px			 = (int)(((radius * (long long)widthRatio) >> FRACTION) + 1);
		py++;
	}
}

// Thick circle
void drawCircleThick(int x, int y, int diameter, int thickness, byte color)
{
	// r is for inverse of remainder
	// 2 suffix is for the inner circle
	// where FRACTION is involved, fixed point values are used
	const int	radius			= diameter / 2;
	const int	radius2			= radius - thickness;
	const long	inverseRadius	= ((1 / (float)radius) * (1 << FRACTION)) + 0.5;
	const long	inverseRadius2	= ((1 / (float)radius2) * (1 << FRACTION)) + 0.5;
	const int	r				= (diameter % 2) ^ 1;
	
	int w;
	int py				= 1;
	int px				= radius;
	int px2				= radius2;
	long distance		= 0x1F;
	long distance2		= 0x1F;
	long widthRatio;
	
	if (!r)
	{
		setPixelsHorizontally(x-px,			y,	thickness,	color);
		setPixelsHorizontally(x+px2+r+1,	y,	thickness,	color);
	}
	
	while (py <= radius2)
	{
		w = px - px2;
		setPixelsHorizontally(x-px+r,	y-py+r,	w,	color);
		setPixelsHorizontally(x-px+r,	y+py,	w,	color);
		setPixelsHorizontally(x+px2+1,	y-py+r,	w,	color);
		setPixelsHorizontally(x+px2+1,	y+py,	w,	color);
		
		distance	+= inverseRadius;
		distance2	+= inverseRadius2;
		widthRatio	 = SinAcosTable[distance >> FRAC_TO_TRIG];
		px			 = (int)(((radius * (long long)widthRatio) >> FRACTION) + 1);
		widthRatio	 = SinAcosTable[distance2 >> FRAC_TO_TRIG];
		px2			 = (int)(((radius2 * (long long)widthRatio) >> FRACTION) + 1);
		py++;
	}
	
	while (py <= radius)
	{
		w = px*2 + (r^1);
		setPixelsHorizontally(x-px+r,	y-py+r,	w,	color);
		setPixelsHorizontally(x-px+r,	y+py,	w,	color);
		
		distance	+= inverseRadius;
		widthRatio	 = (SinAcosTable[distance >> FRAC_TO_TRIG]);
		px			 = (int)(((radius * (long long)widthRatio) >> FRACTION) + 1);
		py++;
	}
}

// Plot a slope and store its x coordinates on an array of y coordinates
// The slope must run downwards, i.e. ay < by
void plotLine(struct Edge* edge, int align, const struct Point2D* p1, const struct Point2D* p2)
{
	int i, x, y, px, py;
	
	const int dx = abs(p2->x - p1->x);
	const int dy = p2->y - p1->y;
	const int x_sign = sign(p2->x - p1->x);
	
	x = dy >> 1;
	y = dx >> 1;
	
	// Starting point
	px = p1->x;
	py = p1->y;
	
	edge->x_at_y[p1->y] = p1->x;
	edge->x_at_y[p2->y] = p2->x;
	
	// Plot the slope, write only when py changes
	if (dx > dy)
	{	
		if (align == TOP)
		{
			for (i = 0; i <= dx; i++)
			{	
				if (y >= dx)
				{
					y -= dx;
					edge->x_at_y[py++] = px;
				}
				
				px += x_sign;
				y += dy;
			}
		}
		else
		{	
			for (i = 0; i <= dx; i++)
			{	
				if (y >= dx)
				{
					y -= dx;
					edge->x_at_y[++py] = px;
				}
				
				px += x_sign;
				y += dy;
			}
		}
	}
	else
	{
		for (i = 0; i <= dy; i++)
		{
			x += dx;
			
			if (x >= dy)
			{
				x -= dy;
				px += x_sign;
			}
			
			py++;
			edge->x_at_y[py] = px;
		}
	}
}

// Draw a triangle frame
void drawTriangleFrame(const struct Triangle2D* triangle)
{
	drawLine(triangle->point[0], triangle->point[1], triangle->color);
	drawLine(triangle->point[1], triangle->point[2], triangle->color);
	drawLine(triangle->point[2], triangle->point[0], triangle->color);
}

// Draw a Triangle2D struct by passing its data to the actual drawing function
void drawTriangleFill(const struct Triangle2D* triangle)
{
	drawTriangleFill_P(triangle->point[0], triangle->point[1], triangle->point[2], triangle->color);
}

// Draw a filled triangle
void drawTriangleFill_P(const struct Point2D* p1_, const struct Point2D* p2_, const struct Point2D* p3_, byte color)
{
	int y;
	int align;
	struct Point2D* p1 = p1_;
	struct Point2D* p2 = p2_;
	struct Point2D* p3 = p3_;
	
	// Sort points vertically from top to bottom
	// Sort horizontally aligned points left to right
	sortPair(&p1, &p2);
	sortPair(&p1, &p3);
	sortPair(&p2, &p3);
	
	// If the triangle is small enough, it can be drawn as a horizontal line or single pixel
	// In that case, do an early return and do not execute the rest of this function
	if (p1->y == p3->y)
	{
		if (p1->x < p3->x)
			setPixelsHorizontally(p1->x, p1->y, (p3->x - p1->x + 1), color);
		else
			setPixel(p1->x, p1->y, color);
		
		return;
	}
	
	// Set the vertical coverage
	coverage.top	= p1->y;
	coverage.bottom	= p3->y;
	
	// Determine on which side the long edge is on
	if (orient2D(p1, p2, p3) > 0)
		align = LEFT;
	else
		align = RIGHT;
	
	// Calculate slope for the long edge
	plotLine(&(coverage.edges[align]), (p1->x < p3->x ? align^1 : align), p1, p3);
	
	// Calculate slope for upper short edge
	if (p1->y < p2->y)
		plotLine(&(coverage.edges[align^1]), (p1->x < p2->x ? align : align^1), p1, p2);
	else
	{
		coverage.edges[0].x_at_y[p1->y] = p1->x;
		coverage.edges[1].x_at_y[p1->y] = p2->x;
	}
	
	// Calculate slope for lower short edge
	if (p2->y < p3->y)
		plotLine(&(coverage.edges[align^1]), (p2->x < p3->x ? align : align^1), p2, p3);
	else
	{
		coverage.edges[0].x_at_y[p2->y] = p2->x;
		coverage.edges[1].x_at_y[p2->y] = p3->x;
	}
	
	// Draw horizontal scanlines
	for (y = coverage.top; y <= coverage.bottom; y++)
	{
		setPixelsHorizontally(coverage.edges[0].x_at_y[y], y, (coverage.edges[1].x_at_y[y] - coverage.edges[0].x_at_y[y] + 1), color);
	}
}

// Draw a flat-colored polygon outline
void drawPolyFrame_P(const struct Point2D* points[], byte color)
{
	int i = 0;
	while (points[++i] != NULL)
		drawLine(points[i-1], points[i], color);
	
	drawLine(points[i-1], points[0], color);
}

// Draw a flat-colored filled polygon
void drawPolyFill_P(const struct Point2D* points[], byte color)
{
	int i = 1;
	while (points[i+1] != NULL)
	{
		drawTriangleFill_P(points[0], points[i], points[i+1], color);
		i++;
	}
}