<?php

$success = $error = false;
$msg = '';

$min_char = 6;

if(isset($_POST['invia'])){
	
	if( ($_POST['old-passwd'] == '') || ($_POST['new-passwd'] == '') || ($_POST['confirm-passwd'] == '')) {
		$error = true;
		$msg = '&Egrave; necessario compilare tutti campi<br>';
	}

	if( (strlen($_POST['new-passwd']) < $min_char) ) {
		$error = true;
		$msg .= 'La chiave deve essere composta da almeno 6 caratteri<br>';
	}
	
	if($_POST['new-passwd'] != $_POST['confirm-passwd']) {
		$error = true;
		$msg .= 'I campi non coincidono<br>';
	}
	
	if( preg_match('/[\'\$%\&\*=;:\<\>,\.\?#]/', $_POST['new-passwd'], $matches) ) {
		if( $matches[0] != $_POST['new-passwd'] ) {
			$error = true;
			$msg .= 'Hai inserito caratteri non ammessi<br>Caratteri non ammessi (\' ;:,*?=&%$<>#)';
		}
	}

	if(!$error) {
		// Update database
		$db = new SQLite3(DB_HOST);
		
		$query = "select username from adminusers where rowid=".$_SESSION['userid']." and password='".md5(pulisci($_POST['old-passwd']))."'";
		$result = $db->query($query);

		if($result->fetchArray() != "") {
			$query = 'UPDATE adminusers SET password = "'.md5(pulisci($_POST['new-passwd'])).'" WHERE rowid = '.$_SESSION['userid'];
			$result = $db->query($query);
			$success = true;
		} else {
			$error = true;
			$msg .= 'Vecchia password errata';
		} 

	}
}
?>

<div class="container">
	<h1>modifica password</h1>
    <div class="separator"></div>
</div>

<div id="change-password" class="container">
	<div class="row">

        <div class="col-lg-offset-3 col-lg-6">
        	<?php if($error) { ?>
            <div class="errorMessage alert alert-danger" role="alert" ><?php echo $msg; ?></div>
        	<?php } ?>
        	<?php if($success) { ?>
            <div class="alert alert-success" role="alert" ><?php echo "Password modificata con successo!"; ?></div>
        	<?php } ?>
            <form action="" method="post">
                <div class="login_campo">
                    <div class="values"><input maxlength="8" type="password" name="old-passwd" class="form-control" placeholder="Inserisci la vecchia password" aria-describedby="old-passwd" value="" ></div>
                </div>
                <div class="login_campo">
                    <div class="values"><input maxlength="8" type="password" name="new-passwd" class="form-control" placeholder="Inserisci la nuova password" aria-describedby="new-passwd" value="" ></div>
                </div>
                <div class="login_campo">
                    <div class="values"><input maxlength="8" type="password" name="confirm-passwd" class="form-control" placeholder="Conferma la nuova password" aria-describedby="confirm-passwd" value="" ></div>
                </div>
                <div class="login_campo">
                    <button type="submit" name="invia" class="btn btn-default btn-danger pull-right">Modifica&nbsp;&nbsp;<span class="glyphicon glyphicon-pencil" aria-hidden="true"></span></button>
                </div>
            </form>
        </div>

    </div>
</div>