import { editor, languages, type Position } from 'monaco-editor';
import { keywords }                         from '../language-config';
import ITextModel = editor.ITextModel;

export function provideKeywordCompletions(
  _monaco: typeof import('monaco-editor'),
  model: ITextModel,
  position: Position
): languages.CompletionItem[] {
  // Get the text on the current line up to the cursor position
  const lineContent = model.getLineContent(position.lineNumber);
  const textUntilPosition = lineContent.substring(0, position.column - 1);

  // Find the start of the current word
  const match = textUntilPosition.match(/[a-zA-Z_]\w*$/);
  const wordPrefix = match ? match[0] : '';

  // Calculate the range to replace
  const startColumn = position.column - wordPrefix.length;
  const endColumn = position.column;

  // Filter keywords based on the typed prefix
  const filteredKeywords = keywords.filter(keyword =>
    keyword.startsWith(wordPrefix)
  );

  return filteredKeywords.map(keyword => ({
    label: keyword,
    kind: languages.CompletionItemKind.Keyword,
    insertText: keyword,
    range: {
      startLineNumber: position.lineNumber,
      endLineNumber: position.lineNumber,
      startColumn,
      endColumn,
    },
  }));
}
