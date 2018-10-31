<?php
$webversion = "v03.11";
$env = php_uname('m');

if(strstr($env, 'arm')) {
	define('DB_HOST', '/data/database/varcolan.db');
	define('DB_EVENTS_HOST', '/tmp/varcolan_events.db');
	define('SUPERVISOR', 'localhost');
	define('FIRMWARE_FOLDER', '/data/firmware_sched/');
	define('DATABASE_FOLDER', '/data/restore_db/');
	define('NEW_FIRMWARE_FOLDER', '/data/firmware_new/');
}
else {
	define('DB_HOST', '/Applications/XAMPP/xamppfiles/htdocs/11_web/database/varcolan.db');
	define('DB_EVENTS_HOST', '/Applications/XAMPP/xamppfiles/htdocs/11_web/database/varcolan_events.db');
	define('SUPERVISOR', 'localhost');
	define('FIRMWARE_FOLDER', '/Applications/XAMPP/xamppfiles/htdocs/11_web/firmware_sched/');
	define('DATABASE_FOLDER', '/Applications/XAMPP/xamppfiles/htdocs/11_web/restore_db/');
	define('NEW_FIRMWARE_FOLDER', '/Applications/XAMPP/xamppfiles/htdocs/11_web/firmware_new/');
}

define('FILE_MAX_SIZE', 3000000);
define('DB_MAX_SIZE', 10000000);

$eventcode = array("","","","","Badge Cancellati","Start tasca","Stop tasca","Transito","Timbratura","","","Fuori Area","Non abilitato nel terminale","No Transito per Fascia Oraria","Fine validità","","","Coercizione","Identificativo Assente","Terminale fuori linea","Terminale in linea","Forzatura porta","Apertura prolungata","Fine forzatura/apertura prolungata","Allarme AUX","Fine allarme AUX","Bassa tensione","Fine bassa tensione","Manomissione","Fine manomissione","Pin Errato","Terminale guasto","Batteria bassa","Batteria critica","Manca Rete", "Nessun Errore alimentazione", "Pulsante apriporta","FW aggiornato","Modo di accesso errato", "", "", "", "", "Profilo aggiornato", "F.O. aggiornata", "Causali aggiornate", "Terminale aggiornato", "Area aggiornata");

$eventIdArray = array(
	"badge_cancellati" => 4,
	"start_tasca" => 5,
	"stop_tasca" => 6,
	"transit" => 7,
	"checkin" => 8,
	"transit_with_code" => 9,
	"checkin_with_code" => 10,
	"out_of_area" => 11,
	"disable_in_terminal" => 12,
	"no_checkin_weektime" => 13,
	"no_valid_visitor" => 14,
	"coercion" => 17,
	"not_present_in_terminal" => 18,
	"terminal_not_inline" => 19,
	"terminal_inline" => 20,
	"forced_gate" => 21,
	"opendoor_timeout" => 22,
	"stop_opendoor_timeout" => 23,
	"alarm_aux" => 24,
	"stop_alarm_aux" => 25,
	"low_voltge" => 26,
	"stop_low_voltge" => 27,
	"start_tamper" => 28,
	"stop_tamper" => 29,
	"wrong_pin" => 30,
	"broken_terminal" => 31,
	"low_battery" => 32,
	"critical_battery" => 33,
	"no_supply" => 34,
	"supply_ok" => 35,
	"transit_button" => 36,
	"FW_updated" => 37,
	"wrong_access" => 38,
	"update_profile" => 43,
	"update_weektime" => 44,
	"update_codes" => 45,
	"update_terminal" => 46,
	"update_area" => 47
	);


define('TIME_COOKIE', 3600);

define('DEFAULT_ADMIN_PASSWORD', '123456');

?>
