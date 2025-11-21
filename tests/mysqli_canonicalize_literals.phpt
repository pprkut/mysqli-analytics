--TEST--
mysqli_canonicalize_literals() Basic test
--EXTENSIONS--
mysqli_analytics
--FILE--
<?php
var_dump(mysqli_canonicalize_literals('PHP'));
?>
--EXPECT--
string(3) "PHP"
