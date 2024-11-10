#include "api.hpp"
#include <sstream>
#include <algorithm>
#include <stdexcept>

const std::vector<std::string> OverpassDataFetcher::highWayExclude = {
    "footway", "street_lamp", "steps", "pedestrian", "track", "path"
};

// GeoPoint implementations
double GeoPoint::calculateDistance(const GeoPoint& p1, const GeoPoint& p2) {
    const double R = 6371.0; // Earth's radius in kilometers
    const double dLat = (p2.lat - p1.lat) * M_PI / 180.0;
    const double dLon = (p2.lon - p1.lon) * M_PI / 180.0;
    const double lat1 = p1.lat * M_PI / 180.0;
    const double lat2 = p2.lat * M_PI / 180.0;

    const double a = std::sin(dLat/2) * std::sin(dLat/2) +
                    std::sin(dLon/2) * std::sin(dLon/2) * 
                    std::cos(lat1) * std::cos(lat2);
    const double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1-a));
    return R * c;
}

double GeoPoint::calculateBearing(const GeoPoint& p1, const GeoPoint& p2) {
    const double dLon = (p2.lon - p1.lon) * M_PI / 180.0;
    const double lat1 = p1.lat * M_PI / 180.0;
    const double lat2 = p2.lat * M_PI / 180.0;
    
    const double y = std::sin(dLon) * std::cos(lat2);
    const double x = std::cos(lat1) * std::sin(lat2) -
                    std::sin(lat1) * std::cos(lat2) * std::cos(dLon);
    return std::atan2(y, x);
}

GeoPoint GeoPoint::destinationPoint(double distance, double bearing) const {
    const double R = 6371.0; // Earth's radius in kilometers
    const double d = distance / R;
    const double lat1 = lat * M_PI / 180.0;
    const double lon1 = lon * M_PI / 180.0;

    const double lat2 = std::asin(std::sin(lat1) * std::cos(d) +
                                 std::cos(lat1) * std::sin(d) * std::cos(bearing));
    const double lon2 = lon1 + std::atan2(std::sin(bearing) * std::sin(d) * std::cos(lat1),
                                        std::cos(d) - std::sin(lat1) * std::sin(lat2));

    return GeoPoint{
        lat2 * 180.0 / M_PI,
        lon2 * 180.0 / M_PI
    };
}

// OverpassDataFetcher implementations
std::string OverpassDataFetcher::constructOverpassQuery(const BoundingBox& boundingBox) {

    std::stringstream exclusionStream;
    for (const auto& exclude : highWayExclude) {
        exclusionStream << "[highway!=\"" << exclude << "\"]";
    }

    std::stringstream queryStream;
    queryStream << "[out:json];("
                << "way[highway]" << exclusionStream.str() << "[footway!=\"*\"]"
                << "(" 
                << boundingBox.min_lat << "," 
                << boundingBox.min_lon << "," 
                << boundingBox.max_lat << "," 
                << boundingBox.max_lon 
                << ");"
                << "node(w);"
                << ");out skel;";

    return queryStream.str();
}

cpr::Response OverpassDataFetcher::fetchOverpassData(const BoundingBox& boundingBox) {
    std::string query = constructOverpassQuery(boundingBox);
    
    return cpr::Post(
        cpr::Url{"https://overpass-api.de/api/interpreter"},
        cpr::Body{query},
        cpr::Timeout{30000} // 30 second timeout
    );
}

// BoundingBoxGenerator implementations
BoundingBoxGenerator::BoundingBoxGenerator(const json& start, const json& end) {
    try {
        start_point = {start.at("lat").get<double>(), start.at("lon").get<double>()};
        end_point = {end.at("lat").get<double>(), end.at("lon").get<double>()};
        
        if (start_point.lat < -90 || start_point.lat > 90 ||
            end_point.lat < -90 || end_point.lat > 90 ||
            start_point.lon < -180 || start_point.lon > 180 ||
            end_point.lon < -180 || end_point.lon > 180) {
            throw std::invalid_argument("Coordinates out of valid range");
        }
    } catch (const json::exception& e) {
        throw std::invalid_argument("Invalid JSON format for coordinates");
    }
}

double BoundingBoxGenerator::calculatePadding() const {
    double routeDistance = GeoPoint::calculateDistance(start_point, end_point);
    return std::clamp(
        routeDistance * PADDING_RATIO,
        MIN_PADDING_KM,
        MAX_PADDING_KM
    );
}

std::vector<GeoPoint> BoundingBoxGenerator::generatePaddingPoints(double padding) const {
    std::vector<GeoPoint> points;
    points.reserve(8);

    double bearing = GeoPoint::calculateBearing(start_point, end_point);
    double perpBearing1 = bearing + M_PI/2;
    double perpBearing2 = bearing - M_PI/2;

    // Start point padding
    points.push_back(start_point.destinationPoint(padding, perpBearing1));
    points.push_back(start_point.destinationPoint(padding, perpBearing2));
    points.push_back(start_point.destinationPoint(padding, bearing - M_PI));

    // End point padding
    points.push_back(end_point.destinationPoint(padding, perpBearing1));
    points.push_back(end_point.destinationPoint(padding, perpBearing2));
    points.push_back(end_point.destinationPoint(padding, bearing));

    // Midpoint padding for long routes
    double routeDistance = GeoPoint::calculateDistance(start_point, end_point);
    if (routeDistance > 10.0) {
        GeoPoint midpoint{
            (start_point.lat + end_point.lat) / 2.0,
            (start_point.lon + end_point.lon) / 2.0
        };
        points.push_back(midpoint.destinationPoint(padding, perpBearing1));
        points.push_back(midpoint.destinationPoint(padding, perpBearing2));
    }

    return points;
}

BoundingBox BoundingBoxGenerator::getBoundingBox() const {
    double padding = calculatePadding();
    auto paddingPoints = generatePaddingPoints(padding);

    BoundingBox bbox{
        paddingPoints[0].lat,
        paddingPoints[0].lon,
        paddingPoints[0].lat,
        paddingPoints[0].lon
    };

    for (const auto& point : paddingPoints) {
        bbox.min_lat = std::min(bbox.min_lat, point.lat);
        bbox.max_lat = std::max(bbox.max_lat, point.lat);
        bbox.min_lon = std::min(bbox.min_lon, point.lon);
        bbox.max_lon = std::max(bbox.max_lon, point.lon);
    }

    return bbox;
}