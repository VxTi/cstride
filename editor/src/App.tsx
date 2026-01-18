import { useEffect } from 'react';
import Editor, { useMonaco } from '@monaco-editor/react';
import { language, conf, languageID } from './StrideLanguage';

function App() {
  const monaco = useMonaco();

  useEffect(() => {
    if (monaco) {
      // Register custom language
      monaco.languages.register({ id: languageID });
      monaco.languages.setMonarchTokensProvider(languageID, language);
      monaco.languages.setLanguageConfiguration(languageID, conf);
    }
  }, [monaco]);

  return (
    <div style={{ height: '100vh', width: '100vw', margin: 0, padding: 0 }}>
      <Editor
        height="100%"
        defaultLanguage={languageID}
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

