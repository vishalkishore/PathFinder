import DeckGL from "@deck.gl/react";
import { Map as MapGL } from "react-map-gl";
import maplibregl from "maplibre-gl";
import { PolygonLayer, ScatterplotLayer } from "@deck.gl/layers";
import { FlyToInterpolator } from "deck.gl";
import { TripsLayer } from "@deck.gl/geo-layers";
import { createGeoJSONCircle } from "../utils/helpers";
import {
  getBoundingBoxFromPolygon,
  getMapGraph,
  getNearestNode,
} from "../services/MapService";
import { spawnDrivers, DistributionType } from "../services/spawnDrivers";
import { createDriverLayer } from "./DriverLayer";
import { useEffect, useRef, useState } from "react";
import Controller from "./Controller";
import useSmoothStateChange from "../hooks/useSmoothStateChange";
import { fetchOverpassData } from "../utils/api";
import {
  INITIAL_COLORS,
  MAP_STYLE,
  INITIAL_LOCATION,
  INITIAL_RADIUS,
} from "../config";

export default function Map() {
  const [viewState, setViewState] = useState(INITIAL_LOCATION);
  const [startNode, setStartNode] = useState(null);
  const [endNode, setEndNode] = useState(null);
  const [selectionRadius, setSelectionRadius] = useState([]);
  const [loading, setLoading] = useState(false);
  const [drivers, setDrivers] = useState([]);
  const [fadeRadiusReverse, setFadeRadiusReverse] = useState(false);
  const [hoveredDriver, setHoveredDriver] = useState(null);
  const fadeRadius = useRef();
  const ui = useRef();
  const selectionRadiusOpacity = useSmoothStateChange(
    0,
    0,
    1,
    400,
    fadeRadius.current,
    fadeRadiusReverse,
  );
  const [colors, setColors] = useState(INITIAL_COLORS);
  const [boundingBox, setBoundingBox] = useState(null);

  async function mapClick(e, info, radius = null) {
    setFadeRadiusReverse(false);
    fadeRadius.current = true;

    if (info.rightButton) {
      if (e.layer?.id !== "selection-radius") {
        alert("Please select a point inside the radius.");
        return;
      }

      if (loading) {
        alert("Please wait for all data to load.");
        return;
      }

      const loadingHandle = setTimeout(() => {
        setLoading(true);
      }, 300);

      const node = await getNearestNode(e.coordinate[1], e.coordinate[0]);
      if (!node) {
        alert("No path was found in the vicinity, please try another location.");
        clearTimeout(loadingHandle);
        setLoading(false);
        return;
      }
      setEndNode(node);

      if (boundingBox) {
        const response = await fetchOverpassData(boundingBox, false);
        const data = await response.json();
        console.log(data);
      }

      clearTimeout(loadingHandle);
      setLoading(false);
      return;
    }

    const loadingHandle = setTimeout(() => {
      setLoading(true);
    }, 300);

    // Fetch nearest node and spawn drivers
    const node = await getNearestNode(e.coordinate[1], e.coordinate[0]);
    if (!node) {
      alert("No path was found in the vicinity, please try another location.");
      clearTimeout(loadingHandle);
      setLoading(false);
      return;
    }
    
    setDrivers([]);
    const newDrivers = await spawnDrivers(node.lat, node.lon, { count: 8 });
    setDrivers(prev => [...prev, ...newDrivers]);

    setStartNode(node);
    setEndNode(null);

    const circle = createGeoJSONCircle(
      [node.lon, node.lat],
      radius || INITIAL_RADIUS,
    );
    setSelectionRadius([{ contour: circle }]);
    setBoundingBox(getBoundingBoxFromPolygon(circle));
    clearTimeout(loadingHandle);
    setLoading(false);
  }

  function changeLocation(location) {
    setViewState({
      ...viewState,
      longitude: location.longitude,
      latitude: location.latitude,
      zoom: 14.5,
      transitionDuration: 1000,
      transitionInterpolator: new FlyToInterpolator(),
    });
  }

  useEffect(() => {
    if ("geolocation" in navigator) {
      navigator.geolocation.getCurrentPosition(
        (res) => {
          changeLocation(res.coords);
        },
        (err) => {
          console.error("Error getting location: ", err.message);
        },
        {
          enableHighAccuracy: true,
          timeout: 10000,
          maximumAge: 0,
        },
      );
    } else {
      console.error("Geolocation is not supported by this browser.");
    }
  }, []);

  // Generate layers array including drivers when available
  const layers = [
    new PolygonLayer({
      id: "selection-radius",
      data: selectionRadius,
      pickable: true,
      stroked: true,
      getPolygon: (d) => d.contour,
      getFillColor: [80, 210, 0, 10],
      getLineColor: [9, 142, 46, 175],
      getLineWidth: 3,
      opacity: selectionRadiusOpacity,
    }),
    new ScatterplotLayer({
      id: "start-end-points",
      data: [
        ...(startNode
          ? [
              {
                coordinates: [startNode.lon, startNode.lat],
                color: colors.startNodeFill,
                lineColor: colors.startNodeBorder,
              },
            ]
          : []),
        ...(endNode
          ? [
              {
                coordinates: [endNode.lon, endNode.lat],
                color: colors.endNodeFill,
                lineColor: colors.endNodeBorder,
              },
            ]
          : []),
      ],
      pickable: true,
      opacity: 1,
      stroked: true,
      filled: true,
      radiusScale: 1,
      radiusMinPixels: 7,
      radiusMaxPixels: 20,
      lineWidthMinPixels: 1,
      lineWidthMaxPixels: 3,
      getPosition: (d) => d.coordinates,
      getFillColor: (d) => d.color,
      getLineColor: (d) => d.lineColor,
    }),
    // Only add driver layer when there are drivers
    ...(drivers.length > 0
      ? [
          createDriverLayer({
            drivers,
            colors,
            options: {
              onClick: (info) => {
                if (info.object) {
                  console.log("Selected driver:", info.object);
                }
              },
              onHover: (info) => {
                setHoveredDriver(info.object || null);
              },
            },
          }),
        ]
      : []),
  ];

  return (
    <>
      <div
        onContextMenu={(e) => {
          e.preventDefault();
        }}
      >
        <DeckGL
          layers={layers}
          initialViewState={viewState}
          controller={{ doubleClickZoom: false, keyboard: false }}
          onClick={mapClick}
        >
          <MapGL
            reuseMaps
            mapLib={maplibregl}
            mapStyle={MAP_STYLE}
            doubleClickZoom={false}
          />
        </DeckGL>

        {/* Hover tooltip for drivers */}
        {hoveredDriver && (
          <div
            style={{
              position: 'absolute',
              zIndex: 1,
              pointerEvents: 'none',
              left: hoveredDriver.x + 10,
              top: hoveredDriver.y + 10,
              backgroundColor: 'white',
              padding: '8px',
              borderRadius: '4px',
              boxShadow: '0 2px 4px rgba(0,0,0,0.2)'
            }}
          >
            <p>Driver ID: {hoveredDriver.id || 'N/A'}</p>
            <p>Location: {hoveredDriver.coordinates?.map(c => c.toFixed(4)).join(', ')}</p>
          </div>
        )}
      </div>
      <Controller />
    </>
  );
}