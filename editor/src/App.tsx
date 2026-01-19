import Editor from '@monaco-editor/react';
import '@xterm/xterm/css/xterm.css';
import { useMemo, useState } from 'react';
import { twMerge } from 'tailwind-merge';
import RunButton from './components/run-button';
import TerminalWindow from './components/terminal';
import { useCodeContext } from './context/code-execution-context';
import { strideLanguageId } from './lib/stride-language/language-config';

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
        className={twMerge(
          'flex-1 relative',
          terminalResizing ? 'pointer-events-none' : 'pointer-events-auto'
        )}
      >
        <Editor
          height="100%"
          className="absolute left-0 top-0 size-full"
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
        <RunButton />
      </div>
      <TerminalWindow />
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
