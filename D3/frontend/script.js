// Adjacency matrix
let adjMatrix = [
  [0, 2, 0, 0],
  [2, 0, 1, 4],
  [0, 1, 0, 1],
  [0, 4, 1, 0]
];

document.getElementById('submitBtn1').addEventListener('click', function() {
  const text = document.getElementById('myTextArea').value;
  try {
    const matrix = JSON.parse(text);
    if (!Array.isArray(matrix) || matrix.length === 0 || !matrix.every(Array.isArray)) {
      throw new Error("Input is not a valid 2D array.");
    }
    const numRows = matrix.length;
    if (!matrix.every(row => row.length === numRows)) {
      throw new Error("Matrix is not square.");
    }
    for (let i = 0; i < numRows; i++) {
      for (let j = 0; j < numRows; j++) {
        if (matrix[i][j] !== matrix[j][i]) {
          throw new Error("Matrix is not symmetric, hence not an undirected graph.");
        }
      }
    }
    console.log(matrix);
    alert("The input represents a valid adjacency matrix for an undirected graph.");
    generateGraphFromMatrix(matrix);
  } catch (error) {
    alert("Error: " + error.message);
  }
});

function generateRandomAdjacencyMatrix(a, b) {
  const numNodes = Math.floor(Math.random() * (b - a + 1)) + a;
  const adjMatrix = Array.from({ length: numNodes }, () => Array(numNodes).fill(0));
  const percentage = 0.30;
  for (let i = 0; i < numNodes; i++) {
    const maxConnections = Math.floor(percentage * numNodes);
    const numConnections = Math.floor(Math.random() * maxConnections) + 1; // At least 1 connection
    const connectedNodes = new Set();
    while (connectedNodes.size < numConnections) {
      const randomNode = Math.floor(Math.random() * numNodes);
      if (randomNode !== i && !connectedNodes.has(randomNode)) {
        const weight = Math.floor(Math.random() * 10) + 1; // Random weight between 1 and 10
        adjMatrix[i][randomNode] = weight;
        adjMatrix[randomNode][i] = weight; // Undirected graph, set both directions
        connectedNodes.add(randomNode);
      }
    }
  }

  return adjMatrix;
}


// Create the SVG canvas
const svg = d3.select("svg"),
  width = +svg.attr("width"),
  height = +svg.attr("height");


function drawGraph(svg, adjMatrix) {
  svg.selectAll("*").remove();
  svg.node().innerHTML = '';
  const nodes = adjMatrix.map((_, i) => ({ id: i }));
  const links = [];
  for (let i = 0; i < adjMatrix.length; i++) {
    for (let j = i + 1; j < adjMatrix[i].length; j++) {
      if (adjMatrix[i][j] !== 0) {
        links.push({
          source: i,
          target: j,
          weight: adjMatrix[i][j],
          above: 0,
        });
        links.push({
          source: i,
          target: j,
          weight: adjMatrix[i][j],
          above: 1,
        });
        links.push({
          source: j,
          target: i,
          weight: adjMatrix[i][j],
          above: 0,
        });
        links.push({
          source: j,
          target: i,
          weight: adjMatrix[i][j],
          above: 1,
        });
      }
    }
  }
  const simulation = d3.forceSimulation(nodes)
    .force("link", d3.forceLink(links).id(d => d.id).distance(150))
    .force("charge", d3.forceManyBody().strength(-500))
    .force("center", d3.forceCenter(width / 2, height / 2));

  // Draw the links (edges)
  const link = svg.append("g")
    .selectAll("line")
    .data(links)
    .enter().append("line")
    .attr("stroke-width", 2)
    .attr("stroke", "#999")

  // Draw the nodes (vertices)
  const node = svg.append("g")
    .selectAll("circle")
    .data(nodes)
    .enter().append("circle")
    .attr("r", 15)
    .attr("fill", "lightblue")
    .call(drag(simulation));

  // Add labels to nodes
  const label = svg.append("g")
    .selectAll("text")
    .data(nodes)
    .enter().append("text")
    .attr("dy", 4)
    .attr("text-anchor", "middle")
    .attr("font-size", "12px")
    .text(d => d.id);

  // Update positions each tick of the simulation
  simulation.on("tick", () => {
    link
      .attr("x1", d => d.source.x)
      .attr("y1", d => d.source.y)
      .attr("x2", d => d.target.x)
      .attr("y2", d => d.target.y);

    node
      .attr("cx", d => d.x)
      .attr("cy", d => d.y);

    label
      .attr("x", d => d.x)
      .attr("y", d => d.y);
  });
}

function generateRandom() {
  adjMatrix = generateRandomAdjacencyMatrix(10, 10);
  const formattedString = '[\n' + adjMatrix.map(row => '  [' + row.join(', ') + ']').join(',\n') + '\n]';
  document.getElementById('myTextArea').value = formattedString;
  generateGraphFromMatrix(adjMatrix);
}

async function generateGraphFromMatrix(adjMatrix) {
  drawGraph(svg, adjMatrix);
  const result = await fetchResult(adjMatrix);
  /* const links2 = [];
  for (let i = 0; i < adjMatrix.length; i++) {
    for (let j = i + 1; j < adjMatrix[i].length; j++) {
      if (adjMatrix[i][j] !== 0) {
        links2.push({
          source: i,
          target: j,
          weight: adjMatrix[i][j],
        });
      }
    }
  } */
  console.log("reached here");
  console.log(result);
  const edgesArray = result.iterations.map((element) => ({
    edges: getEdge(element.current_node, element.neighbors.map((value, index) => value === 1 ? index : -1)
      .filter(index => index !== -1)),
    source: element.current_node,
    distances: (function(element) {
      let formattedString = 'Distance of nodes:\n\n';
      console.log("updated_distances", element);
      element.forEach((distance, index) => {
        const formattedDistance = distance === 2147483647 ? 'infinity' : distance;
        formattedString += `  Node ${index} : ${formattedDistance}\n`;
      });
      return formattedString;
    })(element.updated_distances)
  }));
  console.log(adjMatrix);
  console.log(edgesArray);
  setTimeout(() => animateEdgesSequentially(edgesArray), 1000);
}

generateRandom();


async function fetchResult(adjacencyMatrix) {
  const response = await fetch('http://localhost:8080', {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json'
    },
    body: JSON.stringify(adjacencyMatrix)
  })
  if (!response.ok) {
    throw new Error(`HRRP error! ${response.status}`)
  }
  const data = await response.json();
  const iterations = data.iterations;
  const finalDistances = data.final_distances;
  return { iterations, finalDistances };
  /* .then(response => response.json())
  .then(data => {
    const iterations = data.iterations;
    console.log(iterations);
    const finalDistances = data.final_distances;
    console.log(finalDistances);
    return { iterations, finalDistances };
  })
  .catch(error => {
    console.error('Error:', error);
  }); */
}


// Drag behavior for nodes
function drag(simulation) {
  function dragstarted(event) {
    if (!event.active) simulation.alphaTarget(0.3).restart();
    event.subject.fx = event.subject.x;
    event.subject.fy = event.subject.y;
  }

  function dragged(event) {
    event.subject.fx = event.x;
    event.subject.fy = event.y;
  }

  function dragended(event) {
    if (!event.active) simulation.alphaTarget(0);
    event.subject.fx = null;
    event.subject.fy = null;
  }

  return d3.drag()
    .on("start", dragstarted)
    .on("drag", dragged)
    .on("end", dragended);
}

function animateEdgesSequentially(edges, index = 0) {
  if (index === edges.length) return;
  const current_node = d3.selectAll('circle')._groups[0][edges[index].source];
  current_node.setAttribute("stroke", "red");
  current_node.setAttribute("stroke-width", "5");
  console.log("source", edges[index].source);
  setTimeout(() => {
    animateEdge(edges[index].edges, () => {
      console.log(`Animation of edge ${index + 1} has completed`);
      document.getElementById("result").innerText = edges[index].distances;
      current_node.removeAttribute("stroke");
      current_node.removeAttribute("stroke-width");
      animateEdgesSequentially(edges, index + 1);
    })
  }, 500)
}

// Function to animate a specific link
function animateEdge(edge, callback) {
  if (edge === undefined) {
    callback();
    return;
  }
  edge.transition()
    .duration(1500)
    .attrTween("stroke-dasharray", function() {
      const length = this.getTotalLength();
      const interpolate = d3.interpolateString(`0,${length}`, `${length},${length}`);
      return t => interpolate(t);  // Gradually animate stroke from one end to another
    })
    .attr("stroke", "orange")  // Change stroke color
    .attr("stroke-width", 5)  // Change stroke width
    .on("end", callback);
}

function getEdge(source, targets) {
  console.log("targets", targets);
  const edgeToAnimate = d3.selectAll("line")
    .filter(function(d) {
      return (d.source.id === source && targets.includes(d.target.id) && d.above === 0);
    });
  console.log(edgeToAnimate);
  if (edgeToAnimate._groups[0].length === 0) {
    console.log("returning", source, targets, edgeToAnimate);
    return;
  }
  return edgeToAnimate;
}

// Animate the first link in the links array after 2 seconds
/* setTimeout(() => {
  // const edgeToAnimate = d3.select(link.nodes()[0]);  // Select the first link
  // animateEdge(edgeToAnimate);
  showPath(0, 1);
  showPath(1, 2);
}, 100); */
