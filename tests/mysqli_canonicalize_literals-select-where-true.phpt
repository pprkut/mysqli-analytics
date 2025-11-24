--TEST--
mysqli_canonicalize_literals() with a SELECT query using WHERE NULL
--EXTENSIONS--
mysqli_analytics
--FILE--
<?php
echo mysqli_canonicalize_literals(file_get_contents(__DIR__ . '/fixtures/select-where-true.sql'));
?>
--EXPECT--
SELECT `c`.`identifier`,`c`.`language`,`c`.`language`,`fb`.`identifier`,COALESCE(?,?) as ?
from `table` as `c`
         left join `bookmarks` fb on `c`.`identifier` = `fb`.`identifier`
where `c`.`identifier` = ?
    and `c.`.`language` IS NOT TRUE
    and `c`.`foo` IS TRUE;
