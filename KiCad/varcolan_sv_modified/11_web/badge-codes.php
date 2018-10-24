<?php
$db = new SQLite3(DB_HOST);

$results = $db->query('SELECT * FROM causal_codes WHERE causal_id > 0 ORDER BY causal_id');
?>
<div class="container">
	<h1>configurazione codici causali</h1>
    <div class="separator"></div>
</div>

<div id="causal-codes" class="container">
	<div class="row">
        <div class="col-lg-offset-3 col-lg-6">
        	<div id="aree" class="panel panel-default">
              <div class="panel-heading">
                <h3 class="panel-title">codici causali
                <?php
                $codes = [];
				while($code = $results->fetchArray()) {
					$codes[] = $code;
				}
				
				if(sizeof($codes) != 10) { ?>
                <a  data-toggle="modal" data-target="#edit-badge-codes" class="badge-code-item" data-id="0" href="#"><span class="addButton pull-right">Aggiungi <span class="glyphicon glyphicon-plus pull-right" aria-hidden="true"></span></span></a><?php } ?></h3>
              </div>
              <div class="panel-body">
                <div class="table-striped">
                  <div class="table-header">
                    <ul>
                        <li><strong>Codice</strong></li><li><strong>Descrizione</strong></li>
                    </ul>
                  </div>
                  <div class="table-body">
                    <ul>
	                    <?php foreach($codes as $data) { ?>
                        <li>
                        	<span class="glyphicon glyphicon-remove pull-right" data-id="<?php echo $data['n']; ?>" aria-hidden="true" onClick="deleteCausalCode(<?php echo $data['n']; ?>)"></span>  <a  data-toggle="modal" data-target="#edit-badge-codes" class="area-item" data-id="<?php echo $data['n']; ?>" href="#"><span class="glyphicon glyphicon-pencil pull-right" aria-hidden="true"></span></a>
                        	<ul>
                            	<li><?php echo $data['causal_id']; ?></li><li><?php echo $data['description']; ?></li>
                            </ul>
                         </li>
                      <?php
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
<div id="edit-badge-codes" class="modal fade" role="dialog">
  <div class="modal-dialog modal-lg">
    <!-- Modal content-->
    <div class="modal-content">
    </div>
  </div>
</div>