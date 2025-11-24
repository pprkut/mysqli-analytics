--TEST--
mysqli_canonicalize_literals() with hexadecimal literals
--EXTENSIONS--
mysqli_analytics
--FILE--
<?php
echo mysqli_canonicalize_literals(file_get_contents(__DIR__ . '/fixtures/hexadecimal-literals.sql'));
?>
--EXPECT--
SELECT ?, ?, ?;
