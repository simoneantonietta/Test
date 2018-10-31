<?php
$realTime = isset($_GET['realtime']) ? $_GET['realtime'] : '0';

$db = new SQLite3(DB_HOST);
$db_events = new SQLite3(DB_EVENTS_HOST);

$results = $db_events->query('SELECT * FROM history ORDER BY rowid DESC LIMIT 25');

define('NO_BADGE', 65535);
define('NO_AREA', 255);

// get all badges
//$badgequery = $db->query('SELECT userbadge.user_id AS user_id, userbadge.badge_id AS badge_id, user.first_name AS name, user.second_name AS lastname FROM user INNER JOIN userbadge ON userbadge.user_id = user.id');
$badgequery = $db->query('SELECT * from badge');
while($badge = $badgequery->fetchArray()) {
	$badges[$badge['id']] = $badge;
}

// get all users
$userquery = $db->query('SELECT id, first_name, second_name FROM user');
while($user = $userquery->fetchArray()) {
	$users[$user['id']] = $user;
}

// get all terminals
$terminalquery = $db->query('SELECT id, name FROM terminal');
while($terminal = $terminalquery->fetchArray()) {
	$terminals[$terminal['id']] = $terminal;
}

// get all areas
$areaquery = $db->query('SELECT area_id, name FROM area');
while($area = $areaquery->fetchArray()) {
	$areas[$area['area_id']] = $area;
}

// get all causal codes
$causalquery = $db->query('SELECT * FROM causal_codes');
while($causal = $causalquery->fetchArray()) {
	$causals[$causal['n']] = $causal;
}

?>

<div class="fixed-header-bg"></div>
<form id="exportEventsForm" method="post" action="inc/utils/export_events.php">
<input hidden="hidden" name="rowid" value="0">
<input hidden="hidden" name="limit" value="1000">
<div class="container">
	<div class="pull-right" style="margin-top:30px;"><button id="exportEvents" class="btn btn-lg btn-success" type="button" onClick="document.forms['exportEventsForm'].submit();">Esporta eventi <span class="glyphicon glyphicon-export" aria-hidden="true"></span></button><button id="last-events" class="btn btn-lg btn-warning" type="button" onClick="applyFilters();">Aggiorna <span class="glyphicon glyphicon-refresh" aria-hidden="true"></span></button><button id="start-real-time" class="btn btn-lg btn-primary" type="button" onClick="startRealTime();">Real Time <span class="glyphicon glyphicon-flash" aria-hidden="true"></span></button><button id="stop-real-time" class="btn btn-lg btn-danger" type="button" onClick="stopRealTime();">Stop <span class="glyphicon glyphicon-stop" aria-hidden="true"></span></button></div>
	<h1>Ultimi eventi</h1>
    <div class="separator"></div>
</div>

<div id="event-list" class="event-container">
	<div class="row">
    	<div class="col-lg-12">
            <div id="eventi" class="panel panel-default">
              <div class="panel-heading">
                <h3 class="panel-title">eventi</h3>
              </div>
              <div class="panel-body">
                <table class="eventstable table-striped">
                  <thead>
                    <tr class="filters">
                        <th colspan="2">
                        <div id="fromdate" class="input-group input-append date">
                            <span class="glyphicon glyphicon-remove pull-right" aria-hidden="true" onClick="deleteEventDate(eventfromdate);"></span>
                            <input name="startdate" placeholder="From Date" type="text" data-format="yyyy-MM-dd hh:mm" class="form-control add-on" aria-label="scheduled-date" value="">
                            <span class="input-group-addon add-on"><span data-time-icon="icon-time" data-date-icon="icon-calendar" class="glyphicon glyphicon-calendar" aria-hidden="true"></span></span>
                        </div>
                        <div id="todate" class="input-group input-append date">
                            <input name="enddate" placeholder="To Date" type="text" data-format="yyyy-MM-dd hh:mm" class="form-control add-on" aria-label="scheduled-date" value="">
                            <span class="glyphicon glyphicon-remove pull-right" aria-hidden="true" onClick="deleteEventDate(eventtodate);"></span>
                            <span class="input-group-addon add-on"><span data-time-icon="icon-time" data-date-icon="icon-calendar" class="glyphicon glyphicon-calendar" aria-hidden="true"></span></span>
                        </div>
                        </th>
                        <th>
                        	<select name="badges[]" data-placeholder="Badge ID" multiple id="badge-ids" class="form-control">
                            <?php
                            	while($badge = $badgequery->fetchArray()) {
									echo '<option value="'.$badge['id'].'">'.$badge['id'].'</option>';
								}
							?>
                            </select>
                        </th>
                        <th>
                        	<select name="titolari[]" data-placeholder="Titolare" multiple id="user-ids" class="form-control">
                            <?php
                            	foreach($users as $user_id => $user) {
									$badgeByUser = [];
									foreach($badges as $badge) {
										if($badge['user_id'] == $user['id'])
											$badgeByUser[] = $badge['badge_id'];
									}
									
									echo '<option value="'.$user['first_name'].'|'.$user['second_name'].'">'.$user['first_name'].' '.$user['second_name'].'</option>';
								}
							?>
                            </select>
                        </th>
                        <th>
                        	<select name="event_type[]" data-placeholder="Tipo evento" multiple id="events-ids" class="form-control">
                            <?php
                            	foreach($eventcode as $code => $eventtype) {
									echo '<option value="'.$code.'">'.$eventtype.'</option>';
								}
							?>
                            </select>
                        </th>
                        <th>
                        	<select name="causals[]" data-placeholder="Causale" multiple id="causal-ids" class="form-control">
                            <?php
                            	foreach($causals as $causal) {
									echo '<option value="'.$causal['causal_id'].'">'.$causal['description'].'</option>';
								}
							?>
                            </select>
                        </th>
                        <th>
                        	<select name="terminals[]" data-placeholder="Terminale" multiple id="terminals-ids" class="form-control">
                            <?php
                            	foreach($terminals as $terminalid => $terminal) {
									if($terminal['name'])
										echo '<option value="'.$terminalid.'">'.$terminal['name'].' - id: '.$terminalid.'</option>';
									else
										echo '<option value="'.$terminalid.'">Varco '.$terminalid.'</option>';
								}
							?>
                            </select>
                        </th>
                        <th>
                        	<select name="areas[]" data-placeholder="Destinazione" multiple id="areas-ids" class="form-control">
                            <?php
                            	foreach($areas as $areaid => $area) {
									echo '<option value="'.$areaid.'">'.$area['name'].'</option>';
								}
							?>
                            </select>
                            <div class="pull-right" style="margin-top:10px;">Filtri:&nbsp;
                                <div class="btn-group" role="and-or" aria-label="Logic" data-selected="1">
                                  <button type="button" class="btn btn-default btn-primary" data-selected="1">AND</button>
                                  <button type="button" class="btn btn-default" data-selected="0">OR</button>
                                  <input hidden="hidden" name="logic" value="1">
                                </div>
                            </div>
                        </th>
                    </tr>
                    <tr>
                        <th>Data<div>Data</div></th>
                        <th>Ora<div>Ora</div></th>
                        <th>Badge ID<div>Badge ID</div></th>
                        <th>Titolare<div>Titolare</div></th>
                        <th>Evento<div>Evento</div></th>
                        <th>Causale<div>Causale</div></th>
                        <th>Varco<div>Varco</div></th>
                        <th>Destinazione<div>Destinazione</div></th>
                    </tr>
                  </thead>
                  <tbody>
                  </tbody>
                </table>
              </div>
            </div>
    </div>
  </div>
</div>
</form>

<script type="text/javascript" src="inc/js/chosen.jquery.min.js"></script>
<script>
// CHOSEN
$('#badge-ids').chosen({
	no_results_text: "Non ci sono badge con questo ID",
	width: "100%"
  }).on('change', function(e) {
		applyFilters();
	});

$('#user-ids').chosen({
	no_results_text: "Non ci sono utenti con questo nome",
	width: "100%"
  }).on('change', function(e) {
		applyFilters();
	});

$('#events-ids').chosen({
	no_results_text: "Non ci sono eventi di questo tipo",
	width: "100%"
  }).on('change', function(e) {
		applyFilters();
	});

$('#causal-ids').chosen({
	no_results_text: "Non esiste questo tipo di codice causale",
	width: "100%"
  }).on('change', function(e) {
		applyFilters();
	});

$('#terminals-ids').chosen({
	no_results_text: "Non ci sono terminali con questo nome/tipo",
	width: "100%"
  }).on('change', function(e) {
		applyFilters();
	});

$('#areas-ids').chosen({
	no_results_text: "Non ci sono aree con questo nome",
	width: "100%"
  }).on('change', function(e) {
		applyFilters();
	});

$(document).ready(function(e) {
	getOldEvents();                        
});

</script>