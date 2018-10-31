<?php
$id = $_GET['param_id'];

include('global.php');
$db = new SQLite3(DB_HOST);

$results = $db->query('SELECT * FROM terminal WHERE id = '.$id);
$item = $results->fetchArray();

if( $item['entrance_type'] == 0 ) {
	echo '<style>.bidirezionale, .modal-content .labels.bidirezionale, .modal-content .values.bidirezionale, .modal-content .well .form-control.bidirezionale  { display:none; }</style>';
}

if( $item['entrance_type'] == 2 ) {
	echo '<style>.tasca, .modal-content .labels.tasca, .modal-content .values.tasca, .modal-content .well .form-control.tasca { display:none; }</style>';
}

?>  

      <div class="modal-header">
        <button type="button" class="close" data-dismiss="modal">&times;</button>
        <h4 class="modal-title">Impostazioni Terminale</h4>
      </div>
      <form>
      <div class="modal-body">
        <p><strong>ID Terminale: </strong><?php echo $item['id']; ?></p>
		<p><strong>Versione firmware: </strong>v<?php echo $item['fw_ver']; ?></p>
        <p><strong>Indirizzo IP: </strong><a href="http://<?php echo $item['IP_addr']; ?>" target="_blank"><?php echo $item['IP_addr']; ?></a></p>
        <p><strong>Indirizzo MAC: </strong><?php echo $item['MAC_addr']; ?></p>
        <?php
			
		?>
        <p><strong>Profili: </strong>
		<?php
		$string = '';
		for($i = 0; $i < 64; $i++) {
			if((64 - $i) == $item['id']) {
				$string .= 'A%';
				break;
			}
			else {
				$string .= '_';
			}
		}

        $profiles = $db->query('SELECT * FROM profile WHERE active_terminal LIKE "'.$string.'"');
        $profile = $profiles->fetchArray();
        if($profile) echo $profile['name'];
        while($profile = $profiles->fetchArray()) {
            echo ", ".$profile['name'];
        } ?></p>
        <div class="well well-sm">
        	<div class="labels"><strong>Descrizione</strong></div><div class="values"><input type="text" class="form-control" placeholder="Descrizione terminale" aria-describedby="descrizione-terminale" value="<?php echo $item['name']; ?>" ></div>

        	<div class="labels"><strong>ON Relè</strong></div><div class="values"><input type="number" class="form-control" placeholder="Tempo di relè ON" aria-describedby="open_door_time" value="<?php echo $item['open_door_time']; ?>"> sec.</div>

        	<div class="labels tasca"><strong>Apertura prolungata</strong></div><div class="values tasca"><input type="number" class="form-control" placeholder="Tempo di apertura prolungata" aria-describedby="open_door_timeout" value="<?php echo $item['open_door_timeout']; ?>"> sec.</div>
            
        	<div class="labels tasca"><strong>Anti Passback</strong></div><div id="area-in-out" class="values tasca">
            	<div class="btn-group" role="yes-no" aria-label="antipassback" data-selected="<?php echo $item['antipassback']; ?>">
                  <button type="button" class="btn btn-default<?php if($item['antipassback']) echo ' btn-primary'; ?>">Sì</button>
                  <button type="button" class="btn btn-default<?php if(!$item['antipassback']) echo ' btn-danger'; ?>">No</button>
                </div>
            </div>
            
            <div class="labels"><strong>Fascia oraria</strong></div><div id="weektime" class="values">
                <select class="form-control">
                  <option value="65">Nessuna fascia oraria</option>
                  <?php
				  	$wtquery = $db->query('SELECT * FROM weektime WHERE weektimedata != "24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|"');
					while($wktms = $wtquery->fetchArray()) {
				  ?>
                  <option value="<?php echo $wktms['id']; ?>" <?php if($item['weektime_id'] == $wktms['id']) echo 'selected'; ?>><?php echo $wktms['name']; ?></option>
                  <?php } ?>
                </select>
            </div>

        	<div class="labels"><strong>Tipo di porta</strong></div><div id="entrance_type" class="values">
            	<div class="btn-group" role="radio" aria-label="entrance_type" data-selected="<?php echo $item['entrance_type']; ?>">
                  <button type="button" class="btn btn-default<?php if($item['entrance_type'] == 0) echo ' btn-success'; ?>" onClick="configBidirezionale()" data-selected="0">Bidirezionale</button>
                <?php if($item['hw_ver']) : ?>
                  <button type="button" class="btn btn-default<?php if($item['entrance_type'] == 1) echo ' btn-success'; ?>" onClick="configMonodirezionale()" data-selected="1">Monodirezionale</button>
               	<?php endif; ?>
                  <button type="button" class="btn btn-default<?php if($item['entrance_type'] == 2) echo ' btn-success'; ?>" onClick="configTasca()" data-selected="2">Tasca</button>
                </div>
            </div>
            
            <div class="labels tasca"><h3>Lettore 1</h3></div><div class="values tasca"></div>

        	<div class="labels tasca"><strong>Tipo di accesso Lettore 1</strong></div><div class="values tasca">
            	<div class="btn-group" role="radio" aria-label="access1" data-selected="<?php echo $item['access1']; ?>">
                  <button type="button" class="btn btn-default<?php if($item['access1'] == 0) echo ' btn-success'; ?>" data-selected="0">Nessuno</button>
                  <button type="button" class="btn btn-default<?php if($item['access1'] == 1) echo ' btn-success'; ?>" data-selected="1">Badge Only</button>
                  <button type="button" class="btn btn-default<?php if($item['access1'] == 2) echo ' btn-success'; ?>" data-selected="2">Badge / ID+PIN</button>
                  <button type="button" class="btn btn-default<?php if($item['access1'] == 3) echo ' btn-success'; ?>" data-selected="3">Badge + PIN</button>
                </div>
            </div>

            <div class="labels"><strong>Area Lettore<span class="tasca"> 1</span></strong></div><div class="values"><span id="area1_reader1"><span class="bidirezionale tasca">DA</span>
                <select class="form-control">
                  <option value="255">Nessuna area</option>
                  <?php
                    $areaquery = $db->query('SELECT * FROM area');
                    while($area = $areaquery->fetchArray()) {
                  ?>
                  <option value="<?php echo $area['area_id']; ?>" <?php if($item['area1_reader1'] == $area['area_id']) echo 'selected'; ?>><?php echo $area['name']; ?></option>
                  <?php } ?>
                </select>
                </span>
                <span id="area2_reader1">
                <span class="bidirezionale tasca"></span><span class="bidirezionale tasca">A</span>
                <select class="form-control bidirezionale tasca">
                  <option value="255">Nessuna area</option>
                  <?php
                    $areaquery = $db->query('SELECT * FROM area');
                    while($area = $areaquery->fetchArray()) {
                  ?>
                  <option value="<?php echo $area['area_id']; ?>" <?php if($item['area2_reader1'] == $area['area_id']) echo 'selected'; ?>><?php echo $area['name']; ?></option>
                  <?php } ?>
                </select><br>
                <a href="?menu=aree"> <span class="glyphicon glyphicon-plus" aria-hidden="true"></span> Crea una nuova area</a>
                </span>
            </div>

            <div class="labels tasca"><h3>Lettore 2</h3></div><div class="values tasca"></div>
            
            <div class="labels tasca"><strong>Tipo di accesso Lettore 2</strong></div><div class="values tasca">
            	<div class="btn-group" role="radio" aria-label="access2" data-selected="<?php echo $item['access2']; ?>">
                  <button type="button" class="btn btn-default<?php if($item['access2'] == 0) echo ' btn-success'; ?>" data-selected="0">Nessuno</button>
                  <button type="button" class="btn btn-default<?php if($item['access2'] == 1) echo ' btn-success'; ?>" data-selected="1">Badge Only</button>
                  <button type="button" class="btn btn-default<?php if($item['access2'] == 2) echo ' btn-success'; ?>" data-selected="2">Badge / ID+PIN</button>
                  <button type="button" class="btn btn-default<?php if($item['access2'] == 3) echo ' btn-success'; ?>" data-selected="3">Badge + PIN</button>
                </div>
            </div>

            <div class="labels tasca"><strong>Area Lettore 2</strong></div><div class="values tasca"><span id="area1_reader2"><span class="bidirezionale">DA</span>
                <select class="form-control">
                  <option value="255">Nessuna area</option>
                  <?php
                    while($area = $areaquery->fetchArray()) {
                  ?>
                  <option value="<?php echo $area['area_id']; ?>" <?php if($item['area1_reader2'] == $area['area_id']) echo 'selected'; ?>><?php echo $area['name']; ?></option>
                  <?php } ?>
                </select>
                </span>
                <span id="area2_reader2">
                <span class="bidirezionale tasca"></span><span class="bidirezionale tasca">A</span>
                <select class="form-control bidirezionale tasca">
                  <option value="255">Nessuna area</option>
                  <?php
                    $areaquery = $db->query('SELECT * FROM area');
                    while($area = $areaquery->fetchArray()) {
                  ?>
                  <option value="<?php echo $area['area_id']; ?>" <?php if($item['area2_reader2'] == $area['area_id']) echo 'selected'; ?>><?php echo $area['name']; ?></option>
                  <?php } ?>
                </select><br>
                <a href="?menu=aree" class="tasca"> <span class="glyphicon glyphicon-plus" aria-hidden="true"></span> Crea una nuova area</a>
            	</span>

        </div>
        
        </div>
        
        <div class="submit-buttons">
            <button type="button" class="btn btn-default btn-success pull-right" onClick="saveTerminal(<?php echo $id; ?>);">Salva <span class="glyphicon glyphicon-ok" aria-hidden="true"></span></button>
            <button type="button" class="btn btn-default btn-warning pull-right" data-dismiss="modal">Annulla <span class="glyphicon glyphicon-remove" aria-hidden="true"></span></button>
            <div class="clearfix"></div>
        </div>
        </form>
      </div>