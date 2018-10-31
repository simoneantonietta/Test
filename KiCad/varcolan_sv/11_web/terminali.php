<?php
$db = new SQLite3(DB_HOST);

/*
STATUS: 0 - non esiste
STATUS: 1 - online
STATUS: 2 - offline
*/

$results = $db->query('SELECT * FROM terminal WHERE status > 0');
?>
            <div id="terminali" class="panel panel-default">
              <div class="panel-heading">
                <h3 class="panel-title">terminali</h3>
              </div>
              <div class="panel-body">
                <div class="table-striped">
                  <div class="table-header">
                    <ul>
                        <li><strong>ID</strong></li>
                        <li><strong>Descrizione</strong></li>
                        <li><strong>Status</strong></li>
                    </ul>
                  </div>
                  <div class="table-body">
                    <ul>
                    <?php while($data = $results->fetchArray()) { ?>
                        <li class="draggable<?php if($data['status'] == 1) echo ' online'; else echo ' offline';?>"  data-id="<?php echo $data['id']; ?>">
                        	<?php if($data['status'] == 2) { ?>
                            	<span class="glyphicon glyphicon-remove pull-right" aria-hidden="true" onClick="removeTerminal(<?php echo $data['id']; ?>)"></span>
                            <?php } ?><a data-toggle="modal" data-target="#edit-terminale" class="terminale-item" data-id="<?php echo $data['id']; ?>" href="#"><span class="glyphicon glyphicon-pencil pull-right" aria-hidden="true"></span></a>
                            <ul>
                                <li><?php echo $data['id']; ?></li>
                                <li><?php echo $data['name']; ?></li>
                                <li><span class="glyphicon glyphicon-<?php if($data['status'] == 1) echo 'ok';?>"></span></li>
                            </ul>
                        </li>
                      <?php }
					  	$db->close(); ?>
                    </ul>
                  </div>
                </div>
              </div>
            </div>
