/* mysqli_analytics extension for PHP */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "php.h"
#include "ext/standard/info.h"
#include "php_mysqli_analytics.h"
#include "mysqli_analytics_arginfo.h"

/* For compatibility with older PHP versions */
#ifndef ZEND_PARSE_PARAMETERS_NONE
#define ZEND_PARSE_PARAMETERS_NONE() \
    ZEND_PARSE_PARAMETERS_START(0, 0) \
    ZEND_PARSE_PARAMETERS_END()
#endif

typedef struct {
    const unsigned char *prefix_end;  // points at start of literal content
    const unsigned char *literal_end; // points past end of literal
} literal_pos;

static inline const unsigned char*
skip_quoted_literal(const unsigned char *ptr, const unsigned char *end, unsigned char quote)
{
    // skip opening quote
    ptr++;

    while (ptr < end) {
        if (*ptr == '\\') {
            // skip escaped char
            ptr += 2;
        } else if (*ptr == quote) {
            // position after closing quote
            return ptr + 1;
        } else {
            ptr++;
        }
    }

    // unterminated literal
    return end;
}

/* Detect and skip string/binary/hex literals, with optional prefix */
static inline literal_pos
skip_prefixed_literal(const unsigned char *ptr, const unsigned char *end)
{
    literal_pos lit = { NULL, NULL };

    if (ptr >= end) {
        return lit;
    }

    const unsigned char *p = ptr;

    // _charset
    if (*p == '_' && p + 1 < end && isalpha(p[1])) {
        const unsigned char *start_prefix = p;
        p++;
        while (p < end && (isalnum(*p) || *p == '_')) p++; // charset
        if (p < end && (*p == '\'' || *p == '"')) {
            lit.prefix_end = p;        // preserve _charset
            lit.literal_end = skip_quoted_literal(p, end, *p);
            return lit;
        }
    }

    // N'...' prefix
    if ((*p == 'N' || *p == 'n') && p + 1 < end && p[1] == '\'') {
        lit.prefix_end = p + 1;       // preserve N
        lit.literal_end = skip_quoted_literal(p + 1, end, '\'');
        return lit;
    }

    // Normal quoted string
    if (*p == '\'' || *p == '"') {
        lit.prefix_end = ptr;   // do not preserve quote for ?
        lit.literal_end = skip_quoted_literal(p, end, *p);
        return lit;
    }

    // Hex literals (X'...'/x'...')
    if ((*p == 'X' || *p == 'x') && p + 1 < end && p[1] == '\'') {
        lit.prefix_end = ptr;   // don't preserve X
        lit.literal_end = skip_quoted_literal(p + 1, end, '\'');
        return lit;
    }

    // Hex literals (0xFF)
    if (*p == '0' && p + 1 < end && (p[1] == 'x' || p[1] == 'X')) {
        p += 2;
        const unsigned char *start = p;

        while (p < end && isxdigit(*p)) {
            p++;
        }

        if (p > start) {
            lit.prefix_end = ptr; // don't preserve 0x
            lit.literal_end = p;
            return lit;
        }
    }

    // Bit string literals b'...' or B'...'
    if ((*p == 'b' || *p == 'B') && p + 1 < end && (p[1] == '\'' || p[1] == '"')) {
        lit.prefix_end = ptr;   // don't preserve b/B
        lit.literal_end = skip_quoted_literal(p + 1, end, p[1]);
        return lit;
    }

    // not a literal
    return lit;
}

static zend_string *canonicalize_literals(zend_string *query)
{
    const unsigned char *input_ptr = (const unsigned char*)ZSTR_VAL(query);
    size_t query_length = ZSTR_LEN(query);

    unsigned char *output_buffer = emalloc(query_length + 1);
    unsigned char *output_ptr = output_buffer;

    const unsigned char *input_end = input_ptr + query_length;
    bool inside_backtick_identifier = false;

    while (input_ptr < input_end) {
        char current_char = *input_ptr;

        // Backtick-quoted identifiers
        if (!inside_backtick_identifier && current_char == '`') {
            inside_backtick_identifier = true;
            *output_ptr++ = *input_ptr++;

            while (input_ptr < input_end) {
                *output_ptr++ = *input_ptr;

                if (*input_ptr == '`') {
                    inside_backtick_identifier = false;
                    input_ptr++;
                    break;
                }

                input_ptr++;
            }

            continue;
        }

        // Comments
        if (!inside_backtick_identifier && input_ptr + 1 < input_end && input_ptr[0] == '-' && input_ptr[1] == '-') {
            *output_ptr++ = *input_ptr++;
            *output_ptr++ = *input_ptr++;

            while (input_ptr < input_end && *input_ptr != '\n') {
                *output_ptr++ = *input_ptr++;
            }

            continue;
        }

        if (!inside_backtick_identifier && *input_ptr == '#') {
            *output_ptr++ = *input_ptr++;

            while (input_ptr < input_end && *input_ptr != '\n') {
                *output_ptr++ = *input_ptr++;
            }

            continue;
        }

        if (!inside_backtick_identifier && input_ptr + 1 < input_end && input_ptr[0] == '/' && input_ptr[1] == '*') {
            *output_ptr++ = *input_ptr++;
            *output_ptr++ = *input_ptr++;

            while (input_ptr + 1 < input_end && !(input_ptr[0] == '*' && input_ptr[1] == '/')) {
                *output_ptr++ = *input_ptr++;
            }

            if (input_ptr + 1 < input_end) {
                *output_ptr++ = *input_ptr++;
                *output_ptr++ = *input_ptr++;
            }

            continue;
        }

        // String literals with optional _charset, hex literals or binary literals starting with b or B
        if (!inside_backtick_identifier) {
            literal_pos lit = skip_prefixed_literal(input_ptr, input_end);
            if (lit.literal_end != NULL) {
                // If it has a _charset or N prefix, preserve prefix
                if (lit.prefix_end != input_ptr) {
                    for (const unsigned char *p = input_ptr; p < lit.prefix_end; p++) {
                        *output_ptr++ = *p;
                    }
                }

                // Replace literal content with ?
                *output_ptr++ = '?';
                input_ptr = lit.literal_end;
                continue;
            }
        }

        // 0b binary integer literal (lowercase only)
        if (!inside_backtick_identifier &&
            *input_ptr == '0' && input_ptr + 1 < input_end &&
            input_ptr[1] == 'b')
        {
            *output_ptr++ = '?';
            input_ptr += 2; // skip 0b

            while (input_ptr < input_end && (*input_ptr == '0' || *input_ptr == '1')) {
                input_ptr++;
            }

            continue;
        }

        // Numeric literals
        if (!inside_backtick_identifier && (*input_ptr == '-' || isdigit(*input_ptr) || *input_ptr == '.')) {
            const unsigned char *number_start = input_ptr;
            bool has_digits = false;

            // optional leading minus
            if (*input_ptr == '-') {
                input_ptr++;
            }

            // integer part
            while (input_ptr < input_end && isdigit(*input_ptr)) {
                has_digits = true;
                input_ptr++;
            }

            // fractional part
            if (input_ptr < input_end && *input_ptr == '.') {
                input_ptr++;
                while (input_ptr < input_end && isdigit(*input_ptr)) {
                    has_digits = true;
                    input_ptr++;
                }
            }

            // scientific notation
            if (input_ptr < input_end && (*input_ptr == 'e' || *input_ptr == 'E')) {
                const unsigned char *exp_ptr = input_ptr + 1;

                if (exp_ptr < input_end && (*exp_ptr == '+' || *exp_ptr == '-')) {
                    exp_ptr++;
                }

                if (exp_ptr < input_end && isdigit(*exp_ptr)) {
                    input_ptr = exp_ptr + 1;

                    while (input_ptr < input_end && isdigit(*input_ptr)) {
                        input_ptr++;
                    }

                    has_digits = true;
                }
            }

            if (has_digits) {
                // check if number is part of identifier (letters or underscore adjacent)
                unsigned char prev_char = (output_ptr > output_buffer) ? *(output_ptr - 1) : ' ';
                unsigned char next_char = (input_ptr < input_end) ? *input_ptr : ' ';

                if (!isalnum(prev_char) && prev_char != '_' && !isalnum(next_char) && next_char != '_') {
                    *output_ptr++ = '?';
                    continue;
                } else {
                    input_ptr = number_start; // treat as identifier
                }
            } else {
                input_ptr = number_start;
            }
        }

        // Boolean / NULL
        if (!inside_backtick_identifier &&
            ((input_ptr + 3 < input_end && strncasecmp((const char*)input_ptr, "NULL", 4) == 0) ||
             (input_ptr + 3 < input_end && strncasecmp((const char*)input_ptr, "TRUE", 4) == 0) ||
             (input_ptr + 4 < input_end && strncasecmp((const char*)input_ptr, "FALSE", 5) == 0)))
        {
            bool preceded_by_is_or_is_not = false;
            const unsigned char *ptr = output_ptr - 1;
            char *tokens[2] = {NULL, NULL};
            size_t token_lens[2] = {0,0};
            int t = 0;

            // Collect last two significant tokens
            while (t < 2 && ptr >= output_buffer) {
                while (ptr >= output_buffer && isspace(*ptr)) {
                    ptr--;
                }

                if (ptr < output_buffer) {
                    break;
                }

                if (*ptr == '`' || *ptr == '.' || *ptr == '(' || *ptr == ')' || *ptr == ',') {
                    ptr--;
                    continue;
                }

                const unsigned char *word_end = ptr;

                while (ptr >= output_buffer && (isalnum(*ptr) || *ptr == '_')) {
                    ptr--;
                }

                size_t len = word_end - ptr;

                if (len > 0) {
                    tokens[t] = (char*)(ptr + 1);
                    token_lens[t] = len;
                    t++;
                }
            }

            if (t >= 1 && token_lens[0] == 2 && strncasecmp(tokens[0], "IS", 2) == 0) {
                preceded_by_is_or_is_not = true;
            }
            else if (t >= 2 &&
                     token_lens[1] == 2 && strncasecmp(tokens[1], "IS", 2) == 0 &&
                     token_lens[0] == 3 && strncasecmp(tokens[0], "NOT", 3) == 0) {
                preceded_by_is_or_is_not = true;
            }

            if (preceded_by_is_or_is_not) {
                // Copy token literally
                size_t token_len = 0;
                if (strncasecmp((const char*)input_ptr, "NULL", 4) == 0) {
                    token_len = 4;
                }
                else if (strncasecmp((const char*)input_ptr, "TRUE", 4) == 0) {
                    token_len = 4;
                }
                else if (strncasecmp((const char*)input_ptr, "FALSE", 5) == 0) {
                    token_len = 5;
                }

                for (size_t i = 0; i < token_len; i++) {
                    *output_ptr++ = *input_ptr++;
                }
            } else {
                *output_ptr++ = '?';

                while (input_ptr < input_end && (isalnum(*input_ptr) || *input_ptr == '_')) {
                    input_ptr++;
                }
            }

            continue;
        }

        // Default copy
        *output_ptr++ = *input_ptr++;
    }

    *output_ptr = '\0';

    return zend_string_init((char*)output_buffer, output_ptr - output_buffer, 0);
}

/* {{{ string mysqli_canonicalize_literals( [ string $query ] ) */
PHP_FUNCTION(mysqli_canonicalize_literals)
{
    zend_string *query;
    zend_string *retval;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STR(query)
    ZEND_PARSE_PARAMETERS_END();

    retval = canonicalize_literals(query);

    RETURN_STR(retval);
}
/* }}}*/

/* {{{ PHP_RINIT_FUNCTION */
PHP_RINIT_FUNCTION(mysqli_analytics)
{
#if defined(ZTS) && defined(COMPILE_DL_MYSQLI_ANALYTICS)
    ZEND_TSRMLS_CACHE_UPDATE();
#endif

    return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION */
PHP_MINFO_FUNCTION(mysqli_analytics)
{
    php_info_print_table_start();
    php_info_print_table_row(2, "mysqli_analytics support", "enabled");
    php_info_print_table_end();
}
/* }}} */

/* {{{ mysqli_analytics_module_entry */
zend_module_entry mysqli_analytics_module_entry = {
    STANDARD_MODULE_HEADER,
    "mysqli_analytics",             /* Extension name */
    ext_functions,                  /* zend_function_entry */
    NULL,                           /* PHP_MINIT - Module initialization */
    NULL,                           /* PHP_MSHUTDOWN - Module shutdown */
    PHP_RINIT(mysqli_analytics),    /* PHP_RINIT - Request initialization */
    NULL,                           /* PHP_RSHUTDOWN - Request shutdown */
    PHP_MINFO(mysqli_analytics),    /* PHP_MINFO - Module info */
    PHP_MYSQLI_ANALYTICS_VERSION,   /* Version */
    STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_MYSQLI_ANALYTICS
# ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
# endif
ZEND_GET_MODULE(mysqli_analytics)
#endif
