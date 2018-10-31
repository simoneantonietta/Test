// JavaScript Document

  $(function() {
	$('.Timepickerinizio').datetimepicker({
	  language: 'it-IT',
	  pickDate: false,
	  pickSeconds: false
	}).on('changeDate', function(ev) {
		
	  	$(this).closest('.select-orari').find('.TimepickerFine input').val(ev.date);
	  // ev.date contiene la data scelta
	});
  });
  
  $(function() {
	$('.TimepickerFine').datetimepicker({
	  language: 'it-IT',
	  pickDate: false,
	  pickSeconds: false
	});
  });

