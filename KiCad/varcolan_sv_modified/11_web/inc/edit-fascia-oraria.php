<?php
$idWt = $_GET['param_id'];

$weekDays = ["Luned&igrave;", "Marted&igrave;", "Mercoled&igrave;", "Gioved&igrave;", "Venerd&igrave;", "Sabato", "Domenica"];
include('global.php');
$db = new SQLite3(DB_HOST);

$results = $db->query('SELECT * FROM weektime WHERE id = '.$idWt);
$item = $results->fetchArray();

$orari = explode("|", $item['weektimedata']);
$indexOrari = 0;
?>      

      <div class="modal-header">
        <button type="button" class="close" data-dismiss="modal">&times;</button>
        <h4 class="modal-title">Impostazioni Fascia Oraria</h4>
      </div>
      <div class="modal-body">
        <p><strong>Profili: </strong>
        <?php $profiles = $db->query('SELECT * FROM profile WHERE weektime_id = '.$idWt);
			$profile = $profiles->fetchArray();
			if($profile) echo $profile['name'];
			while($profile = $profiles->fetchArray()) {
				echo ", ".$profile['name'];
			} ?></p>
        <div class="well well-sm">
        	<div class="labels"><strong>Descrizione</strong></div><div class="values"><input type="text" class="form-control" placeholder="Descrizione terminale" aria-describedby="descrizione-weektime" value="<?php echo $item['name']; ?>" id="fascia-name" ></div>
        </div>

        <div class="well well-sm">
        	<div class="labels"><strong>Giorno settimanale</strong></div><div class="values"><ul class="orario"><li><strong>Mattino</strong></li><li><strong>Pomeriggio</strong></li></ul></div>

        	<?php require('fasceorarie.php'); ?>
            
        </div>
        <div class="submit-buttons">
            <button type="button" class="btn btn-danger" onClick="deleteWt(<?php echo $idWt; ?>);">Elimina <span class="glyphicon glyphicon-trash" aria-hidden="true"></span></button>
            <button type="button" class="btn btn-default btn-success pull-right" onClick="saveWt(<?php echo $idWt; ?>);">Salva <span class="glyphicon glyphicon-ok" aria-hidden="true"></span></button>
            <button type="button" class="btn btn-default btn-warning pull-right" data-dismiss="modal">Annulla <span class="glyphicon glyphicon-remove" aria-hidden="true"></span></button>
        </div>
      </div>