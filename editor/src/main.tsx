import React from 'react';
import ReactDOM  from 'react-dom/client';
import AppWindow from './components/app-window';
import './index.css';
import { CodeContextProvider } from './context/code-execution-context';

ReactDOM.createRoot(document.getElementById('root')!).render(
  <React.StrictMode>
    <CodeContextProvider>
      <AppWindow />
    </CodeContextProvider>
  </React.StrictMode>
);
