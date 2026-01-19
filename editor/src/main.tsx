import React from 'react';
import ReactDOM from 'react-dom/client';
import App from './App';
import './index.css';
import { CodeContextProvider } from './context/code-execution-context';

ReactDOM.createRoot(document.getElementById('root')!).render(
  <React.StrictMode>
    <CodeContextProvider>
      <App />
    </CodeContextProvider>
  </React.StrictMode>
);
