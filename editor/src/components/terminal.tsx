import { FitAddon } from '@xterm/addon-fit';
import { Terminal } from '@xterm/xterm';
import { useEffect, useRef, useState } from 'react';
import { twMerge } from 'tailwind-merge';
import { useCodeContext } from '../context/code-execution-context';

export default function TerminalWindow() {
  const { terminalRef, xtermRef, terminalResizing, setTerminalResizing } =
    useCodeContext();

  const fitAddonRef = useRef<FitAddon | null>(null);
  const [height, setHeight] = useState(300);

  useEffect(() => {
    const handleMouseMove = (e: MouseEvent) => {
      if (!terminalResizing) return;

      const newHeight = window.innerHeight - e.clientY;

      console.log(newHeight);
      if (newHeight > 50 && newHeight < window.innerHeight - 50) {
        setHeight(newHeight);
        console.log('Height updated:', newHeight);
      }
    };

    const handleMouseUp = () => {
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
  }, [setTerminalResizing, terminalResizing]);

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
  }, [terminalRef, xtermRef]);

  useEffect(() => {
    if (fitAddonRef.current) {
      requestAnimationFrame(() => fitAddonRef.current?.fit());
    }
  }, [height]);

  return (
    <>
      <TerminalResizeBar />
      <div
        className={twMerge('p-2.5 overflow-hidden')}
        style={{ height: `${height}px`, backgroundColor: '#1E1E1E' }}
      >
        <div ref={terminalRef} className="size-full" />
      </div>
    </>
  );
}

function TerminalResizeBar() {
  const { setTerminalResizing, terminalResizing } = useCodeContext();
  return (
    <div
      onMouseDown={e => {
        setTerminalResizing(true);
        document.body.style.cursor = 'ns-resize';
        document.body.style.userSelect = 'none';
        e.preventDefault();
      }}
      className={twMerge(
        'h-1 cursor-ns-resize z-10 transition-colors duration-200',
        terminalResizing ? 'bg-blue-500' : 'bg-stone-700'
      )}
    />
  );
}
