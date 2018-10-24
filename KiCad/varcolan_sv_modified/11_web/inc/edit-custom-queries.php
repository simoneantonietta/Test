<?php
$id = $_GET['param_id'];

include('global.php');
$db = new SQLite3(DB_HOST);

if($id) {
	$new = 0;
	$results = $db->query('SELECT * FROM custom_queries WHERE query_id = '.$id);
	$item = $results->fetchArray();
	$name = $item['name'];
	$query = $item['query'];
}
else {
	$new = 1;
	$results = $db->query('SELECT MAX(query_id) as max_id FROM custom_queries');
	$item = $results->fetchArray();
	$id = $item['max_id'] + 1;
	$name = '';
	$query = '';
}
?>      

      <div class="modal-header">
        <button type="button" class="close" data-dismiss="modal">&times;</button>
        <h4 class="modal-title">Impostazioni Query</h4>
      </div>
      <div class="modal-body">
        <p><strong>ID query: </strong><?php echo $id; ?></p>
        <div class="well well-sm">
        	<div class="labels"><strong>Descrizione</strong></div><div class="values"><input type="text" class="form-control" placeholder="Descrizione query" aria-describedby="descrizione-query" value="<?php echo $name; ?>" id="query-name" ></div>
        	<div class="labels"><strong>Query</strong></div><div class="values"><textarea class="form-control" placeholder="Inserire la query" aria-describedby="query" id="query-content" ><?php echo $query; ?></textarea></div>
        </div>

        <div class="submit-buttons">
        	<?php if(!$new) { ?>
            <button type="button" class="btn btn-danger" onClick="deleteQuery(<?php echo $id; ?>);">Elimina <span class="glyphicon glyphicon-trash" aria-hidden="true"></span></button>
            <?php } ?>
            <button type="button" class="btn btn-default btn-success pull-right" onClick="saveQuery(<?php echo $id.', '.$new; ?>);">Salva <span class="glyphicon glyphicon-ok" aria-hidden="true"></span></button>
            <button type="button" class="btn btn-default btn-warning pull-right" data-dismiss="modal">Annulla <span class="glyphicon glyphicon-remove" aria-hidden="true"></span></button>
        	<?php if($new) { ?>
            <div class="clearfix"></div>
            <?php } ?>
        </div>
      </div>