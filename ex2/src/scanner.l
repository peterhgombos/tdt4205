%{
#include "parser.h"
#ifdef DUMP_TOKENS
    #define RETURN(t) do {                                      \
        fprintf ( stderr, "TOKEN ( %d,\t'%s' )\n", t, yytext );    \
        return t;                                               \
    } while ( 0 )
#else
    #define RETURN(t) return t
#endif
%}

%option array
%option noyywrap
%option yylineno

d [0-9]
l [a-zA-Z]

%%
FUNC        { RETURN( yytext[0] ); }
PRINT       { RETURN( yytext[0] ); }
RETURN      { RETURN( yytext[0] ); }
[\ \t\n]+   {}
[a-zA-Z][a-zA-Z0-9_]+ { RETURN( yytext[0] ); }
.           { RETURN( yytext[0] ); }

%%
