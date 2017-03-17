#include "provided.h"
#include "MyMap.h"
#include <string>
#include <algorithm>
#include <cctype>
using namespace std;

class AttractionMapperImpl
{
public:
	AttractionMapperImpl();
	~AttractionMapperImpl();
	void init(const MapLoader& ml);
	bool getGeoCoord(string attraction, GeoCoord& gc) const;
private:
	MyMap<string, GeoCoord> attractionMap;
	// just returns a string that's a lowercase version of what was passed in
	string stringToLowerCase(const string &toBeLowered) const;
};

AttractionMapperImpl::AttractionMapperImpl()
{
}

AttractionMapperImpl::~AttractionMapperImpl()
{
}

void AttractionMapperImpl::init(const MapLoader& ml)
{
	StreetSegment seg;
	size_t numSegments = ml.getNumSegments();
	
	for (size_t i = 0; i < numSegments; i++) // iterate through all of line segments
	{
		if (ml.getSegment(i, seg))
		{
			if (seg.attractions.size() != 0) // check if there are attractions in street segment
			{
				for (size_t j = 0; j < seg.attractions.size(); j++) // iterate through all attractions
				{
					// get attraction name and send it to lowercase
					string lowerName = stringToLowerCase(seg.attractions[j].name);
					// add to attraction map
					attractionMap.associate(lowerName, seg.attractions[j].geocoordinates);
				} // end for
			} // end if
		} // end if
	} // end for
}

bool AttractionMapperImpl::getGeoCoord(string attraction, GeoCoord& gc) const
{
	// send attraction name to lowercase because they're stored in map in that form
	// to ensure case-insensitivity
	string lowerName = stringToLowerCase(attraction);
	const GeoCoord* gcPtr = attractionMap.find(lowerName); // returns pointer to GeoCoord with that key
	if (!gcPtr) // if find returns nullptr, !nullptr is true
		return false;
	else
	{
		gc = *(const_cast<GeoCoord*>(gcPtr)); // otherwise, set gc to a dereferenced, const_cast version of returned pointer
		return true;
	}
}

string AttractionMapperImpl::stringToLowerCase(const string &toBeLowered) const
{
	string result;
	for (size_t i = 0; i < toBeLowered.size(); i++)
	{
		result += tolower(toBeLowered[i]); // goes character by character and sends to lowercase
	}
	return result;
}

//******************** AttractionMapper functions *****************************

// These functions simply delegate to AttractionMapperImpl's functions.
// You probably don't want to change any of this code.

AttractionMapper::AttractionMapper()
{
	m_impl = new AttractionMapperImpl;
}

AttractionMapper::~AttractionMapper()
{
	delete m_impl;
}

void AttractionMapper::init(const MapLoader& ml)
{
	m_impl->init(ml);
}

bool AttractionMapper::getGeoCoord(string attraction, GeoCoord& gc) const
{
	return m_impl->getGeoCoord(attraction, gc);
}
