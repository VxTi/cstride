%{
#include <iostream>
#include <string>

extern int yylex();
extern int yyparse();
extern FILE *yyin;

void yyerror(const char *s);
%}

%token FN STRUCT CONST LET EXTERN AS RETURN FOR WHILE MODULE IF ELSE VOID IMPORT
%token I8 I16 I32 I64 U8 U16 U32 U64 F32 F64 BOOL CHAR STRING
%token SEMICOLON COLON COMMA DOT EQ PLUS_PLUS MINUS_MINUS PLUS_EQ MINUS_EQ MUL_EQ DIV_EQ MOD_EQ
%token PLUS MINUS MUL DIV MOD EXCL LT GT LBRACE RBRACE LPAREN RPAREN LBRACKET RBRACKET
%token COLON_COLON EQ_EQ EXCL_EQ LT_EQ GT_EQ AND OR
%token NUMBER_LITERAL STRING_LITERAL BOOLEAN_LITERAL CHAR_LITERAL IDENTIFIER
%token ERROR

%left OR
%left AND
%left EQ_EQ EXCL_EQ LT GT LT_EQ GT_EQ
%left PLUS MINUS
%left MUL DIV MOD
%right AS
%right EXCL PLUS_PLUS MINUS_MINUS UNARY_PLUS UNARY_MINUS
%left LPAREN LBRACKET DOT

%start StrideFile

%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE

%%

StrideFile:
    StandaloneItems
    ;

StandaloneItems:
    /* empty */
    | StandaloneItems StandaloneItem
    ;

StandaloneItem:
    FunctionDeclaration
    | ModuleStatement
    | StructDefinition
    | VariableDeclarationStatement
    | ExternFunctionDeclaration
    | SEMICOLON
    ;

// --- Declarations ---

ExternFunctionDeclaration:
    EXTERN FunctionDeclarationHeader SEMICOLON
    ;

FunctionDeclaration:
    FunctionDeclarationHeader BlockStatement
    ;

FunctionDeclarationHeader:
    FN IDENTIFIER LPAREN OptionalFunctionParameterList RPAREN COLON Type
    ;

OptionalFunctionParameterList:
    /* empty */
    | FunctionParameterList
    ;

FunctionParameterList:
    FunctionParameter
    | FunctionParameterList COMMA FunctionParameter
    | FunctionParameterList COMMA
    ;

FunctionParameter:
    IDENTIFIER COLON Type
    ;

StructDefinition:
    STRUCT IDENTIFIER StructDefinitionBody
    | STRUCT IDENTIFIER StructDefinitionAlias
    ;

StructDefinitionBody:
    LBRACE OptionalStructDefinitionFields RBRACE
    ;

StructDefinitionAlias:
    EQ Type SEMICOLON
    ;

OptionalStructDefinitionFields:
    /* empty */
    | StructDefinitionFields
    ;

StructDefinitionFields:
    StructField
    | StructDefinitionFields COMMA StructField
    | StructDefinitionFields COMMA
    ;

StructField:
    IDENTIFIER COLON Type
    ;

VariableDeclarationInferredType:
    CONST IDENTIFIER EQ Expression
    | LET IDENTIFIER EQ Expression
    ;

VariableDeclaration:
    CONST IDENTIFIER COLON Type EQ Expression
    | LET IDENTIFIER COLON Type EQ Expression
    ;

VariableDeclarationStatement:
    VariableDeclaration SEMICOLON
    | VariableDeclarationInferredType SEMICOLON
    ;

// --- Types ---

Type:
    PrimitiveType
    | UserType
    ;

PrimitiveType:
    OptionalModulePrefix VOID
    | OptionalModulePrefix I8
    | OptionalModulePrefix I16
    | OptionalModulePrefix I32
    | OptionalModulePrefix I64
    | OptionalModulePrefix U8
    | OptionalModulePrefix U16
    | OptionalModulePrefix U32
    | OptionalModulePrefix U64
    | OptionalModulePrefix F32
    | OptionalModulePrefix F64
    | OptionalModulePrefix BOOL
    | OptionalModulePrefix CHAR
    | OptionalModulePrefix STRING
    ;

OptionalModulePrefix:
    /* empty */
    | ModulePrefix
    ;

ModulePrefix:
    IDENTIFIER COLON_COLON
    | ModulePrefix IDENTIFIER COLON_COLON
    ;

UserType:
    IDENTIFIER
    | UserType COLON_COLON IDENTIFIER
    ;

// --- Statements ---

BlockStatement:
    LBRACE Statements RBRACE
    ;

Statements:
    /* empty */
    | Statements Statement
    ;

Statement:
    VariableDeclarationStatement
    | AssignmentStatement
    | ReturnStatement
    | ForLoopStatement
    | WhileLoopStatement
    | IfStatement
    | ExpressionStatement
    | BlockStatement
    | SEMICOLON
    ;

InlineAssignment:
    IDENTIFIER EQ Expression
    | IDENTIFIER PLUS_EQ Expression
    | IDENTIFIER MINUS_EQ Expression
    | IDENTIFIER MUL_EQ Expression
    | IDENTIFIER DIV_EQ Expression
    ;

AssignmentStatement:
    InlineAssignment SEMICOLON
    ;

ReturnStatement:
    RETURN SEMICOLON
    | RETURN Expression SEMICOLON
    ;

ForLoopStatement:
    FOR LPAREN OptionalForInitializerPart SEMICOLON OptionalExpression SEMICOLON OptionalExpression RPAREN BlockStatement
    ;

OptionalForInitializerPart:
    /* empty */
    | VariableDeclaration
    | InlineAssignment
    | Expression
    ;

WhileLoopStatement:
    WHILE LPAREN Expression RPAREN BlockStatement
    ;

ModuleStatement:
    MODULE IDENTIFIER LBRACE StandaloneItems RBRACE
    ;

IfStatement:
    IF LPAREN Expression RPAREN BlockStatement %prec LOWER_THAN_ELSE
    | IF LPAREN Expression RPAREN BlockStatement ELSE IfStatement
    | IF LPAREN Expression RPAREN BlockStatement ELSE BlockStatement
    ;

ExpressionStatement:
    Expression SEMICOLON
    ;

// --- Expressions ---

Expression:
    PrimaryExpression
    | Expression PLUS Expression
    | Expression MINUS Expression
    | Expression MUL Expression
    | Expression DIV Expression
    | Expression MOD Expression
    | Expression EQ_EQ Expression
    | Expression EXCL_EQ Expression
    | Expression LT Expression
    | Expression GT Expression
    | Expression LT_EQ Expression
    | Expression GT_EQ Expression
    | Expression AND Expression
    | Expression OR Expression
    | Expression AS Type
    | PLUS Expression %prec UNARY_PLUS
    | MINUS Expression %prec UNARY_MINUS
    | EXCL Expression
    | PLUS_PLUS Expression
    | MINUS_MINUS Expression
    | Expression PLUS_PLUS
    | Expression MINUS_MINUS
    | Expression LPAREN OptionalArgumentList RPAREN
    | Expression DOT IDENTIFIER
    | Expression LBRACKET Expression RBRACKET
    ;

PrimaryExpression:
    LiteralExpression
    | StructInitialization
    | IDENTIFIER
    | LPAREN Expression RPAREN
    ;

OptionalArgumentList:
    /* empty */
    | ArgumentList
    ;

ArgumentList:
    Expression
    | ArgumentList COMMA Expression
    ;

LiteralExpression:
    NUMBER_LITERAL
    | STRING_LITERAL
    | BOOLEAN_LITERAL
    | CHAR_LITERAL
    ;

StructInitialization:
    IDENTIFIER COLON_COLON LBRACE OptionalStructInitFields RBRACE
    ;

OptionalStructInitFields:
    /* empty */
    | StructInitFields
    ;

StructInitFields:
    StructInitField
    | StructInitFields COMMA StructInitField
    | StructInitFields COMMA
    ;

StructInitField:
    IDENTIFIER COLON Expression
    ;

OptionalExpression:
    /* empty */
    | Expression
    ;

%%

void yyerror(const char *s) {
    std::cerr << "Parse error: " << s << std::endl;
}
