import Editor, { useMonaco } from '@monaco-editor/react';
import { useEffect } from 'react';
import {
  conf,
  language,
  registerCompletionProvider,
  strideLanguageId,
} from './StrideLanguage';

function App() {
  const monaco = useMonaco();

  useEffect(() => {
    if (!monaco) return;

    monaco.languages.register({ id: strideLanguageId });
    monaco.languages.setMonarchTokensProvider(strideLanguageId, language);
    monaco.languages.setLanguageConfiguration(strideLanguageId, conf);
    registerCompletionProvider(monaco);
  }, [monaco]);

  return (
    <div style={{ height: '100vh', width: '100vw', margin: 0, padding: 0 }}>
      <Editor
        height="100%"
        defaultLanguage={strideLanguageId}
        defaultValue={`// Welcome to Stride Editor
// Try writing some Stride code here

fn main() {
    let x: i32 = 10;
    let name: string = "Stride";
    
    if (x > 5) {
        return true;
    }
    
    return false;
}`}
        theme="vs-dark"
        options={{
          minimap: { enabled: true },
          fontSize: 14,
        }}
      />
    </div>
  );
}

export default App;
