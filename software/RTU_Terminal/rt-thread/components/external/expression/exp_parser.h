#ifndef __EXP_PARSER_H__
#define __EXP_PARSER_H__

/**
 * This function is not threadsafe
 *
 * @param expression_string expression string
 * @return the result with float
 */
double evaluate(char *expression_string, const char *defstr);
const char* evaluate_last_error(void);

#endif // __EXP_PARSER_H__
