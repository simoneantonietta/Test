<?php
session_start();

$id = $_GET['param_id'];

include('global.php');
$db = new SQLite3(DB_HOST);

if($id) {
	$new = 0;
	$results = $db->query('SELECT *, rowid AS id FROM adminusers WHERE rowid = '.$id);
	$item = $results->fetchArray();
}
else {
	$new = 1;
	$results = $db->query('SELECT MAX(rowid) as max_id FROM adminusers');
	$item = $results->fetchArray();
	$item['id'] = $item['max_id'] + 1;
	$item['username'] = '';
	$item['first_name'] = '';
	$item['second_name'] = '';
	$item['role'] = 100;
}

?>      

      <div class="modal-header">
        <button type="button" class="close" data-dismiss="modal">&times;</button>
        <h4 class="modal-title">Impostazioni Utente</h4>
      </div>
      <div class="modal-body">
        <div class="well well-sm">

			<?php if($id != $_SESSION['userid']) { ?>
        	<div class="labels"><strong>username</strong></div><div class="values username form-group"><input type="text" class="form-control" placeholder="username" aria-describedby="username" value="<?php echo strtolower($item['username']); ?>" id="area-name" ><span class="glyphicon glyphicon-ok form-control-feedback" aria-hidden="true"></span><span class="glyphicon glyphicon-remove form-control-feedback" aria-hidden="true"></span></div>
            <?php } else { ?>
        	<div class="labels"><strong>username</strong></div><div class="values"><?php echo strtolower($item['username']); ?></div>
			<?php } ?>
            
        	<div class="labels"><strong>Nome</strong></div><div class="values"><input type="text" class="form-control" placeholder="Nome" aria-describedby="name" value="<?php echo $item['first_name']; ?>" id="area-name" ></div>

        	<div class="labels"><strong>Cognome</strong></div><div class="values"><input type="text" class="form-control" placeholder="Cognome" aria-describedby="lastname" value="<?php echo $item['second_name']; ?>" id="area-name" ></div>
            
            <?php if($id != $_SESSION['userid']) { ?>
        	<div class="labels"><strong>Ruolo</strong></div><div class="values">
            	<div class="btn-group" role="radio" aria-label="role" data-selected="<?php echo $item['role']; ?>">
                  <button type="button" class="btn btn-default<?php if($item['role'] == 1) echo ' btn-success'; ?>" data-selected="1">Admin</button>
                  <button type="button" class="btn btn-default<?php if($item['role'] == 10) echo ' btn-success'; ?>" data-selected="10">User</button>
                  <button type="button" class="btn btn-default<?php if($item['role'] == 100) echo ' btn-success'; ?>" data-selected="100">View</button>
                </div>
            </div>
            <?php } else { ?>
        	<div class="labels"><strong>Ruolo</strong></div><div class="values"><?php if($item['role'] == 1) echo 'Admin'; elseif($item['role'] == 10) echo 'User'; else echo 'View'; ?></div>
			<?php } ?>

        </div>

        <div class="submit-buttons">
        	<?php if(!$new && ($id != $_SESSION['userid'])) { ?>
            <button type="button" class="btn btn-danger" onClick="deleteAdminUser(<?php echo $id; ?>);">Elimina <span class="glyphicon glyphicon-trash" aria-hidden="true"></span></button>
            <?php } ?>
            <button type="button" class="btn btn-default btn-success pull-right" onClick="saveAdminUser(<?php echo $item['id'].', '.$new; ?>);">Salva <span class="glyphicon glyphicon-ok" aria-hidden="true"></span></button>
            <button type="button" class="btn btn-default btn-warning pull-right" data-dismiss="modal">Annulla <span class="glyphicon glyphicon-remove" aria-hidden="true"></span></button>
        	<?php if($new || ($id == $_SESSION['userid'])) { ?>
            <div class="clearfix"></div>
            <?php } ?>
        </div>
      </div>
      
