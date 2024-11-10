#ifndef OVERPASS_DATA_FETCHER_HPP
#define OVERPASS_DATA_FETCHER_HPP

#include <vector>
#include <string>
#include <cpr/cpr.h>
#include <cmath>
#include "json.hpp"
#include "graph.hpp"

using json = nlohmann::json;

struct BoundingBox {
    double min_lat;
    double min_lon;
    double max_lat;
    double max_lon;

    // bool (double minLat, double minLon, double maxLat, double maxLon)
    //     : min_lat(minLat), min_lon(minLon), max_lat(maxLat), max_lon(maxLon) {
    //     if (min_lat > max_lat || min_lon > max_lon) {
    //         throw std::invalid_argument("Invalid bounding box coordinates");
    //     }
    // }

    bool contains(const GeoCoord& point) const {
        return point.lat >= min_lat && point.lat <= max_lat &&
               point.lon >= min_lon && point.lon <= max_lon;
    }
};

struct GeoPoint {
    double lat;
    double lon;

    static double calculateDistance(const GeoPoint& p1, const GeoPoint& p2);
    static double calculateBearing(const GeoPoint& p1, const GeoPoint& p2);
    GeoPoint destinationPoint(double distance, double bearing) const;
};

class OverpassDataFetcher {
public:
    /**
     * Fetch Overpass data for a given bounding box
     * @param boundingBox BoundingBox object defining the area
     * @return cpr::Response containing the fetched data
     * @throws std::invalid_argument if bounding box is invalid
     */
    static cpr::Response fetchOverpassData(const BoundingBox& boundingBox);

    /**
     * Get the list of excluded highway types
     * @return Vector of excluded highway types
     */
    static const std::vector<std::string>& getExcludedHighways() { 
        return highWayExclude; 
    }

private:
    static const std::vector<std::string> highWayExclude;
    static std::string constructOverpassQuery(const BoundingBox& boundingBox);
};

class BoundingBoxGenerator {
public:
    /**
     * Constructor that initializes the generator with start and end points
     * @param start JSON object containing start point {lat: double, lon: double}
     * @param end JSON object containing end point {lat: double, lon: double}
     * @throws std::invalid_argument if input JSON is invalid
     */
    BoundingBoxGenerator(const json& start, const json& end);

    /**
     * Generate a bounding box based on start and end points
     * @return BoundingBox object containing the calculated bounds
     */
    BoundingBox getBoundingBox() const;

private:
    GeoPoint start_point;
    GeoPoint end_point;
    static constexpr double MIN_PADDING_KM = 0.5;
    static constexpr double MAX_PADDING_KM = 5.0;
    static constexpr double PADDING_RATIO = 0.15;

    double calculatePadding() const;
    std::vector<GeoPoint> generatePaddingPoints(double padding) const;
};

#endif // OVERPASS_DATA_FETCHER_HPP