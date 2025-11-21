--TEST--
Check if mysqli_analytics is loaded
--EXTENSIONS--
mysqli_analytics
--FILE--
<?php
echo 'The extension "mysqli_analytics" is available';
?>
--EXPECT--
The extension "mysqli_analytics" is available
