import { useMonaco } from '@monaco-editor/react';
import { type Terminal } from '@xterm/xterm';
import { type editor } from 'monaco-editor';
import React, {
  createContext,
  type MutableRefObject,
  type RefObject,
  useContext,
  useEffect,
  useRef,
  useState,
} from 'react';
import {
  type RunConfig,
  websocketMessageConfigDecoder,
  websocketMessageDecoder,
  WsMessageType,
} from '../common';
import { registerLanguage } from '../lib/stride-language/stride-language';

interface TerminalState {
  visible: boolean;
  resizing: boolean;
  ref: RefObject<HTMLDivElement>;
}

interface CodeExecutionContextType {
  monaco: typeof import('monaco-editor') | null;
  editorRef: RefObject<editor.IStandaloneCodeEditor>;
  ws: WebSocket | null;
  xtermRef: MutableRefObject<Terminal | null>;

  terminalState: TerminalState;
  setTerminalState: React.Dispatch<React.SetStateAction<TerminalState>>;

  onEditorMount: (editor: editor.IStandaloneCodeEditor) => void;

  processActive: boolean;
  setProcessActive: React.Dispatch<React.SetStateAction<boolean>>;

  config: RunConfig;
  setConfig: React.Dispatch<React.SetStateAction<RunConfig>>;
}

const CodeExecutionContext = createContext<CodeExecutionContextType | null>(
  null
);

export function useCodeContext() {
  const context = useContext(CodeExecutionContext);

  if (!context) {
    throw new Error(
      'useCodeContext must be used within a CodeExecutionContextProvider'
    );
  }

  return context;
}

export function CodeContextProvider({
  children,
}: {
  children: React.ReactNode;
}) {
  const monaco = useMonaco();
  const editorRef = useRef<editor.IStandaloneCodeEditor | null>(null);
  const xtermRef = useRef<Terminal | null>(null);
  const [ws, setWs] = useState<WebSocket | null>(null);

  const terminalRef = useRef<HTMLDivElement>(null);
  const [terminalState, setTerminalState] = useState<TerminalState>({
    visible: true,
    resizing: false,
    ref: terminalRef,
  });

  const [processActive, setProcessActive] = useState(false);
  const [config, setConfig] = useState<RunConfig>({ debugMode: false });

  useEffect(() => {
    if (!monaco) return;

    return registerLanguage(monaco);
  }, [monaco]);

  useEffect(() => {
    const socket = new WebSocket('ws://localhost:8080');

    socket.onopen = () => {
      console.log('Connected to compilation server');
      xtermRef.current?.writeln('Connected to compilation server');
    };

    socket.onmessage = event => {
      const term = xtermRef.current;
      if (!term) return;

      try {
        const json: unknown = JSON.parse(
          typeof event.data === 'string' ? event.data : ''
        );
        const decoded = websocketMessageDecoder.safeParse(json);
        if (!decoded.success) {
          term.writeln('\x1b[31mFailed to decode server message\x1b[0m');
          return;
        }
        const data = decoded.data;

        switch (data.type) {
          case WsMessageType.PROCESS_TERMINATED:
            setProcessActive(false);
            term.writeln(data.message || 'Process terminated');
            break;
          case WsMessageType.PROCESS_STDOUT:
            term.writeln(data.message || 'Success (No output)');
            break;
          case WsMessageType.UPDATE_CONFIG:
            const result = websocketMessageConfigDecoder.safeParse(
              JSON.parse(data.message)
            );
            if (!result.success) {
              term.writeln('\x1b[31mFailed to decode config message\x1b[0m');
              return;
            }
            setConfig(result.data);
            break;
          case WsMessageType.PROCESS_STDERR:
            data.message
              .split('\n')
              .forEach(line => term.writeln(`\x1b[31m${line}\x1b[0m`));

            break;
        }
      } catch {}
    };

    setWs(socket);

    return () => {
      socket.close();
    };
  }, []);

  const onEditorMount = (editor: editor.IStandaloneCodeEditor) => {
    editorRef.current = editor;
  };

  return (
    <CodeExecutionContext.Provider
      value={{
        onEditorMount,
        ws,
        monaco,
        processActive,
        setProcessActive,
        editorRef,
        xtermRef,
        terminalState,
        setTerminalState,
        config,
        setConfig,
      }}
    >
      {children}
    </CodeExecutionContext.Provider>
  );
}
