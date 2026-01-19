import { useMonaco } from '@monaco-editor/react';
import { type Terminal } from '@xterm/xterm';
import { type editor } from 'monaco-editor';
import {
  createContext,
  type MutableRefObject,
  type RefObject,
  useContext,
  useEffect,
  useRef,
  useState,
} from 'react';
import { registerLanguage } from '../lib/stride-language/stride-language';
import { websocketMessageDecoder, WsMessageType } from '../shared';

interface CodeExecutionContextType {
  monaco: typeof import('monaco-editor') | null;
  terminalRef: RefObject<HTMLDivElement>;
  editorRef: RefObject<editor.IStandaloneCodeEditor>;
  ws: WebSocket | null;
  xtermRef: MutableRefObject<Terminal | null>;

  terminalResizing: boolean;
  setTerminalResizing: React.Dispatch<React.SetStateAction<boolean>>;

  onEditorMount: (editor: editor.IStandaloneCodeEditor) => void;
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
  const terminalRef = useRef<HTMLDivElement>(null);
  const xtermRef = useRef<Terminal | null>(null);
  const [ws, setWs] = useState<WebSocket | null>(null);
  const [terminalResizing, setTerminalResizing] = useState(false);

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

        if (data.type === WsMessageType.PROCESS_STDOUT) {
          term.writeln(data.message || 'Success (No output)');
        } else if (data.type === WsMessageType.PROCESS_STDERR) {
          data.message
            .split('\n')
            .forEach(line => term.writeln(`\x1b[31m${line}\x1b[0m`));
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
        terminalRef,
        editorRef,
        xtermRef,
        terminalResizing,
        setTerminalResizing,
      }}
    >
      {children}
    </CodeExecutionContext.Provider>
  );
}
