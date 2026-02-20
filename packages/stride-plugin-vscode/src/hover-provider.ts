import { editor, type IDisposable } from 'monaco-editor';
import { strideLanguageId } from './language-config';
import ITextModel = editor.ITextModel;

export function registerHoverProvider(
  monaco: typeof import('monaco-editor')
): IDisposable {
  return monaco.languages.registerHoverProvider(strideLanguageId, {
    provideHover: (model, position) => {
      const word = model.getWordAtPosition(position);
      if (!word) return null;

      // Find definition: looks for 'fn <word>'
      // We use a regex that matches 'fn' followed by whitespace and the word.
      // This should match 'fn name', 'public fn name', etc.
      // It will NOT match 'name()' invocation because of the 'fn' prefix.
      const searchString = `\\bfn\\s+${word.word}\\b`;

      // Search (limit to 1 match for performance/simplicity)
      const matches = model.findMatches(
        searchString,
        false,
        true,
        true,
        null,
        false,
        1
      );

      if (matches.length === 0) {
        return null;
      }

      const definitionLine = matches[0].range.startLineNumber;
      const commentText = getDocComment(model, definitionLine);

      if (commentText) {
        return {
          range: new monaco.Range(
            position.lineNumber,
            word.startColumn,
            position.lineNumber,
            word.endColumn
          ),
          contents: [{ value: '**Documentation**' }, { value: commentText }],
        };
      }

      return null;
    },
  });
}

function getDocComment(model: ITextModel, lineNo: number): string | null {
  let endCommentLine = lineNo - 1;

  // Skip whitespace lines between function and comment
  while (
    endCommentLine > 0 &&
    model.getLineContent(endCommentLine).trim().length === 0
  ) {
    endCommentLine--;
  }

  if (endCommentLine < 1) return null;

  const endLineContent = model.getLineContent(endCommentLine).trim();
  let commentText = '';

  if (endLineContent.endsWith('*/')) {
    // Block comment
    let startCommentLine = endCommentLine;
    let foundStart = false;

    // Search backwards for the start of the block comment
    const lines: string[] = [];

    while (startCommentLine >= 1) {
      const line = model.getLineContent(startCommentLine);
      lines.unshift(line);
      if (line.includes('/*')) {
        foundStart = true;
        break;
      }
      startCommentLine--;
    }

    if (foundStart) {
      const fullBlock = lines.join('\n');
      const match = fullBlock.match(/\/\*([\s\S]*?)\*\//);
      if (match) {
        // Clean up stars and whitespace
        commentText = match[1]
          .split('\n')
          .map(l => {
            const trimmed = l.trim();
            if (trimmed.startsWith('*')) {
              return trimmed.substring(1).trim();
            }
            return trimmed;
          })
          .join('\n')
          .trim();
      }
    }
  } else if (endLineContent.startsWith('//')) {
    // Single line comments group
    const lines: string[] = [];
    let curr = endCommentLine;
    while (curr >= 1) {
      const line = model.getLineContent(curr);
      const trimmed = line.trim();
      if (trimmed.startsWith('//')) {
        lines.unshift(trimmed.substring(2).trim());
        curr--;
      } else {
        break;
      }
    }
    commentText = lines.join('\n');
  }

  return commentText || null;
}
