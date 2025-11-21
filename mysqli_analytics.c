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
    zend_string *retval;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STR(query)
    ZEND_PARSE_PARAMETERS_END();

    retval = strpprintf(0, "%s", ZSTR_VAL(query));

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
