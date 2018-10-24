<?php
$orario = explode(":", $orari[$indexOrari]);
?>
<select class="form-control ore">
    <option <?php echo ($orario[0] == 24) ? 'selected' : ''; ?>>hh</option>
	<?php for($ind = 0; $ind < 24; $ind++) {
		$hour = sprintf("%02d", $ind);?>
    <option <?php echo ($orario[0] == $hour) ? 'selected' : ''; ?>><?php echo $hour; ?></option>
    <?php } ?>
</select>