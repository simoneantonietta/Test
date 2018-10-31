<?php
$db = new SQLite3(DB_HOST);

// get all users
$userquery = $db->query('SELECT *, rowid AS id FROM adminusers ORDER BY second_name ASC');

$roles = array('1' => 'admin', '10' => 'user', '100' => 'view');
?>

<div class="container">
	<h1>gestione utenti</h1>
    <div class="separator"></div>
</div>

<div id="settings" class="container">
	<div class="row">

        <div class="col-lg-offset-3 col-lg-6">
            <div id="adminusers" class="panel panel-default">
              <div class="panel-heading">
                <h3 class="panel-title">utenti
                <a  data-toggle="modal" data-target="#edit-adminusers" class="adminusers-item" data-id="" href="#"><span class="addButton pull-right">Aggiungi <span class="glyphicon glyphicon-plus pull-right" aria-hidden="true"></span></span></a></h3>
              </div>
              <div class="panel-body">
                <div class="table-striped">
                  <div class="table-header">
                    <ul>
                        <li><strong>Cognome e Nome</strong></li><li><strong>Username</strong></li><li><strong>Ruolo</strong></li>
                    </ul>
                  </div>
                  <div class="table-body">
                    <ul> <!-- LISTA UTENTI -->
					<?php while($user = $userquery->fetchArray()) { ?>
                        <li class="item-utente" data-id="<?php echo $user['id']; ?>">
	                        <?php if($user['id'] != $_SESSION['userid']) { ?><span class="glyphicon glyphicon-remove pull-right" aria-hidden="true" onClick="deleteAdminUser(<?php echo $user['id']; ?>);"><?php } ?></span>  <a  data-toggle="modal" data-target="#edit-adminusers" class="utenti-item" data-id="<?php echo $user['id']; ?>" href="#"><span class="glyphicon glyphicon-pencil pull-right" aria-hidden="true"></span></a>
                        	<ul>
                            	<li><?php echo $user['second_name']." ".$user['first_name']; ?></li><li><?php echo $user['username'] ?></li><li><?php echo $roles[strval($user['role'])]; ?></li>
                            </ul>
                        </li>
                      <?php }
					  	$db->close(); ?>
                    </ul>
                  </div>
                </div>
              </div>
            </div>
        </div>

    </div>
</div>

<!-- Modal utenti -->
<div id="edit-adminusers" class="modal fade" role="dialog">
  <div class="modal-dialog modal-lg">
    <!-- Modal content-->
    <div class="modal-content">
    </div>
  </div>
</div>
