import { editor, languages, type Position } from 'monaco-editor';
import ITextModel = editor.ITextModel;

export const strideLanguageId = 'stride';
export const languageExtension = 'sr';

export const conf: languages.LanguageConfiguration = {
  comments: {
    lineComment: '//',
    blockComment: ['/*', '*/'],
  },
  brackets: [
    ['{', '}'],
    ['[', ']'],
    ['(', ')'],
  ],
  autoClosingPairs: [
    { open: '{', close: '}' },
    { open: '[', close: ']' },
    { open: '(', close: ')' },
    { open: '"', close: '"' },
    { open: "'", close: "'" },
  ],
  surroundingPairs: [
    { open: '{', close: '}' },
    { open: '[', close: ']' },
    { open: '(', close: ')' },
    { open: '"', close: '"' },
    { open: "'", close: "'" },
  ],
  /* onEnterRules: [
    {
      beforeText: /^\s*\/\*\*(?!\/)([^\*]|\*(?!\/))*$/,
      afterText:  /^\s*\*\/$/,
      action:     {
        indentAction: languages.IndentAction.IndentOutdent,
        appendText:   ' * ',
      },
    },
    {
      beforeText: /^\s*\/\*\*(?!\/)([^\*]|\*(?!\/))*$/,
      action:     {
        indentAction: languages.IndentAction.None,
        appendText:   ' * ',
      },
    },
    {
      beforeText: /^(\t|[ ])*[ ]\*([ ]([^\*]|\*(?!\/))*)?$/,
      action:     {
        indentAction: languages.IndentAction.None,
        appendText:   '* ',
      },
    },
  ],*/
};

export const keywords = [
  'let',
  'use',
  'const',
  'fn',
  'if',
  'else',
  'while',
  'for',
  'return',
  'break',
  'continue',
  'struct',
  'enum',
  'case',
  'default',
  'import',
  'nil',
  'class',
  'this',
  'public',
  'mod',
  'extern',
  'override',
  'as',
  'async',
  'do',
  'switch',
  'try',
  'catch',
  'throw',
  'new',
  'bool',
  'i8',
  'i16',
  'i32',
  'i64',
  'u8',
  'u16',
  'u32',
  'u64',
  'float32',
  'float64',
  'char',
  'string',
  'void',
  'auto',
  'true',
  'false',
];

export const language: languages.IMonarchLanguage = {
  defaultToken: '',
  tokenPostfix: languageExtension,
  keywords,
  operators: [
    '**=',
    '<<=',
    '>>=',
    '*=',
    '/=',
    '%=',
    '+=',
    '-=',
    '&=',
    '|=',
    '^=',
    '~=',
    '!=',
    '==',
    '<=',
    '>=',
    '<<',
    '>>',
    '++',
    '--',
    '**',
    '::',
    '&&',
    '||',
    '->',
    '<-',
    '...',
    '+',
    '-',
    '*',
    '/',
    '%',
    '=',
    '<',
    '>',
    '!',
    '?',
    '&',
    '|',
    '^',
    '~',
    '.',
  ],

  symbols: /[=><!~?:&|+\-*\/^%]+/,

  escapes:
    /\\(?:[abfnrtv\\"']|x[0-9A-Fa-f]{1,4}|u[0-9A-Fa-f]{4}|U[0-9A-Fa-f]{8})/,

  brackets: [
    { open: '{', close: '}', token: 'delimiter.curly' },
    { open: '[', close: ']', token: 'delimiter.square' },
    { open: '(', close: ')', token: 'delimiter.parenthesis' },
  ],

  tokenizer: {
    root: [
      // Identifiers and keywords
      [
        /[a-zA-Z_]\w*/,
        { cases: { '@keywords': 'keyword', '@default': 'identifier' } },
      ],

      // Whitespace
      { include: '@whitespace' },

      // Delimiters and operators
      [/[{}()\[\]]/, '@brackets'],
      [/@symbols/, { cases: { '@operators': 'operator', '@default': '' } }],

      // Punctuation
      [/[;,]/, 'delimiter'],

      // Numbers
      [/\d+\.\d+/, 'number.float'],
      [/\d+/, 'number'],

      // Strings
      [/"([^"\\]|\\.)*$/, 'string.invalid'], // non-terminated string
      [/"/, { token: 'string.quote', bracket: '@open', next: '@string' }],

      // Characters
      [/'/, { token: 'string.quote', bracket: '@open', next: '@character' }],
    ],

        comment: [
            [/[^\/*]+/, 'comment' ],
            [/\/\*/,    'comment', '@push' ],    // nested comment
            ["\\*/",    'comment', '@pop'  ],
            [/[\/*]/,   'comment' ]
        ],

    string: [
      [/[^\\"]+/, 'string'],
      [/@escapes/, 'string.escape'],
      [/\\./, 'string.escape.invalid'],
      [/"/, { token: 'string.quote', bracket: '@close', next: '@pop' }],
    ],

    character: [
      [/[^\\']+/, 'string'],
      [/@escapes/, 'string.escape'],
      [/\\./, 'string.escape.invalid'],
      [/'/, { token: 'string.quote', bracket: '@close', next: '@pop' }],
    ],

    whitespace: [
      [/[ \t\r\n]+/, 'white'],
      [/\/\*/, 'comment', '@comment'],
      [/\/\/.*$/, 'comment'],
    ],
  },
};

export function registerCompletionProvider(
  monaco: typeof import('monaco-editor')
): void {
  monaco.languages.registerCompletionItemProvider(strideLanguageId, {
    provideCompletionItems: (model, position) => {
      const suggestions = provideKeywordCompletions(model, position);

      return { suggestions };
    },
  });
}

function provideKeywordCompletions(
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

  return filteredKeywords.map(
    keyword => ({
      label:      keyword,
      kind:       languages.CompletionItemKind.Keyword,
      insertText: keyword,
      range:      {
        startLineNumber: position.lineNumber,
        endLineNumber:   position.lineNumber,
        startColumn,
        endColumn,
      },
    })
  );
}
