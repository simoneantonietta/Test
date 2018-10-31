<?php
$db = new SQLite3(DB_HOST);

$results = $db->query('SELECT * FROM profile WHERE status = 0 LIMIT 1');
$newPos = $results->fetchArray();

$results = $db->query('SELECT profile.*, weektime.name AS wname FROM profile LEFT JOIN weektime ON profile.weektime_id = weektime.id WHERE status != 0');
?>

        	<div id="profili" class="panel panel-default">
              <div class="panel-heading">
                <h3 class="panel-title">profili
                <?php if($editProfili) { ?><a  data-toggle="modal" data-target="#edit-profilo" class="profilo-item" data-id="<?php echo $newPos['id']; ?>" href="#"><span class="addButton pull-right">Aggiungi <span class="glyphicon glyphicon-plus pull-right" aria-hidden="true"></span></span></a>
                <?php } ?>
                </h3>
              </div>
              <div class="panel-body">
                <div class="table-striped">
                  <div class="table-header">
                    <ul>
                        <li><strong>Nome</strong></li>
                    </ul>
                  </div>
                  <div class="table-body">
                    <ul>
                    	<?php
                    		while($data = $results->fetchArray()) {
						?>
                        <li class="badge-droppable terminali-droppable" data-id="<?php echo $data['id']; ?>"><?php if($editProfili) { if(($data['id'] != 0)) { ?><span class="glyphicon glyphicon-remove pull-right" aria-hidden="true" onClick="deleteProfile(<?php echo $data['id']; ?>);"></span>  <?php } ?><a  data-toggle="modal" data-target="#edit-profilo" class="profilo-item" data-id="<?php echo $data['id']; ?>" href="#"><span class="glyphicon glyphicon-pencil pull-right" aria-hidden="true"></span></a><?php } ?><h3><?php echo $data['name']; ?> <span class="glyphicon glyphicon-triangle-bottom" aria-hidden="true"></span></h3>
                        	<div class="droppable-container">
                            	<div class="fasce-orarie-dropped">
                                    <strong>Fasce Orarie</strong>
                                    <ul data-id="<?php echo $data['id']; ?>"><?php
										if($data['weektime_id'] != 65) {
											$wtid = $data['weektime_id'];
											echo '<li>
                        	<span class="glyphicon glyphicon-minus pull-right" aria-hidden="true" data-id="'.$data['weektime_id'].'" onClick="deleteWeektime('.$data['weektime_id'].', this)"></span><span class="glyphicon glyphicon-link pull-right" aria-hidden="true"></span>
'.$data['wname'].'</li>';
										}
									?>
                                    </ul>
                                </div>
                                <div class="terminali-dropped">
                                    <strong>Terminali</strong>
                                    <ul data-id="<?php echo $data['id']; ?>">
                                    	<?php
										if($data['active_terminal'] != 'IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII') {
											$terminals = $data['active_terminal'];
											for($i = 0; $i < 64; $i++) {
												if($terminals[63 - $i] == "A") {
													$termid = $i + 1;
													$terms = $db->query('SELECT id, name FROM terminal WHERE id = '.$termid);
													$term = $terms->fetchArray();
													echo '<li>
															<span class="glyphicon glyphicon-minus pull-right" aria-hidden="true" data-id="'.$term['id'].'" onClick="deleteTerminal('.$term['id'].', this)"></span><span class="glyphicon glyphicon-link pull-right" aria-hidden="true"></span>
															<ul>
																<li>'.$term['id'].'</li>
																<li>'.$term['name'].'</li>
															</ul>
														</li>';
												}
											}
										}
									?>
                                    </ul>
                                </div>
                            	<div class="badge-dropped">
                                    <strong>Badge</strong>
                                    <ul data-id="<?php echo $data['id']; ?>">
                                    <?php
										$badge_results = $db->query('SELECT * FROM badge WHERE ( (profile_id0 = '.$data['id'].') OR (profile_id1 = '.$data['id'].') OR (profile_id2 = '.$data['id'].') OR (profile_id3 = '.$data['id'].') OR (profile_id4 = '.$data['id'].') OR (profile_id5 = '.$data['id'].') OR (profile_id6 = '.$data['id'].') OR (profile_id7 = '.$data['id'].') OR (profile_id8 = '.$data['id'].') ) AND status != 2');
										while($badge = $badge_results->fetchArray()) { ?>
											<li<?php if($badge['status'] == 2) echo ' class="badge-active"'; ?>>
                                                <span class="glyphicon glyphicon-minus pull-right" aria-hidden="true" onClick="removeBadgeFromProfile(<?php echo $badge['id'].", ".$data['id']; ?>)"></span><span class="glyphicon glyphicon-link pull-right" aria-hidden="true"></span>
                                                <ul>
                                                    <li><?php echo 'ID: '.$badge['id'].' - '.$badge['printed_code']; ?></li>
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