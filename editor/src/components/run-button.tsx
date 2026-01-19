import { PlayIcon, SquareIcon } from 'lucide-react';
import { twMerge } from 'tailwind-merge';
import { sendMessage, WsMessageType } from '../common';
import { useCodeContext } from '../context/code-execution-context';

export default function RunButton() {
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
    <div className="flex items-center gap-2 absolute bottom-2 right-2 z-1000">
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
    </div>
  );
}
