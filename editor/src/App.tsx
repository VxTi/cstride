import Editor, { useMonaco } from '@monaco-editor/react';
import { FitAddon } from '@xterm/addon-fit';
import '@xterm/xterm/css/xterm.css';
import { Terminal } from '@xterm/xterm';
import { useEffect, useRef, useState } from 'react';
import { strideLanguageId } from './stride-language/language-config';
import { registerLanguage } from './stride-language/stride-language';

function App() {
  const monaco = useMonaco();
  const editorRef = useRef<any>(null);
  const terminalRef = useRef<HTMLDivElement>(null);
  const xtermRef = useRef<Terminal | null>(null);
  const fitAddonRef = useRef<FitAddon | null>(null);
  const [height, setHeight] = useState(300);
  const isResizing = useRef(false);
  const [isResizingState, setIsResizingState] = useState(false);
  const [ws, setWs] = useState<WebSocket | null>(null);
  const [initialCode] = useState(
    () => localStorage.getItem('stride_code') || defaultCodeFragment
  );

  useEffect(() => {
    if (!monaco) return;

    return registerLanguage(monaco);
  }, [monaco]);

  useEffect(() => {
    if (!terminalRef.current) return;

    const term = new Terminal({
      convertEol: true,
      theme: {
        background: '#1E1E1E',
      },
      fontFamily: 'monospace',
      fontSize: 14,
    });
    const fitAddon = new FitAddon();
    term.loadAddon(fitAddon);

    term.open(terminalRef.current);
    fitAddon.fit();

    xtermRef.current = term;
    fitAddonRef.current = fitAddon;

    const handleResize = () => fitAddon.fit();
    window.addEventListener('resize', handleResize);

    return () => {
      term.dispose();
      window.removeEventListener('resize', handleResize);
    };
  }, []);

  useEffect(() => {
    const handleMouseMove = (e: MouseEvent) => {
      if (!isResizing.current) return;

      const newHeight = window.innerHeight - e.clientY;
      if (newHeight > 50 && newHeight < window.innerHeight - 50) {
        setHeight(newHeight);
      }
    };

    const handleMouseUp = () => {
      if (!isResizing.current) return;

      isResizing.current = false;
      setIsResizingState(false);
      document.body.style.cursor = '';
      document.body.style.userSelect = '';
    };

    window.addEventListener('mousemove', handleMouseMove);
    window.addEventListener('mouseup', handleMouseUp);

    return () => {
      window.removeEventListener('mousemove', handleMouseMove);
      window.removeEventListener('mouseup', handleMouseUp);
    };
  }, []);

  useEffect(() => {
    if (fitAddonRef.current) {
      requestAnimationFrame(() => fitAddonRef.current?.fit());
    }
  }, [height]);

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

  const handleEditorDidMount = (editor: any) => {
    editorRef.current = editor;
  };

  const runCode = () => {
    if (editorRef.current && ws) {
      const term = xtermRef.current;
      term?.reset();
      term?.writeln('Compiling and running...');
      const code = editorRef.current.getValue();
      ws.send(JSON.stringify({ type: 'compile', code }));
    } else {
      xtermRef.current?.writeln(
        '\x1b[31mEditor not ready or not connected to server.\x1b[0m'
      );
    }
  };

  return (
    <div
      style={{
        height: '100vh',
        width: '100vw',
        margin: 0,
        padding: 0,
        display: 'flex',
        flexDirection: 'column',
      }}
    >
      <div
        style={{
          flex: 1,
          position: 'relative',
          pointerEvents: isResizingState ? 'none' : 'auto',
        }}
      >
        <Editor
          height="100%"
          defaultLanguage={strideLanguageId}
          defaultValue={initialCode}
          theme="vs-dark"
          onMount={handleEditorDidMount}
          onChange={value => {
            if (value) localStorage.setItem('stride_code', value);
          }}
          options={{
            minimap: { enabled: false },
            fontSize: 14,
          }}
        />
        <button
          onClick={runCode}
          style={{
            position: 'absolute',
            bottom: 20,
            right: 20,
            padding: '10px 20px',
            backgroundColor: '#28a745',
            color: 'white',
            border: 'none',
            borderRadius: '4px',
            cursor: 'pointer',
            zIndex: 1000,
          }}
        >
          Run Code
        </button>
      </div>
      <div
        onMouseDown={e => {
          isResizing.current = true;
          setIsResizingState(true);
          document.body.style.cursor = 'ns-resize';
          document.body.style.userSelect = 'none';
          e.preventDefault();
        }}
        style={{
          height: '5px',
          cursor: 'ns-resize',
          backgroundColor: isResizingState ? '#007fd4' : '#333',
          transition: 'background-color 0.2s ease',
          zIndex: 10,
        }}
      />
      <div
        style={{
          height: `${height}px`,
          backgroundColor: '#1E1E1E',
          padding: '10px',
          overflow: 'hidden',
        }}
      >
        <div ref={terminalRef} style={{ width: '100%', height: '100%' }} />
      </div>
    </div>
  );
}

const defaultCodeFragment = `// Welcome to Stride Editor
// Try writing some Stride code here

fn main(): void {
    let x: i32 = 10;
    let name: string = "Stride";
    
    if (x > 5) {
        return true;
    }
    
    return false;
}`;

export default App;
