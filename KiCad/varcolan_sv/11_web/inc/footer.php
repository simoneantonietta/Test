<?php
//	require('utils/open_socket.php');
	
	function getSvFirmwareVersion() {
		$fp = fsockopen(SUPERVISOR, 6000, $errno, $errstr, 5);

		if (!$fp) {
			echo "$errstr ($errno)<br />\n";
		} else {
			$result = trim(fgets($fp, 256));	
		}
		
		echo $result;
	}
?>

	<div id="firmware_version" class="pull-right"><?php echo getSvFirmwareVersion(); ?></div>
	<div class="loaderScreen"></div>
    
    <script src="inc/js/bootstrap.min.js"></script>
    <script src="inc/js/bootstrap-datetimepicker.js"></script>
    <script src="inc/js/bootstrap-datetimepicker_assets_js_bootstrap-datetimepicker.it-IT.js"></script>
    <script src="inc/js/touch-dnd.js"></script>
    <script src="inc/js/jquery.input-ip-address-control-1.0.min.js"></script>
    <script src="inc/js/saet.js"></script>

	<script type="text/javascript">
		function deleteEventDate(obj) {
			"use strict";
			
			obj.setDate();
		}

		$('#datetimepicker1').datetimepicker({
		  language: 'it-IT',
		  pickSeconds: false
		});
		
		var eventfromdate = $('#fromdate').datetimepicker({
		  language: 'it-IT',
		  pickSeconds: false
		}).on('changeDate', function(ev) {
			if(ev.date.valueOf() != null) {
				$(this).find('span.glyphicon-remove').show();
			} else {
				$(this).find('span.glyphicon-remove').hide();
			}
		}).data('datetimepicker');

		var eventtodate = $('#todate').datetimepicker({
		  language: 'it-IT',
		  pickSeconds: false
		}).on('changeDate', function(ev) {
			if(ev.date.valueOf() != null) {
				$(this).find('span.glyphicon-remove').show();
			} else {
				$(this).find('span.glyphicon-remove').hide();
			}
      	}).data('datetimepicker');
	  		  
    </script>

  </body>
</html>
