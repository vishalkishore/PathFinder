import { IconLayer } from "@deck.gl/layers";
import carIcon from "../assets/car.png";

const ICON_MAPPING = {
  marker: { x: 0, y: 0, width: 128, height: 59 },
};

/**
 * Creates a deck.gl IconLayer for drivers with car icons
 * @param {Object} params Configuration object
 * @param {Array} params.drivers Array of driver nodes from spawnDrivers
 * @param {Object} params.colors Color configuration object
 * @param {Object} params.options Additional layer options
 * @returns {IconLayer} deck.gl IconLayer
 */
export function createDriverLayer({ drivers = [], colors = {}, options = {} }) {
  // Transform driver data to match icon format
  const driverPoints = drivers.map((driver) => ({
    coordinates: [driver.lon, driver.lat],
    id: driver.id,
    angle: driver.bearing || 0, // Default to 0 if bearing not provided
    color: colors.driverNodeFill || [0, 150, 255],
  }));
  console.log("Driver layer", driverPoints);
  return new IconLayer({
    id: "driver-icons",
    data: driverPoints,
    pickable: true,
    iconAtlas: carIcon,
    iconMapping: ICON_MAPPING,
    getIcon: (d) => "marker",
    sizeScale: 5,
    getPosition: (d) => d.coordinates,
    getSize: (d) => 5,
    getColor: (d) => d.color,
    ...options,
  });
}
