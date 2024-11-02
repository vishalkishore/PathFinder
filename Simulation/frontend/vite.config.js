import { defineConfig } from "vite";
import react from "@vitejs/plugin-react";

// https://vite.dev/config/
export default defineConfig({
	plugins: [react()],
	server: {
	  host: true,  // will make Vite listen on all available network interfaces
	  port: 5173,
	  watch: {
		usePolling: true  // Required for changes detection in Docker on some systems
	  }
	}
  })
