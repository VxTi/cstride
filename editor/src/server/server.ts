import { WebSocketServer, type WebSocket } from 'ws';
import * as fs from 'fs';
import * as path from 'path';
import { exec } from 'child_process';
import { dirname } from 'node:path';
import { fileURLToPath } from 'node:url';

const __dirname = dirname(fileURLToPath(import.meta.url));

const port = 8080;
const wss = new WebSocketServer({ port });

console.log(`WebSocket server started on port ${port}`);

const CACHE_DIR = path.join(__dirname, 'cache');

const CSTRIDE_BUILD_PATH = path.resolve(__dirname, '../../../build/cstride');
const CSTRIDE_DEBUG_PATH = path.resolve(
  __dirname,
  '../../../cmake-build-debug/cstride'
);

function getCstridePath() {
  if (fs.existsSync(CSTRIDE_DEBUG_PATH)) return CSTRIDE_DEBUG_PATH;

  if (fs.existsSync(CSTRIDE_BUILD_PATH)) return CSTRIDE_BUILD_PATH;

  return 'cstride';
}

const cstrideExe = getCstridePath();
console.log(`Using cstride executable at: ${cstrideExe}`);

// Ensure cache directory exists
if (!fs.existsSync(CACHE_DIR)) {
  fs.mkdirSync(CACHE_DIR, { recursive: true });
}

interface CompileMessage {
  type: 'compile';
  code: string;
}

wss.on('connection', (ws: WebSocket) => {
  console.log('Client connected');

  ws.on('message', (message: string) => {
    try {
      const data: CompileMessage = JSON.parse(message.toString());

      if (data.type === 'compile' && data.code) {
        handleCompile(ws, data.code);
      } else {
        ws.send(JSON.stringify({ error: 'Invalid message format or type' }));
      }
    } catch (e) {
      console.error('Failed to parse message', e);
      ws.send(JSON.stringify({ error: 'Failed to parse JSON' }));
    }
  });

  ws.on('close', () => {
    console.log('Client disconnected');
  });
});

function handleCompile(ws: WebSocket, code: string) {
  // Generate a unique filename for the cache
  const id = 'tmp'; // crypto.randomBytes(8).toString('hex');
  const filename = `code_${id}.sr`; // Stride source file
  const filePath = path.join(CACHE_DIR, filename);

  // Save code to cache file
  fs.writeFile(filePath, code, err => {
    if (err) {
      console.error('Error writing file:', err);
      ws.send(
        JSON.stringify({ type: 'error', message: 'Failed to save code' })
      );
      return;
    }

    console.log(`Code saved to ${filePath}, executing cstride...`);

    // Run cstride <cache file name>
    exec(`${cstrideExe} "${filePath}"`, (error, stdout, stderr) => {
      // Clean up file after execution (optional, dependent on requirement)
      // fs.unlinkSync(filePath);

      if (error) {
        console.error(`exec error: ${error}`);
        ws.send(
          JSON.stringify({
            type: 'execution_error',
            error: error.message,
            stderr,
          })
        );
        return;
      }

      ws.send(
        JSON.stringify({
          type: 'execution_success',
          stdout,
          stderr,
        })
      );
    });
  });
}
