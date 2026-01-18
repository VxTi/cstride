import { languages } from 'monaco-editor';

export const languageID = 'stride';
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
};

export const language: languages.IMonarchLanguage = {
    defaultToken: '',
    tokenPostfix: '.sr',

    keywords: [
        'let', 'use', 'const', 'fn', 'if', 'else', 'while', 'for', 'return', 'break', 'continue',
        'struct', 'enum', 'case', 'default', 'import', 'nil', 'class', 'this', 'public', 'mod',
        'extern', 'override', 'as', 'async', 'do', 'switch', 'try', 'catch', 'throw', 'new',
        'bool', 'i8', 'i16', 'i32', 'i64', 'u8', 'u16', 'u32', 'u64', 'float32', 'float64',
        'char', 'string', 'void', 'auto', 'true', 'false'
    ],

    operators: [
        '**=', '<<=', '>>=', '*=', '/=', '%=', '+=', '-=', '&=', '|=', '^=', '~=',
        '!=', '==', '<=', '>=', '<<', '>>', '++', '--', '**', '::', '&&', '||',
        '->', '<-', '...', '+', '-', '*', '/', '%', '=', '<', '>', '!', '?', '&', '|', '^', '~', '.'
    ],

    symbols: /[=><!~?:&|+\-*\/^%.]+/,

    escapes: /\\(?:[abfnrtv\\"']|x[0-9A-Fa-f]{1,4}|u[0-9A-Fa-f]{4}|U[0-9A-Fa-f]{8})/,

    brackets: [
        { open: '{', close: '}', token: 'delimiter.curly' },
        { open: '[', close: ']', token: 'delimiter.square' },
        { open: '(', close: ')', token: 'delimiter.parenthesis' },
    ],

    tokenizer: {
        root: [
            // Identifiers and keywords
            [/[a-zA-Z_]\w*/, { cases: { '@keywords': 'keyword',
                                        '@default': 'identifier' } }],

            // Whitespace
            { include: '@whitespace' },

            // Delimiters and operators
            [/[{}()\[\]]/, '@brackets'],
            [/[<>](?!@symbols)/, '@brackets'], // HTML-like tags (optional) or just brackets
            [/@symbols/, { cases: { '@operators': 'operator',
                                    '@default': '' } }],

            // Punctuation
            [/[;,]/, 'delimiter'],

            // Numbers
            [/\d+\.\d+/, 'number.float'],
            [/\d+/, 'number'],

            // Strings
            [/"([^"\\]|\\.)*$/, 'string.invalid' ],  // non-terminated string
            [/"/,  { token: 'string.quote', bracket: '@open', next: '@string' } ],

            // Characters
            [/'/, { token: 'string.quote', bracket: '@open', next: '@character' }]
        ],

        comment: [
            [/[^\/*]+/, 'comment' ],
            [/\/\*/,    'comment', '@push' ],    // nested comment
            ["\\*/",    'comment', '@pop'  ],
            [/[\/*]/,   'comment' ]
        ],

        string: [
            [/[^\\"]+/,  'string'],
            [/@escapes/, 'string.escape'],
            [/\\./,      'string.escape.invalid'],
            [/"/,        { token: 'string.quote', bracket: '@close', next: '@pop' } ]
        ],

        character: [
            [/[^\\']+/, 'string'],
            [/@escapes/, 'string.escape'],
            [/\\./, 'string.escape.invalid'],
            [/'/, { token: 'string.quote', bracket: '@close', next: '@pop' }]
        ],

        whitespace: [
            [/[ \t\r\n]+/, 'white'],
            [/\/\*/,       'comment', '@comment' ],
            [/\/\/.*$/,    'comment']
        ],
    },
};
