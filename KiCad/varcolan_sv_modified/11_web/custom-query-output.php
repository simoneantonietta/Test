<?php
$cq = isset($_GET['cq']) ? $_GET['cq'] : '';

if($cq == '') {
    echo 'Nessuna query';
    return;
}

$db = new SQLite3(DB_HOST);
$db->exec("ATTACH '/tmp/varcolan_events.db' as events;");

$results = $db->query('SELECT name, query FROM custom_queries WHERE query_id = '.$cq);
$theQuery = $results->fetchArray();

$gotQuery = $db->query($theQuery['query']);

$numColumn = 0;

define('NO_BADGE', 65535);
define('NO_AREA', 255);

?>

<div class="fixed-header-bg"></div>
<input hidden="hidden" name="rowid" value="0">
<input hidden="hidden" name="limit" value="1000">

<div id="cq-list" class="event-container">
	<div class="row">
    	<div class="col-lg-12">
            <div id="custom_query" class="panel panel-default">
              <div class="panel-heading">
                <h3 class="panel-title"><?php echo $theQuery['name']; ?></h3>
              </div>
              <div class="panel-body">
                <table class="eventstable table-striped">
                  <thead>
                    <tr>
                        <?php
                            $first_row = $gotQuery->fetchArray();
                            $keys =  array_keys($first_row);
                            foreach($keys as $key) {
                                if(!is_numeric($key)) {
                                    echo '<th>'.$key.'<div>'.$key.'</div></th>';
                                }
                            }
                            $gotQuery->reset();
                        ?>
                    </tr>
                  </thead>
                  <tbody>
                      <?php
                        while( $result = $gotQuery->fetchArray() ) {
                            echo '<tr>';
							foreach($keys as $key) {
								if(!is_numeric($key)) {
									echo '<td data-th="'.$key.'">'.$result[$key].'</td>';
								}
							}
                            echo '</tr>';
                        }
                      ?>
                  </tbody>
                </table>
              </div>
            </div>
        </div>
    </div>
</div>