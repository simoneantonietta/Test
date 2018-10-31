<?php
include('exec_command.php');

$timecode = date('YmdHis');
$file = $timecode.'_varcolan.csv';

header('Content-Type: text/csv');
header('Content-disposition: attachment; filename="'.basename($file).'"');

$results = getQueryFromFilters(true);

echo "Data,Ora,Badge ID,Titolare,Evento,Varco,Direzione,Area". "\n";

while($row = $results->fetchArray(SQLITE3_ASSOC)) {
		
	$rawEvents[] = $row;
}

$parsedEvents = parseEvent($rawEvents);

foreach($parsedEvents as $data) {
		
	$line = $data['date'].",".$data['time'].",".$data['badgeID'].",".$data['titolare'].",".$data['event_name'].",".$data['varco'].",".$data['direzione'].",".$data['area_name'];
	echo $line. "\n";
}

?>