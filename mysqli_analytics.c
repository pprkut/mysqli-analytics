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

/* {{{ string mysqli_canonicalize_literals( [ string $query ] ) */
PHP_FUNCTION(mysqli_canonicalize_literals)
{
    zend_string *query;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STR(query)
    ZEND_PARSE_PARAMETERS_END();

    const char *src = ZSTR_VAL(query);
    size_t len = ZSTR_LEN(query);

    // Allocate output buffer at max input size
    zend_string *out = zend_string_alloc(len, 0);
    char *dst = ZSTR_VAL(out);

    size_t i = 0, j = 0;
    int quote = 0;

    while (i < len) {
        char c = src[i];

        if (!quote && (c == '"' || c == '\'')) {
            // Start of quoted string -> write placeholder
            quote = c;
            dst[j++] = '?';
            i++;

            // Skip until closing quote (handle escapes)
            while (i < len) {
                if (src[i] == '\\') {
                    i += 2; // escaped char, skip both
                } else if (src[i] == quote) {
                    i++;    // skip closing quote
                    break;
                } else {
                    i++;
                }
            }

            quote = 0;
        } else {
            // Copy normal characters
            dst[j++] = c;
            i++;
        }
    }


    // Finalize zend_string length
    ZSTR_VAL(out)[j] = '\0';
    ZSTR_LEN(out) = j;

    RETURN_STR(out);
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
