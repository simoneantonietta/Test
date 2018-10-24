<?php
$db = new SQLite3(DB_HOST);

$results = $db->query('SELECT * FROM custom_queries');
?>
<div class="container">
	<h1>configurazione queries</h1>
    <div class="separator"></div>
</div>

<div id="configurazione-sistema" class="container">
	<div class="row">
        <div class="col-lg-offset-3 col-lg-6">
        	<div id="aree" class="panel panel-default">
              <div class="panel-heading">
                <h3 class="panel-title">queries
                <?php if($loggeduser < 100) { ?><a  data-toggle="modal" data-target="#edit-custom-queries" class="query-item" data-id="0" href="#"><span class="addButton pull-right">Aggiungi <span class="glyphicon glyphicon-plus pull-right" aria-hidden="true"></span></span></a><?php } ?></h3>
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
                        <li>
                        	<?php if($loggeduser < 100) { ?><span class="glyphicon glyphicon-remove pull-right" data-id="<?php echo $data['query_id']; ?>" aria-hidden="true" onClick="deleteQuery(<?php echo $data['query_id']; ?>)"></span>  <a  data-toggle="modal" data-target="#edit-custom-queries" class="query-item" data-id="<?php echo $data['query_id']; ?>" href="#"><span class="glyphicon glyphicon-pencil pull-right" aria-hidden="true"></span><?php } ?></a>
                            <?php echo $data['name']; ?><a class="btn btn-primary play_query" href="?menu=custom-query-output&cq=<?php echo $data['query_id']; ?>" target="_blank" ><span class="glyphicon glyphicon-play"></span></a> </li>
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

<!-- Modal Queries -->
<div id="edit-custom-queries" class="modal fade" role="dialog">
  <div class="modal-dialog modal-lg">
    <!-- Modal content-->
    <div class="modal-content">
    </div>
  </div>
</div>