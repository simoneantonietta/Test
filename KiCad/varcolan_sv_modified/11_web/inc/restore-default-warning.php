<?php
$id = $_GET['param_id'];

?>  

      <div class="modal-header">
        <button type="button" class="close" data-dismiss="modal">&times;</button>
        <h4 class="modal-title">Attenzione!</h4>
      </div>
      <div class="modal-body">
        <p>Questa operazione elimina in modo definitivo tutte le impostazioni già memorizzate riportando il sistema alle impostazioni di fabbrica.<br>
Se si vuole soltanto cancellare dei badge, usare la sezione Cancellazione Badge di questa pagina.<br></p>
		<h2>Tutte le impostazioni andranno perse.</h2><h2>Ripristinare il sistema?</h2>

        <div class="submit-buttons">
            <button type="button" class="btn btn-danger" onClick="restoreDefault();">Ripristina <span class="glyphicon glyphicon-erase" aria-hidden="true"></span></button>
            <button type="button" class="btn btn-default btn-warning pull-right" data-dismiss="modal">Annulla <span class="glyphicon glyphicon-remove" aria-hidden="true"></span></button>
            <div class="clearfix"></div>
        </div>
      </div>