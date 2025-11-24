--TEST--
mysqli_canonicalize_literals() with an UPDATE ... ON DUPLICATE KEY ... query
--EXTENSIONS--
mysqli_analytics
--FILE--
<?php
echo mysqli_canonicalize_literals(file_get_contents(__DIR__ . '/fixtures/upsert.sql'));
?>
--EXPECT--
INSERT INTO `database`.`table` (`identifier`, `language`, `content`)
VALUES
    (?,?,?),
	(?,?,?),
	(?,?,?),
	(?,?,?),
	(?,?,?),
	(?,?,?),
	(?,?,?)
        ON DUPLICATE KEY UPDATE `content`=?;
