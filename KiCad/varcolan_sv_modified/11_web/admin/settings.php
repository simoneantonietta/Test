<?php
// gateway
//require('inc/utils/open_socket.php');

$netDataValues = sendSocket('get mynetdata');

$netdata = explode(' ', $netDataValues);
?>

<div class="container">
	<h1>Impostazioni di sistema</h1>
    <div class="separator"></div>
</div>

<div id="settings" class="container">
	<div class="row">

        <div class="col-lg-offset-3 col-lg-6">
            <div class="settings-field">
                <div class="labels"><strong>Invia ora esatta</strong></div><div class="values"><button id="rtcbutton" type="button" class="btn btn-default btn-primary pull-right" onClick="sendRtc()"><span class="timenow"></span>&nbsp;&nbsp;<span class="glyphicon glyphicon-time" aria-hidden="true"></span></button></div>
            </div>
            <div class="settings-field">
            	<h4>Cancellazione badge</h4>
                <div class="labels"><strong>Cancella badge dal Supervisore</strong></div><div class="values"><button id="deleteBadgesFromSupervisor" type="button" class="btn btn-default btn-danger pull-right" onClick="deleteBadgesFromSupervisor();">elimina <span class="glyphicon glyphicon-trash" aria-hidden="true"></span></button></div>
                <div class="labels"><strong>Cancella badge dai terminali</strong></div><div class="values"><button id="deleteBadgesFromTerminal" type="button" class="btn btn-default btn-warning pull-right" onClick="deleteBadgesFromTerminal();">elimina <span class="glyphicon glyphicon-trash" aria-hidden="true"></span></button></div>
            </div>
            <div class="settings-field">
            	<h4>Database Backup/Restore</h4>
                <div class="labels"><strong>Backup database</strong></div><div class="values"><form method="post" id="bkdb"><input type="text" name="backupDB" hidden="hidden"><button id="backup" type="button" class="btn btn-default btn-primary pull-right" onClick="document.forms['bkdb'].submit();">Scarica il database&nbsp;&nbsp;<span class="glyphicon glyphicon-download" aria-hidden="true"></span></button></form></div>
                <div class="labels"><strong>Restore database</strong></div><div class="values"><input style="display:none;" type="file" name="upldb"><button id="open_folder" type="button" class="btn btn-default btn-warning pull-right">Carica il database&nbsp;&nbsp;<span class="glyphicon glyphicon-upload" aria-hidden="true"></span></button><span class="new-db-name pull-right" style="display:none;">varcolan.db</span></div>
                <div id="file-upload-progress" class="progress">
                  <div class="progress-bar progress-bar-striped active" role="progressbar" aria-valuenow="0" aria-valuemin="0" aria-valuemax="100" style="width: 0%">
                  </div>
                </div>
            </div>
            <div class="settings-field">
            	<h4>Cambia IP supervisore</h4>
                <div class="labels"><strong>Indirizzo IP</strong></div><div class="values"><input id="ipaddr" name="ipAddress" class="form-control ipaddress pull-right" value="<?php echo $netdata[0]; ?>"></div>
                <div class="labels"><strong>Subnet Mask</strong></div><div class="values"><input id="subnet" name="subnet" class="form-control ipaddress pull-right" value="<?php echo $netdata[1]; ?>"></div>
                <div class="labels"><strong>Gateway</strong></div><div class="values"><input id="gateway" name="gatewayIP" class="form-control ipaddress pull-right" value="<?php echo $netdata[2]; ?>"></div>
                <!-- <div class="labels"><strong>DNS primario</strong></div><div class="values"><input id="DNS1" name="DNSpri" class="form-control ipaddress pull-right" value="<?php //echo $netdata[0]; ?>"></div>
                <div class="labels"><strong>DNS secondario</strong></div><div class="values"><input id="DNS2" name="DNSsec" class="form-control ipaddress pull-right" value="<?php //echo $netdata[0]; ?>"></div> -->
                <button id="changeIPSubmit" type="button" class="btn btn-default btn-primary pull-right" onClick="changeSupervisorIP()">Invia <span class="glyphicon glyphicon-ok" aria-hidden="true"></span> <span class="glyphicon glyphicon-refresh gly-spin"></span></button>
                <div class="clearfix"></div>
            </div>
            <div class="settings-field">
            	<h4>Ripristino ai parametri di fabbrica</h4>
                <div class="labels"><strong>Cancella tutte le impostazioni</strong></div><div class="values"><a  data-toggle="modal" data-target="#restore-default-warning" href="#"><button type="button" class="btn btn-default btn-danger pull-right" onClick="">Cancella tutto <span class="glyphicon glyphicon-erase" aria-hidden="true"></span></button></a></div>
                <div class="clearfix"></div>
            </div>
            <div class="settings-field">

                <div class="labels"><strong>Versione WebApp</strong></div><div class="values" style="padding-top:13px;"><?php echo $webversion; ?></div>
                <div class="clearfix"></div>
            </div>
        </div>

    </div>
</div>

<!-- Modal Warning -->
<div id="restore-default-warning" class="modal fade" data-backdrop="static" role="dialog">
  <div class="modal-dialog modal-lg">
    <!-- Modal content-->
    <div class="modal-content">
    </div>
  </div>
</div>


<script>	

	outputTime();
	
	function outputTime() {
		var t = new Date();

		var h=t.getHours();
		var m=t.getMinutes();
		var s=t.getSeconds();
		var d=t.getDate();
		var mo=t.getMonth()+1;
		var y=t.getFullYear();
	
		if(m<10)m="0"+m;
		if(s<10)s="0"+s;
		if(d<10)d="0"+d;
		if(mo<10)mo="0"+mo;
		var readableDate = d+"-"+mo+"-"+y+" "+h+":"+m+":"+s;
		
		$('span.timenow').text(readableDate);
	
		setTimeout(function() {
			outputTime();
		}, 1000);
	}
	
	$(function(){
		$('#ipaddr, #subnet, #gateway, #DNS1, #DNS2').ipAddress();
	});

</script>
