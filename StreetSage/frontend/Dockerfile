# Use Node.js for building and serving the frontend
FROM node:20-alpine

# Set working directory
WORKDIR /app

# Copy package.json and install dependencies
COPY package.json package-lock.json ./
RUN npm install

# Copy the rest of the frontend files
COPY . .

EXPOSE 5173

# Start the development server
CMD ["npm", "run", "dev"]
