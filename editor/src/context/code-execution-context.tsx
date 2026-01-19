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
        const data = JSON.parse(event.data);
        if (data.type === 'execution_success') {
          term.writeln(data.stdout || 'Success (No output)');
          if (data.stderr)
            term.writeln(`\x1b[31mStderr:\x1b[0m\r\n${data.stderr}`);
        } else if (data.type === 'execution_error') {
          term.writeln(`\x1b[31mError: ${data.error}\x1b[0m`);
          if (data.stderr)
            term.writeln(`\x1b[31mStderr:\x1b[0m\r\n${data.stderr}`);
        } else if (data.error) {
          term.writeln(`\x1b[31mServer Error: ${data.error}\x1b[0m`);
        }
      } catch (e) {
        console.error('Failed to parse server message', e);
        term.writeln('\x1b[31mFailed to parse server message\x1b[0m');
      }
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
        setTerminalResizing
      }}
    >
      {children}
    </CodeExecutionContext.Provider>
  );
}
