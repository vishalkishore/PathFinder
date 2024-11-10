import { TripsLayer } from "@deck.gl/geo-layers";

const PathLayer = ({ pathData, colors, timestamp }) => {
  if (!pathData || pathData.length === 0) return null;

  const trips = [
    {
      vendor: 1,
      path: pathData.map((point, index) => [point.lon, point.lat, 0, index]),
    },
  ];

  return new TripsLayer({
    id: "path-layer",
    data: trips,
    getPath: (d) => d.path,
    getColor: colors.pathColor || [253, 128, 93],
    opacity: 0.8,
    widthMinPixels: 5,
    fadeTrail: false,
    capRounded: true,
    jointRounded: true,
    trailLength: 0.8,
    currentTime: timestamp,
    shadowEnabled: false,
    getTimestamps: (d) => d.path.map((p) => 0.1 * p[3]),
  });
};

export default PathLayer;