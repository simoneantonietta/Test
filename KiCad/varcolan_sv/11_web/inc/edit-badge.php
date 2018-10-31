<?php
$id = $_GET['param_id'];
$num_badge = isset($_GET['badge_num']) ? $_GET['badge_num'] : 0;

$passwordDefault = '12345';

include('global.php');
$db = new SQLite3(DB_HOST);
$db_events = new SQLite3(DB_EVENTS_HOST);

if($id) {
	$get = 0;

//	$results = $db->query('SELECT * FROM badge WHERE status != 2 AND id = '.$id);
	$results = $db->query('SELECT * FROM badge WHERE id = '.$id);
	$item = $results->fetchArray();

	if($item) {
		$new = 0;
		$arearesult = $db_events->query('SELECT current_area FROM area WHERE badge_id = '.$id);
		$currentArea = $arearesult->fetchArray();
	}
	else {
		$new = 1;
		$item['badge_num'] = $num_badge;
		$item['status'] = 1;
		$item['printed_code'] = '';
		$item['pin'] = $passwordDefault;
		$item['profile_id0'] = 'NULL';
		$item['profile_id1'] = 'NULL';
		$item['profile_id2'] = 'NULL';
		$item['profile_id3'] = 'NULL';
		$item['profile_id4'] = 'NULL';
		$item['profile_id5'] = 'NULL';
		$item['profile_id6'] = 'NULL';
		$item['profile_id7'] = 'NULL';
		$item['profile_id8'] = 'NULL';
		$item['visitor'] = 0;
		$item['validity_stop'] = '';
		$item['contact'] = '';
	}
}
else {
	$get = 1;
	require('new-badge.php');
}

?>  

<?php if(!$get) { ?>
      <div class="modal-header">
        <button type="button" class="close" data-dismiss="modal">&times;</button>
        <h4 class="modal-title">Impostazioni badge</h4>
      </div>
      <div class="modal-body<?php if($item['status'] == 2) echo " badge-deleted"; ?>">
        <p><strong>ID badge: </strong><?php echo $id; ?></p>
        <?php if(1) echo '<p hidden="hidden"><strong>Codice Tessera: </strong><span class="badge_num">'.$item['badge_num'].'</span></p>'; ?>
        <div class="well well-sm">
        	<div class="labels"><strong>Stato</strong></div><div class="values">
            	<div class="btn-group" role="radio" aria-label="Status" data-selected="<?php echo $item['status']; ?>">
                  <button type="button" data-class=" btn-warning" class="btn btn-default<?php if($item['status'] == 0) echo ' btn-warning'; ?>" data-selected="0">Disabilitato</button>
                  <button type="button" data-class=" btn-success" class="btn btn-default<?php if($item['status'] == 1) echo ' btn-success'; ?>" data-selected="1">Abilitato</button>
                </div>
            </div>
            <div class="labels"><strong>Area Attuale</strong></div><div id="area" class="values">
                <select class="form-control">
                      <?php
                        $areaquery = $db->query('SELECT * FROM area');
                        while($area = $areaquery->fetchArray()) {
                      ?>
                      <option value="<?php echo $area['area_id']; ?>" <?php if($currentArea['current_area'] == $area['area_id']) echo 'selected'; ?>><?php echo $area['name']; ?></option>
                      <?php } ?>
                    </select>
                 Modificare per forzare l'area
            </div>
            <div class="labels"><strong>Permessi</strong></div><div class="values">
            	<div class="btn-group" role="radio" aria-label="user_type" data-selected="<?php echo isset($item['user_type']) ? $item['user_type'] : 0; ?>">
                  <button type="button" data-class=" btn-success" class="btn btn-default<?php if($item['user_type'] == 0) echo ' btn-success'; ?>" data-selected="0">Base</button>
                  <button type="button" data-class=" btn-warning" class="btn btn-default<?php if($item['user_type'] == 1) echo ' btn-warning'; ?>" data-selected="1">Installatore</button>
                  <button type="button" data-class=" btn-danger" class="btn btn-default<?php if($item['user_type'] == 2) echo ' btn-danger'; ?>" data-selected="2">Amministratore</button>
                </div>
            </div>

        </div>
        <div class="well well-sm">
        	<div class="labels"><strong>Codice stampato</strong></div><div class="values"><input type="text" class="form-control" placeholder="Codice stampato" aria-describedby="codice-stampato" value="<?php echo $item['printed_code']; ?>" ></div>

        	<div class="labels"><strong>PIN</strong></div><div class="values"><input type="password" class="form-control" placeholder="Password" aria-describedby="password-badge" max="5" value="<?php if($item['pin']) echo $item['pin']; else echo $passwordDefault; ?>" ></div>

        	<div class="labels"><strong>Ridigita PIN</strong></div><div class="values"><input type="password" class="form-control" placeholder="Password" aria-describedby="password-confirm-badge" max="5" value="<?php if($item['pin']) echo $item['pin']; else echo $passwordDefault; ?>" ></div>
            
        	<div class="labels"><strong>Profili associati</strong></div><div class="values">
                <select data-placeholder="Seleziona i profili da associare" multiple id="badge-profili" class="form-control">
                  <?php $profiles = $db->query('SELECT * FROM profile WHERE status = 1');
				  	while($profile = $profiles->fetchArray()) {
				  ?>
                  <option value="<?php echo $profile['id']; ?>" <?php
                  for($i = 0; $i < 9; $i++) {
					  $indexProfile = 'profile_id'.$i;
					  if(sizeof($item[$indexProfile])) {
						  if($item[$indexProfile] == $profile['id']) {
							  echo 'selected';
						  }
					  }
				  } ?>><?php echo $profile['name']; ?></option>
                  <?php } ?>
                </select>
            </div>
            
            <div class="labels"><strong>Visitatore</strong></div><div id="visitatore" class="values">
            	<div class="btn-group open-options" role="yes-no" aria-label="visitor" data-selected="<?php echo $item['visitor']; ?>">
                  <button type="button" class="btn btn-default<?php if($item['visitor']) echo ' btn-primary'; ?>">Sì</button>
                  <button type="button" class="btn btn-default<?php if(!$item['visitor']) echo ' btn-danger'; ?>">No</button>
                </div>
            </div>

			<div class="options" <?php if($item['visitor']) echo 'style="display:block;"'; ?>>
                <div class="labels"><strong>Validità</strong></div><div class="values">
	                <div id="datetimepicker1" class="input-group input-append date">
						<input type="text" data-format="yyyy-MM-dd" class="form-control add-on" aria-label="validity-stop" value="<?php if(!$item['validity_stop']) echo date('Y-m-d', strtotime('+1 day', time())); else echo $item['validity_stop']; ?>">
                  		<span class="input-group-addon add-on"><span data-time-icon="icon-time" data-date-icon="icon-calendar" class="glyphicon glyphicon-calendar" aria-hidden="true"></span></span>
                      </div>
                  
                </div>
				<script type="text/javascript">
                  $(function() {
                    $('#datetimepicker1').datetimepicker({
                      language: 'it-IT',
					  pickTime: false
                    });
                  });
                </script>
                <div class="labels"><strong>Referente</strong></div><div class="values"><input type="text" class="form-control" placeholder="Nome referente" aria-describedby="referente-badge" value="<?php echo $item['contact']; ?>" ></div>
            </div>

        </div>
        <div class="submit-buttons">
			<?php if(!$new) { ?>
				<?php if($item['status'] != 2) { ?>
            <a  data-toggle="modal" data-target="#delete-badge-warning" class="badge-item" data-id="<?php echo $id; ?>" href="#"><button type="button" class="btn btn-danger" href="#">Elimina <span class="glyphicon glyphicon-trash" aria-hidden="true"></span></button></a>
            	<?php } ?>
			<?php } ?>
            <button type="button" class="btn btn-default btn-success pull-right" onClick="saveBadge(<?php echo $id.', '.$new; ?>);">Salva <span class="glyphicon glyphicon-ok" aria-hidden="true"></span></button>
            <button type="button" class="btn btn-default btn-warning pull-right" data-dismiss="modal">Annulla <span class="glyphicon glyphicon-remove" aria-hidden="true"></span></button>
       		<?php if($new || ($item['status'] == 2)) { ?>
            <div class="clearfix"></div>
           	<?php } ?>
        </div>
      </div>
      
    <script type="text/javascript" src="inc/js/chosen.jquery.min.js"></script>
    <script>
	// CHOSEN
	$('#badge-profili').chosen({
		max_selected_options: 9,
		no_results_text: "Non esistono profili con questo nome",
		width: "100%"
	  });
	
	</script>
<?php } ?>
