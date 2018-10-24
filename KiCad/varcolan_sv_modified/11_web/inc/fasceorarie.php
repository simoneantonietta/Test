<div class="labels"><strong>Luned&igrave;</strong></div><div class="values day">
	<div class="select-orari mattino">
    	<div class="alertOrario"></div>
    	<div class="oraInizio">
        	<div id="lun-mattino-start" class="input-group input-append date">
	            <input type="text" data-format="hh:mm" class="form-control add-on" aria-label="ora-inizio" value="<?php if($orari[$indexOrari] != '24:60') { echo $orari[$indexOrari]; } $indexOrari++; ?>">
            	<span class="glyphicon glyphicon-remove pull-right" aria-hidden="true" onClick="deleteDate(lun_mattino_start);"></span>
            </div>
        </div>
        <div class="oraFine">
        	<div id="lun-mattino-end" class="input-group input-append date">
                <input type="text" data-format="hh:mm" class="form-control add-on" aria-label="ora-fine" value="<?php if($orari[$indexOrari] != '24:60') { echo $orari[$indexOrari]; } $indexOrari++; ?>">
            	<span class="glyphicon glyphicon-remove pull-right" aria-hidden="true" onClick="deleteDate(lun_mattino_end);"></span>
            </div>
        </div>
    </div>
    <div class="select-orari pomeriggio">
    	<div class="alertOrario"></div>
    	<div class="oraInizio">
        	<div id="lun-pomeriggio-start" class="input-group input-append date">
                <input type="text" data-format="hh:mm" class="form-control add-on" aria-label="ora-inizio" value="<?php if($orari[$indexOrari] != '24:60') { echo $orari[$indexOrari]; } $indexOrari++; ?>">
            	<span class="glyphicon glyphicon-remove pull-right" aria-hidden="true" onClick="deleteDate(lun_pomeriggio_start);"></span>
        	</div>
        </div>
        <div class="oraFine">
        	<div id="lun-pomeriggio-end" class="input-group input-append date">
                <input type="text" data-format="hh:mm" class="form-control add-on" aria-label="ora-fine" value="<?php if($orari[$indexOrari] != '24:60') { echo $orari[$indexOrari]; } $indexOrari++; ?>">
            	<span class="glyphicon glyphicon-remove pull-right" aria-hidden="true" onClick="deleteDate(lun_pomeriggio_end);"></span>
            </div>
        </div>
    </div>
</div>

<div class="labels"><strong>Marted&igrave;</strong></div><div class="values day">
	<div class="select-orari mattino">
    	<div class="alertOrario"></div>
    	<div class="oraInizio">
        	<div id="mar-mattino-start" class="input-group input-append date">
                <input type="text" data-format="hh:mm" class="form-control add-on" aria-label="ora-inizio" value="<?php if($orari[$indexOrari] != '24:60') { echo $orari[$indexOrari]; } $indexOrari++; ?>">
            	<span class="glyphicon glyphicon-remove pull-right" aria-hidden="true" onClick="deleteDate(mar_mattino_start);"></span>
        	</div>
        </div>
        <div class="oraFine">
        	<div id="mar-mattino-end" class="input-group input-append date">
                <input type="text" data-format="hh:mm" class="form-control add-on" aria-label="ora-fine" value="<?php if($orari[$indexOrari] != '24:60') { echo $orari[$indexOrari]; } $indexOrari++; ?>">
            	<span class="glyphicon glyphicon-remove pull-right" aria-hidden="true" onClick="deleteDate(mar_mattino_end);"></span>
            </div>
        </div>
    </div>
    <div class="select-orari pomeriggio">
    	<div class="alertOrario"></div>
    	<div class="oraInizio">
        	<div id="mar-pomeriggio-start" class="input-group input-append date">
                <input type="text" data-format="hh:mm" class="form-control add-on" aria-label="ora-inizio" value="<?php if($orari[$indexOrari] != '24:60') { echo $orari[$indexOrari]; } $indexOrari++; ?>">
            	<span class="glyphicon glyphicon-remove pull-right" aria-hidden="true" onClick="deleteDate(mar_pomeriggio_start);"></span>
        	</div>
        </div>
        <div class="oraFine">
        	<div id="mar-pomeriggio-end" class="input-group input-append date">
                <input type="text" data-format="hh:mm" class="form-control add-on" aria-label="ora-fine" value="<?php if($orari[$indexOrari] != '24:60') { echo $orari[$indexOrari]; } $indexOrari++; ?>">
            	<span class="glyphicon glyphicon-remove pull-right" aria-hidden="true" onClick="deleteDate(mar_pomeriggio_end);"></span>
            </div>
        </div>
    </div>
</div>

<div class="labels"><strong>Mercoled&igrave;</strong></div><div class="values day">
	<div class="select-orari mattino">
    	<div class="alertOrario"></div>
    	<div class="oraInizio">
        	<div id="mer-mattino-start" class="input-group input-append date">
                <input type="text" data-format="hh:mm" class="form-control add-on" aria-label="ora-inizio" value="<?php if($orari[$indexOrari] != '24:60') { echo $orari[$indexOrari]; } $indexOrari++; ?>">
            	<span class="glyphicon glyphicon-remove pull-right" aria-hidden="true" onClick="deleteDate(mer_mattino_start);"></span>
        	</div>
        </div>
        <div class="oraFine">
        	<div id="mer-mattino-end" class="input-group input-append date">
                <input type="text" data-format="hh:mm" class="form-control add-on" aria-label="ora-fine" value="<?php if($orari[$indexOrari] != '24:60') { echo $orari[$indexOrari]; } $indexOrari++; ?>">
            	<span class="glyphicon glyphicon-remove pull-right" aria-hidden="true" onClick="deleteDate(mer_mattino_end);"></span>
            </div>
        </div>
    </div>
    <div class="select-orari pomeriggio">
    	<div class="alertOrario"></div>
    	<div class="oraInizio">
        	<div id="mer-pomeriggio-start" class="input-group input-append date">
                <input type="text" data-format="hh:mm" class="form-control add-on" aria-label="ora-inizio" value="<?php if($orari[$indexOrari] != '24:60') { echo $orari[$indexOrari]; } $indexOrari++; ?>">
            	<span class="glyphicon glyphicon-remove pull-right" aria-hidden="true" onClick="deleteDate(mer_pomeriggio_start);"></span>
        	</div>
        </div>
        <div class="oraFine">
        	<div id="mer-pomeriggio-end" class="input-group input-append date">
                <input type="text" data-format="hh:mm" class="form-control add-on" aria-label="ora-fine" value="<?php if($orari[$indexOrari] != '24:60') { echo $orari[$indexOrari]; } $indexOrari++; ?>">
            	<span class="glyphicon glyphicon-remove pull-right" aria-hidden="true" onClick="deleteDate(mer_pomeriggio_end);"></span>
            </div>
        </div>
    </div>
</div>

<div class="labels"><strong>Gioved&igrave;</strong></div><div class="values day">
	<div class="select-orari mattino">
    	<div class="alertOrario"></div>
    	<div class="oraInizio">
        	<div id="gio-mattino-start" class="input-group input-append date">
                <input type="text" data-format="hh:mm" class="form-control add-on" aria-label="ora-inizio" value="<?php if($orari[$indexOrari] != '24:60') { echo $orari[$indexOrari]; } $indexOrari++; ?>">
            	<span class="glyphicon glyphicon-remove pull-right" aria-hidden="true" onClick="deleteDate(gio_mattino_start);"></span>
        	</div>
        </div>
        <div class="oraFine">
        	<div id="gio-mattino-end" class="input-group input-append date">
                <input type="text" data-format="hh:mm" class="form-control add-on" aria-label="ora-fine" value="<?php if($orari[$indexOrari] != '24:60') { echo $orari[$indexOrari]; } $indexOrari++; ?>">
            	<span class="glyphicon glyphicon-remove pull-right" aria-hidden="true" onClick="deleteDate(gio_mattino_end);"></span>
            </div>
        </div>
    </div>
    <div class="select-orari pomeriggio">
    	<div class="alertOrario"></div>
    	<div class="oraInizio">
        	<div id="gio-pomeriggio-start" class="input-group input-append date">
                <input type="text" data-format="hh:mm" class="form-control add-on" aria-label="ora-inizio" value="<?php if($orari[$indexOrari] != '24:60') { echo $orari[$indexOrari]; } $indexOrari++; ?>">
            	<span class="glyphicon glyphicon-remove pull-right" aria-hidden="true" onClick="deleteDate(gio_pomeriggio_start);"></span>
        	</div>
        </div>
        <div class="oraFine">
        	<div id="gio-pomeriggio-end" class="input-group input-append date">
                <input type="text" data-format="hh:mm" class="form-control add-on" aria-label="ora-fine" value="<?php if($orari[$indexOrari] != '24:60') { echo $orari[$indexOrari]; } $indexOrari++; ?>">
            	<span class="glyphicon glyphicon-remove pull-right" aria-hidden="true" onClick="deleteDate(gio_pomeriggio_end);"></span>
            </div>
        </div>
    </div>
</div>

<div class="labels"><strong>Venerd&igrave;</strong></div><div class="values day">
	<div class="select-orari mattino">
    	<div class="alertOrario"></div>
    	<div class="oraInizio">
        	<div id="ven-mattino-start" class="input-group input-append date">
                <input type="text" data-format="hh:mm" class="form-control add-on" aria-label="ora-inizio" value="<?php if($orari[$indexOrari] != '24:60') { echo $orari[$indexOrari]; } $indexOrari++; ?>">
            	<span class="glyphicon glyphicon-remove pull-right" aria-hidden="true" onClick="deleteDate(ven_mattino_start);"></span>
        	</div>
        </div>
        <div class="oraFine">
        	<div id="ven-mattino-end" class="input-group input-append date">
                <input type="text" data-format="hh:mm" class="form-control add-on" aria-label="ora-fine" value="<?php if($orari[$indexOrari] != '24:60') { echo $orari[$indexOrari]; } $indexOrari++; ?>">
            	<span class="glyphicon glyphicon-remove pull-right" aria-hidden="true" onClick="deleteDate(ven_mattino_end);"></span>
            </div>
        </div>
    </div>
    <div class="select-orari pomeriggio">
    	<div class="alertOrario"></div>
    	<div class="oraInizio">
        	<div id="ven-pomeriggio-start" class="input-group input-append date">
                <input type="text" data-format="hh:mm" class="form-control add-on" aria-label="ora-inizio" value="<?php if($orari[$indexOrari] != '24:60') { echo $orari[$indexOrari]; } $indexOrari++; ?>">
            	<span class="glyphicon glyphicon-remove pull-right" aria-hidden="true" onClick="deleteDate(ven_pomeriggio_start);"></span>
        	</div>
        </div>
        <div class="oraFine">
        	<div id="ven-pomeriggio-end" class="input-group input-append date">
                <input type="text" data-format="hh:mm" class="form-control add-on" aria-label="ora-fine" value="<?php if($orari[$indexOrari] != '24:60') { echo $orari[$indexOrari]; } $indexOrari++; ?>">
            	<span class="glyphicon glyphicon-remove pull-right" aria-hidden="true" onClick="deleteDate(ven_pomeriggio_end);"></span>
            </div>
        </div>
    </div>
</div>

<div class="labels"><strong>Sabato</strong></div><div class="values day">
	<div class="select-orari mattino">
    	<div class="alertOrario"></div>
    	<div class="oraInizio">
        	<div id="sab-mattino-start" class="input-group input-append date">
                <input type="text" data-format="hh:mm" class="form-control add-on" aria-label="ora-inizio" value="<?php if($orari[$indexOrari] != '24:60') { echo $orari[$indexOrari]; } $indexOrari++; ?>">
            	<span class="glyphicon glyphicon-remove pull-right" aria-hidden="true" onClick="deleteDate(sab_mattino_start);"></span>
        	</div>
        </div>
        <div class="oraFine">
        	<div id="sab-mattino-end" class="input-group input-append date">
                <input type="text" data-format="hh:mm" class="form-control add-on" aria-label="ora-fine" value="<?php if($orari[$indexOrari] != '24:60') { echo $orari[$indexOrari]; } $indexOrari++; ?>">
            	<span class="glyphicon glyphicon-remove pull-right" aria-hidden="true" onClick="deleteDate(sab_mattino_end);"></span>
            </div>
        </div>
    </div>
    <div class="select-orari pomeriggio">
    	<div class="alertOrario"></div>
    	<div class="oraInizio">
        	<div id="sab-pomeriggio-start" class="input-group input-append date">
                <input type="text" data-format="hh:mm" class="form-control add-on" aria-label="ora-inizio" value="<?php if($orari[$indexOrari] != '24:60') { echo $orari[$indexOrari]; } $indexOrari++; ?>">
            	<span class="glyphicon glyphicon-remove pull-right" aria-hidden="true" onClick="deleteDate(sab_pomeriggio_start);"></span>
        	</div>
        </div>
        <div class="oraFine">
        	<div id="sab-pomeriggio-end" class="input-group input-append date">
                <input type="text" data-format="hh:mm" class="form-control add-on" aria-label="ora-fine" value="<?php if($orari[$indexOrari] != '24:60') { echo $orari[$indexOrari]; } $indexOrari++; ?>">
            	<span class="glyphicon glyphicon-remove pull-right" aria-hidden="true" onClick="deleteDate(sab_pomeriggio_end);"></span>
            </div>
        </div>
    </div>
</div>

<div class="labels"><strong>Domenica</strong></div><div class="values day">
	<div class="select-orari mattino">
    	<div class="alertOrario"></div>
    	<div class="oraInizio">
        	<div id="dom-mattino-start" class="input-group input-append date">
                <input type="text" data-format="hh:mm" class="form-control add-on" aria-label="ora-inizio" value="<?php if($orari[$indexOrari] != '24:60') { echo $orari[$indexOrari]; } $indexOrari++; ?>">
            	<span class="glyphicon glyphicon-remove pull-right" aria-hidden="true" onClick="deleteDate(dom_mattino_start);"></span>
        	</div>
        </div>
        <div class="oraFine">
        	<div id="dom-mattino-end" class="input-group input-append date">
                <input type="text" data-format="hh:mm" class="form-control add-on" aria-label="ora-fine" value="<?php if($orari[$indexOrari] != '24:60') { echo $orari[$indexOrari]; } $indexOrari++; ?>">
            	<span class="glyphicon glyphicon-remove pull-right" aria-hidden="true" onClick="deleteDate(dom_mattino_end);"></span>
            </div>
        </div>
    </div>
    <div class="select-orari pomeriggio">
    	<div class="alertOrario"></div>
    	<div class="oraInizio">
        	<div id="dom-pomeriggio-start" class="input-group input-append date">
                <input type="text" data-format="hh:mm" class="form-control add-on" aria-label="ora-inizio" value="<?php if($orari[$indexOrari] != '24:60') { echo $orari[$indexOrari]; } $indexOrari++; ?>">
            	<span class="glyphicon glyphicon-remove pull-right" aria-hidden="true" onClick="deleteDate(dom_pomeriggio_start);"></span>
        	</div>
        </div>
        <div class="oraFine">
        	<div id="dom-pomeriggio-end" class="input-group input-append date">
                <input type="text" data-format="hh:mm" class="form-control add-on" aria-label="ora-fine" value="<?php if($orari[$indexOrari] != '24:60') { echo $orari[$indexOrari]; } $indexOrari++; ?>">
            	<span class="glyphicon glyphicon-remove pull-right" aria-hidden="true" onClick="deleteDate(dom_pomeriggio_end);"></span>
            </div>
        </div>
    </div>
</div>


				
<script type="text/javascript">

	var lun_mattino_start = $('#lun-mattino-start').datetimepicker({
	  language: 'it-IT',
	  pickDate: false,
	  pickSeconds: false
	}).on('changeDate', function(ev) {
		if(ev.date.valueOf() != null) {
			$(this).find('span').show();
		} else {
			$(this).find('span').hide();
		}
	}).on('hide', function(ev) {
		if(ev.date.valueOf() != null) {
			$(this).find('span').show();
		}
		var objEnd = lun_mattino_end;
		if(objEnd.getDate().valueOf() < ev.date.valueOf()) {
			setEndDate(ev, objEnd);
		}
	}).data('datetimepicker');

	var lun_mattino_end = $('#lun-mattino-end').datetimepicker({
	  language: 'it-IT',
	  pickDate: false,
	  pickSeconds: false
	}).on('changeDate', function(ev) {
		if(ev.date.valueOf() != null) {
			$(this).find('span').show();
		} else {
			$(this).find('span').hide();
		}
		if(!checkStartDate(ev, lun_mattino_start)) {
			adjustTime(lun_mattino_end, lun_mattino_start);
			$(this).closest('.select-orari').find('.alertOrario').show().html(alertError('Orario non valido'));
		} else {
			$(this).closest('.select-orari').find('.alertOrario').hide();
		}
		lun_mattino_end.place();
	}).on('hide', function(ev) {
		if(ev.date.valueOf() != null) {
			$(this).find('span').show();
		}

		var objEnd = lun_pomeriggio_start;
		if(objEnd.getDate().valueOf() < ev.date.valueOf()) {
			setEndDate(ev, objEnd);
		}
	}).data('datetimepicker');

	var lun_pomeriggio_start = $('#lun-pomeriggio-start').datetimepicker({
	  language: 'it-IT',
	  pickDate: false,
	  pickSeconds: false
	}).on('changeDate', function(ev) {
		if(ev.date.valueOf() != null) {
			$(this).find('span').show();
		} else {
			$(this).find('span').hide();
		}
		if(!checkStartDate(ev, lun_mattino_end)) {
			adjustTime(lun_pomeriggio_start, lun_mattino_end);
			$(this).closest('.select-orari').find('.alertOrario').show().html(alertError('Questo orario non può sovrapporsi alla fascia mattutina'));
		} else {
			$(this).closest('.select-orari').find('.alertOrario').hide();
		}
		lun_pomeriggio_start.place()
	}).on('hide', function(ev) {
		if(ev.date.valueOf() != null) {
			$(this).find('span').show();
		}

		var objEnd = lun_pomeriggio_end;
		if(objEnd.getDate().valueOf() < ev.date.valueOf()) {
			setEndDate(ev, objEnd);
		}
	}).data('datetimepicker');

	var lun_pomeriggio_end = $('#lun-pomeriggio-end').datetimepicker({
	  language: 'it-IT',
	  pickDate: false,
	  pickSeconds: false
	}).on('changeDate', function(ev) {
		if(ev.date.valueOf() != null) {
			$(this).find('span').show();
		} else {
			$(this).find('span').hide();
		}
		if(!checkStartDate(ev, lun_pomeriggio_start)) {
			adjustTime(lun_pomeriggio_end, lun_pomeriggio_start);
			$(this).closest('.select-orari').find('.alertOrario').show().html(alertError('Orario non valido'));
		} else {
			$(this).closest('.select-orari').find('.alertOrario').hide();
		}
		lun_pomeriggio_end.place();
	}).data('datetimepicker');
	
	
	var mar_mattino_start = $('#mar-mattino-start').datetimepicker({
	  language: 'it-IT',
	  pickDate: false,
	  pickSeconds: false
	}).on('changeDate', function(ev) {
		if(ev.date.valueOf() != null) {
			$(this).find('span').show();
		} else {
			$(this).find('span').hide();
		}
	}).on('hide', function(ev) {
		if(ev.date.valueOf() != null) {
			$(this).find('span').show();
		}
		var objEnd = mar_mattino_end;
		if(objEnd.getDate().valueOf() < ev.date.valueOf()) {
			setEndDate(ev, objEnd);
		}
	}).data('datetimepicker');

	var mar_mattino_end = $('#mar-mattino-end').datetimepicker({
	  language: 'it-IT',
	  pickDate: false,
	  pickSeconds: false
	}).on('changeDate', function(ev) {
		if(ev.date.valueOf() != null) {
			$(this).find('span').show();
		} else {
			$(this).find('span').hide();
		}
		if(!checkStartDate(ev, mar_mattino_start)) {
			adjustTime(mar_mattino_end, mar_mattino_start);
			$(this).closest('.select-orari').find('.alertOrario').show().html(alertError('Orario non valido'));
		} else {
			$(this).closest('.select-orari').find('.alertOrario').hide();
		}
		mar_mattino_end.place();
	}).on('hide', function(ev) {
		if(ev.date.valueOf() != null) {
			$(this).find('span').show();
		}

		var objEnd = mar_pomeriggio_start;
		if(objEnd.getDate().valueOf() < ev.date.valueOf()) {
			setEndDate(ev, objEnd);
		}
	}).data('datetimepicker');

	var mar_pomeriggio_start = $('#mar-pomeriggio-start').datetimepicker({
	  language: 'it-IT',
	  pickDate: false,
	  pickSeconds: false
	}).on('changeDate', function(ev) {
		if(ev.date.valueOf() != null) {
			$(this).find('span').show();
		} else {
			$(this).find('span').hide();
		}
		if(!checkStartDate(ev, mar_mattino_end)) {
			adjustTime(mar_pomeriggio_start, mar_mattino_end);
			$(this).closest('.select-orari').find('.alertOrario').show().html(alertError('Questo orario non può sovrapporsi alla fascia mattutina'));
		} else {
			$(this).closest('.select-orari').find('.alertOrario').hide();
		}
		mar_pomeriggio_start.place()
	}).on('hide', function(ev) {
		if(ev.date.valueOf() != null) {
			$(this).find('span').show();
		}

		var objEnd = mar_pomeriggio_end;
		if(objEnd.getDate().valueOf() < ev.date.valueOf()) {
			setEndDate(ev, objEnd);
		}
	}).data('datetimepicker');

	var mar_pomeriggio_end = $('#mar-pomeriggio-end').datetimepicker({
	  language: 'it-IT',
	  pickDate: false,
	  pickSeconds: false
	}).on('changeDate', function(ev) {
		if(ev.date.valueOf() != null) {
			$(this).find('span').show();
		} else {
			$(this).find('span').hide();
		}
		if(!checkStartDate(ev, mar_pomeriggio_start)) {
			adjustTime(mar_pomeriggio_end, mar_pomeriggio_start);
			$(this).closest('.select-orari').find('.alertOrario').show().html(alertError('Orario non valido'));
		} else {
			$(this).closest('.select-orari').find('.alertOrario').hide();
		}
		mar_pomeriggio_end.place();
	}).data('datetimepicker');
	
	
	var mer_mattino_start = $('#mer-mattino-start').datetimepicker({
	  language: 'it-IT',
	  pickDate: false,
	  pickSeconds: false
	}).on('changeDate', function(ev) {
		if(ev.date.valueOf() != null) {
			$(this).find('span').show();
		} else {
			$(this).find('span').hide();
		}
	}).on('hide', function(ev) {
		if(ev.date.valueOf() != null) {
			$(this).find('span').show();
		}
		var objEnd = mer_mattino_end;
		if(objEnd.getDate().valueOf() < ev.date.valueOf()) {
			setEndDate(ev, objEnd);
		}
	}).data('datetimepicker');

	var mer_mattino_end = $('#mer-mattino-end').datetimepicker({
	  language: 'it-IT',
	  pickDate: false,
	  pickSeconds: false
	}).on('changeDate', function(ev) {
		if(ev.date.valueOf() != null) {
			$(this).find('span').show();
		} else {
			$(this).find('span').hide();
		}
		if(!checkStartDate(ev, mer_mattino_start)) {
			adjustTime(mer_mattino_end, mer_mattino_start);
			$(this).closest('.select-orari').find('.alertOrario').show().html(alertError('Orario non valido'));
		} else {
			$(this).closest('.select-orari').find('.alertOrario').hide();
		}
		mer_mattino_end.place();
	}).on('hide', function(ev) {
		if(ev.date.valueOf() != null) {
			$(this).find('span').show();
		}

		var objEnd = mer_pomeriggio_start;
		if(objEnd.getDate().valueOf() < ev.date.valueOf()) {
			setEndDate(ev, objEnd);
		}
	}).data('datetimepicker');

	var mer_pomeriggio_start = $('#mer-pomeriggio-start').datetimepicker({
	  language: 'it-IT',
	  pickDate: false,
	  pickSeconds: false
	}).on('changeDate', function(ev) {
		if(ev.date.valueOf() != null) {
			$(this).find('span').show();
		} else {
			$(this).find('span').hide();
		}
		if(!checkStartDate(ev, mer_mattino_end)) {
			adjustTime(mer_pomeriggio_start, mer_mattino_end);
			$(this).closest('.select-orari').find('.alertOrario').show().html(alertError('Questo orario non può sovrapporsi alla fascia mattutina'));
		} else {
			$(this).closest('.select-orari').find('.alertOrario').hide();
		}
		mer_pomeriggio_start.place()
	}).on('hide', function(ev) {
		if(ev.date.valueOf() != null) {
			$(this).find('span').show();
		}

		var objEnd = mer_pomeriggio_end;
		if(objEnd.getDate().valueOf() < ev.date.valueOf()) {
			setEndDate(ev, objEnd);
		}
	}).data('datetimepicker');

	var mer_pomeriggio_end = $('#mer-pomeriggio-end').datetimepicker({
	  language: 'it-IT',
	  pickDate: false,
	  pickSeconds: false
	}).on('changeDate', function(ev) {
		if(ev.date.valueOf() != null) {
			$(this).find('span').show();
		} else {
			$(this).find('span').hide();
		}
		if(!checkStartDate(ev, mer_pomeriggio_start)) {
			adjustTime(mer_pomeriggio_end, mer_pomeriggio_start);
			$(this).closest('.select-orari').find('.alertOrario').show().html(alertError('Orario non valido'));
		} else {
			$(this).closest('.select-orari').find('.alertOrario').hide();
		}
		mer_pomeriggio_end.place();
	}).data('datetimepicker');
	
	
	var gio_mattino_start = $('#gio-mattino-start').datetimepicker({
	  language: 'it-IT',
	  pickDate: false,
	  pickSeconds: false
	}).on('changeDate', function(ev) {
		if(ev.date.valueOf() != null) {
			$(this).find('span').show();
		} else {
			$(this).find('span').hide();
		}
	}).on('hide', function(ev) {
		if(ev.date.valueOf() != null) {
			$(this).find('span').show();
		}
		var objEnd = gio_mattino_end;
		if(objEnd.getDate().valueOf() < ev.date.valueOf()) {
			setEndDate(ev, objEnd);
		}
	}).data('datetimepicker');

	var gio_mattino_end = $('#gio-mattino-end').datetimepicker({
	  language: 'it-IT',
	  pickDate: false,
	  pickSeconds: false
	}).on('changeDate', function(ev) {
		if(ev.date.valueOf() != null) {
			$(this).find('span').show();
		} else {
			$(this).find('span').hide();
		}
		if(!checkStartDate(ev, gio_mattino_start)) {
			adjustTime(gio_mattino_end, gio_mattino_start);
			$(this).closest('.select-orari').find('.alertOrario').show().html(alertError('Orario non valido'));
		} else {
			$(this).closest('.select-orari').find('.alertOrario').hide();
		}
		gio_mattino_end.place();
	}).on('hide', function(ev) {
		if(ev.date.valueOf() != null) {
			$(this).find('span').show();
		}

		var objEnd = gio_pomeriggio_start;
		if(objEnd.getDate().valueOf() < ev.date.valueOf()) {
			setEndDate(ev, objEnd);
		}
	}).data('datetimepicker');

	var gio_pomeriggio_start = $('#gio-pomeriggio-start').datetimepicker({
	  language: 'it-IT',
	  pickDate: false,
	  pickSeconds: false
	}).on('changeDate', function(ev) {
		if(ev.date.valueOf() != null) {
			$(this).find('span').show();
		} else {
			$(this).find('span').hide();
		}
		if(!checkStartDate(ev, gio_mattino_end)) {
			adjustTime(gio_pomeriggio_start, gio_mattino_end);
			$(this).closest('.select-orari').find('.alertOrario').show().html(alertError('Questo orario non può sovrapporsi alla fascia mattutina'));
		} else {
			$(this).closest('.select-orari').find('.alertOrario').hide();
		}
		gio_pomeriggio_start.place()
	}).on('hide', function(ev) {
		if(ev.date.valueOf() != null) {
			$(this).find('span').show();
		}

		var objEnd = gio_pomeriggio_end;
		if(objEnd.getDate().valueOf() < ev.date.valueOf()) {
			setEndDate(ev, objEnd);
		}
	}).data('datetimepicker');

	var gio_pomeriggio_end = $('#gio-pomeriggio-end').datetimepicker({
	  language: 'it-IT',
	  pickDate: false,
	  pickSeconds: false
	}).on('changeDate', function(ev) {
		if(ev.date.valueOf() != null) {
			$(this).find('span').show();
		} else {
			$(this).find('span').hide();
		}
		if(!checkStartDate(ev, gio_pomeriggio_start)) {
			adjustTime(gio_pomeriggio_end, gio_pomeriggio_start);
			$(this).closest('.select-orari').find('.alertOrario').show().html(alertError('Orario non valido'));
		} else {
			$(this).closest('.select-orari').find('.alertOrario').hide();
		}
		gio_pomeriggio_end.place();
	}).data('datetimepicker');
	
	
	var ven_mattino_start = $('#ven-mattino-start').datetimepicker({
	  language: 'it-IT',
	  pickDate: false,
	  pickSeconds: false
	}).on('changeDate', function(ev) {
		if(ev.date.valueOf() != null) {
			$(this).find('span').show();
		} else {
			$(this).find('span').hide();
		}
	}).on('hide', function(ev) {
		if(ev.date.valueOf() != null) {
			$(this).find('span').show();
		}
		var objEnd = ven_mattino_end;
		if(objEnd.getDate().valueOf() < ev.date.valueOf()) {
			setEndDate(ev, objEnd);
		}
	}).data('datetimepicker');

	var ven_mattino_end = $('#ven-mattino-end').datetimepicker({
	  language: 'it-IT',
	  pickDate: false,
	  pickSeconds: false
	}).on('changeDate', function(ev) {
		if(ev.date.valueOf() != null) {
			$(this).find('span').show();
		} else {
			$(this).find('span').hide();
		}
		if(!checkStartDate(ev, ven_mattino_start)) {
			adjustTime(ven_mattino_end, ven_mattino_start);
			$(this).closest('.select-orari').find('.alertOrario').show().html(alertError('Orario non valido'));
		} else {
			$(this).closest('.select-orari').find('.alertOrario').hide();
		}
		ven_mattino_end.place();
	}).on('hide', function(ev) {
		if(ev.date.valueOf() != null) {
			$(this).find('span').show();
		}

		var objEnd = ven_pomeriggio_start;
		if(objEnd.getDate().valueOf() < ev.date.valueOf()) {
			setEndDate(ev, objEnd);
		}
	}).data('datetimepicker');

	var ven_pomeriggio_start = $('#ven-pomeriggio-start').datetimepicker({
	  language: 'it-IT',
	  pickDate: false,
	  pickSeconds: false
	}).on('changeDate', function(ev) {
		if(ev.date.valueOf() != null) {
			$(this).find('span').show();
		} else {
			$(this).find('span').hide();
		}
		if(!checkStartDate(ev, ven_mattino_end)) {
			adjustTime(ven_pomeriggio_start, ven_mattino_end);
			$(this).closest('.select-orari').find('.alertOrario').show().html(alertError('Questo orario non può sovrapporsi alla fascia mattutina'));
		} else {
			$(this).closest('.select-orari').find('.alertOrario').hide();
		}
		ven_pomeriggio_start.place()
	}).on('hide', function(ev) {
		if(ev.date.valueOf() != null) {
			$(this).find('span').show();
		}

		var objEnd = ven_pomeriggio_end;
		if(objEnd.getDate().valueOf() < ev.date.valueOf()) {
			setEndDate(ev, objEnd);
		}
	}).data('datetimepicker');

	var ven_pomeriggio_end = $('#ven-pomeriggio-end').datetimepicker({
	  language: 'it-IT',
	  pickDate: false,
	  pickSeconds: false
	}).on('changeDate', function(ev) {
		if(ev.date.valueOf() != null) {
			$(this).find('span').show();
		} else {
			$(this).find('span').hide();
		}
		if(!checkStartDate(ev, ven_pomeriggio_start)) {
			adjustTime(ven_pomeriggio_end, ven_pomeriggio_start);
			$(this).closest('.select-orari').find('.alertOrario').show().html(alertError('Orario non valido'));
		} else {
			$(this).closest('.select-orari').find('.alertOrario').hide();
		}
		ven_pomeriggio_end.place();
	}).data('datetimepicker');
	
	
	var sab_mattino_start = $('#sab-mattino-start').datetimepicker({
	  language: 'it-IT',
	  pickDate: false,
	  pickSeconds: false
	}).on('changeDate', function(ev) {
		if(ev.date.valueOf() != null) {
			$(this).find('span').show();
		} else {
			$(this).find('span').hide();
		}
	}).on('hide', function(ev) {
		if(ev.date.valueOf() != null) {
			$(this).find('span').show();
		}
		var objEnd = sab_mattino_end;
		if(objEnd.getDate().valueOf() < ev.date.valueOf()) {
			setEndDate(ev, objEnd);
		}
	}).data('datetimepicker');

	var sab_mattino_end = $('#sab-mattino-end').datetimepicker({
	  language: 'it-IT',
	  pickDate: false,
	  pickSeconds: false
	}).on('changeDate', function(ev) {
		if(ev.date.valueOf() != null) {
			$(this).find('span').show();
		} else {
			$(this).find('span').hide();
		}
		if(!checkStartDate(ev, sab_mattino_start)) {
			adjustTime(sab_mattino_end, sab_mattino_start);
			$(this).closest('.select-orari').find('.alertOrario').show().html(alertError('Orario non valido'));
		} else {
			$(this).closest('.select-orari').find('.alertOrario').hide();
		}
		sab_mattino_end.place();
	}).on('hide', function(ev) {
		if(ev.date.valueOf() != null) {
			$(this).find('span').show();
		}

		var objEnd = sab_pomeriggio_start;
		if(objEnd.getDate().valueOf() < ev.date.valueOf()) {
			setEndDate(ev, objEnd);
		}
	}).data('datetimepicker');

	var sab_pomeriggio_start = $('#sab-pomeriggio-start').datetimepicker({
	  language: 'it-IT',
	  pickDate: false,
	  pickSeconds: false
	}).on('changeDate', function(ev) {
		if(ev.date.valueOf() != null) {
			$(this).find('span').show();
		} else {
			$(this).find('span').hide();
		}
		if(!checkStartDate(ev, sab_mattino_end)) {
			adjustTime(sab_pomeriggio_start, sab_mattino_end);
			$(this).closest('.select-orari').find('.alertOrario').show().html(alertError('Questo orario non può sovrapporsi alla fascia mattutina'));
		} else {
			$(this).closest('.select-orari').find('.alertOrario').hide();
		}
		sab_pomeriggio_start.place()
	}).on('hide', function(ev) {
		if(ev.date.valueOf() != null) {
			$(this).find('span').show();
		}

		var objEnd = sab_pomeriggio_end;
		if(objEnd.getDate().valueOf() < ev.date.valueOf()) {
			setEndDate(ev, objEnd);
		}
	}).data('datetimepicker');

	var sab_pomeriggio_end = $('#sab-pomeriggio-end').datetimepicker({
	  language: 'it-IT',
	  pickDate: false,
	  pickSeconds: false
	}).on('changeDate', function(ev) {
		if(ev.date.valueOf() != null) {
			$(this).find('span').show();
		} else {
			$(this).find('span').hide();
		}
		if(!checkStartDate(ev, sab_pomeriggio_start)) {
			adjustTime(sab_pomeriggio_end, sab_pomeriggio_start);
			$(this).closest('.select-orari').find('.alertOrario').show().html(alertError('Orario non valido'));
		} else {
			$(this).closest('.select-orari').find('.alertOrario').hide();
		}
		sab_pomeriggio_end.place();
	}).data('datetimepicker');
	
	
	var dom_mattino_start = $('#dom-mattino-start').datetimepicker({
	  language: 'it-IT',
	  pickDate: false,
	  pickSeconds: false
	}).on('changeDate', function(ev) {
		if(ev.date.valueOf() != null) {
			$(this).find('span').show();
		} else {
			$(this).find('span').hide();
		}
	}).on('hide', function(ev) {
		if(ev.date.valueOf() != null) {
			$(this).find('span').show();
		}
		var objEnd = dom_mattino_end;
		if(objEnd.getDate().valueOf() < ev.date.valueOf()) {
			setEndDate(ev, objEnd);
		}
	}).data('datetimepicker');

	var dom_mattino_end = $('#dom-mattino-end').datetimepicker({
	  language: 'it-IT',
	  pickDate: false,
	  pickSeconds: false
	}).on('changeDate', function(ev) {
		if(ev.date.valueOf() != null) {
			$(this).find('span').show();
		} else {
			$(this).find('span').hide();
		}
		if(!checkStartDate(ev, dom_mattino_start)) {
			adjustTime(dom_mattino_end, dom_mattino_start);
			$(this).closest('.select-orari').find('.alertOrario').show().html(alertError('Orario non valido'));
		} else {
			$(this).closest('.select-orari').find('.alertOrario').hide();
		}
		dom_mattino_end.place();
	}).on('hide', function(ev) {
		if(ev.date.valueOf() != null) {
			$(this).find('span').show();
		}

		var objEnd = dom_pomeriggio_start;
		if(objEnd.getDate().valueOf() < ev.date.valueOf()) {
			setEndDate(ev, objEnd);
		}
	}).data('datetimepicker');

	var dom_pomeriggio_start = $('#dom-pomeriggio-start').datetimepicker({
	  language: 'it-IT',
	  pickDate: false,
	  pickSeconds: false
	}).on('changeDate', function(ev) {
		if(ev.date.valueOf() != null) {
			$(this).find('span').show();
		} else {
			$(this).find('span').hide();
		}
		if(!checkStartDate(ev, dom_mattino_end)) {
			adjustTime(dom_pomeriggio_start, dom_mattino_end);
			$(this).closest('.select-orari').find('.alertOrario').show().html(alertError('Questo orario non può sovrapporsi alla fascia mattutina'));
		} else {
			$(this).closest('.select-orari').find('.alertOrario').hide();
		}
		dom_pomeriggio_start.place()
	}).on('hide', function(ev) {
		if(ev.date.valueOf() != null) {
			$(this).find('span').show();
		}

		var objEnd = dom_pomeriggio_end;
		if(objEnd.getDate().valueOf() < ev.date.valueOf()) {
			setEndDate(ev, objEnd);
		}
	}).data('datetimepicker');

	var dom_pomeriggio_end = $('#dom-pomeriggio-end').datetimepicker({
	  language: 'it-IT',
	  pickDate: false,
	  pickSeconds: false
	}).on('changeDate', function(ev) {
		if(ev.date.valueOf() != null) {
			$(this).find('span').show();
		} else {
			$(this).find('span').hide();
		}
		if(!checkStartDate(ev, dom_pomeriggio_start)) {
			adjustTime(dom_pomeriggio_end, dom_pomeriggio_start);
			$(this).closest('.select-orari').find('.alertOrario').show().html(alertError('Orario non valido'));
		} else {
			$(this).closest('.select-orari').find('.alertOrario').hide();
		}
		dom_pomeriggio_end.place();
	}).data('datetimepicker');

	function setEndDate(ev, objEnd) {
		var newDate = new Date(ev.date);
		// dopo le 22 non andare avanti di un'ora sull'end date
		if((newDate.getHours() + (ev.date.getTimezoneOffset()/60)) < 0) {
			newDate.setHours(newDate.getHours());
		} else {
			newDate.setHours(newDate.getHours() + 1);
		}
		objEnd.setValue(newDate);
		objEnd.$element.find('span').show();
	}

	function checkStartDate(ev, objStart) {
		if(objStart.getDate().valueOf()) {
			if(ev.date.valueOf() <= objStart.getDate().valueOf()) {
				return false;
			}
		}
		return true;
	}
	
	function adjustTime(objEnd, objStart) {
		objEnd.hide();
/*		var newDate = new Date();
		newDate.setMinutes(newDate.getMinutes() + 1); */
		objEnd.setValue(objStart.getDate());
	}
	
	function deleteDate(obj) {
		obj.setDate();
		obj.$element.find('span').hide();
	}
	
</script>