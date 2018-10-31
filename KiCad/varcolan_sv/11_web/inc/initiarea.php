<?php
include('global.php');

$db_events = new SQLite3('/data/database/varcolan_events.db');

$line = "insert into area (badge_id, current_area) VALUES (6, 0)";

for($i = 7; $i < 27; $i++) {
	$line .= ", (".$i.", 0)";
}

//echo $line;
$db_events->query($line);

?>