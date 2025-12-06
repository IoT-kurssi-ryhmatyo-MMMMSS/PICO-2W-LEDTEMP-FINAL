import globals from 'globals';
import js from '@eslint/js';
import prettier from 'eslint-plugin-prettier'

export default [
  js.configs.recommended,
  {
    files: ['**/*.js'],
    languageOptions: {
      sourceType: 'module',
      globals: { ...globals.node },
      ecmaVersion: 'latest',
    },
    plugins: {
      prettier: prettier,
    },
    rules: {
      eqeqeq: 'error',
      'no-trailing-spaces': 'error',
      'object-curly-spacing': ['error', 'always'],
      'arrow-spacing': ['error', { before: true, after: true }],
      'no-console': 'off',
      'prettier/prettier': [
        'error',
        {
          singleQuote: false,
          avoidEscape: true,
          semi: true,
          trailingComma: 'es5',
          parser: 'flow',
        },
      ],
    },
  },
  {
    ignores: ['dist/**'],
  },
];
