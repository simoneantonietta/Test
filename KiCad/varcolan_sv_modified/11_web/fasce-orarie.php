<?php
$db = new SQLite3(DB_HOST);

$results = $db->query('SELECT * FROM weektime WHERE weektimedata = "24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|" LIMIT 1');
$newPos = $results->fetchArray();

$results = $db->query('SELECT * FROM weektime WHERE weektimedata != "24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|"');
?>

        	<div id="fasce-orarie" class="panel panel-default">
              <div class="panel-heading">
                <h3 class="panel-title">fasce orarie
                <a  data-toggle="modal" data-target="#edit-fascia-oraria" class="fascia-oraria-item" data-id="<?php echo $newPos['id']; ?>" href="#"><span class="addButton pull-right">Aggiungi <span class="glyphicon glyphicon-plus pull-right" aria-hidden="true"></span></span></a></h3>
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
	                    <?php while($data = $results->fetchArray()) { ?>
                        <li class="wt_draggable" data-id="<?php echo $data['id']; ?>">
                        	<span class="glyphicon glyphicon-remove pull-right" data-id="<?php echo $data['id']; ?>" aria-hidden="true" onClick="deleteWt(<?php echo $data['id']; ?>)"></span>  <a  data-toggle="modal" data-target="#edit-fascia-oraria" class="fascia-oraria-item" data-id="<?php echo $data['id']; ?>" href="#"><span class="glyphicon glyphicon-pencil pull-right" aria-hidden="true"></span></a>
                            <?php echo $data['name']; ?></li>
                      <?php }
					  	$db->close(); ?>
                    </ul>
                  </div>
                </div>
               </div>
            </div>