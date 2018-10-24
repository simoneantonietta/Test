<?php
$id = $_GET['param_id'];

include('global.php');
$db = new SQLite3(DB_HOST);

$results = $db->query('SELECT * FROM profile WHERE id = '.$id);
$item = $results->fetchArray();

?>     
<script>
	// EDITOR PROFILI - PROFILI MODAL
	$('#edit-profilo li.draggable').draggable({ activeClass: 'dragging' })
								.on('draggable:start', function() {
									$('#edit-profilo .droppable-container').slideDown(0);
									$('#edit-profilo .droppable-container .terminali-dropped').slideDown();
									$('.terminali-profilo').css('z-index', '99');
									$(this).offsetParent().css('z-index', '101');
								})
								.on('draggable:stop', function() {
									if( $('#terminali-da-associare .terminali-dropped > ul').height() > $('#terminali-associati .terminali-dropped > ul').height() ) {
										$('#terminali-associati .terminali-dropped > ul').height($('#terminali-da-associare .terminali-dropped > ul').height());
									} else {
										$('#terminali-da-associare .terminali-dropped > ul').height($('#terminali-associati .terminali-dropped > ul').height());
									}
								});
								
	$('#edit-profilo .terminali-dropped ul').droppable({ accept: '.terminali-profilo li.draggable', activeClass: 'prepare-to-drop', hoverClass: 'dropping' });
</script>

      <div class="modal-header">
        <button type="button" class="close" data-dismiss="modal">&times;</button>
        <h4 class="modal-title">Impostazioni Profilo</h4>
      </div>
      <div class="modal-body">
	    <div class="well well-sm">
        	<div class="labels"><strong>Descrizione</strong></div><div class="values"><input type="text" class="form-control" placeholder="Descrizione profilo" aria-describedby="descrizione-profilo" value="<?php echo $item['name']; ?>" id="nome-profilo" ></div>
        	<div class="labels"><strong>Coercizione</strong></div><div class="values">
            	<div class="btn-group" role="yes-no" aria-label="coercion" data-selected="<?php if($item['coercion']) echo '1';else echo '0'; ?>">
                  <button type="button" class="btn btn-default <?php if($item['coercion']) echo 'btn-primary'; ?>">Sì</button>
                  <button type="button" class="btn btn-default <?php if(!$item['coercion']) echo 'btn-danger'; ?>">No</button>
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

        </div>
        
        <div class="row">
            <div class="col-lg-5"> <!-- colonna terminali -->
                <?php require('terminali-da-associare.php'); ?>
            </div>
            
            <div class="col-lg-2">
                <p style="color:#999999;">
                    Trascina il terminale sulla colonna di destra per associarlo<br> <span class="glyphicon glyphicon-arrow-right pull-left" aria-hidden="true"></span>
                </p>
                <p>&nbsp;</p>
                <p style="color:#999999;">
                    Trascina il terminale sulla colonna di sinistra per dissociarlo<br> <span class="glyphicon glyphicon-arrow-left pull-left" aria-hidden="true"></span>
                </p>
            </div>
            
            <div class="col-lg-5"> <!-- colonna terminali -->
                <?php require('terminali-associati.php'); ?>
            </div>
        </div>
        <div class="submit-buttons">
        <?php if($id != 0) { // il profilo 0 non può essere cancellato ?>
            <button type="button" class="btn btn-danger" onClick="deleteProfile(<?php echo $id; ?>);">Elimina <span class="glyphicon glyphicon-trash" aria-hidden="true"></span></button>
        <?php } ?>
            <button type="button" class="btn btn-default btn-success pull-right" onClick="saveProfile(<?php echo $id; ?>);">Salva <span class="glyphicon glyphicon-ok" aria-hidden="true"></span></button>
            <button type="button" class="btn btn-default btn-warning pull-right" data-dismiss="modal">Annulla <span class="glyphicon glyphicon-remove" aria-hidden="true"></span></button>
      		<?php if($id == 0) { ?>
            <div class="clearfix"></div>
           	<?php } ?>

        </div>
      </div>

