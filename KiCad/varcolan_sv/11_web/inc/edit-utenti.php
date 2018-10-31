<?php
$id = $_GET['param_id'];

include('global.php');
$db = new SQLite3(DB_HOST);

if($id) {
	$new = 0;
	$results = $db->query('SELECT * FROM user WHERE id = '.$id);
	$item = $results->fetchArray();
}
else {
	$new = 1;
	$results = $db->query('SELECT MAX(id) as max_id FROM user');
	$item = $results->fetchArray();
	$item['id'] = $item['max_id'] + 1;
	$item['first_name'] = '';
	$item['second_name'] = '';
	$item['matricola'] = '';
}

?>      

      <div class="modal-header">
        <button type="button" class="close" data-dismiss="modal">&times;</button>
        <h4 class="modal-title">Impostazioni Utente</h4>
      </div>
      <div class="modal-body">
        <div class="well well-sm">
        	<div class="labels"><strong>Nome</strong></div><div class="values"><input type="text" class="form-control" placeholder="Nome" aria-describedby="name" value="<?php echo $item['first_name']; ?>" id="area-name" ></div>
        	<div class="labels"><strong>Cognome</strong></div><div class="values"><input type="text" class="form-control" placeholder="Cognome" aria-describedby="lastname" value="<?php echo $item['second_name']; ?>" id="area-name" ></div>
        	<div class="labels"><strong>Matricola</strong></div><div class="values"><input type="text" class="form-control" placeholder="Matricola" aria-describedby="matricola" value="<?php echo $item['matricola']; ?>" id="area-name" ></div>
            
           	<div class="labels"><strong>Badge associati</strong></div><div class="values">
                <select data-placeholder="Seleziona i badge da associare" multiple id="badge-utenti" class="form-control">
                  <?php
                  $badges = $db->query('SELECT * FROM badge WHERE visible = 1 ORDER BY printed_code');
				  if(!$new) {
                  	$bres = $db->query('SELECT badge_id FROM userbadge WHERE user_id = '.$item['id']);
					while($singleBadge = $bres->fetchArray()) {
						$badgeAssociati[] = $singleBadge['badge_id'];
					}
					while($badge = $badges->fetchArray()) {
				  ?>
                  <option value="<?php echo $badge['id']; ?>" <?php if(in_array($badge['id'], $badgeAssociati)) echo 'selected' ?>><?php echo $badge['printed_code']; ?></option>
                  <?php }
				  } else { 
					while($badge = $badges->fetchArray()) {
				  ?>
                  <option value="<?php echo $badge['id']; ?>"><?php echo $badge['printed_code']; ?></option>
                  <?php }
				  }
				  ?>
                </select>
            </div>

        </div>

        <div class="submit-buttons">
        	<?php if(!$new) { ?>
            <button type="button" class="btn btn-danger" onClick="deleteUser(<?php echo $id; ?>);">Elimina <span class="glyphicon glyphicon-trash" aria-hidden="true"></span></button>
            <?php } ?>
            <button type="button" class="btn btn-default btn-success pull-right" onClick="saveUser(<?php echo $item['id'].', '.$new; ?>);">Salva <span class="glyphicon glyphicon-ok" aria-hidden="true"></span></button>
            <button type="button" class="btn btn-default btn-warning pull-right" data-dismiss="modal">Annulla <span class="glyphicon glyphicon-remove" aria-hidden="true"></span></button>
        	<?php if($new) { ?>
            <div class="clearfix"></div>
            <?php } ?>
        </div>
      </div>
      
	<script type="text/javascript" src="inc/js/chosen.jquery.min.js"></script>
    <script>
    // CHOSEN
    $('#badge-utenti').chosen({
        no_results_text: "Non esistono badge con questo codice",
        width: "100%"
      });
    
    </script>
