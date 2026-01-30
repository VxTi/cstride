import { type ChildProcessWithoutNullStreams } from 'node:child_process';
import { WebSocketServer, type WebSocket } from 'ws';
import * as fs from 'fs';
import * as path from 'path';
import { spawn } from 'child_process';
import { dirname } from 'node:path';
import { fileURLToPath } from 'node:url';
import { type z } from 'zod';
import {
  type RunConfig,
  sendMessage,
  websocketMessageConfigDecoder,
  websocketMessageDecoder,
  WsMessageType,
} from '../common';

const __dirname = dirname(fileURLToPath(import.meta.url));

const port = 8080;
const wss = new WebSocketServer({ port });

console.log(`WebSocket server started on port ${port}`);

const config: RunConfig = {
  debugMode: false,
};

const CACHE_DIR = path.join(__dirname, './../../.cache');

let process: ChildProcessWithoutNullStreams | null = null;

const CSTRIDE_DEBUG_PATH = path.resolve(
  __dirname,
  '../../../compiler/cmake-build-debug/cstride'
);

function getCstridePath() {
  if (fs.existsSync(CSTRIDE_DEBUG_PATH)) return CSTRIDE_DEBUG_PATH;

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
      const data: CompileMessage = websocketMessageDecoder.parse(
        JSON.parse(message)
      );

      switch (data.type) {
        case WsMessageType.EXECUTE_CODE:
          handleCompile(ws, data.message);
          break;
        case WsMessageType.TERMINATE_PROCESS:
          process?.kill('SIGTERM');
          sendMessage(ws, WsMessageType.PROCESS_TERMINATED);
          break;
        case WsMessageType.GET_CONFIG:
          console.log("Getting config")
          sendMessage(ws, WsMessageType.UPDATE_CONFIG, JSON.stringify(config));
          break;
        case WsMessageType.UPDATE_CONFIG:
          const decoded = websocketMessageConfigDecoder.safeParse(
            JSON.parse(data.message)
          );

          if (!decoded.success) {
            sendMessage(
              ws,
              WsMessageType.PROCESS_STDERR,
              `Failed to parse config JSON: ${decoded.error.message}`
            );
            break;
          }

          Object.assign(config, decoded.data);

          break;
      }
    } catch (e) {
      console.error('Failed to parse message', e, message);

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

      sendMessage(
        ws,
        WsMessageType.PROCESS_STDERR,
        `Failed to save code to file: ${err.message
          .replace(/\n/g, ' ')
          .replace(/\s+/g, '')}`
      );
      return;
    }

    console.log(`Code saved to ${filePath}, executing cstride...`);

    const args = ['--compile', filePath];

    if (config.debugMode) args.push('--debug');

    process = spawn(cstrideExe, args);

    process.stdout.on('data', (data: Buffer) => {
      sendMessage(ws, WsMessageType.PROCESS_STDOUT, data.toString());
    });
    process.stderr.on('data', (data: Buffer) => {
      sendMessage(ws, WsMessageType.PROCESS_STDERR, data.toString());
    });

    process.on('close', (code: number | null) => {
      sendMessage(
        ws,
        WsMessageType.PROCESS_TERMINATED,
        `Process exited with status code ${code ?? 0}`
      );
    });
  });
}
