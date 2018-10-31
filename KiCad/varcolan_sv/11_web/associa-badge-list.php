<?php
	$editProfili = false;
?>

<div class="container">
    <!-- <div class="pull-right" style="margin-top:30px;"><button class="btn btn-lg btn-warning" type="button">Esporta utenti <span class="glyphicon glyphicon-export" aria-hidden="true"></span></button>&emsp;<button class="btn btn-lg btn-success" type="button">Importa utenti <span class="glyphicon glyphicon-import" aria-hidden="true"></span></button></div> -->
	<h1>Configurazione badge</h1>
    <div class="separator"></div>
</div>

<div id="configurazione-utenti" class="container">
	<div class="row">
        <div class="col-lg-3"> <!-- colonna utenti -->
            <?php require('profili.php'); ?>
        </div>
        
        <div class="col-lg-1">
            <p style="color:#999999;">
                Trascina il badge sul profilo per associarlo<br> <span class="glyphicon glyphicon-arrow-left pull-left" aria-hidden="true"></span>
            </p>
        </div>

        <div class="col-lg-4"> <!-- colonna utenti -->
            <?php require('badge.php'); ?>
        </div>
        
        <div class="col-lg-1">
            <p style="color:#999999;">
                Trascina il badge sull'utente per associarlo<br> <span class="glyphicon glyphicon-arrow-right pull-left" aria-hidden="true"></span>
            </p>
        </div>
        
        <div class="col-lg-3">
            <?php require('utenti.php'); ?>
        </div>
    </div>
</div>

<!-- Modal Badge -->
<div id="edit-badge" class="modal fade" data-backdrop="static" role="dialog">
  <div class="modal-dialog modal-lg">
    <!-- Modal content-->
    <div class="modal-content">
    </div>
  </div>
</div>


<!-- Modal utenti -->
<div id="edit-utenti" class="modal fade" role="dialog">
  <div class="modal-dialog modal-lg">
    <!-- Modal content-->
    <div class="modal-content">
    </div>
  </div>
</div>


<!-- Modal Cancellazione Badge -->
<div id="delete-badge-warning" class="modal fade" role="dialog">
  <div class="modal-dialog modal-lg">
    <!-- Modal content-->
    <div class="modal-content">
    </div>
  </div>
</div>


