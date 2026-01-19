// eslint.config.ts
import { defineConfig } from 'eslint/config';
import tseslint from 'typescript-eslint';
import prettierPlugin from 'eslint-plugin-prettier';
import unusedImports from 'eslint-plugin-unused-imports';
import prettierConfig from './prettier.config';
import importPlugin from 'eslint-plugin-import';
import reactHooksPlugin from 'eslint-plugin-react-hooks';

export default defineConfig([
  // TypeScript recommendations
  ...tseslint.configs.recommendedTypeChecked,
  {
    files: ['**/*.{mts,ts,tsx,js}'],
    ignores: ['**/*.json', '**/*.md', '**/*.cjs', '**/*.mjs'],
    languageOptions: {
      parser: tseslint.parser,
      parserOptions: {
        project: './tsconfig.json',
        tsconfigRootDir: process.cwd(),
      },
    },
  },

  // Custom setup
  {
    plugins: {
      'unused-imports': unusedImports,
      prettier: prettierPlugin,
      import: importPlugin,
      // eslint-disable-next-line @typescript-eslint/no-unsafe-assignment
      'react-hooks': reactHooksPlugin as any,
    },
    rules: {
      'prettier/prettier': ['error', prettierConfig],

      'unused-imports/no-unused-imports': 'error',
      '@typescript-eslint/no-unused-vars': [
        'error',
        {
          argsIgnorePattern: '^_',
          varsIgnorePattern: '^_',
          ignoreRestSiblings: true,
        },
      ],

      'arrow-body-style': 0,
      'prefer-arrow-callback': 0,
      'prefer-template': 2,
      'react-hooks/exhaustive-deps': 2,
      'object-shorthand': ['error', 'always'],

      '@typescript-eslint/no-explicit-any': 0,
      '@typescript-eslint/no-empty-object-type': 0,
      '@typescript-eslint/no-misused-promises': 0,
      'no-duplicate-imports': 2,

      'import/export': 2, // No re-exporting the same name
      'import/no-mutable-exports': 2, // Disallow the export of non-constants
      'import/no-self-import': 2, // Prevent a module from importing itself
      'import/no-useless-path-segments': 2, // Prevent unnecessary path segments in import and require statements

      '@typescript-eslint/consistent-type-imports': [
        'error',
        {
          fixStyle: 'inline-type-imports',
          prefer: 'type-imports',
          disallowTypeAnnotations: false,
        },
      ],
    },
  },
]);
