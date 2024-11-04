// overpass_data_fetcher.hpp
#ifndef OVERPASS_DATA_FETCHER_HPP
#define OVERPASS_DATA_FETCHER_HPP

#include <vector>
#include <string>
#include <cpr/cpr.h>

struct BoundingBox {
    double min_lat;
    double min_lon;
    double max_lat;
    double max_lon;
};

class OverpassDataFetcher {
public:
    /**
     * Fetch Overpass data for a given bounding box
     * @param boundingBox Vector of two Location objects defining the area
     * @return cpr::Response containing the fetched data
     */
    
    static cpr::Response fetchOverpassData(const BoundingBox& boundingBox);

private:
    static const std::vector<std::string> highWayExclude;
};

#endif // OVERPASS_DATA_FETCHER_HPP