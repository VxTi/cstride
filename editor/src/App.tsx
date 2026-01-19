import Editor from '@monaco-editor/react';
import '@xterm/xterm/css/xterm.css';
import { useMemo, useState } from 'react';
import { twMerge } from 'tailwind-merge';
import TerminalWindow from './components/terminal';
import { useCodeContext } from './context/code-execution-context';
import { strideLanguageId }           from './lib/stride-language/language-config';
import { PlayIcon, SquareIcon } from 'lucide-react';
import { sendMessage, WsMessageType } from './common';

function App() {
  const [initialCode] = useState(
    () => localStorage.getItem('stride_code') || defaultCodeFragment
  );

  const { onEditorMount, terminalResizing } = useCodeContext();

  const debounceSave = useMemo(() => {
    let timeout: number | null = null;

    return (value: string | undefined) => {
      if (timeout) window.clearTimeout(timeout);

      timeout = window.setTimeout(() => {
        if (value) localStorage.setItem('stride_code', value);
      }, 200);
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
          <ExecuteCodeButton />
        </div>
      </div>
      <TerminalWindow />
    </div>
  );
}
function ExecuteCodeButton() {
  const { editorRef, ws, xtermRef, processActive, setProcessActive } =
    useCodeContext();

  const runCode = () => {
    if (editorRef.current && ws) {
      const term = xtermRef.current;
      if (processActive) {
        term?.writeln('Process termination requested...');
        sendMessage(ws, WsMessageType.TERMINATE_PROCESS);
        setProcessActive(false);
      } else {
        term?.writeln('Compiling and running...');
        const code = editorRef.current.getValue();

        sendMessage(ws, WsMessageType.EXECUTE_CODE, code);
        setProcessActive(true);
      }
    } else {
      xtermRef.current?.writeln(
        '\x1b[31mEditor not ready or not connected to server.\x1b[0m'
      );
    }
  };

  const Icon = processActive ? SquareIcon : PlayIcon;

  return (
    <button
      onClick={runCode}
      className="rounded-2xl p-2 hover:bg-gray-100/10 transition-colors duration-200 text-white cursor-pointer"
    >
      <Icon
        className={twMerge(
          processActive ? 'stroke-red-500' : 'stroke-green-500',
          'stroke-2'
        )}
      />
    </button>
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
