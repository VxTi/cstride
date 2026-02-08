import './index.css';

import React from 'react';
import ReactDOM from 'react-dom/client';
import AppWindow from './components/app-window';
import { CodeContextProvider } from './context/code-execution-context';

ReactDOM.createRoot(document.getElementById('root') as HTMLElement).render(
  <React.StrictMode>
    <CodeContextProvider>
      <AppWindow />
    </CodeContextProvider>
  </React.StrictMode>
);
