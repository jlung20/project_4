#ifndef SUPPORT_H
#define SUPPORT_H

#include "provided.h"
#include <string>

// need this operator for comparing geocoords
inline bool operator <(const GeoCoord &LHS, const GeoCoord &RHS)
{
	if (LHS.latitudeText < RHS.latitudeText)
		return true;
	else if (RHS.latitudeText < LHS.latitudeText)
		return false;
	else if (LHS.longitudeText < RHS.longitudeText)
		return true;
	else
		return false;
}

// used for finding the direction that a geosegment goes
std::string directionOfLine(const GeoSegment& gs);

#endif // for SUPPORT_H