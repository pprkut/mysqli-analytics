--TEST--
mysqli_canonicalize_literals() with binary literals
--EXTENSIONS--
mysqli_analytics
--FILE--
<?php
echo mysqli_canonicalize_literals(file_get_contents(__DIR__ . '/fixtures/binary-literals.sql'));
?>
--EXPECT--
SELECT ?;
SELECT ?+?;
SELECT ?;
SELECT ?+?;
