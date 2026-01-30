import {
  BugIcon,
  BugOffIcon,
  BugPlayIcon,
  PlayIcon,
  SquareIcon,
  Trash2Icon,
} from 'lucide-react';
import { type ComponentProps, useCallback, useEffect } from 'react';
import { twMerge } from 'tailwind-merge';
import { sendMessage, WsMessageType } from '../common';
import { useCodeContext } from '../context/code-execution-context';

export default function ActionButtons() {
  const {
    editorRef,
    ws,
    xtermRef,
    processActive,
    setProcessActive,
    debugMode,
    setDebugMode,
  } = useCodeContext();

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

  const toggleDebugMode = () => {
    setDebugMode(prev => {
      if (ws)
        sendMessage(
          ws,
          WsMessageType.UPDATE_CONFIG,
          JSON.stringify({ debugMode: !prev })
        );
      return !prev;
    });
  };

  const Icon = processActive ? SquareIcon : debugMode ? BugPlayIcon : PlayIcon;
  const DebugIcon = debugMode ? BugOffIcon : BugIcon;

  return (
    <div className="flex items-center w-full z-1000 px-3 pt-0.5">
      <div className="flex flex-row items-center justify-end gap-1 grow">
        <ActionButton
          keybind="Ctrl+Shift+C"
          tooltip="Clear terminal"
          onClick={clearTerminal}
        >
          <Trash2Icon className="stroke-2 stroke-white" />
        </ActionButton>
        <ActionButton
          keybind="Cmd+Shift+B"
          tooltip={debugMode ? 'Disable debug mode' : 'Enable debug mode'}
          onClick={toggleDebugMode}
        >
          <DebugIcon />
        </ActionButton>
        <ActionButton
          keybind="Ctrl+R"
          tooltip={
            processActive
              ? 'Stop process'
              : debugMode
                ? 'Run code'
                : 'Debug code'
          }
          onClick={runCode}
        >
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

type ActionButtonProps = Omit<ComponentProps<'button'>, 'onClick'> & {
  tooltip: string;
  keybind?: string;
  onClick: () => void;
};

function ActionButton({
  className,
  tooltip,
  keybind,
  onClick,
  ...props
}: ActionButtonProps) {
  useEffect(() => {
    if (!keybind) return;

    const lowercased = keybind.toLowerCase();
    const hasCtrl = lowercased.includes('ctrl');
    const hasShift = lowercased.includes('shift');
    const hasCmd = lowercased.includes('cmd');

    // Must have ctrl, shift, or cmd, otherwise it would disturb editing
    if (!hasCtrl && !hasShift && !hasCmd) return;

    const keys = lowercased
      .split('+')
      .filter(key => !['ctrl', 'shift', 'cmd'].includes(key))
      .map(key => key.toLowerCase());

    const keyDownHandler = (event: KeyboardEvent) => {
      const key = event.key.toLowerCase();
      if (hasCtrl && !event.ctrlKey) return;
      if (hasShift && !event.shiftKey) return;
      if (hasCmd && !event.metaKey) return;

      if (!keys.includes(key)) return;

      event.preventDefault();
      event.stopPropagation();

      onClick?.();
    };

    window.addEventListener('keydown', keyDownHandler);
    return () => {
      window.removeEventListener('keydown', keyDownHandler);
    };
  }, [keybind, onClick]);

  const formattedKeybind = keybind
    ? keybind
        .split('+')
        .map(key => {
          const upper = key.toUpperCase();
          switch (upper) {
            case 'CMD':
              return '⌘';
            case 'CTRL':
              return '^';
            case 'SHIFT':
              return '⇧';
            case 'ALT':
              return '⌥';
            default:
              return upper;
          }
        })
        .join('')
    : undefined;

  return (
    <div className="relative group" title={tooltip}>
      <div className="absolute top-0 right-0 -translate-y-full pointer-events-none">
        <div className="bg-neutral-900 max-w-46 w-max flex gap-2 items-center wrap-break-word text-white rounded-xl border border-neutral-700 px-2.5 py-1 opacity-0 group-hover:opacity-100 transition-opacity duration-200">
          <span className="grow">{tooltip}</span>
          {keybind && (
            <span className="mx-0.5 justify-end font-mono text-neutral-300 bg-neutral-800 rounded-lg px-1">
              {formattedKeybind}
            </span>
          )}
        </div>
      </div>
      <button
        onClick={onClick}
        className={twMerge(
          'rounded-sm p-1.5 hover:bg-neutral-100/10 transition-colors duration-200 text-white' +
            ' cursor-pointer *:stroke-2 size-8 *:size-full'
        )}
        {...props}
      ></button>
    </div>
  );
}
