import { registerCompletionProvider } from './completions/provider';
import { registerHoverProvider } from './hover-provider';
import { conf, language, strideLanguageId } from './language-config';

export function registerLanguage(editor: typeof import('monaco-editor')) {
  editor.languages.register({ id: strideLanguageId });
  editor.languages.setMonarchTokensProvider(strideLanguageId, language);
  editor.languages.setLanguageConfiguration(strideLanguageId, conf);

  const completionDisposable = registerCompletionProvider(editor);
  const hoverDisposable = registerHoverProvider(editor);

  return () => {
    completionDisposable.dispose();
    hoverDisposable.dispose();
  };
}
