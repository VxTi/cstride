import Editor from '@monaco-editor/react';
import { useEffect } from 'react';
import useCustomMonaco from './hooks/monaco';
import {
  conf,
  language,
  registerCompletionProvider,
  strideLanguageId,
} from './StrideLanguage';

function App() {
  const monaco = useCustomMonaco();

  useEffect(() => {
    if (!monaco) return;

    monaco.languages.register({ id: strideLanguageId });
    monaco.languages.setMonarchTokensProvider(strideLanguageId, language);
    monaco.languages.setLanguageConfiguration(strideLanguageId, conf);

    const disposable = registerCompletionProvider(monaco);
    return () => disposable.dispose();
  }, [monaco]);

  return (
    <div style={{ height: '100vh', width: '100vw', margin: 0, padding: 0 }}>
      <Editor
        height="100%"
        defaultLanguage={strideLanguageId}
        defaultValue={defaultCodeFragment}
        theme="vs-dark"
        options={{
          minimap: { enabled: false },
          fontSize: 14,
        }}
      />
    </div>
  );
}

const defaultCodeFragment = `// Welcome to Stride Editor
// Try writing some Stride code here

fn main() {
    let x: i32 = 10;
    let name: string = "Stride";
    
    if (x > 5) {
        return true;
    }
    
    return false;
}`;

export default App;
