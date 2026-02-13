import { type ReactNode } from 'react';
import { twMerge } from 'tailwind-merge';

type TooltipSide = 'top-left' | 'top-right';

export interface TooltipProps {
  tooltip: ReactNode;
  tooltipSide?: TooltipSide;
  children: ReactNode;
}

export default function Tooltip({
  tooltip,
  tooltipSide,
  children,
}: TooltipProps) {
  return (
    <div className="relative group">
      <div
        className={twMerge(
          'absolute top-0 -translate-y-full pointer-events-none',
          tooltipSide === 'top-right' ? 'left-0' : 'right-0'
        )}
      >
        <div className="bg-neutral-900 max-w-46 w-max flex gap-2 items-center wrap-break-word text-white rounded-xl border border-neutral-700 px-2.5 py-1 opacity-0 group-hover:opacity-100 transition-opacity duration-200">
          {tooltip}
        </div>
      </div>
      {children}
    </div>
  );
}
