import {
  BugIcon,
  BugOffIcon,
  BugPlayIcon,
  PlayIcon,
  SquareIcon,
  Trash2Icon,
  MinusIcon,
} from 'lucide-react';
import { type ComponentProps, useCallback, useEffect } from 'react';
import { twMerge } from 'tailwind-merge';
import { sendMessage, WsMessageType } from '../common';
import { useCodeContext } from '../context/code-execution-context';
import Tooltip from './tooltip';

export default function ActionButtons() {
  const {
    editorRef,
    ws,
    xtermRef,
    processActive,
    setProcessActive,
    config,
    setConfig,
    setTerminalState,
    terminalState,
  } = useCodeContext();

  const { debugMode } = config;

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
    setConfig(prev => {
      if (ws)
        sendMessage(
          ws,
          WsMessageType.UPDATE_CONFIG,
          JSON.stringify({ debugMode: !prev.debugMode })
        );
      return { ...prev, debugMode: !prev.debugMode };
    });
  };

  const toggleVisibility = () => {
    setTerminalState(prev => ({ ...prev, visible: !prev.visible }));
  };

  const Icon = processActive ? SquareIcon : debugMode ? BugPlayIcon : PlayIcon;
  const DebugIcon = debugMode ? BugOffIcon : BugIcon;

  return (
    <div className="flex items-center w-full z-1000 px-3 pt-0.5">
      <div className="flex flex-row items-center justify-between grow">
        <div className="flex items-center">
          <ActionButton
            tooltipSide="top-right"
            keybind="Ctrl+Shift+_"
            tooltip={terminalState.visible ? 'Hide terminal' : 'Show terminal'}
            onClick={toggleVisibility}
          >
            <MinusIcon className="stroke-2 stroke-white" />
          </ActionButton>
        </div>
        <div className="flex flex-row items-center justify-end gap-1 grow">
          <ActionButton
            tooltipSide="top-left"
            keybind="Ctrl+Shift+C"
            tooltip="Clear terminal"
            onClick={clearTerminal}
          >
            <Trash2Icon className="stroke-2 stroke-white" />
          </ActionButton>
          <ActionButton
            keybind="Ctrl+Shift+B"
            tooltipSide="top-left"
            tooltip={debugMode ? 'Disable debug mode' : 'Enable debug mode'}
            onClick={toggleDebugMode}
          >
            <DebugIcon />
          </ActionButton>
          <ActionButton
            keybind="Ctrl+R"
            tooltipSide="top-left"
            tooltip={
              processActive
                ? 'Stop process'
                : debugMode
                  ? 'Debug code'
                  : 'Run code'
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
    </div>
  );
}

type ActionButtonProps = Omit<ComponentProps<'button'>, 'onClick'> & {
  tooltip: string;
  tooltipSide?: 'top-left' | 'top-right';
  keybind?: string;
  onClick: () => void;
};

export function ActionButton({
  className,
  tooltip,
  tooltipSide,
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
    <Tooltip
      tooltipSide={tooltipSide}
      tooltip={
        <>
          <span className="grow">{tooltip}</span>
          {keybind && (
            <span className="mx-0.5 justify-end font-mono text-neutral-300 bg-neutral-800 rounded-lg px-1">
              {formattedKeybind}
            </span>
          )}
        </>
      }
    >
      <button
        onClick={onClick}
        className={twMerge(
          'rounded-sm p-1.5 hover:bg-neutral-100/10 transition-colors duration-200 text-white' +
            ' cursor-pointer *:stroke-2 size-8 *:size-full'
        )}
        {...props}
      />
    </Tooltip>
  );
}
