#include "provided.h"
#include "MyMap.h"
#include "support.h"
#include <vector>
using namespace std;

class SegmentMapperImpl
{
public:
	SegmentMapperImpl();
	~SegmentMapperImpl();
	void init(const MapLoader& ml);
	vector<StreetSegment> getSegments(const GeoCoord& gc) const;
private:
	MyMap<GeoCoord, vector<StreetSegment>> segmentMap;
	void addToMap(const GeoCoord &coord, const StreetSegment &seg);
};

SegmentMapperImpl::SegmentMapperImpl()
{
}

SegmentMapperImpl::~SegmentMapperImpl()
{
}

void SegmentMapperImpl::init(const MapLoader& ml)
{
	StreetSegment seg;
	size_t numSegments = ml.getNumSegments();
	for (size_t segNum = 0; segNum < numSegments; segNum++)
	{
		if (ml.getSegment(segNum, seg))
		{
			addToMap(seg.segment.start, seg); // add starting coordinate
			addToMap(seg.segment.end, seg); // add ending coordinate

			if (seg.attractions.size() != 0) // there are attractions in street segment
				for (size_t j = 0; j < seg.attractions.size(); j++)
					addToMap(seg.attractions[j].geocoordinates, seg);
		} // end if
	} // end for
}

void SegmentMapperImpl::addToMap(const GeoCoord &coord, const StreetSegment &seg)
{
	vector<StreetSegment> *segmentPtr;
	segmentPtr = segmentMap.find(coord);
	if (segmentPtr == nullptr) // if none exists, make a new vector
	{
		vector<StreetSegment> segmentVector;
		segmentVector.push_back(seg);
		segmentMap.associate(coord, segmentVector); // associate coordinates and vector
	}
	else // coord found in map
	{
		segmentPtr->push_back(seg); // so just add street segment to coord's vector
	}
}

vector<StreetSegment> SegmentMapperImpl::getSegments(const GeoCoord& gc) const
{
	const vector<StreetSegment> *segVector;
	vector<StreetSegment> segments;
	segVector = segmentMap.find(gc);
	if (segVector == nullptr)
		return segments;
	else
	{
		// not sure if i want to provide access to the insides of the map so i'm making
		// and returning the "segments" vector so it won't mess up the map
		segments = *segVector;
		return segments;
	}
}

//******************** SegmentMapper functions ********************************

// These functions simply delegate to SegmentMapperImpl's functions.
// You probably don't want to change any of this code.

SegmentMapper::SegmentMapper()
{
	m_impl = new SegmentMapperImpl;
}

SegmentMapper::~SegmentMapper()
{
	delete m_impl;
}

void SegmentMapper::init(const MapLoader& ml)
{
	m_impl->init(ml);
}

vector<StreetSegment> SegmentMapper::getSegments(const GeoCoord& gc) const
{
	return m_impl->getSegments(gc);
}
