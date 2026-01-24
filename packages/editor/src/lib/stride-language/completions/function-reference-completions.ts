import { editor, languages, type Position } from 'monaco-editor';
import ITextModel = editor.ITextModel;

export function provideFunctionReferenceCompletions(
  monaco: typeof import('monaco-editor'),
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

  // Get all text from the model to parse function definitions
  const fullText = model.getValue();

  // Find all function definitions using regex pattern: fn functionName(
  const functionPattern = /\bfn\s+([a-zA-Z_]\w*)\s*\(/g;
  const userDefinedFunctions: string[] = [];
  let regexMatch;

  while ((regexMatch = functionPattern.exec(fullText)) !== null) {
    userDefinedFunctions.push(regexMatch[1]);
  }

  // Combine user-defined and built-in functions, removing duplicates
  const allFunctions = [...userDefinedFunctions];

  // Filter functions based on the typed prefix
  const filteredFunctions = allFunctions.filter(func =>
    func.startsWith(wordPrefix)
  );

  return filteredFunctions.map(func => ({
    label: func,
    kind: languages.CompletionItemKind.Function,
    insertText: `${func}($1);`,
    insertTextRules:
      monaco.languages.CompletionItemInsertTextRule.InsertAsSnippet,
    range: {
      startLineNumber: position.lineNumber,
      endLineNumber: position.lineNumber,
      startColumn,
      endColumn,
    },
  }));
}
