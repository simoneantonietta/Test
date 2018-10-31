<?php
$db = new SQLite3(DB_HOST);

// get all users
$userquery = $db->query('SELECT id, first_name, second_name FROM user ORDER BY second_name ASC');

?>

            
            <div id="utenti" class="panel panel-default">
              <div class="panel-heading">
                <h3 class="panel-title">utenti
                <a  data-toggle="modal" data-target="#edit-utenti" class="utenti-item" data-id="" href="#"><span class="addButton pull-right">Aggiungi <span class="glyphicon glyphicon-plus pull-right" aria-hidden="true"></span></span></a></h3>
              </div>
              <div class="panel-body">
                <div class="table-striped">
                  <div class="table-header">
                     <ul class="filters">
                        <li>
                        	<select data-placeholder="Titolare" multiple id="users-ids" class="form-control">
                            <?php
                            	//foreach($users as $user_id => $user) {
								while($user = $userquery->fetchArray(SQLITE3_ASSOC)) {	
									echo '<option value="'.$user['id'].'">'.$user['second_name'].' '.$user['first_name'].'</option>';
								}
							?>
                            </select>
                        </li>
                        <li class="refresh"><a href="#" onClick="applyUserFilters();"><span class="glyphicon glyphicon-refresh" aria-hidden="true"></span></a></li>
                    </ul>
                    <ul>
                        <li><strong>Cognome e Nome</strong></li>
                    </ul>
                  </div>
                  <div class="table-body">
                    <ul> <!-- LISTA UTENTI -->
					<?php while($user = $userquery->fetchArray(SQLITE3_ASSOC)) { ?>
                        <li class="badge-droppable item-utente" data-id="<?php echo $user['id']; ?>">
	                        <span class="glyphicon glyphicon-remove pull-right" aria-hidden="true" onClick="deleteUser(<?php echo $user['id']; ?>);"></span>  <a  data-toggle="modal" data-target="#edit-utenti" class="utenti-item" data-id="<?php echo $user['id']; ?>" href="#"><span class="glyphicon glyphicon-pencil pull-right" aria-hidden="true"></span></a>
                        	<ul>
                            	<li><?php echo $user['second_name']." ".$user['first_name']; ?></li>
                            </ul>
                            <div class="droppable-container">
                                <div class="badge-dropped">
                                    <strong>Badge associati</strong>
                                    <ul data-id="<?php echo $user['id']; ?>">
                                    <?php
										$badge_results = $db->query('SELECT userbadge.user_id AS user_id, userbadge.badge_id AS badge_id, user.first_name AS name, user.second_name AS lastname, badge.status AS status, badge.printed_code AS printed_code FROM user INNER JOIN userbadge ON userbadge.user_id = user.id INNER JOIN badge ON badge.id = userbadge.badge_id WHERE user_id = '.$user['id']);
										while($badge = $badge_results->fetchArray()) { ?>
											<li<?php if($badge['status'] == 2) echo ' class="badge-active"'; ?>>
                                                <span class="glyphicon glyphicon-link pull-right" aria-hidden="true"></span><span class="glyphicon glyphicon-minus pull-right" aria-hidden="true" onClick="removeBadgeFromUser(<?php echo $badge['badge_id'].", ".$user['id']; ?>)"></span>
                                                <ul>
                                                    <li><?php echo 'ID: '.$badge['badge_id'].' - '.$badge['printed_code']; ?></li>
                                                </ul>
                                            </li>
										<?php }
									?>
                                    </ul>
                                </div>
                            </div>
                        </li>
                      <?php }
					  	$db->close(); ?>
                    </ul>
                  </div>
                </div>
              </div>
            </div>

<script type="text/javascript" src="inc/js/chosen.jquery.min.js"></script>
<script>
// CHOSEN
$('#users-ids').chosen({
	no_results_text: "Non ci sono utenti con questo nome",
	width: "100%"
  }).on('change', function(e) {
		applyUserFilters();
	});
</script>