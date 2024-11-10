import { useEffect, useRef, useState, useCallback } from "react";
import DeckGL from "@deck.gl/react";
import { Map as MapGL } from "react-map-gl";
import maplibregl from "maplibre-gl";
import { PolygonLayer, ScatterplotLayer } from "@deck.gl/layers";
import { FlyToInterpolator } from "deck.gl";

import { createGeoJSONCircle } from "../utils/helpers";
import {
  getBoundingBoxFromPolygon,
  getNearestNode,
} from "../services/MapService";
import { spawnDrivers } from "../services/spawnDrivers";
import createDriverLayer from "./DriverLayer";
import Controller from "./Controller";
import PathLayer from "./PathLayer";
import useSmoothStateChange from "../hooks/useSmoothStateChange";
import {
  INITIAL_COLORS,
  MAP_STYLE,
  INITIAL_LOCATION,
  INITIAL_RADIUS,
} from "../config";

// Custom hooks
const useMapAnimation = (pathData, speed = 0.5) => {
  const [timestamp, setTimestamp] = useState(0);
  const animationFrameId = useRef(null);
  const startTimeRef = useRef(null);

  useEffect(() => {
    if (!pathData?.length) {
      if (animationFrameId.current) {
        cancelAnimationFrame(animationFrameId.current);
        animationFrameId.current = null;
      }
      return;
    }

    const animate = (currentTime) => {
      if (!startTimeRef.current) startTimeRef.current = currentTime;
      const elapsedTime = currentTime - startTimeRef.current;
      
      // Calculate timestamp based on elapsed time and speed
      const newTimestamp = (elapsedTime * speed) / 1000;
      setTimestamp(newTimestamp);
      
      animationFrameId.current = requestAnimationFrame(animate);
    };

    animationFrameId.current = requestAnimationFrame(animate);

    return () => {
      if (animationFrameId.current) {
        cancelAnimationFrame(animationFrameId.current);
      }
      startTimeRef.current = null;
    };
  }, [pathData, speed]);

  return timestamp;
};

const useGeolocation = (changeLocation) => {
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
        }
      );
    } else {
      console.error("Geolocation is not supported by this browser.");
    }
  }, [changeLocation]);
};

export default function Map() {
  // State management
  const [viewState, setViewState] = useState(INITIAL_LOCATION);
  const [startNode, setStartNode] = useState(null);
  const [endNode, setEndNode] = useState(null);
  const [pathData, setPathData] = useState([]);
  const [selectionRadius, setSelectionRadius] = useState([]);
  const [loading, setLoading] = useState(false);
  const [drivers, setDrivers] = useState([]);
  const [fadeRadiusReverse, setFadeRadiusReverse] = useState(false);
  const [hoveredDriver, setHoveredDriver] = useState(null);
  const [colors] = useState(INITIAL_COLORS);
  const [boundingBox, setBoundingBox] = useState(null);
  const [started, setStarted] = useState(false);
  const [animationEnded, setAnimationEnded] = useState(false);
  const [playbackOn, setPlaybackOn] = useState(false);
  const [settings, setSettings] = useState({
    algorithm: "astar",
    radius: INITIAL_RADIUS,
    speed: 5,
  });

  // Refs
  const fadeRadius = useRef();
  const timer = useRef(0);

  // Custom hooks
  const timestamp = useMapAnimation(pathData, settings.speed);
  const selectionRadiusOpacity = useSmoothStateChange(
    0,
    0,
    1,
    400,
    fadeRadius.current,
    fadeRadiusReverse
  );

  // Handlers
  const changeLocation = useCallback(
    (location) => {
      setViewState({
        ...viewState,
        longitude: location.longitude,
        latitude: location.latitude,
        zoom: 14.5,
        transitionDuration: 1000,
        transitionInterpolator: new FlyToInterpolator(),
      });
    },
    [viewState]
  );

  useGeolocation(changeLocation);

  const clearPath = useCallback(() => {
    setStarted(false);
    setEndNode(null);
    setDrivers([]);
    setPathData([]);
    timer.current = 0;
    setAnimationEnded(false);
  }, []);

  const handlePathResponse = useCallback(async (data) => {
    if (data.status === "success" && data.path) {
      setPathData(data.path);
      setStarted(true);
    }
  }, []);

  const mapClick = useCallback(async (e, info, radius = null) => {
    // if (started && !animationEnded) return;

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

      const loadingHandle = setTimeout(() => setLoading(true), 300);

      try {
        const node = await getNearestNode(e.coordinate[1], e.coordinate[0]);
        if (!node) {
          throw new Error("No path found in vicinity");
        }
        
        setEndNode(node);

        if (boundingBox && startNode && node) {
          const data = {
            "start-node": startNode,
            "end-node": node,
            "bounding-box": boundingBox,
          };

          const response = await fetch("http://localhost:8080/direct-path", {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify(data),
          });
          
          const responseData = await response.json();
          await handlePathResponse(responseData);
        }
      } catch (error) {
        console.error("Error:", error);
        alert(error.message || "An error occurred. Please try another location.");
      } finally {
        clearTimeout(loadingHandle);
        setLoading(false);
      }
      return;
    }

    const loadingHandle = setTimeout(() => setLoading(true), 300);

    try {
      const node = await getNearestNode(e.coordinate[1], e.coordinate[0]);
      if (!node) {
        throw new Error("No path found in vicinity");
      }

      if (node === startNode && drivers.length > 0) {
        return;
      }

      const newDrivers = await spawnDrivers(node.lat, node.lon, { count: 8 });
      setDrivers(newDrivers);
      setStartNode(node);
      setEndNode(null);

      const circle = createGeoJSONCircle(
        [node.lon, node.lat],
        radius || settings.radius
      );
      setSelectionRadius([{ contour: circle }]);
      setBoundingBox(getBoundingBoxFromPolygon(circle));
    } catch (error) {
      console.error("Error:", error);
      alert(error.message || "An error occurred. Please try another location.");
    } finally {
      clearTimeout(loadingHandle);
      setLoading(false);
    }
  }, [loading, boundingBox, startNode, drivers.length, clearPath, handlePathResponse, settings.radius, started, animationEnded]);

  // Layer generation
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
    PathLayer({ pathData, colors, timestamp }),
    new ScatterplotLayer({
      id: "start-end-points",
      data: [
        ...(startNode ? [{
          coordinates: [startNode.lon, startNode.lat],
          color: colors.startNodeFill,
          lineColor: colors.startNodeBorder,
        }] : []),
        ...(endNode ? [{
          coordinates: [endNode.lon, endNode.lat],
          color: colors.endNodeFill,
          lineColor: colors.endNodeBorder,
        }] : []),
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
  ];

  return (
    <>
      <div onContextMenu={(e) => e.preventDefault()}>
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
      <Controller
        started={started}
        animationEnded={animationEnded}
        playbackOn={playbackOn}
        settings={settings}
        setSettings={setSettings}
      />
    </>
  );
}