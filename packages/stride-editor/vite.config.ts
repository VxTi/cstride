import { spawn }        from 'child_process';
import { defineConfig } from "vite";
import tailwindcss from '@tailwindcss/vite';
import react            from "@vitejs/plugin-react";

const host = process.env.TAURI_DEV_HOST;

// https://vite.dev/config/
export default defineConfig(async () => ({
  plugins: [react(), tailwindcss(), startServerPlugin()],

  // Vite options tailored for Tauri development and only applied in `tauri dev` or `tauri build`
  //
  // 1. prevent Vite from obscuring rust errors
  clearScreen: false,
  // 2. tauri expects a fixed port, fail if that port is not available
  server: {
    port: 1420,
    strictPort: true,
    host: host || false,
    hmr: host
      ? {
          protocol: "ws",
          host,
          port: 1421,
        }
      : undefined,
    watch: {
      // 3. tell Vite to ignore watching `src-tauri`
      ignored: ["**/src-tauri/**"],
    },
  },
}));


const startServerPlugin = () => {
  return {
    name: 'start-server',
    configureServer(_server) {
      console.log('Starting compilation server...');
      const child = spawn('npx', ['tsx', 'src/server/server.ts'], {
        stdio: 'inherit',
        shell: true,
      });

      child.on('error', err => {
        console.error('Failed to start compilation server:', err);
      });

      process.on('SIGINT', () => {
        child.kill();
        process.exit();
      });

      process.on('SIGTERM', () => {
        child.kill();
        process.exit();
      });

      process.on('exit', () => {
        child.kill();
      });
    },
  };
};
