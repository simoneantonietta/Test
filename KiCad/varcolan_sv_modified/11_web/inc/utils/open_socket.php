<?php

function sendSocket($msg) {
	
	$fp = fsockopen(SUPERVISOR, 6000, $errno, $errstr, 5);

	if (!$fp) {
		echo "$errstr ($errno)<br />\n";
	} else {
		$result = trim(fgets($fp, 256));		
		$out = $msg."\n";
		fwrite($fp, $out);		
		$result = trim(fgets($fp, 256));

		if($result == 'wait') {
			$result = fgets($fp, 256);
		}
		
		return $result;

	}
	
}

?>