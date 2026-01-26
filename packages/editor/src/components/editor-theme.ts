import type { editor } from 'monaco-editor';

export const oneDarkThemeName = 'stride-one-dark';

/**
 * Registers the Stride One Dark-like theme with Monaco.
 * Call this once before rendering the editor (or inside `beforeMount`).
 */
export function installStrideOneDarkTheme(monaco: { editor: typeof editor }) {
  monaco.editor.defineTheme(oneDarkThemeName, {
    base: 'vs-dark',
    inherit: true,
    rules: [
      // Core
      { token: '', foreground: 'ABB2BF' },
      { token: 'comment', foreground: '5C6370', fontStyle: 'italic' },
      { token: 'number', foreground: 'D19A66' },
      { token: 'string', foreground: '98C379' },
      { token: 'keyword', foreground: 'C678DD' },
      { token: 'operator', foreground: '56B6C2' },
      { token: 'delimiter', foreground: 'ABB2BF' },

      // Identifiers / calls
      { token: 'identifier', foreground: 'E06C75' },
      { token: 'function', foreground: '61AFEF' },
      { token: 'type.identifier', foreground: 'E5C07B' },

      // Common Monaco tokens
      { token: 'variable', foreground: 'E06C75' },
      { token: 'type', foreground: 'E5C07B' },
    ],
    colors: {
      // Editor background/foreground
      'editor.background': '#121418',
      'editor.foreground': '#ABB2BF',

      // Selection
      'editor.selectionBackground': '#3E4451',
      'editor.inactiveSelectionBackground': '#21242b',
      'editor.selectionHighlightBackground': '#16191e',

      // Cursor / line highlight
      'editorCursor.foreground': '#528BFF',
      'editor.lineHighlightBackground': '#171a20',

      // Find
      'editor.findMatchBackground': '#42557B',
      'editor.findMatchHighlightBackground': '#314365',

      // Gutter
      'editorLineNumber.foreground': '#4B5263',
      'editorLineNumber.activeForeground': '#ABB2BF',
      'editorGutter.background': '#121418',

      // Indent guides
      'editorIndentGuide.background1': '#3B4048',
      'editorIndentGuide.activeBackground1': '#4B5263',

      // Brackets
      'editorBracketMatch.background': '#3E4451',
      'editorBracketMatch.border': '#ABB2BF',

      // Suggestions
      'editorSuggestWidget.background': '#21252B',
      'editorSuggestWidget.border': '#181A1F',
      'editorSuggestWidget.foreground': '#ABB2BF',
      'editorSuggestWidget.selectedBackground': '#2C313A',

      // Hover
      'editorHoverWidget.background': '#21252B',
      'editorHoverWidget.border': '#181A1F',

      // Scrollbar
      'scrollbarSlider.background': '#4B526340',
      'scrollbarSlider.hoverBackground': '#4B526360',
      'scrollbarSlider.activeBackground': '#4B526380',
    },
  });
}

