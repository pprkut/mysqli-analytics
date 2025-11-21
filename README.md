My
====
*PHP MySQLi Analytics Extension*

![Tests](https://github.com/pprkut/mysqli-analytics/actions/workflows/tests.yml/badge.svg)

The ```mysqli_analytics``` extension provides functions that can help with query analytics.

API
===
*The PHP API for ```mysqli_analytics```*

```php
/**
 * Replace all literals in an SQL query with `?`.
 *
 * @param string $query The SQL query
 *
 * @return string
 **/
function mysqli_canonicalize_literals(string $query): string {}
```

Installation
===

```
$> phpize
$> ./configure
$> make
$> make install
```

Once installed you need to add

```
extension=mysqli_analytics.so
```

to your ```php.ini``` file.
