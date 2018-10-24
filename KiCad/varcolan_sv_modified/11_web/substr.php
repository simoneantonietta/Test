<?php
$terminals = 'AIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIAIIIIIIIIIIIIIIIIIAIIIIA';
var_dump(substr($terminals, -64, 1));
for($i = 1; $i < 65; $i++) {
	if(substr($terminals, -$i, 1) == "A") {
		echo 'i: '.$i.'<br>';
	}
}
?>