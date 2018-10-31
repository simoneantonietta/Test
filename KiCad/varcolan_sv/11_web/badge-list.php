<?php
	$editProfili = false;
?>

<div class="container">
    <!-- <div class="pull-right" style="margin-top:30px;"><button class="btn btn-lg btn-warning" type="button">Esporta badge <span class="glyphicon glyphicon-export" aria-hidden="true"></span></button>&emsp;<button class="btn btn-lg btn-success" type="button">Importa badge <span class="glyphicon glyphicon-import" aria-hidden="true"></span></button></div> -->
	<h1>configurazione badge</h1>
    <div class="separator"></div>
</div>

<div id="configurazione-badge" class="container">
	<div class="row">
        <div class="col-lg-6"> <!-- colonna badge -->
            <?php require('badge.php'); ?>
        </div>
        
        <div class="col-lg-2">
            <p style="color:#999999;">
                Trascina il badge sul profilo per associarlo<br> <span class="glyphicon glyphicon-arrow-right pull-left" aria-hidden="true"></span>
            </p>
            <p>&nbsp;</p>
            <p style="color:#999999;">
                Trascina il badge sulla colonna di sinistra per dissociarlo<br> <span class="glyphicon glyphicon-arrow-left pull-left" aria-hidden="true"></span>
            </p>
        </div>
        
        <div class="col-lg-4">
            <?php require('profili.php'); ?>
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
