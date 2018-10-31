<?php
$db = new SQLite3(DB_HOST);

$results = $db->query('SELECT * FROM area');
?>
<div class="container">
	<h1>configurazione aree</h1>
    <div class="separator"></div>
</div>

<div id="configurazione-sistema" class="container">
	<div class="row">
        <div class="col-lg-offset-3 col-lg-6">
        	<div id="aree" class="panel panel-default">
              <div class="panel-heading">
                <h3 class="panel-title">aree
                <a  data-toggle="modal" data-target="#edit-area" class="area-item" data-id="0" href="#"><span class="addButton pull-right">Aggiungi <span class="glyphicon glyphicon-plus pull-right" aria-hidden="true"></span></span></a></h3>
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
	                    <?php while($data = $results->fetchArray()) {
                        	if($data['area_id'] < 2) {
						?>
                        <li>
                            <?php echo $data['name']; ?></li>
                        <?php } else { ?>
                        <li>
                        	<span class="glyphicon glyphicon-remove pull-right" data-id="<?php echo $data['area_id']; ?>" aria-hidden="true" onClick="deleteArea(<?php echo $data['area_id']; ?>)"></span>  <a  data-toggle="modal" data-target="#edit-area" class="area-item" data-id="<?php echo $data['area_id']; ?>" href="#"><span class="glyphicon glyphicon-pencil pull-right" aria-hidden="true"></span></a>
                            <?php echo $data['name']; ?></li>
                      <?php }
						}
					  	$db->close(); ?>
                    </ul>
                  </div>
                </div>
               </div>
            </div>
        </div>    
    </div>
</div>

<!-- Modal Fasce orarie -->
<div id="edit-area" class="modal fade" role="dialog">
  <div class="modal-dialog modal-lg">
    <!-- Modal content-->
    <div class="modal-content">
    </div>
  </div>
</div>