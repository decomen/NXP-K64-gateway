#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>

#include "tokenizer.h"

/*
  expression = ["+"|"-"] term {("+"|"-"|"OR") term} .

  term = factor {( "*" | "/" | "AND" ) factor} .

  factor = 
    func "(" expression ")" 
    | number
    | "(" expression ")" .

  func =
    ABS
    | AND
    | ATN
    | COS
    | EXP
    | INT
    | LOG
    | NOT
    | OR
    | SGN
    | SIN
    | SQR
    | TAN
*/


static double
_abs(double n) {
    return ((n>=0)?(n):(-n));
}

static double
_int(double n) {
    int i = (int)n;
    return 1.0 * i;
}

static double
_sqr(double n) {
    return sqrt((double)n);
}

static double
_sgn(double n) {
    if (n < 0) {
        return -1.0;
    } else if (n > 0) {
        return 1.0;
    } else {
        return 0.0;
    }
}

static double
_or(double a, double b) {
    return (double)((int)a | (int)b);
}

static double
_and(double a, double b) {
    return (double)((int)a & (int)b);
}

static double
_not(double number) {
    return (double)(~(int)number);
}

static double
_bcd_2_dec(double number) {
    unsigned long bcd = (unsigned long)number;
    unsigned long ret_dec = 0;
    for (int i = 0; i < 2 * sizeof(long); i++)
    {
        ret_dec *= 10;
        ret_dec += ((bcd >> 28) & 0x0F);
        bcd <<= 4;
    }
    return (double)ret_dec;
}


#include "varmanage.h"
const char *_exp_defstr = NULL;
static double 
_getvar(const char *name) {
    double value = 0;
    if (name || _exp_defstr) bVarManage_GetExtValueWithName((name?name:_exp_defstr), &value);
    return value;
}

typedef double (*function)(double number);

typedef struct
{
    token _token;
    function _function;
} token_to_function;

const token_to_function token_to_functions[] =
{
    { T_FUNC_ABS, _abs },
    { T_FUNC_SIN, sin },
    { T_FUNC_COS, cos },
    { T_FUNC_INT, _int },
    { T_FUNC_TAN, tan },
    { T_FUNC_SQR, _sqr },
    { T_FUNC_SGN, _sgn },
    { T_FUNC_LOG, log },
    { T_FUNC_EXP, exp },
    { T_FUNC_ATN, atan },
    { T_FUNC_NOT, _not },
    { T_FUNC_BCD2N, _bcd_2_dec },
    { T_EOF, NULL }
};

typedef double (*varfunc)(const char *name);

typedef struct
{
    token _token;
    varfunc _function;
} token_to_varfunc;

const token_to_varfunc token_to_varfuncs[] =
{
    { T_FUNC_VAR, _getvar },
    { T_EOF, NULL }
};

token sym;
const char *last_error;

static double _expression(void);

static void
_get_sym(void) {
    sym = tokenizer_get_next_token(T_EOF);
    // printf("t: %d\n", sym);
}

static void
_error(const char *error_msg) {
    last_error = error_msg;
}

static bool
_accept(token t) {
    if (t == sym) {
        _get_sym();
        return true;
    }
    return false;
}

static bool
_expect(token t) {
    if (_accept(t)) {
        return true;
    }
    _error("Expect: unexpected symbol");
    return false;
}

static bool
_is_function_token(token sym) {
    for (size_t i = 0; token_to_functions[i]._token != T_EOF; i++) {
        if (sym == token_to_functions[i]._token) {
            return true;
        }
    }

    return false;
}

static bool
_is_varfunc_token(token sym) {
    for (size_t i = 0; token_to_varfuncs[i]._token != T_EOF; i++) {
        if (sym == token_to_varfuncs[i]._token) {
            return true;
        }
    }

    return false;
}

static function
_get_function(token sym) {
    token_to_function ttf;
    for (size_t i = 0;; i++) {
        ttf = token_to_functions[i];
        if (ttf._token == T_EOF) {
            break;
        }
        if (ttf._token == sym) {
            return ttf._function;
        }
    }
    return NULL;
}

static varfunc
_get_varfunc(token sym) {
    token_to_varfunc ttf;
    for (size_t i = 0;; i++) {
        ttf = token_to_varfuncs[i];
        if (ttf._token == T_EOF) {
            break;
        }
        if (ttf._token == sym) {
            return ttf._function;
        }
    }
    return NULL;
}

static double
_factor(void) {
    double number;
    if (_is_function_token(sym)) {
        token function_sym = sym;
        _accept(sym);
        _expect(T_LEFT_BANANA);
        function func = _get_function(function_sym);
        number = func(_expression());
        _expect(T_RIGHT_BANANA);
    } else if (_is_varfunc_token(sym)) {
        token function_sym = sym;
        _accept(sym);
        _expect(T_LEFT_BANANA);
        varfunc func = _get_varfunc(function_sym);
        if( sym!=T_RIGHT_BANANA ) {
            sym=tokenizer_get_next_token(T_RIGHT_BANANA);
        }
        if( T_RIGHT_BANANA==sym ) {
            number = func(tokenizer_get_str());
        }
        _expect(T_RIGHT_BANANA);
    } else if (sym == T_NUMBER) {
        number = tokenizer_get_number();
        _accept(T_NUMBER);
    } else if (_accept(T_LEFT_BANANA)) {
        number = _expression();
        _expect(T_RIGHT_BANANA);
    } else {
        _error("Factor: syntax error");
        _get_sym();
    }

    return number;
}

static double
_term(void) {
    double f1 = _factor();
    while (sym == T_MULTIPLY || sym == T_DIVIDE || sym == T_OP_AND) {
        token operator =sym;
        _get_sym();
        double f2 = _factor();
        switch (operator) {
        case T_MULTIPLY:
            f1 = f1 * f2;
            break;
        case T_DIVIDE:
            f1 = f1 / f2;
            break;
        case T_OP_AND:
            f1 = _and(f1, f2);
            break;
        default:
            _error("term: oops");
        }
    }
    return f1;
}

static double
_expression(void) {
    token _operator = T_PLUS;
    if (sym == T_PLUS || sym == T_MINUS) {
        _operator =sym;
        _get_sym();
    }
    double t1 = _term();
    if (_operator == T_MINUS){
        t1 = -1 * t1;
    }
    while (sym == T_PLUS || sym == T_MINUS || sym == T_OP_OR) {
        _operator =sym;
        _get_sym();
        double t2 = _term();
        switch (_operator) {
        case T_PLUS:
            t1 = t1 + t2;
            break;
        case T_MINUS:
            t1 = t1 - t2;
            break;
        case T_OP_OR:
            t1 = _or(t1, t2);
            break;
        default:
            _error("expression: oops");
        }
    }
    return t1;
}

#include "rtdef.h"
void rt_enter_critical(void);
void rt_exit_critical(void);
rt_mutex_t rt_mutex_create(const char *name, rt_uint8_t flag);
rt_err_t rt_mutex_take(rt_mutex_t mutex, rt_int32_t time);
rt_err_t rt_mutex_release(rt_mutex_t mutex);

double evaluate(char *expression_string, const char *defstr) {
    //static rt_mutex_t _lock = RT_NULL;
    //if( !_lock ) _lock = rt_mutex_create("exp", RT_IPC_FLAG_FIFO);
    //if( _lock ) rt_mutex_take(_lock, RT_WAITING_FOREVER);
    double result = 0;
    rt_enter_critical();
    last_error = NULL;
    _exp_defstr = defstr;
    tokenizer_init(expression_string);
    _get_sym();
    result =  _expression();
    _expect(T_EOF);
    rt_exit_critical();
    //if( _lock ) rt_mutex_release(_lock);
    return result;
}

const char* evaluate_last_error(void) {
    return last_error;
}

