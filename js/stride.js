hljs.registerLanguage('stride', function(hljs) {
  const KEYWORDS = [
    'let', 'const', 'fn', 'if', 'else', 'while', 'for', 'return', 'break', 'continue',
    'struct', 'enum', 'case', 'default', 'import', 'nil', 'class', 'this', 'public',
    'module', 'package', 'extern', 'override', 'as', 'async', 'do', 'switch', 'try',
    'catch', 'throw', 'new', 'bool', 'i8', 'i16', 'i32', 'i64', 'u8', 'u16', 'u32',
    'u64', 'f32', 'f64', 'char', 'string', 'void', 'auto', 'true', 'false'
  ];

  return {
    name: 'Stride',
    aliases: ['sr'],
    keywords: {
      keyword: KEYWORDS.join(' '),
      literal: 'true false nil',
      built_in: 'i8 i16 i32 i64 u8 u16 u32 u64 f32 f64 char string void bool auto'
    },
    contains: [
      hljs.C_LINE_COMMENT_MODE,
      hljs.C_BLOCK_COMMENT_MODE,
      {
        className: 'string',
        variants: [
          hljs.QUOTE_STRING_MODE,
          { begin: "'", end: "'" }
        ]
      },
      {
        className: 'number',
        variants: [
          { begin: hljs.C_NUMBER_RE + '[lL]?' },
          { begin: '\\b0[xX][a-fA-F0-9]+' },
          { begin: '\\b\\d*\\.\\d+([eE][\\-+]?\\d+)?[dD]?' }
        ],
        relevance: 0
      },
      {
        className: 'function',
        beginKeywords: 'fn',
        end: /\{/,
        excludeEnd: true,
        contains: [
          hljs.UNDERSCORE_TITLE_MODE,
          {
            className: 'params',
            begin: /\(/,
            end: /\)/,
            contains: [
              hljs.C_LINE_COMMENT_MODE,
              hljs.C_BLOCK_COMMENT_MODE
            ]
          }
        ]
      },
      {
        className: 'class',
        beginKeywords: 'struct enum class',
        end: /\{/,
        excludeEnd: true,
        contains: [
          hljs.UNDERSCORE_TITLE_MODE
        ]
      },
      {
        className: 'operator',
        begin: /[=><!~?:&|+\-*\/^%]+/,
        relevance: 0
      }
    ]
  };
});

document.addEventListener('DOMContentLoaded', (event) => {
  document.querySelectorAll('pre.language-stride code').forEach((block) => {
    hljs.highlightElement(block);
  });
  document.querySelectorAll('pre code.language-stride').forEach((block) => {
    hljs.highlightElement(block);
  });
});
