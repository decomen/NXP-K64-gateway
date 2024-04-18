#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <stdio.h>

#include "tokenizer.h"

typedef struct {
    char _char;
    token _token;
} char_to_token;

char_to_token char_to_tokens[] =
{

    { '+', T_PLUS },
    { '-', T_MINUS },
    { '*', T_MULTIPLY },
    { '/', T_DIVIDE },
    { '(', T_LEFT_BANANA },
    { ')', T_RIGHT_BANANA },
    { '\0', T_EOF }

};

typedef struct {
    char *_keyword;
    token _token;
} keyword_to_token;

const keyword_to_token keyword_to_tokens[] =
{

    { "ABS", T_FUNC_ABS },
    { "SIN", T_FUNC_SIN },
    { "COS", T_FUNC_COS },
    { "INT", T_FUNC_INT },
    { "TAN", T_FUNC_TAN },
    { "SQR", T_FUNC_SQR },
    { "SGN", T_FUNC_SGN },
    { "LOG", T_FUNC_LOG },
    { "EXP", T_FUNC_EXP },
    { "ATN", T_FUNC_ATN },
    { "NOT", T_FUNC_NOT },
    { "BCD2N", T_FUNC_BCD2N },
    { "FVAR", T_FUNC_VAR },
    { "OR",  T_OP_OR },
    { "AND", T_OP_AND },
    { NULL,  T_EOF }

};

char *tokenizer_line = NULL;
char *tokenizer_p = NULL;
char *tokenizer_next_p = NULL;

token tokenizer_actual_token;
double tokenizer_actual_number;
char tokenizer_actual_char;

char s_tokenizer_actual_str[32];
char *tokenizer_actual_str = NULL;
int tokenizer_actual_size;

void tokenizer_init(char *input) {
    tokenizer_line = input;
    tokenizer_p = tokenizer_next_p = tokenizer_line;
}

token tokenizer_get_next_token(token tk) {
    tokenizer_actual_size = 0;
    tokenizer_actual_str = NULL;
    
    if (!*tokenizer_p) {
        return T_EOF;
    }

    // Skip white space
    while (*tokenizer_p && isspace(*tokenizer_p)) {
        tokenizer_p++;
    }

    if( tk!=T_EOF ) {
        for (size_t i = 0;; i++) {
            char_to_token ctt = char_to_tokens[i];
            if (ctt._char == '\0') {
                break;
            }
            if (ctt._token==tk) {
                tokenizer_actual_str=tokenizer_p;
                while (*tokenizer_p!=ctt._char) {
                    tokenizer_actual_size++;
                    tokenizer_p++;
                    if (!*tokenizer_p) {
                        tokenizer_actual_str = NULL;
                        return T_EOF;
                    }
                }
                tokenizer_p++;
                return tk;
            }
        }
        tokenizer_actual_str = NULL;
        return T_ERROR;
    }

    // read single char token
    for (size_t i = 0;; i++) {
        char_to_token ctt = char_to_tokens[i];
        if (ctt._char == '\0') {
            break;
        }
        if (*tokenizer_p == ctt._char) {
            tokenizer_actual_char = ctt._char;
            tokenizer_actual_number = NAN;
            tokenizer_actual_token = ctt._token;

            tokenizer_p++;
            tokenizer_next_p = tokenizer_p;

            return ctt._token;
        }
    }

    // Check for number
    if (isdigit(*tokenizer_p)) {
        // puts("read a number");
        tokenizer_next_p = tokenizer_p;
        size_t l = 0;
        while (*tokenizer_next_p && (isdigit(*tokenizer_next_p) || *tokenizer_next_p == '.')) {
            l++;
            tokenizer_next_p++;
        }
        char number[16];
        strncpy(number, tokenizer_p, sizeof(number));
        tokenizer_p = tokenizer_next_p;
        double d;
        sscanf(number, "%lf", &d);
        // printf("Got double: '%lf'\n", d);
        tokenizer_actual_number = d;
        return T_NUMBER;
    }

    // Check for function
    for (size_t i = 0;; i++) {
        keyword_to_token ktt = keyword_to_tokens[i];
        if (ktt._keyword == NULL) {
            break;
        }
        if (strncasecmp(tokenizer_p, ktt._keyword, strlen(ktt._keyword)) == 0) {
            // printf("%s\n",ktt._keyword);
            tokenizer_next_p = tokenizer_p + strlen(ktt._keyword);
            tokenizer_p = tokenizer_next_p;
            return ktt._token;
        }

    }

    return T_ERROR;
}

double tokenizer_get_number(void) {
    return tokenizer_actual_number;
}

const char *tokenizer_get_str(void) {
    if (tokenizer_actual_str&&tokenizer_actual_size>0&&tokenizer_actual_size<32) {
        memcpy(s_tokenizer_actual_str, tokenizer_actual_str, tokenizer_actual_size);
        s_tokenizer_actual_str[tokenizer_actual_size]='\0';
        return (const char *)s_tokenizer_actual_str;
    }
    return NULL;
}


