<?php
$id = $_GET['param_id'];

include('global.php');
$db = new SQLite3(DB_HOST);

if($id) {
	$new = 0;
	$results = $db->query('SELECT * FROM causal_codes WHERE n = '.$id);
	$item = $results->fetchArray();
	$name = $item['description'];
	$code = $item['causal_id'];
}
else {
	$new = 1;
	$results = $db->query('SELECT * FROM causal_codes WHERE causal_id = 0 LIMIT 1');
	$item = $results->fetchArray();
	$id = $item['n'];
	$code = '';
	$name = '';
}
?>      

      <div class="modal-header">
        <button type="button" class="close" data-dismiss="modal">&times;</button>
        <h4 class="modal-title">Impostazioni Area</h4>
      </div>
      <div class="modal-body">
        <p><strong>ID Codice Causale: </strong><?php echo $id; ?></p>
        <div class="well well-sm">
        	<div class="labels"><strong>Codice</strong></div><div class="values causal_code form-group"><input type="text" class="form-control" placeholder="Codice da 1 a 65000" aria-describedby="codice-badge" value="<?php echo $code; ?>" id="causal-code" ><span class="glyphicon glyphicon-ok form-control-feedback" aria-hidden="true"></span><span class="glyphicon glyphicon-remove form-control-feedback" aria-hidden="true"></span></div>
        	<div class="labels"><strong>Descrizione</strong></div><div class="values"><input type="text" class="form-control" placeholder="Descrizione codice causale" aria-describedby="descrizione-causal-code" value="<?php echo $name; ?>" id="causal-code-name" ></div>
        </div>

        <div class="submit-buttons">
        	<?php if(!$new) { ?>
            <button type="button" class="btn btn-danger" onClick="deleteCausalCode(<?php echo $id; ?>);">Elimina <span class="glyphicon glyphicon-trash" aria-hidden="true"></span></button>
            <?php } ?>
            <button type="button" class="btn btn-default btn-success pull-right" onClick="saveCausalCode(<?php echo $id ?>);">Salva <span class="glyphicon glyphicon-ok" aria-hidden="true"></span></button>
            <button type="button" class="btn btn-default btn-warning pull-right" data-dismiss="modal">Annulla <span class="glyphicon glyphicon-remove" aria-hidden="true"></span></button>
        	<?php if($new) { ?>
            <div class="clearfix"></div>
            <?php } ?>
        </div>
      </div>