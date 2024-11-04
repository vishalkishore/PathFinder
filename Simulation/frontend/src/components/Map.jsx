import DeckGL from "@deck.gl/react";
import { Map as MapGL } from "react-map-gl";
import maplibregl from "maplibre-gl";
import { PolygonLayer, ScatterplotLayer } from "@deck.gl/layers";
import { FlyToInterpolator } from "deck.gl";
import { createGeoJSONCircle } from "../utils/helpers";
import {
  getBoundingBoxFromPolygon,
  getMapGraph,
  getNearestNode,
} from "../services/MapService";
import { spawnDrivers, DistributionType } from "../services/spawnDrivers";
import { createDriverLayer } from "./DriverLayer";
import PathLayer from "./PathLayer";
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
  const [lastUpdateTime, setLastUpdateTime] = useState(Date.now());
  const [timestamp, setTimestamp] = useState(0);
  const [pathData, setPathData] = useState([]);
  const animationFrameId = useRef(null);
  const startTimeRef = useRef(null);
  const timestampRef = useRef(0);
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
    clearPath();
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
        alert(
          "No path was found in the vicinity, please try another location.",
        );
        clearTimeout(loadingHandle);
        setLoading(false);
        return;
      }
      setEndNode(node);

      if (boundingBox && startNode && node) {
        let data = JSON.stringify({
          "start-node": startNode,
          "end-node": node,
          "bounding-box": boundingBox,
        });
        console.log(data);
        // Send POST request with bounding box data
        fetch("http://localhost:8080/direct-path", {
          method: "POST",
          headers: {
            "Content-Type": "application/json",
          },
          body: data,
        })
          .then((response) => response.json())
          .then(handlePathResponse)
          .catch((error) => console.error("Error:", error));
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

    if (node == startNode && drivers.length > 0) {
      clearTimeout(loadingHandle);
      setEndNode(null);
      setLoading(false);
      return;
    }

    const newDrivers = await spawnDrivers(node.lat, node.lon, { count: 8 });
    console.log("Drivers spawned:", newDrivers);
    setDrivers((prev) => [...newDrivers]);

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

  useEffect(() => {
    if (!pathData || pathData.length === 0) {
      if (animationFrameId.current) {
        cancelAnimationFrame(animationFrameId.current);
        animationFrameId.current = null;
      }
      return;
    }

    const loopLength = pathData.length;
    const animationSpeed = 0.5;
    startTimeRef.current = performance.now();

    const animate = (currentTime) => {
      const elapsedTime = (currentTime - startTimeRef.current) * animationSpeed;
      const loopTime = (elapsedTime / 1000) % loopLength;

      // Update ref instead of state for internal animation timing
      timestampRef.current = loopTime;
      // Only update state once per frame for rendering
      setTimestamp(loopTime);

      animationFrameId.current = requestAnimationFrame(animate);
    };

    animationFrameId.current = requestAnimationFrame(animate);

    return () => {
      if (animationFrameId.current) {
        cancelAnimationFrame(animationFrameId.current);
        animationFrameId.current = null;
      }
      startTimeRef.current = null;
      timestampRef.current = 0;
    };
  }, [pathData]);

  function clearPath() {
    setEndNode(null);
    setDrivers([]);
    setPathData([]);
    timestampRef.current = 0;
  }

  const handlePathResponse = (data) => {
    if (data.status === "success") {
      setPathData(data.path || []);
      // Reset animation timing
      timestampRef.current = 0;
      if (animationFrameId.current) {
        cancelAnimationFrame(animationFrameId.current);
        animationFrameId.current = null;
      }
      startTimeRef.current = null;
    }
  };

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
    ...(pathData && pathData.length > 0
      ? [
          PathLayer({
            pathData,
            colors,
            timestamp: timestamp,
          }),
        ]
      : []),
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
              getAngle: (d) => {
                const angle = Number(d.angle) || 0;
                return angle;
              },
              transitions: {
                getPosition: 120,
                getAngle: 120,
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
              position: "absolute",
              zIndex: 1,
              pointerEvents: "none",
              left: hoveredDriver.x + 10,
              top: hoveredDriver.y + 10,
              backgroundColor: "white",
              padding: "8px",
              borderRadius: "4px",
              boxShadow: "0 2px 4px rgba(0,0,0,0.2)",
            }}
          >
            <p>Driver ID: {hoveredDriver.id || "N/A"}</p>
            <p>
              Location:{" "}
              {hoveredDriver.coordinates?.map((c) => c.toFixed(4)).join(", ")}
            </p>
          </div>
        )}
      </div>
      <Controller />
    </>
  );
}
