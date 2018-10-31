<?php
$id = $_GET['param_id'];

?>  

      <div class="modal-header">
        <button type="button" class="close" data-dismiss="modal">&times;</button>
        <h4 class="modal-title">Attenzione!</h4>
      </div>
      <div class="modal-body">
        <p>Questa operazione elimina in modo definitivo il badge e non sarà più possibile utilizzarlo in seguito.<br>
Se lo si vuole soltanto disassociare dall'utente, agire nella scheda degli utenti.<br>
Se lo si vuole soltanto disabilitare cliccare sul pulsante di modifica.</p>
		<h2>Eliminare definitivamente il badge?</h2>

        <div class="submit-buttons">
            <button type="button" class="btn btn-danger" onClick="deleteBadge(<?php echo $id; ?>);">Elimina <span class="glyphicon glyphicon-trash" aria-hidden="true"></span></button>
            <button type="button" class="btn btn-default btn-warning pull-right" data-dismiss="modal">Annulla <span class="glyphicon glyphicon-remove" aria-hidden="true"></span></button>
            <div class="clearfix"></div>
        </div>
      </div>