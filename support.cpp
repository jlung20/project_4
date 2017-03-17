#include "support.h"
#include <string>
using namespace std;

string directionOfLine(const GeoSegment& gs)
{
	double angle = angleOfLine(gs);
	if (angle >= 0 && angle <= 22.5)
		return "east";
	else if (angle <= 67.5)
		return "northeast";
	else if (angle <= 112.5)
		return "north";
	else if (angle <= 157.5)
		return "northwest";
	else if (angle <= 202.5)
		return "west";
	else if (angle <= 247.5)
		return "southwest";
	else if (angle <= 292.5)
		return "south";
	else if (angle <= 337.5)
		return "southeast";
	else
		return "east";
}