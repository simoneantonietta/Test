<?php
include('global.php');

$db = new SQLite3(DB_HOST);

$now = date('Y-m-d H:i');
$results = $db->query('SELECT * FROM scheduled_jobs ORDER BY scheduled_time DESC');
?>

      <div class="modal-header">
        <button type="button" class="close" data-dismiss="modal">&times;</button>
        <h4 class="modal-title">Impostazioni Terminale</h4>
      </div>

      <div class="modal-body">

        <div class="table-striped jobs-history">
          <div class="table-header">
            <ul>
                <li><strong>Nome file</strong></li>
                <li><strong>Data</strong></li>
                <li><strong>Data cancellazione</strong></li>
            </ul>
          </div>
          <div class="table-body">
            <ul><?php
                    while($item = $results->fetchArray()) {
						if($item['status']) {
                ?>
                <li class="job-done">
                    <ul>
                        <li><?php echo $item['file_name']; ?></li><li><?php echo $item['scheduled_time']; ?></li><li></li>
                    </ul>
                </li>
				 <?php } else { ?>
                <li class="job-canceled">
                    <ul>
                        <li><?php echo $item['file_name']; ?></li><li><?php echo $item['scheduled_time']; ?></li><li><?php echo $item['canceled_time']; ?></li>
                    </ul>
                </li>
                <?php 	}
					}?>
            </ul>
          </div>
        </div>
                    
	</div>