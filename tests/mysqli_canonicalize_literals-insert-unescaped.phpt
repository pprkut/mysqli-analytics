--TEST--
mysqli_canonicalize_literals() with an INSERT query
--EXTENSIONS--
mysqli_analytics
--FILE--
<?php
echo mysqli_canonicalize_literals(file_get_contents(__DIR__ . '/fixtures/insert-unescaped.sql'));
?>
--EXPECT--
INSERT INTO database.table (param1, param2, param3) VALUES
    (?, ?, ?),
    (?, ?, ?),
    (?, ?, ?),
    (?, ?, ?),
    (?, ?, ?);
