### USAGE

docker build -t dijkstra_visual .
docker run -p 8080:8080 dijkstra_visual
docker stop $(docker ps -q)
