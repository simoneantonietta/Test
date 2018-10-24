<?php
$active_terminals = $item['active_terminal'];
$terminalQuery = $db->query('SELECT id, status, name FROM terminal WHERE status > 0');
?>
            <div id="terminali-associati" class="panel panel-default terminali-profilo">
              <div class="panel-heading">
                <h3 class="panel-title">terminali già associati</h3>
              </div>
              <div class="panel-body">
                <div class="table-striped">
                  <div class="table-header">
                    <ul>
                        <li><strong>ID</strong></li>
                        <li><strong>Descrizione</strong></li>
                    </ul>
                  </div>
                  <div class="table-body">
                  	<div class="droppable-container">
                        <div class="terminali-dropped">
                            <ul data-id="<?php echo $id ?>">
								<?php
                                if($active_terminals != 'IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII') {
									while($terminal = $terminalQuery->fetchArray()) {
										if($active_terminals[64 - $terminal['id']] == "A") {
											echo '<li class="draggable" data-id="'.$terminal['id'].'">
													<ul>
														<li>'.$terminal['id'].'</li>
														<li>'.$terminal['name'].'</li>
													</ul>
												</li>';
										}
									}
                                }
                            ?>
                            </ul>
                        </div>
                    </div>
                  </div>
                </div>
              </div>
            </div>
