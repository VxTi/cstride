import React, { type ComponentProps } from 'react';

const MINIMUM_SIDEBAR_WIDTH_PX = 100;

export default function Sidebar() {
  // const [widthOffset, setWidthOffset] = React.useState(0);

  const onSidebarMouseDown = (event: React.MouseEvent) => {
    // dragStartRef.current = { startY: event.clientY, startHeight: widthOffset };
    document.body.style.cursor = 'ns-resize';
    document.body.style.userSelect = 'none';
    event.preventDefault();
  };

  return (
    <div className="flex">
      <div
        className="flex flex-col "
        style={{ width: MINIMUM_SIDEBAR_WIDTH_PX }}
      ></div>
      <SidebarResizeBar onMouseDown={onSidebarMouseDown} />
      <div className="bg-blue-400 w-px h-full" />
    </div>
  );
}

function SidebarResizeBar({
  onMouseDown,
}: Pick<ComponentProps<'div'>, 'onMouseDown'>) {
  return (
    <div onMouseDown={onMouseDown} className="h-4 z-10 group cursor-ns-resize!">
      <div className="h-px  transition-colors duration-200 group-hover:bg-blue-400/50" />
    </div>
  );
}
