import { FitAddon } from '@xterm/addon-fit';
import { Terminal } from '@xterm/xterm';
import { AnimatePresence, motion } from 'framer-motion';
import { TerminalIcon } from 'lucide-react';
import React, { type ComponentProps, useEffect, useRef, useState } from 'react';
import { twMerge } from 'tailwind-merge';
import { useCodeContext } from '../context/code-execution-context';
import ActionButtons from './action-buttons';
import Tooltip from './tooltip';

export default function TerminalWindow() {
  const { terminalState, xtermRef, setTerminalState } = useCodeContext();

  const { ref, resizing, visible } = terminalState;

  const fitAddonRef = useRef<FitAddon | null>(null);
  const [height, setHeight] = useState(300);
  const dragStartRef = useRef<{ startY: number; startHeight: number } | null>(
    null
  );

  useEffect(() => {
    const handleMouseMove = (e: MouseEvent) => {
      if (!resizing || !dragStartRef.current) return;

      const delta = dragStartRef.current.startY - e.clientY;
      const newHeight = dragStartRef.current.startHeight + delta;

      if (newHeight > 50 && newHeight < window.innerHeight - 50) {
        setHeight(newHeight);
      }
    };

    const handleMouseUp = () => {
      setTerminalState(prev => ({ ...prev, resizing: false }));
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
  }, [setTerminalState, resizing]);

  /*
   * xterm initialization
   */
  useEffect(() => {
    if (!ref.current) return;

    const term = new Terminal({
      convertEol: true,
      fontFamily: 'monospace',
      fontSize: 14,
      allowTransparency: true,
      theme: {
        background: '#121418',
        red: '#ff5555',
      },
    });
    const fitAddon = new FitAddon();
    term.loadAddon(fitAddon);

    term.open(ref.current);
    fitAddon.fit();

    xtermRef.current = term;
    fitAddonRef.current = fitAddon;

    const handleResize = () => fitAddon.fit();
    window.addEventListener('resize', handleResize);

    return () => {
      term.dispose();
      window.removeEventListener('resize', handleResize);
    };
  }, [ref, xtermRef]);

  useEffect(() => {
    if (!fitAddonRef.current || !terminalState.visible) return;

    requestAnimationFrame(() => fitAddonRef.current?.fit());
  }, [height, terminalState.visible]);

  const handleMouseDown = (e: React.MouseEvent) => {
    setTerminalState(prev => ({ ...prev, resizing: true }));
    dragStartRef.current = { startY: e.clientY, startHeight: height };
    document.body.style.cursor = 'ns-resize';
    document.body.style.userSelect = 'none';
    e.preventDefault();
  };

  return (
    <>
      <div
        className={twMerge(
          'origin-bottom-left',
          !visible && 'hidden pointer-events-none'
        )}
      >
        <motion.div
          key="terminal-open"
          initial={{ opacity: 0, transform: 'scale(0)' }}
          animate={
            visible
              ? { opacity: 1, transform: 'scale(1)' }
              : { opacity: 0, transform: 'scale(0)' }
          }
          transition={{ duration: 0.25 }}
          className="origin-bottom-left"
        >
          <div
            style={{ height: `${height}px` }}
            className={twMerge(
              'bg-[#121418] flex flex-col rounded-t-3xl mx-1 border border-neutral-700',
              resizing && 'border-t-blue-500'
            )}
          >
            <TerminalResizeBar onMouseDown={handleMouseDown} />
            <ActionButtons />
            <div className="p-2.5 overflow-hidden flex-1 min-h-0">
              <div ref={ref} className="size-full" />
            </div>
          </div>
        </motion.div>
      </div>

      <AnimatePresence>
        {!visible && (
          <motion.div
            key="terminal-closed"
            initial={{ opacity: 0, transform: 'scale(2)' }}
            animate={{ opacity: 1, transform: 'scale(1)' }}
            exit={{ opacity: 0, transform: 'scale(1.1)' }}
            transition={{
              duration: 0.2,
            }}
            className="origin-bottom-left max-w-max"
          >
            <Tooltip tooltip="Open Terminal" tooltipSide="top-right">
              <button
                className="bg-neutral-900 border-neutral-600 rounded-2xl border-2 p-1 cursor-pointer"
                title="Open Terminal"
                onClick={() =>
                  setTerminalState(prev => ({ ...prev, visible: true }))
                }
              >
                <TerminalIcon className="stroke-white" />
              </button>
            </Tooltip>
          </motion.div>
        )}
      </AnimatePresence>
    </>
  );
}

function TerminalResizeBar({
  onMouseDown,
}: Pick<ComponentProps<'div'>, 'onMouseDown'>) {
  return (
    <div onMouseDown={onMouseDown} className="h-2 cursor-ns-resize z-10">
      <div className="h-1  transition-colors duration-200" />
    </div>
  );
}
