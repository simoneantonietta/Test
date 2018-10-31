<?php

//require('inc/utils/open_socket.php');

$error = false;
$msg = '';

if(isset($_POST['invia'])){
	
	if( ($_POST['old-key-krypt'] == '') || ($_POST['new-key-krypt'] == '') || ($_POST['confirm-key-krypt'] == '')) {
		$error = true;
		$msg = 'Devi compilare tutti campi<br>';
	}

	if( (strlen($_POST['old-key-krypt']) < 8) || (strlen($_POST['new-key-krypt']) < 8) || (strlen($_POST['confirm-key-krypt']) < 8) ) {
		$error = true;
		$msg = 'La chiave deve essere composta da 8 caratteri<br>';
	}
	
	if($_POST['new-key-krypt'] != $_POST['confirm-key-krypt']) {
		$error = true;
		$msg .= 'I campi non coincidono<br>';
	}
	
	if( preg_match('/[0-9a-fA-F]+/', $_POST['new-key-krypt'], $matches) ) {
		if( $matches[0] != $_POST['new-key-krypt'] ) {
			$error = true;
			$msg .= 'Hai inserito caratteri non ammessi<br>Sono ammessi solo caratteri esadecimali (0-9, a-f)';
		}
	}

	if(!$error) {
		sendSocket('newkey '.$_POST['new-key-krypt']);
	}
}
?>

<div class="container">
	<h1>modifica chiave di cifratura</h1>
    <div class="separator"></div>
</div>

<div id="change-krypt" class="container">
	<div class="row">

        <div class="col-lg-offset-3 col-lg-6">
        	<?php if($error) { ?>
            <div class="errorMessage alert alert-danger" role="alert" ><?php echo $msg; ?></div>
        	<?php } ?>
            <form action="" method="post">
                <div class="login_campo">
                    <div class="values"><input maxlength="8" type="password" name="old-key-krypt" class="form-control" placeholder="Inserisci la vecchia chiave di cifratura" aria-describedby="old-key-krypt" value="" ></div>
                </div>
                <div class="login_campo">
                    <div class="values"><input maxlength="8" type="password" name="new-key-krypt" class="form-control" placeholder="Inserisci la nuova chiave di cifratura" aria-describedby="new-key-krypt" value="" ></div>
                </div>
                <div class="login_campo">
                    <div class="values"><input maxlength="8" type="password" name="confirm-key-krypt" class="form-control" placeholder="Conferma la nuova chiave di cifratura" aria-describedby="confirm-key-krypt" value="" ></div>
                </div>
                <div class="login_campo">
                    <button type="submit" name="invia" class="btn btn-default btn-danger pull-right">Modifica&nbsp;&nbsp;<span class="glyphicon glyphicon-pencil" aria-hidden="true"></span></button>
                </div>
            </form>
        </div>

    </div>
</div>