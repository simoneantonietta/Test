<?php
$db = new SQLite3(DB_HOST);

$today = date('Y-m-d');
$now = date('Y-m-d H:i:s');
$results = $db->query('SELECT * FROM scheduled_jobs WHERE scheduled_time > datetime("'.$today.'") AND status > 0 ORDER BY scheduled_time DESC');
?>


<div class="container">
	<h1>aggiornamento firmware</h1>
    <div class="separator"></div>
</div>

<div id="aggiornamento-firmware" class="container">
	<div class="row">
        <div class="col-lg-6">
            <div id="scheduled_update" class="panel panel-default">
                <div class="panel-heading">
	                <h3 class="panel-title">Aggiornamenti programmati</h3>
                </div>
                <div class="panel-body">
                    <div class="table-striped">
                      <div class="table-header">
                        <ul>
                            <li><strong>Nome file</strong></li>
                            <li><strong>Data</strong></li>
                        </ul>
                      </div>
                      <div class="table-body">
                        <ul><?php
								while($item = $results->fetchArray()) {
							?>
                            <li>
                                <span class="glyphicon glyphicon-<?php if($item['scheduled_time'] <= $now) echo 'ok text-success'; else echo 'time text-primary'; ?> pull-right" aria-hidden="true"></span>
                            	<ul>
                                	<li><?php echo $item['file_name']; ?></li><li><?php echo $item['scheduled_time']; ?></li>
                                </ul>
							</li>
                         <?php } ?>
                        </ul>
                      </div>
                    </div>
                </div>
            </div>
            <div class="submit-buttons">
                <button type="button" data-toggle="modal" data-target="#scheduled-jobs-history" class="btn btn-default btn-primary">Archivio <span class="glyphicon glyphicon-folder-open" aria-hidden="true"></span></button>
                <button type="button" class="btn btn-default btn-danger pull-right" onClick="deleteAllJobs();">Elimina tutti i lavori programmati <span class="glyphicon glyphicon-trash" aria-hidden="true"></span></button>
            </div>
        </div>
        <div class="col-lg-6">
            <div class="well well-sm">
                <div class="upload-files">
                	<div class="upload-iniziale">
                        <p>Trascina qui i firmware da caricare oppure clicca qui sotto</p>
                        <button id="open_folder" type="button" class="btn btn-default btn-primary">Sfoglia</button>
                        <input name="upl" multiple type="file" style="display:none;">
                    </div>
                </div>
                <div id="file-upload-progress" class="progress">
                  <div class="progress-bar progress-bar-striped active" role="progressbar" aria-valuenow="0" aria-valuemin="0" aria-valuemax="100" style="width: 0%">
                  </div>
                </div>
                <div class="schedule labels"><strong>Aggiornamento programmato</strong></div><div id="scheduled" class="schedule  values">
            	<div class="btn-group open-options" role="yes-no" aria-label="scheduled" data-selected="0">
                  <button type="button" class="btn btn-default">Sì</button>
                  <button type="button" class="btn btn-default btn-danger">No</button>
                </div>
            </div>

			<div class="schedule options">
                <div class="labels"><strong>Inizio aggiornamento</strong></div><div class="values">
	                <div id="datetimepicker1" class="input-group input-append date">
						<input type="text" data-format="yyyy-MM-dd hh:mm" class="form-control add-on" aria-label="scheduled-date" value="">
                  		<span class="input-group-addon add-on"><span data-time-icon="icon-time" data-date-icon="icon-calendar" class="glyphicon glyphicon-calendar" aria-hidden="true"></span></span>
                      </div>
                  
                </div>
            </div>
            
            </div>
            <div class="submit-buttons">
                <button type="submit" class="btn btn-default btn-success pull-right" onClick="uploadFirmware();" disabled="disabled">Invia <span class="glyphicon glyphicon-ok" aria-hidden="true"></span></button>
	            <button type="button" class="btn btn-default btn-warning pull-right" onClick="location.reload();">Annulla <span class="glyphicon glyphicon-remove" aria-hidden="true"></span></button>
                <div class="clearfix"></div>
            </div>
        </div>    
    </div>
</div>

<!-- Modal Terminali -->
<div id="scheduled-jobs-history" class="modal fade" role="dialog">
  <div class="modal-dialog modal-lg">
    <!-- Modal content-->
    <div class="modal-content">
    </div>
  </div>
</div>