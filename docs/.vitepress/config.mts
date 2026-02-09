import { defineConfig, type DefaultTheme , UserConfig} from "vitepress";

type Grammar = UserConfig<DefaultTheme.Config>['markdown']['languages'][number];

const strideGrammar: Grammar = {
  name: "stride",
  repository: {},
  scopeName: "source.stride",
  displayName: "Stride",
  patterns: [
    {
      name: "comment.line.double-slash.stride",
      begin: "//",
      end: "$",
    },
    {
      name: "comment.block.stride",
      begin: "/\\*",
      end: "\\*/",
    },
    {
      name: "string.quoted.double.stride",
      begin: '"',
      end: '"',
      patterns: [{ name: "constant.character.escape.stride", match: "\\\\." }],
    },
    {
      name: "string.quoted.single.stride",
      begin: "'",
      end: "'",
      patterns: [{ name: "constant.character.escape.stride", match: "\\\\." }],
    },
    {
      name: "keyword.control.stride",
      match:
        "\\b(let|const|fn|if|else|while|for|return|break|continue|struct|enum|case|default|import|nil|class|this|public|module|package|extern|override|as|async|do|switch|try|catch|throw|new)\\b",
    },
    {
      name: "storage.type.stride",
      match:
        "\\b(bool|i8|i16|i32|i64|u8|u16|u32|u64|f32|f64|char|string|void|auto)\\b",
    },
    {
      name: "constant.language.stride",
      match: "\\b(true|false|nil)\\b",
    },
    {
      name: "constant.numeric.stride",
      match:
        "\\b(0[xX][a-fA-F0-9]+|\\d*\\.\\d+([eE][\\-+]?\\d+)?[dD]?|\\d+[lL]?)\\b",
    },
    {
      name: "entity.name.function.stride",
      match: "\\b(?<=fn\\s+)[a-zA-Z_][a-zA-Z0-9_]*",
    },
  ],
};

// https://vitepress.dev/reference/site-config
export default defineConfig({
  title: "Stride Language",
  description: "A statically typed, JIT-compiled language using LLVM.",
  base: '/cstride/',
  outDir: './.vitepress/dist',
  markdown: {
    languages: [strideGrammar]
  },
  themeConfig: {
    // https://vitepress.dev/reference/default-theme-config
    nav: [
      { text: 'Home', link: '/' },
      { text: 'Getting Started', link: '/getting-started' },
      { text: 'Examples', link: '/examples' },
      { text: 'Standard Library', link: '/stdlib' },
      { text: 'Contributing', link: '/contributing' }
    ],

    sidebar: [
      {
        text: 'Guide',
        items: [
          { text: 'Getting Started', link: '/getting-started' },
          { text: 'Examples', link: '/examples' },
        ]
      },
      {
        text: 'Language Reference',
        items: [
          { text: 'Variables & Types', link: '/reference/variables' },
          { text: 'Functions', link: '/reference/functions' },
          { text: 'Structs', link: '/reference/structs' },
        ]
      },
      {
        text: 'Standard Library',
        link: '/stdlib'
      },
      {
        text: 'Contributing',
        link: '/contributing'
      }
    ],

    socialLinks: [
      { icon: 'github', link: 'https://github.com/vxti/cstride' }
    ],

    footer: {
      message: 'Released under the MIT License.',
      copyright: 'Copyright © 2024-present'
    }
  }
})
