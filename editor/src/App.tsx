import Editor from '@monaco-editor/react';
import { FitAddon } from '@xterm/addon-fit';
import '@xterm/xterm/css/xterm.css';
import { Terminal } from '@xterm/xterm';
import { useCallback, useEffect, useRef, useState } from 'react';
import { useCodeContext } from './context/code-execution-context';
import { strideLanguageId } from './lib/stride-language/language-config';
import { PlayIcon, SquareIcon } from 'lucide-react';
import { sendMessage, WsMessageType } from './shared';

function App() {
  const [initialCode] = useState(
    () => localStorage.getItem('stride_code') || defaultCodeFragment
  );

  const { onEditorMount, terminalResizing } = useCodeContext();

  const debounceSave = useCallback(() => {
    let timeout: number | null = null;

    return (value: string | null) => {
      if (timeout) window.clearTimeout(timeout);
      timeout = window.setTimeout(() => {
        if (value) localStorage.setItem('stride_code', value);
      }, 1000);
    };
  }, []);

  return (
    <div className="w-screen h-screen flex flex-col m-0 p-0">
      <div
        className="flex-1 relative"
        style={{ pointerEvents: terminalResizing ? 'none' : 'auto' }}
      >
        <Editor
          height="100%"
          defaultLanguage={strideLanguageId}
          defaultValue={initialCode}
          theme="vs-dark"
          onMount={onEditorMount}
          onChange={debounceSave}
          options={{
            minimap: { enabled: false },
            fontSize: 14,
          }}
        />
        <div className="flex items-center gap-2 absolute bottom-2 right-2 z-1000">
          <TerminateCodeButton />
          <ExecuteCodeButton />
        </div>
      </div>
      <TerminalWindow />
    </div>
  );
}

function TerminateCodeButton() {
  const { editorRef, ws, xtermRef } = useCodeContext();

  const runCode = () => {
    if (editorRef.current && ws) {
      const term = xtermRef.current;
      term?.reset();
      term?.writeln('Compiling and running...');

      const code = editorRef.current.getValue();

      sendMessage(ws, WsMessageType.EXECUTE_CODE, code);
    } else {
      xtermRef.current?.writeln(
        '\x1b[31mEditor not ready or not connected to server.\x1b[0m'
      );
    }
  };

  return (
    <button
      onClick={runCode}
      className="rounded-2xl px-4 py-2 text-white cursor-pointer hover:bg-gray-100/10 transition-colors duration-200"
    >
      <SquareIcon className="stroke-red-500 stroke-2" />
    </button>
  );
}

function ExecuteCodeButton() {
  const { editorRef, ws, xtermRef } = useCodeContext();

  const runCode = () => {
    if (editorRef.current && ws) {
      const term = xtermRef.current;
      term?.reset();
      term?.writeln('Compiling and running...');
      const code = editorRef.current.getValue();

      sendMessage(ws, WsMessageType.EXECUTE_CODE, code);
    } else {
      xtermRef.current?.writeln(
        '\x1b[31mEditor not ready or not connected to server.\x1b[0m'
      );
    }
  };

  return (
    <button
      onClick={runCode}
      className="rounded-2xl p-2 hover:bg-gray-100/10 transition-colors duration-200 text-white cursor-pointer"
    >
      <PlayIcon className="stroke-green-500 stroke-2" />
    </button>
  );
}

function TerminalWindow() {
  const { terminalRef, xtermRef, terminalResizing, setTerminalResizing } =
    useCodeContext();

  const fitAddonRef = useRef<FitAddon | null>(null);
  const [height, setHeight] = useState(300);
  const isResizing = useRef(false);

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
      setTerminalResizing(false);
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
    if (fitAddonRef.current) {
      requestAnimationFrame(() => fitAddonRef.current?.fit());
    }
  }, [height]);

  return (
    <>
      <div
        onMouseDown={e => {
          isResizing.current = true;
          setTerminalResizing(true);
          document.body.style.cursor = 'ns-resize';
          document.body.style.userSelect = 'none';
          e.preventDefault();
        }}
        style={{
          height: '5px',
          cursor: 'ns-resize',
          backgroundColor: terminalResizing ? '#007fd4' : '#333',
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
    </>
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
