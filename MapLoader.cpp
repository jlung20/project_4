#include "provided.h"
#include <string>
#include <fstream>
#include <iostream>
using namespace std;

class MapLoaderImpl
{
public:
	MapLoaderImpl();
	~MapLoaderImpl();
	bool load(string mapFile);
	size_t getNumSegments() const;
	bool getSegment(size_t segNum, StreetSegment& seg) const;
private:
	size_t m_numSegments;
	vector<StreetSegment> segmentVector;
};

MapLoaderImpl::MapLoaderImpl()
	: m_numSegments(0) // just to be safe
{
}

MapLoaderImpl::~MapLoaderImpl() // nothing to do because no dynamic memory allocation or pointers
{
}

bool MapLoaderImpl::load(string mapFile)
{
	ifstream loader(mapFile);
	if (!loader) // if file is bad, this is true
		return false;
	else
	{
		segmentVector.resize(20000); // only expand the vector if file loaded correctly
		string input, streetName, attractionName;
		int numAttractions = 0;
		string latitude, longitude;
		while (getline(loader, input))
		{
			streetName = input;
			segmentVector[m_numSegments].streetName = streetName; // add street name to segment

			latitude = longitude = "";
			char ch;
			loader.get(ch);
			while (ch != ',') // get latitude
			{
				latitude += ch;
				loader.get(ch);
			}
			loader.get(ch);
			while (ch == ' ') // consume any empty space between
			{
				loader.get(ch);
			}
			while (ch != ' ') // get longitude
			{
				longitude += ch;
				loader.get(ch);
			}

			GeoCoord start(latitude, longitude);
			segmentVector[m_numSegments].segment.start = start; // add start GeoCoord

			latitude = longitude = "";
			loader.get(ch);
			while (ch != ',') // get latitude
			{
				latitude += ch;
				loader.get(ch);
			}
			loader.get(ch);
			while (ch == ' ') // consume any empty space
			{
				loader.get(ch);
			}
			while (ch != '\n') // get longitude
			{
				longitude += ch;
				loader.get(ch);
			}
			GeoCoord end(latitude, longitude);
			segmentVector[m_numSegments].segment.end = end; // add end GeoCoord


			loader >> numAttractions; // read in number of attractions
			loader.ignore(10000, '\n');

			for (int i = 0; i < numAttractions; i++) // look at all attractions
			{
				loader.get(ch);
				attractionName = "";
				while (ch != '|') // read in attraction name, char by char
				{
					attractionName += ch;
					loader.get(ch);
				}

				latitude = longitude = "";
				loader.get(ch);
				while (ch != ',') // read in latitude
				{
					latitude += ch;
					loader.get(ch);
				}
				loader.get(ch);
				while (ch == ' ')
				{
					loader.get(ch);
				}
				while (ch != '\n') // read in longitude
				{
					longitude += ch;
					loader.get(ch);
				}

				GeoCoord attractionLoc(latitude, longitude);

				Attraction newAttraction; // make an attraction
				newAttraction.name = attractionName; // initialize name
				newAttraction.geocoordinates = attractionLoc; // initialize geocoord
				segmentVector[m_numSegments].attractions.push_back(newAttraction); // add to vector
			} // end for
			m_numSegments++; // increment segment count
		} // end while
	} // end else

	return true; // file was loaded successfully
}

size_t MapLoaderImpl::getNumSegments() const
{
	return m_numSegments;
}

bool MapLoaderImpl::getSegment(size_t segNum, StreetSegment &seg) const
{
	if ((m_numSegments < segNum + 1) || segNum > 10000000)
		return false;
	else
	{
		seg = segmentVector[segNum];
		return true;
	}
}

//******************** MapLoader functions ************************************

// These functions simply delegate to MapLoaderImpl's functions.
// You probably don't want to change any of this code.

MapLoader::MapLoader()
{
	m_impl = new MapLoaderImpl;
}

MapLoader::~MapLoader()
{
	delete m_impl;
}

bool MapLoader::load(string mapFile)
{
	return m_impl->load(mapFile);
}

size_t MapLoader::getNumSegments() const
{
	return m_impl->getNumSegments();
}

bool MapLoader::getSegment(size_t segNum, StreetSegment& seg) const
{
	return m_impl->getSegment(segNum, seg);
}
