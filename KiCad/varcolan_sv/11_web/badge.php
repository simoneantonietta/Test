<?php
$db = new SQLite3(DB_HOST);

// get all badges
$badgequery = $db->query('SELECT badge.id AS badge_id, badge.status AS status, badge.printed_code AS printed_code, userbadge.user_id AS user_id, user.first_name AS name, user.second_name AS lastname FROM badge LEFT JOIN userbadge ON userbadge.badge_id = badge.id LEFT JOIN user ON user.id = userbadge.user_id WHERE badge.status = 0 OR badge.status = 1 ORDER BY badge_id ASC');

// get all users
$userquery = $db->query('SELECT id, first_name, second_name FROM user ORDER BY second_name ASC');

?>

            
            <div id="badge" class="panel panel-default">
              <div class="panel-heading">
                <h3 class="panel-title">badge
                <a id="new-badge-button" data-toggle="modal" data-target="#edit-badge" class="badge-item" data-id="0" href="#"><span class="addButton pull-right">Aggiungi / Modifica&nbsp;&nbsp;&nbsp;<span class="glyphicon glyphicon-pencil pull-right" aria-hidden="true"></span></span></a></h3>
              </div>
              <div class="panel-body">
                <div class="table-striped">
                  <div class="table-header">
                  	<ul class="filters">
                    	<li>
                        	<select data-placeholder="ID" multiple id="badge-ids" class="form-control">
                            <?php
                            	while($badge = $badgequery->fetchArray()) {
									echo '<option value="'.$badge['badge_id'].'">'.$badge['badge_id'].'</option>';
								}
							?>
                            </select>
                        </li>
                        <li>
                        	<select data-placeholder="Titolare" multiple id="user-ids" class="form-control">
                            <?php
                            	while($user = $userquery->fetchArray()) {
									$badgeByUser = [];
									while($badge = $badgequery->fetchArray()) {
										if($badge['user_id'] == $user['id'])
											$badgeByUser[] = $badge['badge_id'];
									}
									$values = 0;
									for($i = 0; $i < sizeof($badgeByUser); $i++) {
										if(!$i)
											$values = $badgeByUser[$i];
										else
											$values .= '-'.$badgeByUser[$i];
									}
									echo '<option value="'.$values.'">'.$user['second_name'].' '.$user['first_name'].'</option>';
								}
							?>
                            </select>
                        </li>
                    	<li>
                        	<select data-placeholder="Cod. stampato" multiple id="printed-ids" class="form-control">
                            <?php
                            	while($badge = $badgequery->fetchArray()) {
									if($badge['printed_code'] != "")
										$identifier = $badge['printed_code'];
									else
										$identifier = 'ID: '.$badge['badge_id'];
									echo '<option value="'.$badge['badge_id'].'">'.$identifier.'</option>';
								}
							?>
                            </select>
                        </li>
                        <li class="refresh"><a href="#" onClick="applyBadgeFilters();"><span class="glyphicon glyphicon-refresh" aria-hidden="true"></span></a></li>
                    </ul>
                    <ul>
                        <li><strong>ID</strong></li><li><strong>Titolare</strong></li><li><strong>Cod. stampato</strong></li>
                    </ul>
                  </div>
                  <div class="table-body">
                    <ul>
                    <?php while($badge = $badgequery->fetchArray()) { ?>
                        <li class="draggable<?php if($badge['status'] == 1) echo ' badge-active'; ?>" data-id="<?php echo $badge['badge_id']; ?>">
                        	<a  data-toggle="modal" data-target="#delete-badge-warning" class="badge-item" data-id="<?php echo $badge['badge_id']; ?>" href="#"><span class="glyphicon glyphicon-remove pull-right" aria-hidden="true" ></span></a>  <a  data-toggle="modal" data-target="#edit-badge" class="badge-item" data-id="<?php echo $badge['badge_id']; ?>" href="#"><span class="glyphicon glyphicon-pencil pull-right" aria-hidden="true"></span></a>
                            <ul>
                                <li><?php echo $badge['badge_id'] ?></li><li><?php echo $badge['lastname'].' '.$badge['name']; ?></li><li><?php if($badge['printed_code'] != "") echo $badge['printed_code']; else echo 'ID: '.$badge['badge_id']; ?></li>
                            </ul>
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
$('#badge-ids').chosen({
	no_results_text: "Non ci sono badge con questo ID",
	width: "100%"
  }).on('change', function(e) {
		applyBadgeFilters();
	});
	
$('#user-ids').chosen({
	no_results_text: "Non ci sono utenti con questo nome",
	width: "100%"
  }).on('change', function(e) {
		applyBadgeFilters();
	});

$('#printed-ids').chosen({
	no_results_text: "Non ci sono badge con questo codice stampato",
	width: "100%"
  }).on('change', function(e) {
		applyBadgeFilters();
	});
	
</script>
