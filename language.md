# Backus-Naur form for Expressions.

```backus-naur-form 

<expression> ::= <literal>
               | <variable>
               | <function_call>
               | <binary_operation>
               | <unary_operation>
               | <ternary_operation>
               | <parenthesized_expression>
               | <conditional_expression>

<literal> ::= <number_literal> | <string_literal> | <char_literal> | <boolean_literal>
<variable> ::= <identifier>
<parenthesized_expression> ::= "(" <expression> ")"
<function_call> ::= <identifier> "::" <identifier> "(" <argument_list> ")"
<unary_operation> ::= <unary_operator> <expression>
<unary_operator> ::= "-" | "!" | "++" | "--"
<argument_list> ::= [ <expression> { "," <expression> } ]

<number_literal> ::= <digit> { <digit> }
<string_literal> ::= '"' { <character> } '"'
<char_literal> ::= "'" <character> "'"
<boolean_literal> ::= "true" | "false"


<identifier> ::= <letter> { <letter> | <digit> | "_" }
<binary_operation> ::= <expression> <binary_operator> <expression>
<binary_operator> ::= "+" | "-" | "*" | "/" | "&&" | "||" | "<" | ">" | "==" | "!="
<conditional_expression> ::= <expression> "?" <expression> ":" <expression>
<assignment_expression> ::= <variable> "=" <expression>

```