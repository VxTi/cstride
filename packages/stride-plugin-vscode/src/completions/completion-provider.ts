import { type IDisposable, type languages }    from 'monaco-editor';
import { strideLanguageId }                    from '../language-config';
import { provideFunctionReferenceCompletions } from './function-reference-completions';
import { provideKeywordCompletions }           from './keyword-completions';

export function registerCompletionProvider(
  monaco: typeof import('monaco-editor')
): IDisposable {
  return monaco.languages.registerCompletionItemProvider(strideLanguageId, {
    provideCompletionItems: (model, position) => {
      const lineContent = model.getLineContent(position.lineNumber);
      const textUntilPosition = lineContent.substring(0, position.column - 1);

      const suggestions: languages.CompletionItem[] = [];

      // Add doc comment snippet if /** is typed
      if (textUntilPosition.endsWith('/**')) {
        suggestions.push({
          label: '/** */',
          kind: monaco.languages.CompletionItemKind.Snippet,
          documentation: 'JSDoc-style documentation comment',
          insertText: '/**\n * $0\n */',
          insertTextRules:
            monaco.languages.CompletionItemInsertTextRule.InsertAsSnippet,
          range: {
            startLineNumber: position.lineNumber,
            endLineNumber: position.lineNumber,
            startColumn: position.column - 3,
            endColumn: position.column,
          },
        });
      }

      suggestions.push(
        ...provideKeywordCompletions(monaco, model, position),
        ...provideFunctionReferenceCompletions(monaco, model, position)
      );

      return { suggestions };
    },
  });
}
