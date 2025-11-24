--TEST--
mysqli_canonicalize_literals() with an UPDATE query
--EXTENSIONS--
mysqli_analytics
--FILE--
<?php
echo mysqli_canonicalize_literals(file_get_contents(__DIR__ . '/fixtures/update.sql'));
?>
--EXPECT--
UPDATE `database`.`table`
SET `content` = ?, `language` = ?
    WHERE `identifier` = ? AND `id` = ?;
