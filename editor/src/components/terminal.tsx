import { FitAddon } from '@xterm/addon-fit';
import { Terminal } from '@xterm/xterm';
import { useEffect, useRef, useState } from 'react';
import { useCodeContext } from '../context/code-execution-context';

export default function TerminalWindow() {
  const { terminalRef, xtermRef, terminalResizing, setTerminalResizing } =
    useCodeContext();

  const fitAddonRef = useRef<FitAddon | null>(null);
  const [height, setHeight] = useState(300);
  const isResizing = useRef(false);

  useEffect(() => {
    const handleMouseMove = (e: MouseEvent) => {
      if (!isResizing.current) return;

      const newHeight = window.innerHeight - e.clientY;
      if (newHeight > 50 && newHeight < window.innerHeight - 50) {
        setHeight(newHeight);
      }
    };

    const handleMouseUp = () => {
      if (!isResizing.current) return;

      isResizing.current = false;
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
  }, []);

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
  }, []);

  useEffect(() => {
    if (fitAddonRef.current) {
      requestAnimationFrame(() => fitAddonRef.current?.fit());
    }
  }, [height]);

  return (
    <>
      <div
        onMouseDown={e => {
          isResizing.current = true;
          setTerminalResizing(true);
          document.body.style.cursor = 'ns-resize';
          document.body.style.userSelect = 'none';
          e.preventDefault();
        }}
        style={{
          height: '5px',
          cursor: 'ns-resize',
          backgroundColor: terminalResizing ? '#007fd4' : '#333',
          transition: 'background-color 0.2s ease',
          zIndex: 10,
        }}
      />
      <div
        style={{
          height: `${height}px`,
          backgroundColor: '#1E1E1E',
          padding: '10px',
          overflow: 'hidden',
        }}
      >
        <div ref={terminalRef} style={{ width: '100%', height: '100%' }} />
      </div>
    </>
  );
}
