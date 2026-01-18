import Editor, { useMonaco } from '@monaco-editor/react';
import { useEffect } from 'react';
import { strideLanguageId } from './stride-language/language-config';
import { registerLanguage } from './stride-language/stride-language';

function App() {
  const monaco = useMonaco();

  useEffect(() => {
    if (!monaco) return;

    return registerLanguage(monaco);
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
