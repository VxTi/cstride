import { FitAddon } from '@xterm/addon-fit';
import { Terminal } from '@xterm/xterm';
import { type ComponentProps, useEffect, useRef, useState } from 'react';
import { twMerge } from 'tailwind-merge';
import { useCodeContext } from '../context/code-execution-context';
import ActionButtons from './action-buttons';

export default function TerminalWindow() {
  const { terminalRef, xtermRef, terminalResizing, setTerminalResizing } =
    useCodeContext();

  const fitAddonRef = useRef<FitAddon | null>(null);
  const [height, setHeight] = useState(300);
  const dragStartRef = useRef<{ startY: number; startHeight: number } | null>(
    null
  );

  useEffect(() => {
    const handleMouseMove = (e: MouseEvent) => {
      if (!terminalResizing || !dragStartRef.current) return;

      const delta = dragStartRef.current.startY - e.clientY;
      const newHeight = dragStartRef.current.startHeight + delta;

      if (newHeight > 50 && newHeight < window.innerHeight - 50) {
        setHeight(newHeight);
      }
    };

    const handleMouseUp = () => {
      setTerminalResizing(false);
      dragStartRef.current = null;
      document.body.style.cursor = '';
      document.body.style.userSelect = '';
    };

    window.addEventListener('mousemove', handleMouseMove);
    window.addEventListener('mouseup', handleMouseUp);

    return () => {
      window.removeEventListener('mousemove', handleMouseMove);
      window.removeEventListener('mouseup', handleMouseUp);
    };
  }, [setTerminalResizing, terminalResizing]);

  /*
   * xterm initialization
   */
  useEffect(() => {
    if (!terminalRef.current) return;

    const term = new Terminal({
      convertEol: true,
      fontFamily: 'monospace',
      fontSize: 14,
      allowTransparency: true,
      theme: {
        background: '#262626',
      },
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
  }, [terminalRef, xtermRef]);

  useEffect(() => {
    if (!fitAddonRef.current) return;

    requestAnimationFrame(() => fitAddonRef.current?.fit());
  }, [height]);

  const handleMouseDown = (e: React.MouseEvent) => {
    setTerminalResizing(true);
    dragStartRef.current = { startY: e.clientY, startHeight: height };
    document.body.style.cursor = 'ns-resize';
    document.body.style.userSelect = 'none';
    e.preventDefault();
  };

  return (
    <div
      style={{ height: `${height}px` }}
      className="bg-neutral-800 flex flex-col rounded-t-3xl"
    >
      <TerminalResizeBar onMouseDown={handleMouseDown} />
      <ActionButtons />
      <div className="p-2.5 overflow-hidden flex-1 min-h-0">
        <div ref={terminalRef} className="size-full" />
      </div>
    </div>
  );
}

function TerminalResizeBar({
  onMouseDown,
}: Pick<ComponentProps<'div'>, 'onMouseDown'>) {
  const { terminalResizing } = useCodeContext();
  return (
    <div onMouseDown={onMouseDown} className="h-2 cursor-ns-resize z-10">
      <div
        className={twMerge(
          'h-1  transition-colors duration-200',
          terminalResizing ? 'bg-blue-500' : 'bg-transparent'
        )}
      />
    </div>
  );
}
