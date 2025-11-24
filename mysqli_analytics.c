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

        // String literals
        if (!inside_backtick_identifier &&
            (current_char == '\'' || current_char == '"' ||
             ((input_ptr + 5 < input_end && strncasecmp((const char*)input_ptr, "_utf8", 5) == 0 && input_ptr[5] == '\'') ||
              ((input_ptr + 1 < input_end && (*input_ptr == 'N' || *input_ptr == 'n') && input_ptr[1] == '\'')))))
        {
            unsigned char quote_char;

            if (strncasecmp((const char*)input_ptr, "_utf8", 5) == 0 && input_ptr[5] == '\'') {
                input_ptr += 5;
            }
            else if ((*input_ptr == 'N' || *input_ptr == 'n') && input_ptr[1] == '\'') {
                input_ptr += 1;
            }

            quote_char = *input_ptr++;

            *output_ptr++ = '?';

            while (input_ptr < input_end) {
                if (*input_ptr == '\\') {
                    input_ptr += 2;
                }
                else if (*input_ptr == quote_char) {
                    input_ptr++; break;
                }
                else {
                    input_ptr++;
                }
            }

            continue;
        }

        // Hexadecimal literals (0x..., X'...', x'...')
        if (!inside_backtick_identifier) {
            if ((*input_ptr == '0' && input_ptr + 1 < input_end && (input_ptr[1] == 'x' || input_ptr[1] == 'X'))) {
                // 0xFF style
                *output_ptr++ = '?';
                input_ptr += 2; // skip 0x

                while (input_ptr < input_end && isxdigit(*input_ptr)) {
                    input_ptr++;
                }

                continue;
            }
            if ((input_ptr[0] == 'X' || input_ptr[0] == 'x') && input_ptr + 1 < input_end && input_ptr[1] == '\'') {
                // X'...' or x'...' style
                *output_ptr++ = '?';
                input_ptr += 2; // skip X'

                while (input_ptr < input_end && *input_ptr != '\'') {
                    input_ptr++;
                }

                if (input_ptr < input_end) {
                    // skip closing '
                    input_ptr++;
                }

                continue;
            }
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
            *output_ptr++ = '?';

            while (input_ptr < input_end && (isalnum(*input_ptr) || *input_ptr == '_')) {
                input_ptr++;
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
