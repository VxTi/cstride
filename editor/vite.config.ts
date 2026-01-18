import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'
import { spawn } from 'child_process'

const startServerPlugin = () => {
  return {
    name: 'start-server',
    configureServer(_server) {
      console.log('Starting compilation server...');
      const child = spawn('npx', ['tsx', 'src/server/server.ts'], {
        stdio: 'inherit',
        shell: true
      });

      child.on('error', (err) => {
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
  }
}

// https://vitejs.dev/config/
export default defineConfig({
  plugins: [react(), startServerPlugin()],
})
