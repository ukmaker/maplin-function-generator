<?php
/**
* Print out the c-code to initialise an array with the values of sin
**/

echo "byte sinbytes[256] = {\n";

$numPoints = 1024;
$start = 256;
$end=768;

$n = 0;
for($i=$start; $i<$end; $i++) {

	$sinx = 127 * sin(2 * 3.14159265453 * $i / $numPoints);
	$intsinx = round($sinx)+128;
	if($n > 0) echo ",";
	echo "$intsinx";
	$n++;
	if($n > 7 && $i<($numPoints-1)) {
		echo ",\n";
		$n = 0;
	}
}

echo "\n};";
