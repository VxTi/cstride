from pygments.lexer import RegexLexer, words
from pygments.token import Text, Comment, Operator, Keyword, Name, String, Number, Punctuation

class StrideLexer(RegexLexer):
    name = 'Stride'
    aliases = ['stride']
    filenames = ['*.sr']

    keywords = (
        'let', 'const', 'fn', 'if', 'else', 'while', 'for', 'return', 'break',
        'continue', 'struct', 'enum', 'case', 'default', 'import', 'nil',
        'class', 'this', 'public', 'module', 'package', 'extern', 'override',
        'as', 'async', 'do', 'switch', 'try', 'catch', 'throw', 'new', 'bool',
        'i8', 'i16', 'i32', 'i64', 'u8', 'u16', 'u32', 'u64', 'f32', 'f64',
        'char', 'string', 'void', 'auto', 'true', 'false', 'mut'
    )

    tokens = {
        'root': [
            (r'\s+', Text),
            (r'//.*?\n', Comment.Single),
            (r'/\*', Comment.Multiline, 'comment'),
            (words(keywords, suffix=r'\b'), Keyword),
            (r'[a-zA-Z_]\w*(?=\s*\()', Name.Function),
            (r'[a-zA-Z_]\w*', Name),
            (r'\d*\.\d+([eE][\-+]?\d+)?[dD]?', Number.Float),
            (r'0[xX][0-9a-fA-F]+', Number.Hex),
            (r'\d+[lL]?', Number.Integer),
            (r'"', String, 'string'),
            (r"'", String, 'character'),
            (r'[{}()\[\]]', Punctuation),
            (r'[;,]', Punctuation),
            (r'[=><!~?:&|+\-*\/^%]+', Operator),
        ],
        'comment': [
            (r'[^/*]+', Comment.Multiline),
            (r'/\*', Comment.Multiline, '#push'),
            (r'\*/', Comment.Multiline, '#pop'),
            (r'[/*]', Comment.Multiline),
        ],
        'string': [
            (r'[^\\"]+', String),
            (r'\\([abfnrtv\\"\'\\]|x[0-9A-Fa-f]{1,4}|u[0-9A-Fa-f]{4}|U[0-9A-Fa-f]{8})', String.Escape),
            (r'\\.', String.Escape),
            (r'"', String, '#pop'),
        ],
        'character': [
            (r'[^\\ \']+', String),
            (r'\\([abfnrtv\\"\'\\]|x[0-9A-Fa-f]{1,4}|u[0-9A-Fa-f]{4}|U[0-9A-Fa-f]{8})', String.Escape),
            (r'\\.', String.Escape),
            (r"'", String, '#pop'),
        ],
    }
