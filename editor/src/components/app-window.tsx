import '@xterm/xterm/css/xterm.css';
import { twMerge } from 'tailwind-merge';
import EditorWindow from './editor-window';
import TerminalWindow from './terminal';
import { useCodeContext } from '../context/code-execution-context';

function AppWindow() {
  const { terminalResizing } = useCodeContext();

  return (
    <div className="w-screen h-screen flex flex-col m-0 p-0">
      <div
        className={twMerge(
          'flex-1 relative',
          terminalResizing ? 'pointer-events-none' : 'pointer-events-auto'
        )}
      >
        <EditorWindow />
      </div>
      <TerminalWindow />
    </div>
  );
}

export default AppWindow;
