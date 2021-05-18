#include <assert.h>
#include <stdio.h>
#include <ctype.h>
#include "./tokenizer.h"

bool is_name(char x)
{
    return isalnum(x) || x == '_';
}

bool is_number(char x)
{
    return isalnum(x) || x == '.';
}

bool tokenizer_peek(Tokenizer *tokenizer, Token *output, File_Location location)
{
    if (tokenizer->peek_buffer_full) {
        if (output) {
            *output = tokenizer->peek_buffer;
        }
        return true;
    }

    tokenizer->source = sv_trim_left(tokenizer->source);

    if (tokenizer->source.count == 0) {
        return false;
    }

    Token token = {0};
    switch (*tokenizer->source.data) {
    case '(': {
        token.kind = TOKEN_KIND_OPEN_PAREN;
        token.text = sv_chop_left(&tokenizer->source, 1);
    }
    break;

    case ')': {
        token.kind = TOKEN_KIND_CLOSING_PAREN;
        token.text = sv_chop_left(&tokenizer->source, 1);
    }
    break;

    case '{': {
        token.kind = TOKEN_KIND_OPEN_CURLY;
        token.text = sv_chop_left(&tokenizer->source, 1);
    }
    break;

    case '}': {
        token.kind = TOKEN_KIND_CLOSING_CURLY;
        token.text = sv_chop_left(&tokenizer->source, 1);
    }
    break;

    case '/': {
        token.kind = TOKEN_KIND_DIV;
        token.text = sv_chop_left(&tokenizer->source, 1);
    }
    break;

    case ',': {
        token.kind = TOKEN_KIND_COMMA;
        token.text = sv_chop_left(&tokenizer->source, 1);
    }
    break;

    case '%': {
        token.kind = TOKEN_KIND_MOD;
        token.text = sv_chop_left(&tokenizer->source, 1);
    }
    break;

    case '>': {
        token.kind = TOKEN_KIND_GT;
        token.text = sv_chop_left(&tokenizer->source, 1);
    }
    break;

    case '<': {
        token.kind = TOKEN_KIND_LT;
        token.text = sv_chop_left(&tokenizer->source, 1);
    }
    break;

    case '*': {
        token.kind = TOKEN_KIND_MULT;
        token.text = sv_chop_left(&tokenizer->source, 1);
    }
    break;

    case '+': {
        token.kind = TOKEN_KIND_PLUS;
        token.text = sv_chop_left(&tokenizer->source, 1);
    }
    break;

    case '-': {
        token.kind = TOKEN_KIND_MINUS;
        token.text = sv_chop_left(&tokenizer->source, 1);
    }
    break;

    case '=': {
        if (tokenizer->source.count <= 1 || tokenizer->source.data[1] != '=') {
            token.kind = TOKEN_KIND_EQ;
            token.text = sv_chop_left(&tokenizer->source, 1);
        } else {
            token.kind = TOKEN_KIND_EE;
            token.text = sv_chop_left(&tokenizer->source, 2);
        }
    }
    break;

    case '"': {
        sv_chop_left(&tokenizer->source, 1);

        size_t index = 0;

        if (sv_index_of(tokenizer->source, '"', &index)) {
            String_View text = sv_chop_left(&tokenizer->source, index);
            sv_chop_left(&tokenizer->source, 1);
            token.kind = TOKEN_KIND_STR;
            token.text = text;
        } else {
            fprintf(stderr, FL_Fmt": ERROR: Could not find closing \"\n",
                    FL_Arg(location));
            exit(1);
        }
    }
    break;

    case '\'': {
        sv_chop_left(&tokenizer->source, 1);

        size_t index = 0;

        if (sv_index_of(tokenizer->source, '\'', &index)) {
            String_View text = sv_chop_left(&tokenizer->source, index);
            sv_chop_left(&tokenizer->source, 1);
            token.kind = TOKEN_KIND_CHAR;
            token.text = text;
        } else {
            fprintf(stderr, FL_Fmt": ERROR: Could not find closing \'\n",
                    FL_Arg(location));
            exit(1);
        }
    }
    break;

    default: {
        if (isalpha(*tokenizer->source.data)) {
            token.text = sv_chop_left_while(&tokenizer->source, is_name);

            if (sv_eq(token.text, SV("to"))) {
                token.kind = TOKEN_KIND_TO;
            } else if (sv_eq(token.text, SV("from"))) {
                token.kind = TOKEN_KIND_FROM;
            } else if (sv_eq(token.text, SV("if"))) {
                token.kind = TOKEN_KIND_IF;
            } else {
                token.kind = TOKEN_KIND_NAME;
            }
        } else if (isdigit(*tokenizer->source.data)) {
            token.kind = TOKEN_KIND_NUMBER;
            token.text = sv_chop_left_while(&tokenizer->source, is_number);
        } else {
            fprintf(stderr, FL_Fmt": ERROR: Unknown token starts with %c\n",
                    FL_Arg(location), *tokenizer->source.data);
            exit(1);
        }
    }
    }

    tokenizer->peek_buffer_full = true;
    tokenizer->peek_buffer = token;

    if (output) {
        *output = token;
    }

    return true;
}

bool tokenizer_next(Tokenizer *tokenizer, Token *token, File_Location location)
{
    if (tokenizer_peek(tokenizer, token, location)) {
        tokenizer->peek_buffer_full = false;
        return true;
    }

    return false;
}

Tokenizer tokenizer_from_sv(String_View source)
{
    return (Tokenizer) {
        .source = source
    };
}

void expect_no_tokens(Tokenizer *tokenizer, File_Location location)
{
    Token token = {0};
    if (tokenizer_next(tokenizer, &token, location)) {
        fprintf(stderr, FL_Fmt": ERROR: unexpected token `"SV_Fmt"`\n",
                FL_Arg(location), SV_Arg(token.text));
        exit(1);
    }
}

Token expect_token_next(Tokenizer *tokenizer, Token_Kind expected_kind, File_Location location)
{
    Token token = {0};

    if (!tokenizer_next(tokenizer, &token, location)) {
        fprintf(stderr, FL_Fmt": ERROR: expected token `%s`\n",
                FL_Arg(location), token_kind_name(expected_kind));
        exit(1);
    }

    if (token.kind != expected_kind) {
        fprintf(stderr, FL_Fmt": ERROR: expected token `%s`, but got `%s`\n",
                FL_Arg(location),
                token_kind_name(expected_kind),
                token_kind_name(token.kind));
        exit(1);
    }

    return token;
}
