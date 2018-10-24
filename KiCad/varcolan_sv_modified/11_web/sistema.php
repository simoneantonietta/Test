<?php
	$editProfili = true;
?>

<div class="container">
	<h1>configurazione sistema</h1>
    <div class="separator"></div>
</div>

<div id="configurazione-sistema" class="container">
	<div class="row">
    	<div class="col-lg-4"> <!-- colonna terminali -->
	      	<?php require('terminali.php'); ?>
        </div> <!-- colonna terminali -->
    
        <div class="col-lg-1">
            <p style="color:#999999;">
                Trascina il terminale su un profilo per associarlo <span class="glyphicon glyphicon-arrow-right pull-right" aria-hidden="true"></span>
            </p>
        </div>
    
        <div class="col-lg-3">
	      	<?php require('profili.php'); ?>
        </div>
        
    
        <div class="col-lg-1">
            <p style="color:#999999;">
                Trascina la fascia oraria su un profilo per associarla <span class="glyphicon glyphicon-arrow-left" aria-hidden="true"></span>
            </p>
        </div>

        <div class="col-lg-3">
	      	<?php require('fasce-orarie.php'); ?>
        </div>    

    </div>
</div>

<!-- Modal Profili -->
<div id="edit-profilo" class="modal fade" role="dialog">
  <div class="modal-dialog modal-lg">
    <!-- Modal content-->
    <div class="modal-content">
    </div>
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

<!-- Modal Fasce orarie -->
<div id="edit-fascia-oraria" class="modal fade" role="dialog">
  <div class="modal-dialog modal-lg">
    <!-- Modal content-->
    <div class="modal-content">
    </div>
  </div>
</div>