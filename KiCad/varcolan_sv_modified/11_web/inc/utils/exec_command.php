<?php

include("../global.php");
//include("exec_query.php");
require("open_socket.php");

$db = new SQLite3(DB_HOST);
$db_events = new SQLite3(DB_EVENTS_HOST);

$command = isset($_POST["command"]) ? $_POST["command"] : "";
//$command = $_GET["command"];

switch($command) {
	case "getTerminals" :
		getTerminals();
		break;
	case "saveWt" :
		saveWt();
		break;
	case "deleteWt" :
		deleteWt();
		break;
	case "addWeektime" :
		addWeektime();
		break;
	case "deleteWeektime" :
		deleteWeektime();
		break;
	case "addTerminal" :
		addTerminal();
		break;
	case "deleteTerminal" :
		deleteTerminal();
		break;
	case "removeTerminal" :
		removeTerminal();
		break;
	case "saveProfile" :
		saveProfile();
		break;
	case "saveTerminal" :
		saveTerminal();
		break;
	case "saveArea" :
		saveArea();
		break;
	case "deleteArea" :
		deleteArea();
		break;
	case "saveBadge" :
		saveBadge();
		break;
	case "deleteBadge" :
		deleteBadge();
		break;
	case "newBadge" :
		newBadge();
		break;
	case "addBadgeToProfile" :
		addBadgeToProfile();
		break;
	case "removeBadgeFromProfile" :
		removeBadgeFromProfile();
		break;
	case "getFilteredBadges" :
		getFilteredBadges();
		break;
	case "deleteCausalCode" :
		deleteCausalCode();
		break;
	case "checkCausalCode" :
		checkCausalCode();
		break;
	case "saveCausalCode" :
		saveCausalCode();
		break;
	case "saveUser" :
		saveUser();
		break;
	case "deleteUser" :
		deleteUser();
		break;
	case "addBadgeToUser" :
		addBadgeToUser();
		break;
	case "removeBadgeFromUser" :
		removeBadgeFromUser();
		break;
	case "uploadFirmware" :
		uploadFirmware();
		break;
	case "deleteAllJobs" :
		deleteAllJobs();
		break;
	case "saveQuery" :
		saveQuery();
		break;
	case "deleteQuery" :
		deleteQuery();
		break;
	case "getLastRowid" :
		getLastRowid();
		break;
	case "getLastEvents" :
		getLastEvents();
		break;
	case "getOldEvents" :
		getOldEvents();
		break;
	case "sendRtc" :
		sendRtc();
		break;
	case "changeSupervisorIP" :
		changeSupervisorIP();
		break;
	case "checkUserName" :
		checkUserName();
		break;
	case "saveAdminUser" :
		saveAdminUser();
		break;
	case "deleteAdminUser" :
		deleteAdminUser();
		break;
	case "exportEvents" :
		exportEvents();
		break;
	case "uploadDB" :
		uploadDB();
		break;
	case "deleteBadgesFromTerminal" :
		deleteBadgesFromTerminal();
		break;
	case "deleteBadgesFromSupervisor" :
		deleteBadgesFromSupervisor();
		break;
	case "restoreDefault" :
		restoreDefault();
		break;
	default:
		break;
}

//$db->close();
//$db_events->close();

function saveWt() {

	global $db;
	
	$query = 'UPDATE weektime SET name = "'.$_POST["name"].'", weektimedata = "'.$_POST["weektime"].'" WHERE id = '.$_POST["id"];
	$result = $db->query($query);
    if(!$result) {
        echo 'query: '.$query;
        return;
    }

	sendSocket("reload weektimes");
    echo "ok";
}

function deleteWt() {

	global $db;

    $weektime = '24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|';

    $name = 'Weektime'.$_POST["id"];

    $query = 'UPDATE weektime SET name = "'.$name.'", weektimedata = "'.$weektime.'" WHERE id = '.$_POST["id"];
    $result = $db->query($query);
    if(!$result) {
        echo 'query: '.$query;
        return;
    }

    $query = 'UPDATE profile SET weektime_id = 65 WHERE weektime_id = '.$_POST["id"];
    $result = $db->query($query);
    if(!$result) {
        echo 'query: '.$query;
        return;
    }

    $query = 'UPDATE terminal SET weektime_id = 65 WHERE weektime_id = '.$_POST["id"];
    $result = $db->query($query);

    if(!$result) {
        echo 'query: '.$query;
        return;
    }

    sendSocket("reload weektimes");
    sendSocket("reload profiles");
    sendSocket("reload terminals");
    echo "ok";
}

function addWeektime() {

	global $db;
	
	$query = 'UPDATE profile SET weektime_id = "'.$_POST["idItem"].'" WHERE id = '.$_POST["idProfile"];
	$result = $db->query($query);
	
    if(!$result) {
        echo 'query: '.$query;
        return;
    }
    
    sendSocket("reload profiles");
    echo "ok";
}

function deleteWeektime() {

	global $db;
	
	$query = 'UPDATE profile SET weektime_id = "65" WHERE id = '.$_POST["idProfile"];
	$result = $db->query($query);
	
    if(!$result) {
        echo 'query: '.$query;
        return;
    }

    sendSocket("reload profiles");
    echo "ok";
}

function addTerminal() {

	global $db;
	
	$query = 'SELECT * FROM profile WHERE id = '.$_POST["idProfile"];
	$result = $db->query($query);
    if(!$result) {
        echo 'query: '.$query;
        return;
    }

    $profile = $result->fetchArray();
	
	$active_terminal = addTerminalToProfile($_POST["idItem"], $profile["active_terminal"]);
	$query = 'UPDATE profile SET active_terminal = "'.$active_terminal.'" WHERE id = '.$_POST["idProfile"];
	$result = $db->query($query);

    if(!$result) {
        echo 'query: '.$query;
    }
    
    sendSocket("reload profiles");
    echo "ok";
}

function deleteTerminal() {

	global $db;
	
	$query = 'SELECT * FROM profile WHERE id = '.$_POST["idProfile"];
	$result = $db->query($query);
    if(!$result) {
        echo 'query: '.$query;
        return;
    }
	$profile = $result->fetchArray();
	
	$active_terminal = removeTerminalFromProfile($_POST["idItem"], $profile["active_terminal"]);
	$query = 'UPDATE profile SET active_terminal = "'.$active_terminal.'" WHERE id = '.$_POST["idProfile"];
	$result = $db->query($query);

    if(!$result) {
        echo 'query: '.$query;
    }
    
    sendSocket("reload profiles");
    echo "ok";
}

function setTerminalInProfile($id, $terminals, $value) {
	'use strict';
	
	$string = "";
	
	for($i = 0; $i < 64; $i++) {
		if((63 - $i) == ($id-1)) {
			$string .= $value;
		}
		else {
			$string .= $terminals[$i];
		}
	}
	
	return $string;
}

function isTerminalInProfile($id, $terminals) {
	'use strict';
	
	$string = "";
	
	for($i = 0; $i < 64; $i++) {
		if((63 - $i) == ($id-1)) {
			return true;
		}
	}
	
	return false;
}

function addTerminalToProfile($id, $terminals) {
	'use strict';
	
	return setTerminalInProfile($id, $terminals, "A");
}

function removeTerminalFromProfile($id, $terminals) {
	'use strict';
	
	return setTerminalInProfile($id, $terminals, "I");
}


function saveProfile() {

	global $db;
	
	$query = 'UPDATE profile SET name = "'.$_POST["name"].'", weektime_id = "'.$_POST["weektime_id"].'", active_terminal = "'.$_POST["active_terminals"].'", coercion = '.$_POST["coercion"].', status = '.$_POST["status"].' WHERE id = '.$_POST["id"];
	$result = $db->query($query);

    if(!$result) {
        echo 'query: '.$query;
        return;
    }
    
    sendSocket("reload profiles");
    echo "ok";
}

function saveTerminal() {

	global $db;
	
	if( ($_POST["entrance_type"] == 0) || ($_POST["entrance_type"] == 2) ) {
		$area2_reader1 = $_POST["area1_reader2"];
		$area2_reader2 = $_POST["area1_reader1"];
	} else {
		$area2_reader1 = $_POST["area2_reader1"];
		$area2_reader2 = $_POST["area2_reader2"];
	}
	
	$query = 'UPDATE terminal SET name = "'.$_POST["name"].'", open_door_time = "'.$_POST["open_door_time"].'", open_door_timeout = "'.$_POST["open_door_timeout"].'", access1 = '.$_POST["access1"].', access2 = '.$_POST["access2"].', filtro_out = '.$_POST["filtro_out"].', antipassback = '.$_POST["antipassback"].', entrance_type = '.$_POST["entrance_type"].', area1_reader1 = '.$_POST["area1_reader1"].', area2_reader1 = '.$area2_reader1.', area1_reader2 = '.$_POST["area1_reader2"].', area2_reader2 = '.$area2_reader2.', weektime_id = '.$_POST["weektime_id"].' WHERE id = '.$_POST["id"];
	$result = $db->query($query);
	
//	echo $query;
    if(!$result) {
        echo 'query: '.$query;
        return;
    }
    
    sendSocket("reload terminal ".$_POST["id"]);
    echo "ok";
}

function removeTerminal() {

	global $db;
	
	$query = 'UPDATE terminal SET name = "", open_door_time = "", open_door_timeout = "", entrance_type = 0, access1 = 1, access2 = 1, filtro_out = 1, antipassback = 0, area1_reader1 = 1, area2_reader1 = 0, area1_reader1 = 2, area2_reader2 = 0, weektime_id = 65, status = 0 WHERE id = '.$_POST["id"];
	$result = $db->query($query);
    if(!$result) {
        echo 'query: '.$query;
        return;
    }
	
	$terminal_ID = $_POST["id"];
	
	$query = 'SELECT * FROM profile';
	$results = $db->query($query);

    if(!$results) {
        echo 'query: '.$query;
        return;
    }

	while( $profile = $results->fetchArray() ) {
		if($profile["active_terminal"] != "IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII") {
			if(isTerminalInProfile($terminal_ID, $profile["active_terminal"])) {
				$active_terminal = removeTerminalFromProfile($terminal_ID, $profile["active_terminal"]);
				$query = 'UPDATE profile SET active_terminal = "'.$active_terminal.'" WHERE id = '.$profile["id"];
				$result = $db->query($query);
                if(!$result) {
                    echo 'query: '.$query;
                    return;
                }
			}
		}
	}

	sendSocket("reload terminals;reload profiles");
    echo "ok";
}

function saveArea() {

	global $db;
	
	if($_POST["newInsert"]) {
		$query = 'INSERT INTO area (area_id, name) VALUES ('.$_POST["area_id"].', "'.$_POST["name"].'")';
	}
	else {
		$query = 'UPDATE area SET name = "'.$_POST["name"].'" WHERE area_id = '.$_POST["area_id"];
	}
	
	$result = $db->query($query);
    
    if(!$result) {
        echo 'query: '.$query;
    }
    
    sendSocket("reload areas");
    echo "ok";
}

function deleteArea() {

	global $db;
	
	$query = 'DELETE FROM area WHERE area_id = '.$_POST["area_id"];
	$result = $db->query($query);

    if(!$result) {
        echo 'query: '.$query;
    }
    
    sendSocket("reload areas");
    echo "ok";
}

// BADGE MANAGEMENT

function saveBadge() {

	global $db;
	global $db_events;
    
	$profili = isset($_POST["profili"]) ? $_POST["profili"] : 0;
	
	$currentarea = isset($_POST["current_area"]) ? $_POST["current_area"] : 0;
	
	if($profili) {
		$profile_id0 = isset($profili[0]) ? $profili[0] : 250;
		$profile_id1 = isset($profili[1]) ? $profili[1] : 250;
		$profile_id2 = isset($profili[2]) ? $profili[2] : 250;
		$profile_id3 = isset($profili[3]) ? $profili[3] : 250;
		$profile_id4 = isset($profili[4]) ? $profili[4] : 250;
		$profile_id5 = isset($profili[5]) ? $profili[5] : 250;
		$profile_id6 = isset($profili[6]) ? $profili[6] : 250;
		$profile_id7 = isset($profili[7]) ? $profili[7] : 250;
		$profile_id8 = isset($profili[8]) ? $profili[8] : 250;
	}
	else {
		$profile_id0 = 0;
		$profile_id1 = 250;
		$profile_id2 = 250;
		$profile_id3 = 250;
		$profile_id4 = 250;
		$profile_id5 = 250;
		$profile_id6 = 250;
		$profile_id7 = 250;
		$profile_id8 = 250;
	}
	
	$validity_start = isset($_POST["validity_start"]) ? $_POST["validity_start"] : 0;
	if(!$validity_start) {
		$validity_start = date("Y-m-d");
	}

	$validity_stop = isset($_POST["validity_stop"]) ? $_POST["validity_stop"] : 0;
	if(!$validity_stop) {
		$validity_stop = date("Y-m-d", strtotime("+1 day", time()));
	}
	
    $timestamp = time();

	$results = $db->query("SELECT * FROM badge WHERE badge_num = '".$_POST["badge_num"]."'");
    if(!$results) {
        echo 'query: '.$query;
        return;
    }

	$item = $results->fetchArray();
	
	if( !$item ) {
		$query = 'INSERT INTO badge (id, badge_num, status, user_type, printed_code, pin, profile_id0, profile_id1, profile_id2, profile_id3, profile_id4, profile_id5, profile_id6, profile_id7, profile_id8, visitor, validity_start, validity_stop, contact, timestamp) VALUES ('.$_POST["id"].', "'.$_POST["badge_num"].'", '.$_POST["status"].', '.$_POST["user_type"].', "'.$_POST["printed_code"].'", "'.$_POST["pin"].'", '.$profile_id0.', '.$profile_id1.', '.$profile_id2.', '.$profile_id3.', '.$profile_id4.', '.$profile_id5.', '.$profile_id6.', '.$profile_id7.', '.$profile_id8.', '.$_POST["visitor"].', "'.$validity_start.'", "'.$validity_stop.'", "'.$_POST["contact"].'", '.$timestamp.')';
		$result = $db->query($query);

		$queryEvents = 'INSERT INTO area (badge_id, current_area) VALUES ('.$_POST["id"].', '.$currentarea.')' ;
		$resultEvents = $db_events->query($queryEvents);
        if(!$resultEvents) {
            echo 'query: '.$query;
            return;
        }
	}
	else {
		$query = 'UPDATE badge SET status = '.$_POST["status"].', user_type =  '.$_POST["user_type"].',printed_code = "'.$_POST["printed_code"].'", pin = "'.$_POST["pin"].'", profile_id0 = '.$profile_id0.', profile_id1 = '.$profile_id1.', profile_id2 = '.$profile_id2.', profile_id3 = '.$profile_id3.', profile_id4 = '.$profile_id4.', profile_id5 = '.$profile_id5.', profile_id6 = '.$profile_id6.', profile_id7 = '.$profile_id7.', profile_id8 = '.$profile_id8.', visitor = '.$_POST["visitor"].', validity_start = "'.$validity_start.'", validity_stop = "'.$validity_stop.'", contact = "'.$_POST["contact"].'", timestamp = '.$timestamp.' WHERE id = '.$_POST["id"];
		$result = $db->query($query);

		$queryEvents = 'UPDATE area SET current_area = '.$currentarea.' WHERE badge_id = '.$_POST["id"];
		$resultEvents = $db_events->query($queryEvents);
        if(!$resultEvents) {
            echo 'query: '.$query;
            return;
        }

	}
	
	sendSocket("reload badge ".$_POST["id"]." ".$_POST["badge_num"]);
    echo "ok";
}

function deleteBadge() {

	global $db;
	
	$query = 'SELECT badge_num FROM badge WHERE id = '.$_POST["id"];
	$result = $db->query($query);
    if(!$result) {
        echo 'query: '.$query;
        return;
    }

	$item = $result->fetchArray();
	$badgeNum = $item["badge_num"];
	
	$results = $db->query("DELETE FROM userbadge WHERE badge_id = ".$_POST["id"]);
    if(!$results) {
        echo 'query: '.$query;
        return;
    }

	$query = 'UPDATE badge SET status = 2, user_type = 0, visitor = 0, pin = "12345", validity_start = null, validity_stop = null, profile_id0 = 255, profile_id1 = 255, profile_id2 = 255, profile_id3 = 255, profile_id4 = 255, profile_id5 = 255, profile_id6 = 255, profile_id7 = 255, profile_id8 = 255, contact = null, timestamp = 0 WHERE id = '.$_POST["id"];
	$result = $db->query($query);
    if(!$result) {
        echo 'query: '.$query;
        return;
    }

	sendSocket("reload badge ".$_POST["id"]." ".$badgeNum);
    echo "ok";
}

function newBadge() {
	
	global $db;
	
	$numBadge = sendSocket("get badge");	
	$numBadge = trim($numBadge);

	$results = $db->query("SELECT * FROM badge WHERE badge_num = '".$numBadge."'");
    if(!$results) {
        echo 'query: '.$query;
        return;
    }
	$item = $results->fetchArray();
	
	if( !$item ) {
        $query = "SELECT MAX(id) as max_id FROM badge";
		$results = $db->query($query);
        if(!$results) {
            echo 'query: '.$query;
            return;
        }

		$item = $results->fetchArray();
		$id = $item["max_id"] + 1;
	} else {
		$id = $item["id"];
	}
	
	$data["id"] = $id;
	$data["badge_num"] = $numBadge; 

	echo json_encode($data);
}

function addBadgeToProfile() {

	global $db;
	
	$id_badge = $_POST["idItem"];
	$id_profile = $_POST["idProfile"];
	
    $query = "SELECT * FROM badge WHERE id = ".$id_badge;
	$results = $db->query($query);	
    if(!$results) {
        echo 'query: '.$query;
        return;
    }

    $item = $results->fetchArray();
		
	$associato = 0;
	for($i = 0; $i < 9; $i++) {
		$profile_id = "profile_id".$i;
		if($item[$profile_id] == $id_profile) {
			$associato = 1;
			return;
		}
	}
	
	if(!$associato) {
		for($i = 0; $i < 9; $i++) {
			$profile_id = "profile_id".$i;
			if($item[$profile_id] == 250) {
				break;
			}
		}
	}

	if($i == 9) {
		echo "Il badge non può essere associato ad altri profili.";
		return;
	}
	else {
		$results = $db->query('UPDATE badge SET '.$profile_id.' = '.$id_profile.' WHERE id = '.$id_badge);
        if(!$results) {
            echo 'query: '.$query;
            return;
        }
		echo 'ok';
	}
	
	sendSocket("reload badge ".$id_badge." ".$item["badge_num"]);
    echo "ok";
	
}

function removeBadgeFromProfile() {

	global $db;
	
	$id_badge = $_POST["idItem"];
	$id_profile = $_POST["idProfile"];
	
    $query = "SELECT * FROM badge WHERE id = ".$id_badge;
	$results = $db->query($query);
    if(!$results) {
        echo 'query: '.$query;
        return;
    }

    $item = $results->fetchArray();
	
	for($i = 0; $i < 9; $i++) {
		$profile_id = "profile_id".$i;
		if($item[$profile_id] == $id_profile) {
			break;
		}
	}
	
	if(!$i) {
		for($j = 1; $j < 9; $j++) {
			$profile_jd = "profile_id".$j;
			if($item[$profile_jd] != 250) {
				break;
			}
		}
		if($j == 9) {
            $query = 'UPDATE badge SET '.$profile_id.' = 0 WHERE id = '.$id_badge;
			$results = $db->query($query);
            if(!$results) {
                echo 'query: '.$query;
                return;
            }
        }
		else {
            $query = 'UPDATE badge SET '.$profile_id.' = 250 WHERE id = '.$id_badge;
			$results = $db->query($query);
            if(!$results) {
                echo 'query: '.$query;
                return;
            }
		}
	}
	else {
        $query = 'UPDATE badge SET '.$profile_id.' = 250 WHERE id = '.$id_badge;
		$results = $db->query($query);
        if(!$results) {
            echo 'query: '.$query;
            return;
        }

        for($j = 0; $j < 9; $j++) {
			if($j == $i)
				continue;
			$profile_jd = "profile_id".$j;
			if($item[$profile_jd] != 250) {
				break;
			}
		}
		if($j == 9) {
            $query = 'UPDATE badge SET '.$profile_id.' = 0 WHERE id = '.$id_badge;
			$results = $db->query($query);
            if(!$results) {
                echo 'query: '.$query;
                return;
            }
        }
	}

	sendSocket("reload badge ".$id_badge." ".$item["badge_num"]);
    echo "ok";
}

function deleteCausalCode() {
	global $db;
	
	$query = 'UPDATE causal_codes SET causal_id = 0, description = "" WHERE n = '.$_POST["n"];
	$result = $db->query($query);
    if(!$result) {
        echo 'query: '.$query;
        return;
    }

	sendSocket("reload causals");
    echo "ok";
}

function checkCausalCode() {
	
	global $db;
	
	$causal_id = $_POST["causal_id"];
	
	$query = 'SELECT causal_id FROM causal_codes WHERE causal_id = "'.$causal_id.'"';
	$result = $db->query($query);
	$code = $result->fetchArray();

    echo $code["causal_id"] ? 1 : 0;
}

function saveCausalCode() {
	global $db;
	
	$causal_id = $_POST["causal_id"];
	$description = $_POST["description"];
	
	$query = 'UPDATE causal_codes SET causal_id = '.$causal_id.', description = "'.$description.'" WHERE n = '.$_POST["n"];
	$result = $db->query($query);
    if(!$result) {
        echo 'query: '.$query;
        return;
    }

	sendSocket("reload causals");
    echo "ok";
}

function saveUser() {

	global $db;
	
	$id = $_POST["id"];
	$newInsert = $_POST["newInsert"];
	$name = $_POST["name"];
	$lastname = $_POST["lastname"];
	$matricola = $_POST["matricola"];
	$badges = isset($_POST["badges"]) ? $_POST["badges"] : [];
//	$badges = $_POST["badges"];

	$query = 'DELETE FROM userbadge WHERE user_id = '.$id;
	$deleteBadge = $db->query($query);
    if(!$deleteBadge) {
        echo 'query: '.$query;
        return;
    }

	if(sizeof($badges)) {
		$flag = 0;
		foreach($badges as $badge) {
			if(!$flag) {
				$flag = 1;
				$values = "(".$badge.", ".$id.")";
				$bagdesToDelete = " badge_id = ".$badge;
			}
			else
				$values .= ", (".$badge.", ".$id.")";
				$bagdesToDelete .= " OR badge_id = ".$badge;
		}

        $query = "DELETE FROM userbadge WHERE".$bagdesToDelete;
        $result = $db->query($query);
        if(!$result) {
            echo 'query: '.$query;
            return;
        }

        $query = 'INSERT INTO userbadge (badge_id, user_id) VALUES '.$values;
		$result = $db->query($query);
        if(!$result) {
            echo 'query: '.$query;
            return;
        }
	}
	
	if($_POST["newInsert"]) {
		$query = 'INSERT INTO user (id, first_name, second_name, matricola) VALUES ('.$id.', "'.$name.'", "'.$lastname.'", "'.$matricola.'")';
	}
	else {
		$query = 'UPDATE user SET first_name = "'.$name.'", second_name = "'.$lastname.'", matricola = "'.$matricola.'" WHERE id = '.$id;
	}
	
	$result = $db->query($query);
    if(!$result) {
        echo 'query: '.$query;
        return;
    }
    echo "ok";
}

function deleteUser() {

	global $db;

	$query = 'DELETE FROM userbadge WHERE user_id = '.$_POST["id"];
	$result = $db->query($query);
    if(!$result) {
        echo 'query: '.$query;
        return;
    }
	
	$query = 'DELETE FROM user WHERE id = '.$_POST["id"];
	$result = $db->query($query);
    if(!$result) {
        echo 'query: '.$query;
        return;
    }

    echo "ok";

}

function addBadgeToUser() {

	global $db;
	
	$id_badge = $_POST["idItem"];
	$id_user = $_POST["idUser"];
	
    $query = "SELECT * FROM userbadge WHERE badge_id = ".$id_badge;
	$results = $db->query($query);
    if(!$results) {
        echo 'query: '.$query;
        return;
    }

    $item = $results->fetchArray();
	
	if(!$item) {
        $query = "INSERT INTO userbadge (badge_id, user_id) VALUES (".$id_badge.", ".$id_user.")";
		$results = $db->query($query);
        if(!$results) {
            echo 'query: '.$query;
            return;
        }
    }
	else {
        $query = "UPDATE userbadge SET user_id = ".$id_user." WHERE badge_id = ".$id_badge;
		$results = $db->query($query);
        if(!$results) {
            echo 'query: '.$query;
            return;
        }
	}
    echo "ok";
}

function removeBadgeFromUser() {

	global $db;
	
	$id_badge = $_POST["idItem"];
	$id_user = $_POST["idUser"];
	
	$results = $db->query("DELETE FROM userbadge WHERE badge_id = ".$id_badge." AND user_id = ".$id_user);
    if(!$results) {
        echo 'query: '.$query;
        return;
    }

    $item = $results->fetchArray();
    echo "ok";
}

function getFilteredBadges() {
	
	global $db;
	
	$appliedFilters = apply_badge_filters();
	
	$data = [];
	
	$query = "SELECT userbadge.user_id AS user_id, userbadge.badge_id AS badge_id, user.first_name AS name, user.second_name AS lastname, badge.status AS status, badge.printed_code AS printed_code FROM user INNER JOIN userbadge ON userbadge.user_id = user.id INNER JOIN badge ON badge.id = userbadge.badge_id";
	
	if($appliedFilters != "")
		$query .= " WHERE (";
	
	$query .= $appliedFilters;
	
	if($appliedFilters != "")
		$query .= " )";
			
	//echo $query;
	
	$results = $db->query($query);
	
	if($results)	{
		while($event = $results->fetchArray()) {
			$data[] = $event;
		}
	} else {
        echo 'query: '.$query;
        return;
    }
		
	echo json_encode($data);
}

function apply_badge_filters() {
	
	$first = 1;
	
	$badges = isset($_POST["badges"]) ? $_POST["badges"] : 0;
	$badgeSelected = "";
	if($badges) {
		if(!$first)
			$badgeSelected .= $logic;
		else
			$first = 0;

		foreach($badges as $index => $badge) {
			if(!$index)
				$badgeSelected .= " ((badge_id = ".$badge.")";
			else
				$badgeSelected .= " || (badge_id = ".$badge.")";
		}
		$badgeSelected .= ")";
	}
	
	$titolari = isset($_POST["titolari"]) ? $_POST["titolari"] : 0;
	$titolariSelected = "";
	if($titolari) {
		if(!$first)
			$titolariSelected .= $logic;
		else
			$first = 0;

		foreach($titolari as $index => $titolare) {
			if(!$index)
				$titolariSelected .= " ((badge_id = ".$titolare.")";
			else
				$titolariSelected .= " || (badge_id = ".$titolare.")";
		}
		$titolariSelected .= ")";
	}
	
	return	$badgeSelected
			.$titolariSelected;
}


function getTerminals() {
	
	$now = date("YmdHis", $_POST["date"]);
	sendSocket("time rtc ".$now);
	sendSocket("terminals");
}

function uploadFirmware() {
	
	global $db;
	
	$uploadedFiles = $_FILES["file"];
	$scheduledDate = $_POST["scheduled"];
	$eptime = strtotime($scheduledDate);
	$timeinterval = 30; // minuti

	for($i = 0; $i < sizeof($uploadedFiles["name"]); $i++) {
		if(!$uploadedFiles["error"][$i]) {
			if(!$uploadedFiles["size"][$i] < FILE_MAX_SIZE) {
				$uploadfile = FIRMWARE_FOLDER . basename($uploadedFiles["name"][$i]);
				if (move_uploaded_file($uploadedFiles["tmp_name"][$i], $uploadfile)) {
					$schedtime = strtotime("+".$timeinterval*$i." minute", $eptime);
					$schedate = date("Y-m-d H:i:s", $schedtime);
					$query = "INSERT INTO scheduled_jobs (file_name, scheduled_time, status) VALUES ('".$uploadedFiles["name"][$i]."', datetime('".$schedate."'), 1)";
					$result = $db->query($query);
                    if(!$result) {
                        echo 'query: '.$query;
                        return;
                    }

					// send socket to the server
					$now = date("YmdHis", $_POST["date"]);
					sendSocket("time rtc ".$now);
					sendSocket("sched ".$schedtime." {system [mv -f ".FIRMWARE_FOLDER.$uploadedFiles["name"][$i]." ".NEW_FIRMWARE_FOLDER.$uploadedFiles["name"][$i]."]}");
					
					echo 'ok';
				} else {
					echo 'Errore nella copia del file '.$uploadedFiles["name"][$i].'. Non è possibile scrivere nella cartella destinazione.<br>';
				}
			}
			else
				echo 'Il file '.$uploadedFiles["name"][$i].' eccede la dimensione massima consentita: '.FILE_MAX_SIZE.' Byte<br>';
		}
		else
			echo 'Errore nell"upload del file '.$uploadedFiles["name"][$i].' - error number: '.$uploadedFiles["error"][$i].'<br>';
	}
	
}

function saveQuery() {

	global $db;
	
	if($_POST["newInsert"]) {
		$query = 'INSERT INTO custom_queries (query_id, name, query) VALUES ('.$_POST["query_id"].', "'.$_POST["name"].'", "'.$_POST["query"].'")';
	}
	else {
		$query = 'UPDATE custom_queries SET name = "'.$_POST["name"].'", query = "'.$_POST["query"].'" WHERE query_id = '.$_POST["query_id"];
	}
	
	$result = $db->query($query);

    if(!$result) {
        echo 'query: '.$query;
        return;
    }

//	sendSocket("reload queries");
    echo "ok";
}

function deleteQuery() {

	global $db;
	
	$query = 'DELETE FROM custom_queries WHERE query_id = '.$_POST["query_id"];
	$result = $db->query($query);

    if(!$result) {
        echo 'query: '.$query;
        return;
    }
//	sendSocket("reload queries");
    echo "ok";
}


function deleteAllJobs() {
	
	global $db;
	
	$now = date("Y-m-d H:i:s");
	$query = "UPDATE scheduled_jobs SET status = 0, canceled_time = datetime('".$now."') WHERE scheduled_time > '".$now."'";
	$result = $db->query($query);

    if(!$result) {
        echo 'query: '.$query;
        return;
    }

    array_map( 'unlink', glob( FIRMWARE_FOLDER."*" ) );

	sendSocket("delete jobs");
    echo "ok";
}

function getLastRowid() {
	global $db_events;
	
	$results = $db_events->query("SELECT MAX(rowid) as rowid FROM history");
    if(!$results) {
        echo 'query: '.$query;
        return;
    }

    $item = $results->fetchArray();
	
	$rowid = $item["rowid"];
	echo $rowid;
}

function getLastEvents() {
	
	global $db_events;

	$rowid = $_POST["rowid"];
	
	$appliedFilters = apply_filters(false);

	$data = array();
	
	$query = "SELECT *, rowid FROM history WHERE rowid > ".$rowid;
	
	if($appliedFilters != "")
		$query .= " AND (".$appliedFilters." )";
		
	$results = $db_events->query($query);

    if(!$results) {
        echo 'query: '.$query;
        return;
    }

    $rawEvents = [];
	if($results)	{
		while($event = $results->fetchArray()) {
			$rawEvents[] = $event;
		}
        if(sizeof($rawEvents) > 0)
            $data = parseEvent($rawEvents);
	}
		
	echo json_encode($data);
}

function getOldEvents() {
	
	$results = getQueryFromFilters(false);
	
	$data = array();

    $rawEvents = [];
    
	if($results)	{
		while($event = $results->fetchArray()) {
			$rawEvents[] = $event;
		}
		$data = parseEvent($rawEvents);
	}
		
	echo json_encode($data);
}

function parseEvent($rows) {
	
	global $db, $eventcode, $eventIdArray;

	$data = array();
	
    $query = "SELECT id, name FROM terminal";
	$terminalResults = $db->query($query);
    if(!$terminalResults) {
        echo 'query: '.$query;
        return;
    }

    while($terminalRiga = $terminalResults->fetchArray(SQLITE3_ASSOC)) {
		$terminals[$terminalRiga["id"]] = $terminalRiga["name"];
	}
	
/*	$userBadgesResult = $db->query("SELECT badge.id AS badge_id, userbadge.user_id AS user_id, user.first_name AS name, user.second_name AS lastname FROM badge LEFT JOIN userbadge ON badge.id = userbadge.badge_id LEFT JOIN user ON userbadge.user_id = user.id");
	while($riga = $userBadgesResult->fetchArray(SQLITE3_ASSOC)) {
		$userBadges[$riga["badge_id"]] = $riga;
	} */
	
    $query = "SELECT * FROM area";
	$areaResults = $db->query($query);
    if(!$areaResults) {
        echo 'query: '.$query;
        return;
    }
	while($areaRiga = $areaResults->fetchArray(SQLITE3_ASSOC)) {
		$areas[$areaRiga["area_id"]] = $areaRiga;
	}
	
    $query = "SELECT * FROM causal_codes WHERE causal_id > 0";
	$causal_codes = $db->query($query);
    if(!$causal_codes) {
        echo 'query: '.$query;
        return;
    }
	while($causal_code = $causal_codes->fetchArray(SQLITE3_ASSOC)) {
		$causals[$causal_code["causal_id"]] = $causal_code;
	}
	
	foreach($rows as $row) {
	
		$bagdeID = $Titolare = $Evento = $Varco = $Causale = $Area = '-';
		
		$dataOra = explode(' ', $row["timestamp"]);
		
        if($row["event"] > sizeof($eventcode)) {
            $Evento = $row["event"];
        } else {
            $Evento = $eventcode[$row["event"]];
        }
        
        if($row["terminal_id"] <= 64) {
            $Varco = ($terminals[$row["terminal_id"]] != '') ? $terminals[$row["terminal_id"]].' - id: '.$row["terminal_id"] : 'Varco '.$row["terminal_id"];
        } else {
            $Varco = 'Varco '.$row["terminal_id"];
        }
	
		switch($row["event"])
		{
			/*********************************************************
			 * No ID - No AREA to be shown
			 *********************************************************/
			case $eventIdArray['badge_cancellati']:
			case $eventIdArray['not_present_in_terminal']:
			case $eventIdArray['terminal_not_inline']:
			case $eventIdArray['terminal_inline']:
			case $eventIdArray['forced_gate']:
			case $eventIdArray['opendoor_timeout']:
			case $eventIdArray['start_tamper']:
			case $eventIdArray['stop_tamper']:
			case $eventIdArray['stop_opendoor_timeout']:
			case $eventIdArray['broken_terminal']:
			case $eventIdArray['low_battery']:
			case $eventIdArray['critical_battery']:
			case $eventIdArray['no_supply']:
			case $eventIdArray['supply_ok']:	
			case $eventIdArray['transit_button']:	
			case $eventIdArray['update_profile']:	
			case $eventIdArray['update_weektime']:	
			case $eventIdArray['update_codes']:	
			
				break;
		
				/*********************************************************
				 * Just ID to be shown
				 *********************************************************/
			case $eventIdArray['no_valid_visitor']:
			case $eventIdArray['disable_in_terminal']:
			case $eventIdArray['no_checkin_weektime']:
			case $eventIdArray['wrong_pin']:
			case $eventIdArray['out_of_area']:
		
				/* ID of badge */
				$bagdeID = $row["badge_id"];
				$Titolare = $row["user_first_name"].' '.$row["user_second_name"];
				break;
		
				/*********************************************************
				 * ID and AREA to be shown
				 *********************************************************/
			case $eventIdArray['start_tasca']:
			case $eventIdArray['stop_tasca']:
			case $eventIdArray['transit']:
			case $eventIdArray['checkin']:
			case $eventIdArray['transit_with_code']:
			case $eventIdArray['checkin_with_code']:
			case $eventIdArray['coercion']:
			case $eventIdArray['alarm_aux']:
			case $eventIdArray['stop_alarm_aux']:
			case $eventIdArray['low_voltge']:
			case $eventIdArray['stop_low_voltge']:
			case $eventIdArray['update_area']:	
			
				/* Causal code */
				if($row["causal_code"] && ($row["causal_code"] != 65535)) {
					$Causale = isset($causals[$row["causal_code"]]["description"]) ? $causals[$row["causal_code"]]["description"] : $row["causal_code"];
				}
				
				/* ID of badge */
				$bagdeID = $row["badge_id"];
				$Titolare = $row["user_first_name"].' '.$row["user_second_name"];
		
				/* Area */
				if(array_key_exists($row["area"], $areas)) {
					$Direzione = "Verso";
					if( ($row["event"] == $eventIdArray['start_tasca']) || ($row["event"] == $eventIdArray['stop_tasca']) ) {
						$Direzione = "In";
					}
					$Area = ($areas[$row["area"]]["name"] != '') ? $areas[$row["area"]]["name"] : 'id: '.$row["area"];
				}
				break;
	
		} // switch
		
		$row["date"] = $dataOra[0];
		$row["time"] = $dataOra[1];
		$row["badgeID"] = $bagdeID;
		$row["titolare"] = $Titolare;
		$row["event_name"] = $Evento;
		$row["causale"] = $Causale;
		$row["varco"] = $Varco;
		$row["area_name"] = $Area;
		
		$data[] = $row;
	} // foreach
	
	return $data;
}

function apply_filters($separateBadge) {
	
	$first = 1;
	
	$logic = ($_POST["logic"]) ? " AND" : " OR";

	$startdate = isset($_POST["startdate"]) ? $_POST["startdate"] : 0;
	$startDateSelected = "";
	if($startdate) {
		$first = 0;
		$startDateSelected = " (timestamp > datetime('".$startdate."'))";
	}

	$enddate = isset($_POST["enddate"]) ? $_POST["enddate"] : 0;
	$endDateSelected = "";
	if($enddate) {
		if(!$first)
			$endDateSelected .= $logic;
		else
			$first = 0;

		$endDateSelected .= " (timestamp < datetime('".$enddate."'))";
	}
	
	$badges = isset($_POST["badges"]) ? $_POST["badges"] : 0;
	$badgeSelected = "";
	if($badges) {
		if(!$first)
			$badgeSelected .= $logic;
		else
			$first = 0;

		foreach($badges as $index => $badge) {
			if(!$index)
				$badgeSelected .= " ((badge_id = ".$badge.")";
			else
				$badgeSelected .= " || (badge_id = ".$badge.")";
		}
		$badgeSelected .= ")";
	}
	
	$titolari = isset($_POST["titolari"]) ? $_POST["titolari"] : 0;
	$titolariSelected = "";
	if($titolari) {
		if(!$first)
			$titolariSelected .= $logic;
		else
			$first = 0;
		
		foreach($titolari as $index => $titolare) {
			$titolare_name = explode("|", $titolare);
			if(!$index)
				$titolariSelected .= " ((user_first_name = '".$titolare_name[0]."' AND user_second_name = '".$titolare_name[1]."')";
			else
				$titolariSelected .= " || (user_first_name = '".$titolare_name[0]."' AND user_second_name = '".$titolare_name[1]."')";
		}
		$titolariSelected .= ")";
	}

	$event_type = isset($_POST["event_type"]) ? $_POST["event_type"] : 0;
	$event_typeSelected = "";
	if($event_type) {
		if(!$first)
			$event_typeSelected .= $logic;
		else
			$first = 0;

		foreach($event_type as $index => $singleevent) {
			if(!$index)
				$event_typeSelected .= " ((event = ".$singleevent.")";
			else
				$event_typeSelected .= " || (event = ".$singleevent.")";
		}
		$event_typeSelected .= ")";
	}

	$terminals = isset($_POST["terminals"]) ? $_POST["terminals"] : 0;
	$terminalsSelected = "";
	if($terminals) {
		if(!$first)
			$terminalsSelected .= $logic;
		else
			$first = 0;

		foreach($terminals as $index => $terminal) {
			if(!$index)
				$terminalsSelected .= " ((terminal_id = ".$terminal.")";
			else
				$terminalsSelected .= " || (terminal_id = ".$terminal.")";
		}
		$terminalsSelected .= ")";
	}

	$causals = isset($_POST["causals"]) ? $_POST["causals"] : 0;
	$causalsSelected = "";
	if($causals) {
		if(!$first)
			$causalsSelected .= $logic;
		else
			$first = 0;

		foreach($causals as $index => $causal) {
			if(!$index) {
				$causalsSelected .= " ((causal_code = ".$causal.")";
			}
			else {
				$causalsSelected .= " || (causal_code = ".$causal.")";
			}
		}
		$causalsSelected .= ")";
	}
	
	$areas = isset($_POST["areas"]) ? $_POST["areas"] : 0;
	$areasSelected = "";
	if($areas) {
		if(!$first)
			$areasSelected .= $logic;
		else
			$first = 0;

		foreach($areas as $index => $area) {
			if(!$index)
				$areasSelected .= " ((area = ".$area.")";
			else
				$areasSelected .= " || (area = ".$area.")";
		}
		$areasSelected .= ")";
	}
	
	return	$startDateSelected
			.$endDateSelected
			.$badgeSelected
			.$titolariSelected
			.$event_typeSelected
			.$terminalsSelected
			.$causalsSelected
			.$areasSelected;
}

function getQueryFromFilters($separateBadge) {

	global $db_events;
	
	$rowid = $_POST["rowid"];

	$limit = isset($_POST["limit"]) ? $_POST["limit"] : 25;
		
	$appliedFilters = apply_filters($separateBadge);
	
	if(!$rowid) {
		$query = "SELECT *, rowid FROM history WHERE 1 ";
		
		if($appliedFilters != "")
			$query .= " AND ";
		
		$query .= $appliedFilters." ORDER BY rowid DESC LIMIT ".$limit;
				
		//echo $query;
		
		$results = $db_events->query($query);
        if(!$results) {
            echo 'query: '.$query;
            return;
        }
        
	} else {
		$query = "SELECT *, rowid FROM history WHERE rowid < ".$rowid;
		
		if($appliedFilters != "")
			$query .= " AND (";
		
		$query .= $appliedFilters;
		
		
		if($appliedFilters != "")
			$query .= " ) ORDER BY rowid DESC LIMIT ".$limit;
		else
			$query .= " ORDER BY rowid DESC LIMIT ".$limit;
				
		//echo $query;
		
		$results = $db_events->query($query);
        if(!$results) {
            echo 'query: '.$query;
            return;
        }

	}
	
	return $results;
}

/* ADMIN FUNCTIONS */
function sendRtc() {
	
	$now = $_POST["time"];
	
	sendSocket("time rtc ".$now);
    echo "ok";
}

function restoreDefault() {
	
	sendSocket("vacuum");
    echo "ok";
}

function changeSupervisorIP() {
	
	$command = "myip ".$_POST["ip"]." ".$_POST["subnet"]; //." mydns ".$_POST["DNS1"]." ".$_POST["DNS1"];
	sendSocket($command);
	sendSocket("mygateway ".$_POST["gateway"]);
    echo "ok";
}

function saveAdminUser() {

	global $db;
	
	$id = $_POST["id"];
	$newInsert = $_POST["newInsert"];
	$username = $_POST["username"];
	$password = md5(DEFAULT_ADMIN_PASSWORD);
	$name = $_POST["name"];
	$lastname = $_POST["lastname"];
	$role = $_POST["role"];
	
	if($_POST["newInsert"]) {
		$query = 'INSERT INTO adminusers (username, password, first_name, second_name, role) VALUES ("'.strtolower($username).'", "'.$password.'", "'.$name.'", "'.$lastname.'", '.$role.')';
	}
	else {
		$query = 'UPDATE adminusers SET username = "'.strtolower($username).'", first_name = "'.$name.'", second_name = "'.$lastname.'", role = '.$role.' WHERE rowid = '.$id;
	}
	
	$result = $db->query($query);
    if(!$result) {
        echo 'query: '.$query;
        return;
    }
}

function checkUserName() {
	
	global $db;
	
	$username = $_POST["username"];
	
	$query = 'SELECT username FROM adminusers WHERE username = "'.strtolower($username).'"';
	$result = $db->query($query);
    if(!$result) {
        echo 'query: '.$query;
        return;
    }

    $user = $result->fetchArray();
//	echo $result->numColumns();
	echo $user["username"] ? 1 : 0;
}

function deleteAdminUser() {

	global $db;

	$query = 'DELETE FROM adminusers WHERE rowid = '.$_POST["id"];
	$result = $db->query($query);
    if(!$result) {
        echo 'query: '.$query;
        return;
    }

    echo "ok";
}

function uploadDB() {
	
	global $db;
	
	$uploadedFile = $_FILES["file"];
	
	if(!$uploadedFile["error"]) {
		if(!$uploadedFile["size"] < DB_MAX_SIZE) {
			$uploadfile = DATABASE_FOLDER . basename("varcolan.db");
			if (move_uploaded_file($uploadedFile["tmp_name"], $uploadfile)) {
				sendSocket("restoredb");
				
				echo '';
			} else {
				echo 'Errore nella copia del file '.$uploadedFile["name"][$i].'. Non è possibile scrivere nella cartella destinazione.<br>';
			}
		}
		else
			echo 'Il file '.$uploadedFile["name"][$i].' eccede la dimensione massima consentita: '.DB_MAX_SIZE.' Byte<br>';
	}
	else
		echo 'Errore nell"upload del file '.$uploadedFile["name"][$i].' - error number: '.$uploadedFile["error"][$i].'<br>';
	
}

function deleteBadgesFromTerminal() {
	
	sendSocket("clean all_badges");
    echo "ok";
}

function deleteBadgesFromSupervisor() {
	
	sendSocket("database clean badges");
    echo "ok";
}


?>
