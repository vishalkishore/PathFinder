import { fetchOverpassData } from "../utils/api";
import { createGeoJSONCircle } from "../utils/helpers";

/**
 * Distribution types for node selection
 * @readonly
 * @enum {string}
 */
export const DistributionType = {
  UNIFORM: "uniform",
  GAUSSIAN: "gaussian",
  DISTANCE_WEIGHTED: "distance_weighted",
  DENSITY_BASED: "density_based",
};

/**
 * @typedef {Object} SelectionOptions
 * @property {DistributionType} distribution - Type of distribution to use
 * @property {number} [count=1] - Number of nodes to select
 * @property {Object} [distributionParams] - Parameters for the distribution
 * @property {string[]} [requiredTags] - OSM tags that nodes must have
 * @property {boolean} [allowDuplicates=false] - Whether to allow duplicate selections
 */

/**
 * Calculate Haversine distance between two points
 * @param {number} lat1 First point latitude
 * @param {number} lon1 First point longitude
 * @param {number} lat2 Second point latitude
 * @param {number} lon2 Second point longitude
 * @returns {number} Distance in kilometers
 */
function calculateDistance(lat1, lon1, lat2, lon2) {
  const R = 6371; // Earth's radius in km
  const dLat = ((lat2 - lat1) * Math.PI) / 180;
  const dLon = ((lon2 - lon1) * Math.PI) / 180;
  const a =
    Math.sin(dLat / 2) * Math.sin(dLat / 2) +
    Math.cos((lat1 * Math.PI) / 180) *
      Math.cos((lat2 * Math.PI) / 180) *
      Math.sin(dLon / 2) *
      Math.sin(dLon / 2);
  const c = 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1 - a));
  return R * c;
}

/**
 * Calculate Gaussian weight
 * @param {number} distance Distance from center in km
 * @param {number} sigma Standard deviation parameter
 * @returns {number} Weight between 0 and 1
 */
function calculateGaussianWeight(distance, sigma = 0.05) {
  return Math.exp(-(distance * distance) / (2 * sigma * sigma));
}

/**
 * Calculate linear distance weight
 * @param {number} distance Distance from center in km
 * @param {number} maxDistance Maximum distance in km
 * @returns {number} Weight between 0 and 1
 */
function calculateDistanceWeight(distance, maxDistance) {
  return Math.max(0, 1 - distance / maxDistance);
}

/**
 * Calculate node density weight
 * @param {Object} node Current node
 * @param {Array} allNodes All available nodes
 * @param {number} radius Radius to consider for density
 * @returns {number} Density-based weight
 */
function calculateDensityWeight(node, allNodes, radius = 0.01) {
  const nearbyNodes = allNodes.filter(
    (other) =>
      calculateDistance(node.lat, node.lon, other.lat, other.lon) <= radius,
  );
  return nearbyNodes.length / allNodes.length;
}

/**
 * Get bounding box from circle coordinates
 * @param {Number[][]} circleCoords Array of [lon, lat] coordinates
 * @returns {{latitude: number, longitude: number}[]} Array with min and max points
 */
function getBoundingBoxFromCircle(circleCoords) {
  let minLat = Infinity,
    maxLat = -Infinity;
  let minLon = Infinity,
    maxLon = -Infinity;

  circleCoords.forEach(([lon, lat]) => {
    minLat = Math.min(minLat, lat);
    maxLat = Math.max(maxLat, lat);
    minLon = Math.min(minLon, lon);
    maxLon = Math.max(maxLon, lon);
  });

  return [
    { latitude: minLat, longitude: minLon },
    { latitude: maxLat, longitude: maxLon },
  ];
}

/**
 * Select nodes based on distribution
 * @param {Array} nodes Available nodes
 * @param {SelectionOptions} options Selection options
 * @param {number} centerLat Center latitude
 * @param {number} centerLon Center longitude
 * @returns {Array} Selected nodes
 */
function selectNodes(nodes, options, centerLat, centerLon) {
  const {
    distribution = DistributionType.UNIFORM,
    count = 1,
    distributionParams = {},
    allowDuplicates = false,
  } = options;

  // Calculate weights based on distribution type
  const weights = nodes.map((node) => {
    const distance = calculateDistance(
      node.lat,
      node.lon,
      centerLat,
      centerLon,
    );

    switch (distribution) {
      case DistributionType.GAUSSIAN:
        return calculateGaussianWeight(distance, distributionParams.sigma);
      case DistributionType.DISTANCE_WEIGHTED:
        return calculateDistanceWeight(
          distance,
          distributionParams.maxDistance || 0.15,
        );
      case DistributionType.DENSITY_BASED:
        return calculateDensityWeight(node, nodes, distributionParams.radius);
      default: // UNIFORM
        return 1;
    }
  });

  // Normalize weights
  const totalWeight = weights.reduce((sum, w) => sum + w, 0);
  const normalizedWeights = weights.map((w) => w / totalWeight);

  // Select nodes
  const selected = [];
  const availableIndices = new Set(nodes.map((_, i) => i));

  while (selected.length < count && availableIndices.size > 0) {
    let random = Math.random();
    let cumulativeWeight = 0;
    let selectedIndex = -1;

    for (const index of availableIndices) {
      cumulativeWeight += normalizedWeights[index];
      if (random <= cumulativeWeight) {
        selectedIndex = index;
        break;
      }
    }

    if (selectedIndex === -1) {
      selectedIndex = Array.from(availableIndices)[0];
    }

    selected.push(nodes[selectedIndex]);
    if (!allowDuplicates) {
      availableIndices.delete(selectedIndex);
    }
  }

  return selected;
}

/**
 * Updates driver positions with movement simulation
 * @param {Array} drivers Current array of drivers
 * @returns {Array} Updated array of drivers with new positions
 */
export function updateDriverPositions(drivers) {
  return drivers.map(driver => {
    // Generate small random movement
    const speedFactor = 0.0001; // Adjust this to control movement speed
    const bearingChange = (Math.random() - 0.5) * 10; // Random bearing change (-5 to 5 degrees)
    
    // Update bearing
    const newBearing = ((driver.bearing || 0) + bearingChange) % 360;
    
    // Calculate new position based on bearing
    const bearingRad = newBearing * (Math.PI / 180);
    const lonChange = Math.sin(bearingRad) * speedFactor;
    const latChange = Math.cos(bearingRad) * speedFactor;
    
    return {
      ...driver,
      lon: driver.lon + lonChange,
      lat: driver.lat + latChange,
      bearing: newBearing
    };
  });
}

/**
 * Spawn drivers at random locations using various distributions
 * @param {number} latitude Center latitude
 * @param {number} longitude Center longitude
 * @param {SelectionOptions} options Selection options
 * @returns {Promise<Array>} Selected nodes
 */
export async function spawnDrivers(latitude, longitude, options = {}) {
  try {
    // Create circle coordinates
    const circleCoords = createGeoJSONCircle([longitude, latitude], 0.15);
    const boundingBox = getBoundingBoxFromCircle(circleCoords);

    // Fetch OSM data
    const response = await fetchOverpassData(boundingBox);
    const data = await response.json();

    if (!data || !data.elements) {
      throw new Error("Invalid response from Overpass API");
    }

    // Filter nodes based on tags and location
    const validNodes = data.elements.filter((node) => {
      if (
        !node ||
        typeof node.lat !== "number" ||
        typeof node.lon !== "number"
      ) {
        return false;
      }

      // Check required tags
      if (options.requiredTags) {
        const hasAllTags = options.requiredTags.every((tag) => {
          const [key, value] = tag.split("=");
          return value ? node.tags?.[key] === value : node.tags?.[key];
        });
        if (!hasAllTags) return false;
      }

      // Check if within circle
      const distance = calculateDistance(
        node.lat,
        node.lon,
        latitude,
        longitude,
      );
      return distance <= 1;
    });

    // Select nodes based on distribution
    return selectNodes(validNodes, options, latitude, longitude);
  } catch (error) {
    console.error("Error in spawnDrivers:", error);
    throw error;
  }
}
