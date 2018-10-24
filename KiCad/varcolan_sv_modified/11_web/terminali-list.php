<div class="container">
    <div class="pull-right" style="margin-top:30px;"><a class="get-terminals" href="#" data-toggle="modal" data-target="#get-terminals" ><button class="btn btn-lg btn-success" type="button">Scansione terminali <span class="glyphicon glyphicon-transfer" aria-hidden="true"></span></button></a></div>
	<h1>configurazione terminali</h1>
    <div class="separator"></div>
</div>

<div id="configurazione-sistema" class="container">
	<div class="row">
    	<div class="col-lg-offset-2 col-lg-8"> <!-- colonna terminali -->
	      	<?php require('terminali.php'); ?>
        </div> <!-- colonna terminali -->
    </div>
</div>

<!-- Modal Terminali -->
<div id="edit-terminale" class="modal fade" role="dialog">
  <div class="modal-dialog modal-lg">
    <!-- Modal content-->
    <div class="modal-content">
    </div>
  </div>
</div>

<!-- Modal get terminals -->
<div id="get-terminals" class="modal fade" data-backdrop="static" role="dialog">
  <div class="modal-dialog modal-lg">
    <!-- Modal content-->
    <div class="modal-content">
    </div>
  </div>
</div>
