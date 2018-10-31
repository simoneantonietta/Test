<?php 
	$loggeduser = 1000;

	session_start();
	
	include('inc/global.php');

	/** funzione per eliminare i caratteri pericolosi dai campi provenienti dal form **/
	function pulisci($login){
		$login=str_replace(" ","",$login);
		$login=str_replace(";","",$login);
		$login=str_replace(":","",$login);
		$login=str_replace(",","",$login);
		$login=str_replace("'","",$login);
		$login=str_replace("*","",$login);
		$login=str_replace("?","",$login);
		$login=str_replace("=","",$login);
		$login=str_replace("&","",$login);
		$login=str_replace("%","",$login);
		$login=str_replace("$","",$login);
		$login=str_replace("<","",$login);
		$login=str_replace(">","",$login);
		$login=str_replace("#","",$login);
		return $login;
	}
	
	function check_role($menu){
		
		global $loggeduser;
		
		if( !isset($_SESSION['autorizzato']) ) {
		  return false;
		}
		 
		$loggeduser = $_SESSION['role'];
		
		if( ($menu == 'logout') ||
			($menu == 'login') ||
			($menu == 'admin_change-password') ) {
				return true;
		}
		
		if($loggeduser < 1000) {
			if(	($menu == '') ||
				($menu == 'queries') ||
				($menu == 'eventi') ) {
				return true;
			}
		}
		
		if($loggeduser < 100) {
			if(	$menu == 'associa-badge-list' ) {
				return true;
			}
		}

		if($loggeduser < 10) {
			if(isset($_POST['backupDB'])) {
	
				$file = '/data/database/varcolan.db';
			
				if (file_exists($file)) {
					header('Content-Type: application/octet-stream');
					header("Content-Transfer-Encoding: Binary"); 
					header('Content-disposition: attachment; filename="'.basename($file).'"');
					readfile($file);
				}
			}

			return true;
		}
		
		return false;
	}
	
	$menu = isset($_GET['menu']) ? $_GET['menu'] : '';
	
	$error=false;
	if(isset($_POST['username'])){
		
		$db = new SQLite3(DB_HOST);
	
		//recupero i dati dell'utente, se esiste
		$q="select *, rowid from adminusers where username='".pulisci(strtolower($_POST['username']))."' and password='".md5(pulisci($_POST['password']))."'";
		$results = $db->query($q);
	
		if($row = $results->fetchArray()){
					
			$_SESSION['autorizzato'] = 1;
			
			$_SESSION['userid'] = $row['rowid'];
			$_SESSION['username'] = $row['username'];
			
			/*Registro il codice dell'utente*/
			$_SESSION['role'] = $row['role'];
			$loggeduser = $_SESSION['role'];
			
			if($_POST['password'] == DEFAULT_ADMIN_PASSWORD) {
				$port = isset($_SERVER['SERVER_PORT']) ? ':'.$_SERVER['SERVER_PORT'] : "";
				header('location: http://'.$_SERVER["SERVER_NAME"].$port.$_SERVER["SCRIPT_NAME"].'?menu=admin_change-password');
//				$menu = 'admin_change-password';
				//$firsttime = true;
			}
			
		//nessuna corrispondenza con gli utenti: non mi loggo e ritorno al form
		} else
			$error=true;
			
		$db->close();
	}
	
	if($menu == 'logout') {
		$_SESSION = array();
		session_destroy();
		$port = isset($_SERVER['SERVER_PORT']) ? ':'.$_SERVER['SERVER_PORT'] : "";
		header('location: http://'.$_SERVER["SERVER_NAME"].$port.$_SERVER["SCRIPT_NAME"]);
	} else {
		if(!check_role($menu))
			$menu = 'login';
	}
	
	require('inc/header.php');
	
	$defaultUrl = 'eventi.php';
	
	if($menu == '')
		require($defaultUrl);
	else {
		$menuitem = str_replace('_', '/', $menu);
		require($menuitem.'.php');
	}
	
	require('inc/footer.php');

?>	