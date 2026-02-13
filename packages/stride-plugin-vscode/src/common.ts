import { z } from 'zod';

export enum WsMessageType {
  GET_CONFIG = 'get_config',
  UPDATE_CONFIG = 'update_config',
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

export const websocketMessageConfigDecoder = z.object({
  debugMode: z.boolean(),
});

export type RunConfig = z.infer<typeof websocketMessageConfigDecoder>;

type WebSocketType = { send: (message: string) => void };

export function sendMessage(
  ws: WebSocketType,
  type: WsMessageType,
  message: string = ''
): void {
  ws.send(JSON.stringify({ type, message }));
}
