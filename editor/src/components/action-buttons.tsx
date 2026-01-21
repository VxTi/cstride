import { PlayIcon, SquareIcon, Trash2Icon } from 'lucide-react';
import { type ComponentProps, useCallback } from 'react';
import { twMerge } from 'tailwind-merge';
import { sendMessage, WsMessageType } from '../common';
import { useCodeContext } from '../context/code-execution-context';

export default function ActionButtons() {
  const { editorRef, ws, xtermRef, processActive, setProcessActive } =
    useCodeContext();

  const runCode = useCallback(() => {
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
  }, [editorRef, ws, xtermRef, processActive, setProcessActive]);

  const clearTerminal = () => {
    xtermRef.current?.clear();
  };

  const Icon = processActive ? SquareIcon : PlayIcon;

  return (
    <div className="flex items-center w-full z-1000 px-3 pt-0.5">
      <div className="flex flex-row items-center justify-end gap-1 grow">
        <ActionButton onClick={clearTerminal}>
          <Trash2Icon className="stroke-2 stroke-white" />
        </ActionButton>
        <ActionButton onClick={runCode}>
          <Icon
            className={twMerge(
              processActive ? 'stroke-red-500' : 'stroke-green-500',
              'stroke-2'
            )}
          />
        </ActionButton>
      </div>
    </div>
  );
}

function ActionButton({ className, ...props }: ComponentProps<'button'>) {
  return (
    <button
      className={twMerge(
        'rounded-sm p-1.5 hover:bg-gray-100/10 transition-colors duration-200 text-white' +
          ' cursor-pointer *:stroke-2 size-8 *:size-full'
      )}
      {...props}
    ></button>
  );
}
