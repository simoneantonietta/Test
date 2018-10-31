<select class="form-control minuti">
    <option <?php echo ($orario[1] == 60) ? 'selected' : ''; ?>>mm</option>
    <?php for($ind = 0; $ind < 60; $ind++) {
		$minute = sprintf("%02d", $ind);?>
    <option <?php echo ($orario[1] == $minute) ? 'selected' : ''; ?>><?php echo $minute; ?></option>
    <?php } ?>
</select>
<?php
$indexOrari++;
?>
