#include "provided.h"
#include "MyMap.h"
#include "support.h"
#include <string>
#include <vector>
#include <queue>
#include <list>
#include <stack>
#include <functional>
using namespace std;

// equality comparison operator for geocoords. compares their strings
bool operator ==(const GeoCoord &LHS, const GeoCoord &RHS)
{
	return LHS.latitudeText == RHS.latitudeText && LHS.longitudeText == RHS.longitudeText;
}

class NavigatorImpl
{
public:
	NavigatorImpl();
	~NavigatorImpl();
	bool loadMapData(string mapFile);
	NavResult navigate(string start, string end, vector<NavSegment>& directions) const;

private:
	MapLoader* mapper;
	SegmentMapper segMapper;
	AttractionMapper attractMapper;
	// determines direction by calling angleOfLine()
	string directionToTravel(const GeoCoord &begin, const GeoCoord &end) const;
	// determines distance by calling distanceEarthMiles()
	double distanceToTravel(const GeoCoord &begin, const GeoCoord &end) const;
	// surprisingly tricky function. takes the end coordinate and traces back to determine order in which geoCoords
	// were reached. then it constructs NavSegments from that knowledge
	void reconstructPath(GeoCoord endCoord, vector<NavSegment> &path, MyMap<GeoCoord, GeoCoord> *ptrToPriorsMap) const;

	// used in priority_queue. contains two GeoCoords, itself and its parent.
	// operator < is sort of confusing. it does THE REVERSE of what might be expected.
	// this is done so that priority_queue can order from least to greatest f_score
	class AugmentedGeoCoord {
	public:
		AugmentedGeoCoord(const GeoCoord &gc, const GeoCoord &end, const GeoCoord &previousCoord, double g_sc);
		bool operator <(const AugmentedGeoCoord &RHS) const
		{
			return f_score > RHS.f_score;
		}
		bool operator ==(const AugmentedGeoCoord &RHS) const
		{
			return coord == RHS.coord;
		}
		void updateGScore(double newScore) { g_score = newScore; f_score = g_score + h_score; }
		GeoCoord getGeoCoord() const { return coord; }
		GeoCoord getPreviousCoord() const { return prevCoord; }
		double getFScore() const { return f_score; }
		double getGScore() const { return g_score; }
	private:
		GeoCoord coord;
		GeoCoord prevCoord; // make sure that this is nullptr for the starting location
		double g_score; // distance from start to coord. initialized to 10000 for all but start
		double h_score; // distance from coord to end location
		double f_score; // sum of g and h scores
	};

	class HashTable
	{
	public:
		HashTable();
		bool find(const GeoCoord &coord);
		void insert(GeoCoord coord);
		//void remove(GeoCoord coord);
		~HashTable();
	private:
		// less memory-intensive at the start than have an array of vectors
		/*vector<GeoCoord> *gcArr[2500];*/
		list<GeoCoord> *gcArr[2500];
		// using this hash function to hash strings (latitudeText to be exact) so I'm praying
		// that two geocoords only have the exact same latitude when they're actually the same
		// just going to has the latitude of geocoord
		hash<string> string_hash;
	};

	bool shouldAddToOpenSet(GeoCoord coordToCheck, HashTable* ptrToClosedSet, MyMap <GeoCoord, double> *ptrToFScoreMap, double fScore) const;
};

NavigatorImpl::NavigatorImpl()
{
}

NavigatorImpl::~NavigatorImpl()
{
	delete mapper;
}

bool NavigatorImpl::loadMapData(string mapFile)
{
	mapper = new MapLoader;
	if (!mapper->load(mapFile)) // if there was some issue loading the file, return false
		return false;
	else // otherwise, initialize the other mappers
	{
		segMapper.init(*mapper);
		attractMapper.init(*mapper);
		return true;
	}
}

NavResult NavigatorImpl::navigate(string start, string end, vector<NavSegment> &directions) const
{
	GeoCoord startGC, endGC;
	if (!attractMapper.getGeoCoord(start, startGC))
		return NAV_BAD_SOURCE;
	if (!attractMapper.getGeoCoord(end, endGC))
		return NAV_BAD_DESTINATION;

	MyMap<GeoCoord, GeoCoord> previousGeoCoordMap; // associates GeoCoords with parent
	MyMap<GeoCoord, double> fScoresOfClosedSet; // useful in deciding if something needs to be added to openSet
	HashTable closedSet; // contains geocoords that have already been "relaxed"

	// contains geocoords that need to be relaxed. ordered based on lowest f_score (sum of g and h scores)
	priority_queue<AugmentedGeoCoord> openSet;
	GeoCoord sentinel("-360", "-360");
	openSet.emplace(startGC, endGC, sentinel, 0);
	while (!openSet.empty())
	{
		AugmentedGeoCoord current = openSet.top(); // get a new AugmentedGeoCoord for each iteration
		openSet.pop();
		// if current geocoord is the end geocoord, it means we've found the most efficient way to the end
		if (current.getGeoCoord() == endGC)
		{
			directions.clear(); // clear the vector of NavSegments
			previousGeoCoordMap.associate(current.getGeoCoord(), current.getPreviousCoord()); // add to map of children to parents
			reconstructPath(endGC, directions, &previousGeoCoordMap); // go from geocoords to NavSegments by interpolation
			return NAV_SUCCESS;
		}
		if (closedSet.find(current.getGeoCoord())) // see if its coordinates match something on the closed list
		{
			// there should always be a corresponding entry for the coord in fScores map because
			// it's added whenever an entry is added to closedSet
			if (*fScoresOfClosedSet.find(current.getGeoCoord()) < current.getFScore())
				continue; // if it didn't arrive faster, don't do anything with it
			else
			{
				previousGeoCoordMap.associate(current.getGeoCoord(), current.getPreviousCoord());
				fScoresOfClosedSet.associate(current.getGeoCoord(), current.getFScore());
			}
		}
		else
		{
			closedSet.insert(current.getGeoCoord()); // add it to closed set if it's not already there
			// associate current geocoord to its parent
			previousGeoCoordMap.associate(current.getGeoCoord(), current.getPreviousCoord());
			// add f_score to map for later use in checking what should be added to open set
			fScoresOfClosedSet.associate(current.getGeoCoord(), current.getFScore());
		}

		vector<StreetSegment> neighboringSegments = segMapper.getSegments(current.getGeoCoord());
		// iterate through all segments and add whatever coords can be added to the open set
		for (auto iter : neighboringSegments)
		{
			for (size_t i = 0; i < iter.attractions.size(); i++) // check if any segment contains the end destination
			{
				if (iter.attractions[i].geocoordinates == endGC) // if it does,
				{
					double newGScore = current.getGScore() + distanceEarthMiles(current.getGeoCoord(), endGC);
					openSet.emplace(endGC, endGC, current.getGeoCoord(), newGScore); // add it to open set so it will eventually rise to the top
				}
			}

				// g score is sum of current coord's g scores and distance between current coord and neighbor
				double newGScoreOfStart = current.getGScore() + distanceEarthMiles(current.getGeoCoord(), iter.segment.start);
				if (closedSet.find(iter.segment.start)) // see if its coordinates match something on the closed list
				{
					double hScore = distanceEarthMiles(iter.segment.start, endGC);
					//if (*fScoresOfClosedSet.find(current.getGeoCoord) < current.getFScore())
					//	continue;
					if (shouldAddToOpenSet(iter.segment.start, &closedSet, &fScoresOfClosedSet, newGScoreOfStart + hScore))
						openSet.emplace(iter.segment.start, endGC, current.getGeoCoord(), newGScoreOfStart);
				}
				else
					openSet.emplace(iter.segment.start, endGC, current.getGeoCoord(), newGScoreOfStart);

				// g score is sum of current coord's g scores and distance between current coord and neighbor
				double newGScoreOfEnd = current.getGScore() + distanceEarthMiles(current.getGeoCoord(), iter.segment.end);
				if (closedSet.find(iter.segment.end)) // see if its coordinates match something on the closed list
				{
					double hScore = distanceEarthMiles(iter.segment.end, endGC);
					if (shouldAddToOpenSet(iter.segment.end, &closedSet, &fScoresOfClosedSet, newGScoreOfEnd + hScore))
						openSet.emplace(iter.segment.end, endGC, current.getGeoCoord(), newGScoreOfEnd);
				}
				else
					openSet.emplace(iter.segment.end, endGC, current.getGeoCoord(), newGScoreOfEnd);

			} // end for
	} // end while

	return NAV_NO_ROUTE;  // if you've made it all the way to here, there must not be a valid route
}

// determines if a certain coord should be added to the open set
bool NavigatorImpl::shouldAddToOpenSet(GeoCoord coordToCheck, HashTable* ptrToClosedSet, MyMap <GeoCoord, double> *ptrToFScoreMap, double fScore) const
{
	// if pointer is not already in the closed set, it should be added to the open set
	if (!ptrToClosedSet->find(coordToCheck))
		return true;
	else
		// if f_score of coord is lower than that which was already relaxed, should return true
		// otherwise, return false
		return *ptrToFScoreMap->find(coordToCheck) > fScore;
}

void NavigatorImpl::reconstructPath(GeoCoord endCoord, vector<NavSegment> &path, MyMap<GeoCoord, GeoCoord> *ptrToPriorsMap) const
{
	// records all coords that were visited. the fact that it's a stack makes reconstructing the path easier
	// because of last in, first out nature
	stack<GeoCoord> coordsVisited;

	coordsVisited.push(endCoord);
	GeoCoord *ptrToCoord = ptrToPriorsMap->find(endCoord);
	while (ptrToCoord->latitudeText != "-360") // -360 is sentinel value. means we've traced back to start.
	{
		coordsVisited.push(*ptrToCoord);
		ptrToCoord = ptrToPriorsMap->find(*ptrToCoord);
	}

	while (coordsVisited.size() >= 2) // as long as there's at least a pair of geocoords, go through this
	{
		GeoCoord first = coordsVisited.top();
		coordsVisited.pop(); // only pop off one each time through. 
		GeoCoord second = coordsVisited.top();
		vector<StreetSegment> segmentsOfFirst, segmentsOfSecond;
		segmentsOfFirst = segMapper.getSegments(first);
		segmentsOfSecond = segMapper.getSegments(second);

		StreetSegment associatedSegment; // segment that is associated with both first and second geocoords

		for (StreetSegment segmentToCompare : segmentsOfFirst)
		{
			if ((segmentToCompare.segment.start == first && segmentToCompare.segment.end == second) ||
				(segmentToCompare.segment.end == first && segmentToCompare.segment.start == second))
			{
				// if endpoints match first and second coords, segment must be associated with them
				associatedSegment = segmentToCompare;
				break;
			}
			else
			{
				for (size_t i = 0; i < segmentToCompare.attractions.size(); i++)
				{
					if (segmentToCompare.attractions[i].geocoordinates == first ||
						segmentToCompare.attractions[i].geocoordinates == second)
					{
						// if one of the geocoords is an attraction, then it must be associated with
						// one of its geosegments
						associatedSegment = segmentToCompare;
						break;
					}
				}
			}
		}

		if (!path.empty())
		{
			// if there's a change in street name, there must have been a turn!
			if (associatedSegment.streetName != path.back().m_streetName)
			{
				// construct a geosegment to represent the street. have it go in the proper
				// direction, otherwise angle between 2 lines might return exactly the wrong direction
				GeoCoord oldStreetStart;
				GeoCoord oldStreetEnd = first;
				if (path.back().m_geoSegment.start == first)
					oldStreetStart = path.back().m_geoSegment.start;
				else
					oldStreetStart = path.back().m_geoSegment.end;
				GeoSegment oldStreet(oldStreetStart, oldStreetEnd);

				// construct the new street segment from the two current geocoords (found much earlier)
				GeoSegment newStreet(first, second);

				double angle = angleBetween2Lines(oldStreet, newStreet);
				string direction;
				if (angle < 180)
					direction = "left";
				else
					direction = "right";
				path.emplace_back(direction, associatedSegment.streetName);
			}
		}
		// add a proceed statement from first coord to second coord for every pair of coords
		path.emplace_back(directionToTravel(first, second), associatedSegment.streetName,
			distanceToTravel(first, second), associatedSegment.segment);
	}
}

string NavigatorImpl::directionToTravel(const GeoCoord &begin, const GeoCoord &end) const
{
	// to make life easier, just pass in two geocoords and function will construct geosegment
	GeoSegment segment(begin, end);
	return directionOfLine(segment); // fn returns direction in form of string
}

double NavigatorImpl::distanceToTravel(const GeoCoord &begin, const GeoCoord & end) const
{
	return distanceEarthMiles(begin, end);
}

NavigatorImpl::HashTable::HashTable()
	: gcArr{ nullptr } // just to be safe. initialize all elements to nullptr
{}

bool NavigatorImpl::HashTable::find(const GeoCoord &coord)
{
	// use STL's hash function and hash to array of 2500
	size_t hashedValue = string_hash(coord.latitudeText) % 2500;
	// if the list pointer is nullptr, coord can't be there
	if (gcArr[hashedValue] != nullptr)
		for (GeoCoord geo : *gcArr[hashedValue]) // iterate through list
			if (geo == coord) // and check if there's a match
				return true;
	return false;
}

void NavigatorImpl::HashTable::insert(GeoCoord coord)
{
	// use hash function to find where coord should be hashed to
	size_t hashedValue = string_hash(coord.latitudeText) % 2500;
	// if the list doesn't already exist, make one
	if (gcArr[hashedValue] == nullptr)
		gcArr[hashedValue] = new list<GeoCoord>;
	for (GeoCoord gc : *gcArr[hashedValue])
		if (gc == coord) // don't insert something that's already there
			return;
	gcArr[hashedValue]->push_back(coord); // otherwise, push_back coord into list
}

NavigatorImpl::HashTable::~HashTable()
{
	for (auto gcIter : gcArr)
		if (gcIter != nullptr) // if vector pointer points to a vector, 
			delete gcIter;  // delete that vector
}

NavigatorImpl::AugmentedGeoCoord::AugmentedGeoCoord(const GeoCoord &gc, const GeoCoord &end, const GeoCoord &previousCoord, double g_sc)
{
	coord = gc;
	prevCoord = previousCoord;
	// distance required to get to point
	g_score = g_sc;
	// save some work on the call by doing this computation during initailization
	h_score = distanceEarthMiles(coord, end); // compute distance from geocoord to end
	// sum of scores. priority_queue compares f_scores and selects lowest
	f_score = g_score + h_score;
}

//******************** Navigator functions ************************************

// These functions simply delegate to NavigatorImpl's functions.
// You probably don't want to change any of this code.

Navigator::Navigator()
{
	m_impl = new NavigatorImpl;
}

Navigator::~Navigator()
{
	delete m_impl;
}

bool Navigator::loadMapData(string mapFile)
{
	return m_impl->loadMapData(mapFile);
}

NavResult Navigator::navigate(string start, string end, vector<NavSegment>& directions) const
{
	return m_impl->navigate(start, end, directions);
}
