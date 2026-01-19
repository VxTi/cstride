import { z } from 'zod';

export enum WsMessageType {
  EXECUTE_CODE = 'run',
  TERMINATE_PROCESS = 'terminate',

  PROCESS_TERMINATED = 'terminated',
  PROCESS_STDOUT = 'execution_success',
  PROCESS_STDERR = 'execution_failed',
}

export const websocketMessageDecoder = z.object({
  type: z.enum(WsMessageType),
  message: z.string(),
});

type WebSocketType = { send: (message: string) => void };

export function sendMessage(
  ws: WebSocketType,
  type: WsMessageType,
  message: string = ""
) {
  ws.send(JSON.stringify({ type, message }));
}
