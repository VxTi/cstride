import Editor from '@monaco-editor/react';
import { useMemo, useState } from 'react';
import { useCodeContext } from '../context/code-execution-context';
import { strideLanguageId } from '../lib/stride-language/language-config';
import {
  installStrideOneDarkTheme,
  oneDarkThemeName,
} from './editor-theme';

export default function EditorWindow() {
  const [initialCode] = useState(
    () => localStorage.getItem('stride_code') || defaultCodeFragment
  );
  const { onEditorMount } = useCodeContext();

  const debounceSave = useMemo(() => {
    let timeout: number | null = null;

    return (value: string | undefined) => {
      if (timeout) window.clearTimeout(timeout);

      timeout = window.setTimeout(() => {
        if (!value) {
          localStorage.removeItem('stride_code');
          return;
        }
        localStorage.setItem('stride_code', value);
      }, 200);
    };
  }, []);
  return (
    <Editor
      height="100%"
      className="absolute left-0 top-0 size-full bg-neutral-950"
      defaultLanguage={strideLanguageId}
      defaultValue={initialCode}
      theme={oneDarkThemeName}
      beforeMount={installStrideOneDarkTheme}
      onMount={onEditorMount}
      onChange={debounceSave}
      options={{
        minimap: { enabled: false },
        fontSize: 14,
      }}
    />
  );
}
const defaultCodeFragment = `// Welcome to Stride Editor
// Try writing some Stride code here

fn main(): void {
    let x: i32 = 10;
    let name: string = "Stride";
    
    if (x > 5) {
        return true;
    }
    
    return false;
}`;
