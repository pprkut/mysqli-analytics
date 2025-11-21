/* mysqli_analytics extension for PHP */

#ifndef PHP_MYSQLI_ANALYTICS_H
# define PHP_MYSQLI_ANALYTICS_H

extern zend_module_entry mysqli_analytics_module_entry;
# define phpext_mysqli_analytics_ptr &mysqli_analytics_module_entry

# define PHP_MYSQLI_ANALYTICS_VERSION "0.1.0"

# if defined(ZTS) && defined(COMPILE_DL_MYSQLI_ANALYTICS)
ZEND_TSRMLS_CACHE_EXTERN()
# endif

#endif	/* PHP_MYSQLI_ANALYTICS_H */
