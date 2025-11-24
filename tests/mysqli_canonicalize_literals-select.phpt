--TEST--
mysqli_canonicalize_literals() with a SELECT query
--EXTENSIONS--
mysqli_analytics
--FILE--
<?php
echo mysqli_canonicalize_literals(file_get_contents(__DIR__ . '/fixtures/select.sql'));
?>
--EXPECT--
SELECT `c`.`identifier`,`c`.`language`,`c`.`language`,`fb`.`identifier`,COALESCE(NULL,1) as ?
from `table` as `c`
         left join `bookmarks` fb on `c`.`identifier` = `fb`.`identifier`
where `c`.`identifier` = ?;
