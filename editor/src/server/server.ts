import { WebSocketServer, type WebSocket } from 'ws';
import * as fs from 'fs';
import * as path from 'path';
import { exec } from 'child_process';
import { dirname } from 'node:path';
import { fileURLToPath } from 'node:url';
import { type z } from 'zod';
import { sendMessage, websocketMessageDecoder, WsMessageType } from '../shared';

const __dirname = dirname(fileURLToPath(import.meta.url));

const port = 8080;
const wss = new WebSocketServer({ port });

console.log(`WebSocket server started on port ${port}`);

const CACHE_DIR = path.join(__dirname, '../../.c-cache');

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

type CompileMessage = z.infer<typeof websocketMessageDecoder>;

wss.on('connection', (ws: WebSocket) => {
  console.log('Client connected');

  ws.on('message', (message: string) => {
    try {
      const json: unknown = JSON.parse(message);
      const data: CompileMessage = websocketMessageDecoder.parse(json);

      if (data.type === WsMessageType.EXECUTE_CODE) {
        handleCompile(ws, data.message);
      }
    } catch (e) {
      console.error('Failed to parse message', e);

      sendMessage(ws, WsMessageType.PROCESS_STDERR, 'Failed to parse JSON');
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

      sendMessage(ws, WsMessageType.PROCESS_STDERR, 'Failed to save code');
      return;
    }

    console.log(`Code saved to ${filePath}, executing cstride...`);

    // Run cstride <cache file name>
    exec(`${cstrideExe} "${filePath}"`, (_error, stdout, stderr) => {
      // Clean up file after execution (optional, dependent on requirement)
      // fs.unlinkSync(filePath);

      if (stderr.length) {
        sendMessage(ws, WsMessageType.PROCESS_STDERR, stderr);
      } else {
        sendMessage(ws, WsMessageType.PROCESS_STDOUT, stdout);
      }
    });
  });
}
