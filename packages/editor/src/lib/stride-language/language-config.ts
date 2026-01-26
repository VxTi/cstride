import { languages } from 'monaco-editor';

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
    { open: '/**', close: ' */' },
  ],
  surroundingPairs: [
    { open: '{', close: '}' },
    { open: '[', close: ']' },
    { open: '(', close: ')' },
    { open: '"', close: '"' },
    { open: "'", close: "'" },
    { open: '/**', close: ' */' },
  ],
  onEnterRules: [
    {
      // Matches /** followed by anything except / and ending the line.
      beforeText: /^\s*\/\*\*(?!\/)([^*]|\*(?!\/))*$/,
      afterText: /^\s*\*\/$/,
      action: {
        indentAction: languages.IndentAction.IndentOutdent,
        appendText: ' * ',
      },
    },
    {
      // Matches /** followed by anything except / and ending the line.
      beforeText: /^\s*\/\*\*(?!\/)([^*]|\*(?!\/))*$/,
      action: {
        indentAction: languages.IndentAction.None,
        appendText: ' * ',
      },
    },
    {
      // Matches * optionally followed by a space and anything except /
      beforeText: /^(\t|[ ])*[ ]\*([ ]([^*]|\*(?!\/))*)?$/,
      action: {
        indentAction: languages.IndentAction.None,
        appendText: '* ',
      },
    },
  ],
};

export const keywords = [
  'let',
  'use',
  'mut',
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
  'module',
  'package',
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
  'f32',
  'f64',
  'char',
  'string',
  'void',
  'auto',
  'true',
  'false',
];

export const language: languages.IMonarchLanguage = {
  defaultToken: '',
  tokenPostfix: `.${languageExtension}`,
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
        /[a-zA-Z_]\w*(?=\s*\()/,
        {
          cases: {
            '@keywords': 'keyword',
            '@default': 'function',
          },
        },
      ],
      [
        /[a-zA-Z_]\w*/,
        {
          cases: {
            '@keywords': 'keyword',
            '@default': 'identifier',
          },
        },
      ],

      // Whitespace
      { include: '@whitespace' },

      // Delimiters and operators
      [/[{}()\[\]]/, '@brackets'],
      [
        /@symbols/,
        {
          cases: {
            '@operators': 'operator',
            '@default': '',
          },
        },
      ],

      // Punctuation
      [/[;,]/, 'delimiter'],

      // Numbers
      [/\d*\.\d+([eE][\-+]?\d+)?[dD]?/, 'number.float'],
      [/0[xX][0-9a-fA-F]+/, 'number.hex'],
      [/\d+[lL]?/, 'number'],

      // Strings
      [/"([^"\\]|\\.)*$/, 'string.invalid'], // non-terminated string
      [/"/, { token: 'string.quote', bracket: '@open', next: '@string' }],

      // Characters
      [/'/, { token: 'string.quote', bracket: '@open', next: '@character' }],
    ],

    whitespace: [
      [/[ \t\r\n]+/, 'white'],
      [/\/\*/, 'comment', '@comment'],
      [/\/\/.*$/, 'comment'],
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

    comment: [
      [/[^\/*]+/, 'comment'],
      [/\/\*/, 'comment', '@push'], // nested comment
      [/\*\//, 'comment', '@pop'],
      [/[\/*]/, 'comment'],
    ],
  },
};
