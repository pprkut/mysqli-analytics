--TEST--
mysqli_canonicalize_literals() with string literals specifying a charset
--EXTENSIONS--
mysqli_analytics
--FILE--
<?php
echo mysqli_canonicalize_literals(file_get_contents(__DIR__ . '/fixtures/charset-literals.sql'));
?>
--EXPECT--
SELECT _latin1 ?, _utf8 ? COLLATE utf8_unicode_ci;
SELECT _latin1?, _utf8? COLLATE utf8_unicode_ci;
SELECT N?;
SELECT n?;
