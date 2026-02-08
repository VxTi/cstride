import '@xterm/xterm/css/xterm.css';
import { twMerge } from 'tailwind-merge';
import EditorWindow from './editor-window';
import TerminalWindow from './terminal';
import { useCodeContext } from '../context/code-execution-context';

function AppWindow() {
  const { terminalState } = useCodeContext();

  return (
    <div className="w-screen h-screen flex flex-col m-0 p-0 bg-[#121418]">
      <div
        className={twMerge(
          'flex-1 relative',
          terminalState.resizing ? 'pointer-events-none' : 'pointer-events-auto'
        )}
      >
        <EditorWindow />
      </div>
      <TerminalWindow />
    </div>
  );
}

export default AppWindow;
