import { useState } from "react";
import "./App.css";
import Map from "./components/Map";
function App() {
  const [count, setCount] = useState(0);

  return (
    <>
      <h1 className="z-10 absolute font-semibold decoration-sky-500">
        True Map
      </h1>
      <Map />
    </>
  );
}

export default App;
