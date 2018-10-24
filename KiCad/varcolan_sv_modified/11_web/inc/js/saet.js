// JavaScript Document
var files = [];
var NO_BADGE = 65535;
var NO_AREA = 255;
var usersByBadge = {};

$(document).ready(function() {
    
	"use strict";
	
	// Apri profilo
	$('#configurazione-sistema #profili .table-body > ul > li h3').on('click', function() {
        $(this).parent().find('.droppable-container .fasce-orarie-dropped, .droppable-container .terminali-dropped').slideToggle(0);
		$(this).parent().find('.droppable-container').slideToggle();
    });

	$('#configurazione-utenti #profili .table-body > ul > li h3').on('click', function() {
        $(this).parent().find('.droppable-container .badge-dropped').slideToggle(0);
		$(this).parent().find('.droppable-container').slideToggle();
    });

	$('#configurazione-utenti #utenti .table-body li.item-utente > ul > li').on('click', function() {
        $(this).closest('.item-utente').find('.droppable-container .badge-dropped').slideToggle(0);
		$(this).closest('.item-utente').find('.droppable-container').slideToggle();
    });
	
	// Drag&Drop Configurazione sistema
	$('#configurazione-sistema #terminali li.draggable').draggable({ clone: true, cloneClass: 'dragging' });
/*								.on('draggable:start', function() {
									$('#profili .droppable-container').slideDown(0);
									$('#profili .droppable-container .fasce-orarie-dropped').slideUp();
									$('#profili .droppable-container .terminali-dropped').slideDown();
								});
								.on('draggable:stop', function() {
									$('#profili .droppable-container').slideUp();
									$('#profili .droppable-container .terminali-dropped').slideUp();
								}); */

	$('#configurazione-sistema #fasce-orarie li.wt_draggable').draggable({ clone: true, cloneClass: 'dragging' })
								.on('draggable:start', function() {
									$('#profili .droppable-container').slideDown(0);
//									$('#profili .droppable-container .terminali-dropped').slideUp();
									$('#profili .droppable-container .fasce-orarie-dropped').slideDown();
								})
								.on('draggable:stop', function() {
									$('#profili .droppable-container').slideUp();
									$('#profili .droppable-container .fasce-orarie-dropped').slideUp();
								});
								
	$('#configurazione-utenti #badge li.draggable').draggable({ clone: true, cloneClass: 'dragging' });
	
	$('#configurazione-sistema .terminali-droppable').droppable({ accept: '#terminali li.draggable', activeClass: 'prepare-to-drop', hoverClass: 'dropping' }).on('droppable:drop', function(e, ui) {
                    if(ui.item[0].classList[0] == 'draggable') {
                        addTerminal(ui.item, this);
                    }
				});
				
	$('#configurazione-sistema .fasce-orarie-dropped > ul').droppable({ accept: '#fasce-orarie li.wt_draggable', activeClass: 'prepare-to-drop', hoverClass: 'dropping' }).on('droppable:drop', function(e, ui) {
					addWeektime(ui.item, this);
				});
	
	// CONFIGURAZIONE BADGE
	// Drag&Drop Configurazione sistema
	$('#configurazione-utenti #profili .badge-droppable').droppable({ accept: '#badge li.draggable', activeClass: 'prepare-to-drop', hoverClass: 'dropping' }).on('droppable:drop', function(e, ui) {
					addBadgeToProfile(ui.item, this);
				});

	$('#configurazione-utenti #utenti .badge-droppable').droppable({ accept: '#badge li.draggable', activeClass: 'prepare-to-drop', hoverClass: 'dropping' }).on('droppable:drop', function(e, ui) {
					addBadgeToUser(ui.item, this);
				});
	
	$(document).on('click', 'a[data-toggle="modal"]', function(){
		openModal($(this).data('target'), this);
	});

	// JOBS HISTORY
	$('button[data-target="#scheduled-jobs-history"]').on('click', function(){
		openModal('scheduled-jobs-history', this);
	});
	
	// MODALI - DINAMICALLY AJAX LOADED
	
	$(document).on('click', '.btn-group[role="yes-no"] button', function() {
		$(this).siblings().removeClass('btn-danger').removeClass('btn-primary');
		if ( $(this).is(':first-child') ) {
			$(this).addClass('btn-primary');
			$(this).parent().attr('data-selected', '1');
		}
		else {
			$(this).addClass('btn-danger');
			$(this).parent().attr('data-selected', '0');
		}
	});
	
	// BUTTON GROUP ROLE=RADIO
	$(document).on('click', '.btn-group[role="radio"] button', function() {
		$(this).siblings().removeClass('btn-success').removeClass('btn-warning').removeClass('btn-danger');
		$(this).parent().attr('data-selected', $(this).attr('data-selected'));
		if($(this).attr('data-class') != null) {
			$(this).addClass($(this).attr('data-class'));
		}
		else {
			$(this).addClass('btn-success');
		}
	});
	
	$(document).on('click', '.open-options button:first-child', function() {
		$(this).closest('.values').next('.options').slideDown();
	});
	$(document).on('click', '.open-options button:last-child', function() {
		$(this).closest('.values').next('.options').slideUp();
	});
	
	$(document).on('focus', '#datepicker', function() {
	   $('#datepicker').datepicker();
	});
	
	// NEW BADGE
	$('#new-badge-button').on('click', function() {
		$.post('inc/utils/exec_command.php', {
			command: 'newBadge'
		}, function(data) {
			if(data.badge_num === "") {
				location.reload();
			} else {
				$('#edit-badge .modal-content').empty();
				$.get('inc/edit-badge.php', {
					param_id: data.id,
					badge_num: data.badge_num
				}).done( function(data) {
					$('#edit-badge .modal-content').html(data);
				});
			}
		}, "json");    
	});
	
	// DRAG AND DROP FIRMWARE
	var doc = document.documentElement;
	doc.ondragover = function () { $('.upload-files').addClass('hover'); return false; };
	doc.ondragend = function () { $('.upload-files').removeClass('hover'); return false; };
	doc.ondrop = function (event) {
		event.preventDefault();
		
		// now do something with:
		transferFiles(event.dataTransfer.files);
	};
	
	$(document).on('shown.bs.modal', '#edit-profilo', function(event) {
		if( $('#terminali-da-associare .terminali-dropped > ul').height() > $('#terminali-associati .terminali-dropped > ul').height() ) {
			$('#terminali-associati .terminali-dropped > ul').height($('#terminali-da-associare .terminali-dropped > ul').height());
		} else {
			$('#terminali-da-associare .terminali-dropped > ul').height($('#terminali-associati .terminali-dropped > ul').height());
		}
    }); 
	
	$(document).on('change', 'input[aria-describedby="username"]', function(e) {
		$('.username .errorMessage').remove();
		if( $('input[aria-describedby="username"]').val().length < 3 ) {
			$('#edit-adminusers .submit-buttons button.btn-success').addClass('disabled');
			$('#edit-adminusers .submit-buttons button.btn-success').attr('disabled', 'disabled');
			$('.username').removeClass('has-success');
			$('.username').addClass('has-error');
			$('input[aria-describedby="username"]').after(alertError('Lo username deve essere composto da almeno 3 caratteri'));
		} else {
			checkUserName( $('input[aria-describedby="username"]').val() );
		}
	});

	$(document).on('keypress', 'input[aria-describedby="codice-badge"]', function(e) {
		$('.causal_code .errorMessage').remove();
		if (e.which != 8 && e.which != 0 && (e.which < 48 || e.which > 57)) {
			$('input[aria-describedby="codice-badge"]').after(alertError('Si accetta solo un codice numerico compreso tra 1 e 65000'));
			return false;
		}
	});

	$(document).on('change', 'input[aria-describedby="codice-badge"]', function(e) {
		$('.causal_code .errorMessage').remove();
		checkCausalCode( $('input[aria-describedby="codice-badge"]').val() );
	});
	
});

function openModal(element_name, elem) {

	"use strict";
	var elem_id = $(elem).attr('data-id');
	
	var elem_name = element_name.replace("#", "");
	
	$.ajax({
	  type : 'get',
	   url : 'inc/'+elem_name+'.php',
	  data :  'param_id='+elem_id,

	success : function(r)
	   {
		  // now you can show output in your modal 
		 $('#'+elem_name+' .modal-content').show().html(r);
	   }
	});
}

function alertError(msg) {
	"use strict";
	
	return '<div class="errorMessage alert alert-danger alert-dismissable" role="alert" >'+msg+'</div>';
}

function configBidirezionale() {
	"use strict";
	
	$('#edit-terminale .tasca').fadeIn();
	$('#edit-terminale .tasca').css('display', 'inline-block');
	$('#edit-terminale .bidirezionale').fadeOut();
}

function configTasca() {
	"use strict";
	
	$('#edit-terminale .tasca').fadeOut();
}

function configMonodirezionale() {
	"use strict";
	
	$('#edit-terminale .bidirezionale, #edit-terminale .tasca').fadeIn();
	$('#edit-terminale .tasca, #edit-terminale .bidirezionale').css('display', 'inline-block');
}

$('#saveWt').submit( function(e) {
	"use strict";
	
	e.preventDefault();
});

$('#open_folder').click( function(e) {
	"use strict";
	
	$('input[type="file"]').click();
});

$('input[name="upl"]').change( function(event) {
	"use strict";
	
	transferFiles($(this).get(0).files);
});

$('input[name="upldb"]').change( function(event) {
	"use strict";
	
	uploadDB($(this).get(0).files);
});

// WEEKTIME MANAGEMENT

function saveWt(id) {
	"use strict";
	
	$('.loaderScreen').fadeIn();
	
	var weektime = "";
	$('#edit-fascia-oraria .select-orari').each(function() {
		var orainizio = $(this).find('.oraInizio input').val();
		if(orainizio == '') {
			orainizio = '24:60';
		}
		var orafine = $(this).find('.oraFine input').val();
		if(orafine == '') {
			orafine = '24:60';
		}
	    weektime += orainizio+"|"+orafine+"|";
    });
	
	var name = $('#edit-fascia-oraria input[aria-describedby="descrizione-weektime"]').val();
	
	$.post('inc/utils/exec_command.php', {
		command: 'saveWt',
		name: name,
		weektime: weektime,
		id: id
	}).done( function(data) {
		processData(data);
	}).fail( function(error) {
        console.log(error);
    });
}

function deleteWt(id) {
	"use strict";
	
	var r = confirm("Sei sicuro di voler eliminare la fascia oraria?");
	
	if (r == true) {
		$('.loaderScreen').fadeIn();
	
		$.post('inc/utils/exec_command.php', {
			command: 'deleteWt',
			id: id
		}).done( function(data) {
			processData(data);
		}).fail( function(error) {
            console.log(error);
        });
	}
}

// Addweektime to profile
function addWeektime(item, objPt) {
	"use strict";
	
	$('.loaderScreen').fadeIn();
    	
	var idItem = $(item).attr('data-id');
	var idProfile = $(objPt).attr('data-id');
	
	$.post('inc/utils/exec_command.php', {
		command: 'addWeektime',
		idItem: idItem,
		idProfile: idProfile
	}).done( function(data) {
        processData(data);
    });
}

function deleteWeektime(id, objPt) {
	"use strict";
	
	$('.loaderScreen').fadeIn();
	
	var idProfile = $(objPt).closest('ul').attr('data-id');
	
	$.post('inc/utils/exec_command.php', {
		command: 'deleteWeektime',
		idProfile: idProfile
	}).done( function(data) {
		processData(data);
	}).fail( function(error) {
        console.log(error);
    });
}

// TERMINALS IN PROFILE
 function addTerminal(item, objPt) {
	"use strict";
	
	$('.loaderScreen').fadeIn();
	
	var idItem = $(item).attr('data-id');
	var idProfile = $(objPt).attr('data-id');
	
	$.post('inc/utils/exec_command.php', {
		command: 'addTerminal',
		idItem: idItem,
		idProfile: idProfile
	}).done( function(data) {
		processData(data);
	});
}

function deleteTerminal(id, objPt) {
	"use strict";
	
	$('.loaderScreen').fadeIn();
	
	var idProfile = $(objPt).closest('ul').attr('data-id');
	
	$.post('inc/utils/exec_command.php', {
		command: 'deleteTerminal',
		idItem: id,
		idProfile: idProfile		
	}).done( function(data) {
		processData(data);
	});
}

function saveProfile(id) {
	"use strict";
	
	$('.loaderScreen').fadeIn();
	
	var name = $('#edit-profilo input[aria-describedby="descrizione-profilo"]').val();
	var coercion = $('#edit-profilo .btn-group[aria-label="coercion"]').attr('data-selected');
	var weektime_id = $('#edit-profilo #weektime option:selected').val();
	
	var active_terminals = "IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII";
	$('#edit-profilo #terminali-associati .terminali-dropped ul > li').each(function(index, element) {
		active_terminals = addTerminalToProfile(($(this).attr('data-id') - 1), active_terminals);
	});

	$.post('inc/utils/exec_command.php', {
		command: 'saveProfile',
		name: name,
		coercion: coercion,
		weektime_id: weektime_id,
		active_terminals: active_terminals,
		status: 1,
		id: id
	}).done( function(data) {
        processData(data);
	});
}

function deleteProfile(id) {
	"use strict";
	
	var r = confirm("Sei sicuro di voler eliminare il profilo?");
	
	if (r == true) {
		$('.loaderScreen').fadeIn();
		
		var name = 'Profile'+id;
		var coercion = 0;
		var weektime_id = 65;
		
		var active_terminals = "IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII";
	
		$.post('inc/utils/exec_command.php', {
			command: 'saveProfile',
			name: name,
			coercion: coercion,
			weektime_id: weektime_id,
			active_terminals: active_terminals,
			status: 0,
			id: id
		}).done( function(data) {
			processData(data);
		});
	}
}

function setTerminalInProfile(id, terminals, value) {
	"use strict";
	
	var string = '';
	
	for(var i = 0; i < 64; i++) {
		if((63 - i) == id) {
			string += value;
		}
		else {
			string += terminals.charAt(i);
		}
	}
	
	return string;
}

function addTerminalToProfile(id, terminals) {
	"use strict";
	
	return setTerminalInProfile(id, terminals, 'A');
}


// TERMINAL MANAGEMENT

function saveTerminal(id) {
	"use strict";
	
	$('.loaderScreen').fadeIn();
	
	var name = $('#edit-terminale input[aria-describedby="descrizione-terminale"]').val();
	var open_door_time = $('#edit-terminale input[aria-describedby="open_door_time"]').val();
	if(open_door_time < 0) {
		$('#edit-terminale input[aria-describedby="open_door_time"]').val(0);
		open_door_time = 0;
	}
	if(open_door_time > 999) {
		$('#edit-terminale input[aria-describedby="open_door_time"]').val(999);
		open_door_time = 999;
	}
	
	var open_door_timeout = $('#edit-terminale input[aria-describedby="open_door_timeout"]').val();
	if(open_door_timeout < 0) {
		$('#edit-terminale input[aria-describedby="open_door_timeout"]').val(0);
		open_door_timeout = 0;
	}
	if(open_door_timeout > 999) {
		$('#edit-terminale input[aria-describedby="open_door_timeout"]').val(999);
		open_door_timeout = 999;
	}
	var antipassback = $('#edit-terminale .btn-group[aria-label="antipassback"]').attr('data-selected');
	var weektime_id = $('#edit-terminale #weektime option:selected').val();

	var entrance_type = $('#edit-terminale .btn-group[aria-label="entrance_type"]').attr('data-selected');
	var access1 = $('#edit-terminale .btn-group[aria-label="access1"]').attr('data-selected');
	var access2 = $('#edit-terminale .btn-group[aria-label="access2"]').attr('data-selected');
	var area1_reader1 = $('#edit-terminale #area1_reader1 option:selected').val();
	var area2_reader1 = $('#edit-terminale #area2_reader1 option:selected').val();
	var area1_reader2 = $('#edit-terminale #area1_reader2 option:selected').val();
	var area2_reader2 = $('#edit-terminale #area2_reader2 option:selected').val();

	$.post('inc/utils/exec_command.php', {
		command: 'saveTerminal',
		id: id,
		name: name,
		open_door_time: open_door_time,
		open_door_timeout: open_door_timeout,
		weektime_id: weektime_id,
		antipassback: antipassback,
		access1: access1,
		access2: access2,
		entrance_type:entrance_type,
		area1_reader1: area1_reader1,
		area2_reader1: area2_reader1,
		area1_reader2: area1_reader2,
		area2_reader2: area2_reader2,
		filtro_out: 1
	}).done( function(data) {
		processData(data);
	});
}

function removeTerminal(id) {
	"use strict";
	
	var r = confirm("Sei sicuro di voler eliminare il terminale?");
	
	if (r == true) {
		$('.loaderScreen').fadeIn();
	
		$.post('inc/utils/exec_command.php', {
			command: 'removeTerminal',
			id: id
		}).done( function(data) {
			processData(data);
		});
	}
}

// AREA MANAGEMENT

function saveArea(id, newInsert) {
	"use strict";
	
	$('.loaderScreen').fadeIn();
	
	var area_id = id;
	var name = $('#edit-area input[aria-describedby="descrizione-area"]').val();

	$.post('inc/utils/exec_command.php', {
		command: 'saveArea',
		newInsert: newInsert,
		area_id: area_id,
		name: name
	}).done( function(data) {
		processData(data);
	});
}

function deleteArea(id) {
	"use strict";
	
	var r = confirm("Sei sicuro di voler eliminare l'area?");
	
	if (r == true) {
		$('.loaderScreen').fadeIn();
		
		$.post('inc/utils/exec_command.php', {
			command: 'deleteArea',
			area_id: id
		}).done( function(data) {
			processData(data);
		});
	}
}

// BADGE MANAGEMENT

function saveBadge(id, newInsert) {
	"use strict";
	
	$('.errorMessage').remove();
	
	var errors = false;
	
	var badge_num = $('span.badge_num').text();
	var status = $('div[aria-label="Status"]').attr('data-selected');
	
	var user_type = $('#edit-badge .btn-group[aria-label="user_type"]').attr('data-selected');
	
	var current_area = $('#edit-badge #area option:selected').val();
	
	var printed_code = $('#edit-badge input[aria-describedby="codice-stampato"]').val();

	var pin = $('#edit-badge input[aria-describedby="password-badge"]').val();
	var regex=/^[0-9]+$/;
    if(!pin.match(regex))
    {
		errors = true;
		$('input[aria-describedby="password-badge"]').after(alertError('Il pin è composto solo da numeri'));
    }
	
	if(pin.length > 5)
    {
		errors = true;
		$('input[aria-describedby="password-badge"]').after(alertError('Il pin è composto al massimo da 5 cifre'));
    }
	
	var confirmpin = $('#edit-badge input[aria-describedby="password-confirm-badge"]').val();
	if(confirmpin != pin) {
		errors = true;
		$('input[aria-describedby="password-confirm-badge"]').after(alertError('I valori non coincidono'));
	}
	
//	var profili = $('#edit-badge .chosen-drop ul.chosen-results li.result-selected').toArray();
	var profili = [];
	
	$('#edit-badge #badge_profili_chosen ul.chosen-choices li.search-choice a').each(function(index, element) {
        profili.push($(this).attr('data-option-array-index'));
    });
	
	var visitor = $('#edit-badge .btn-group[aria-label="visitor"]').attr('data-selected');
	var validity_stop = $('#edit-badge input[aria-label="validity-stop"]').val();
	var contact = $('#edit-badge input[aria-describedby="referente-badge"]').val();

	if(!errors) {
		$('.loaderScreen').fadeIn();
		
		$.post('inc/utils/exec_command.php', {
			command: 'saveBadge',
			newInsert: newInsert,
			id: id,
			badge_num: badge_num,
			status: status,
			user_type: user_type,
			current_area: current_area,
			printed_code: printed_code,
			pin: pin,
			profili: profili,
			visitor: visitor,
			validity_stop: validity_stop,
			contact: contact
		}).done( function(data) {
			processData(data);
		});
	}
}

function deleteBadge(id) {
	"use strict";
	
	$('.loaderScreen').fadeIn();
	
	$.post('inc/utils/exec_command.php', {
		command: 'deleteBadge',
		id: id
	}).done( function(data) {
		processData(data);
	});
}

function addBadgeToProfile(item, objPt) {
	"use strict";
	
	$('.loaderScreen').fadeIn();
	
	var idItem = $(item).attr('data-id');
	var idProfile = $(objPt).attr('data-id');
	
	$.post('inc/utils/exec_command.php', {
		command: 'addBadgeToProfile',
		idItem: idItem,
		idProfile: idProfile
	}).done( function(data) {
		processData(data);
	});
}

function removeBadgeFromProfile(id_badge, id_profile) {
	"use strict";
	
	$('.loaderScreen').fadeIn();
	
	$.post('inc/utils/exec_command.php', {
		command: 'removeBadgeFromProfile',
		idItem: id_badge,
		idProfile: id_profile
	}).done( function(data) {
		processData(data);
	});
}

function removeBadgeFromProfile(id_badge, id_profile) {
	"use strict";
	
	$('.loaderScreen').fadeIn();
	
	$.post('inc/utils/exec_command.php', {
		command: 'removeBadgeFromProfile',
		idItem: id_badge,
		idProfile: id_profile
	}).done( function(data) {
		processData(data);
	});
}

// CAUSAL BADGE CODE
function deleteCausalCode(id) {
	"use strict";
	
	var r = confirm("Sei sicuro di voler eliminare il codice causale badge?");
	
	if (r == true) {
		$('.loaderScreen').fadeIn();
		
		$.post('inc/utils/exec_command.php', {
			command: 'deleteCausalCode',
			n: id
		}).done( function(data) {
			processData(data);
		});
	}
}

function saveCausalCode(id) {
	"use strict";
	
	var errors = false;
	
	var description = $('input[aria-describedby="descrizione-causal-code"]').val();
	if(description === '') {
		errors = true;
		$('input[aria-describedby="descrizione-causal-code"]').after(alertError('Campo obbligatorio'));
	}

	var causal_id = $('input[aria-describedby="codice-badge"]').val();
	if(causal_id === '') {
		errors = true;
		$('input[aria-describedby="codice-badge"]').after(alertError('Campo obbligatorio'));
	}
	
	if(!errors) {
		$('.loaderScreen').fadeIn();
		
		$.post('inc/utils/exec_command.php', {
			command: 'saveCausalCode',
			n: id,
			causal_id: causal_id,
			description: description
		}).done( function(data) {
			processData(data);
		});
	}
}

function checkCausalCode(code) {
	"use strict";
	
	if( ($('input[aria-describedby="codice-badge"]').val() < 1) || ($('input[aria-describedby="codice-badge"]').val() > 65000) ) {
		$('.causal_code').removeClass('has-success');
		$('.causal_code').addClass('has-error');
		$('#edit-badge-codes .submit-buttons button.btn-success').addClass('disabled');
		$('#edit-badge-codes .submit-buttons button.btn-success').attr('disabled', 'disabled');
		$('input[aria-describedby="codice-badge"]').after(alertError('Si accetta solo un codice numerico compreso tra 1 e 65000'));
		return;
	}

	$.post('inc/utils/exec_command.php', {
		command: 'checkCausalCode',
		causal_id: code
	}).done( function(data) {
		if(data !== '0') {
			$('.causal_code').removeClass('has-success');
			$('.causal_code').addClass('has-error');
			$('#edit-badge-codes .submit-buttons button.btn-success').addClass('disabled');
			$('#edit-badge-codes .submit-buttons button.btn-success').attr('disabled', 'disabled');
			$('input[aria-describedby="codice-badge"]').after(alertError('Il codice esiste gi&agrave;'));
		} else {
			$('.causal_code').removeClass('has-error');
			$('.causal_code').addClass('has-success');
			$('#edit-badge-codes .submit-buttons button.btn-success').removeClass('disabled');
			$('#edit-badge-codes .submit-buttons button.btn-success').removeAttr('disabled');
		}
	});
}

// BADGE TO USERS
function saveUser(id, newInsert) {
	"use strict";
	
	var errors = false;
	
	var name = $('input[aria-describedby="name"]').val();
	if(name === '') {
		errors = true;
		$('input[aria-describedby="name"]').after(alertError('Campo obbligatorio'));
	}
	
	var lastname = $('input[aria-describedby="lastname"]').val();
	if(lastname === '') {
		errors = true;
		$('input[aria-describedby="lastname"]').after(alertError('Campo obbligatorio'));
	}
	
	var matricola = $('input[aria-describedby="matricola"]').val();
	
	var badges = [];
	
	$('#edit-utenti #badge_utenti_chosen ul.chosen-choices li.search-choice a').each(function(index, element) {
        badges.push($('#edit-utenti #badge-utenti option').eq($(this).attr('data-option-array-index')).val());
    });
	
	if(!errors) {
		$('.loaderScreen').fadeIn();
	
		$.post('inc/utils/exec_command.php', {
			command: 'saveUser',
			newInsert: newInsert,
			id: id,
			name: name,
			lastname: lastname,
			matricola: matricola,
			badges: badges
		}).done( function(data) {
			processData(data);
		});
	}
}

function deleteUser(id) {
	"use strict";
	
	var r = confirm("Sei sicuro di voler eliminare l'utente?");
	
	if (r == true) {
		$('.loaderScreen').fadeIn();
		
		$.post('inc/utils/exec_command.php', {
			command: 'deleteUser',
			id: id
		}).done( function(data) {
			processData(data);
		});
	}
}

function addBadgeToUser(item, objPt) {
	"use strict";
	
	$('.loaderScreen').fadeIn();
	
	var idItem = $(item).attr('data-id');
	var idUser = $(objPt).attr('data-id');
	
	$.post('inc/utils/exec_command.php', {
		command: 'addBadgeToUser',
		idItem: idItem,
		idUser: idUser
	}).done( function(data) {
		processData(data);
	});
}

function removeBadgeFromUser(id_badge, id_user) {
	"use strict";
	
	$('.loaderScreen').fadeIn();
	
	$.post('inc/utils/exec_command.php', {
		command: 'removeBadgeFromUser',
		idItem: id_badge,
		idUser: id_user
	}).done( function(data) {
		processData(data);
	});
}

function getTerminals() {
	"use strict";
	
	var t = new Date();
	var date = Math.floor((t.getTime()/1000)+(t.getTimezoneOffset()*(-60)));
	
	$('#get-terminals .modal-content').empty();
	$('#get-terminals .modal-content').html('<img src="img/network.gif" />');
		
	$.post('inc/utils/exec_command.php', {
		command: 'getTerminals',
		date: date
	}).done( function(data) {
		location.reload();
	});
}

function saveQuery(id, newInsert) {
	"use strict";
	
	$('.loaderScreen').fadeIn();
	
	var query_id = id;
	var name = $('#edit-custom-queries input[aria-describedby="descrizione-query"]').val();
	var query = $('#edit-custom-queries textarea').val();

	$.post('inc/utils/exec_command.php', {
		command: 'saveQuery',
		newInsert: newInsert,
		query_id: query_id,
		name: name,
		query: query
	}).done( function(data) {
		processData(data);
	});
}

function deleteQuery(id) {
	"use strict";
	
	var r = confirm("Sei sicuro di voler eliminare la query?");
	
	if (r == true) {
		$('.loaderScreen').fadeIn();
		
		$.post('inc/utils/exec_command.php', {
			command: 'deleteQuery',
			query_id: id
		}).done( function(data) {
			processData(data);
		});
	}
}


function uploadDB(obj) {
	"use strict";
	
	var name = obj[0].name;
	files.push(obj[0]);
	
	$('#open_folder').fadeOut();
	$('#file-upload-progress').show();

	var formData = new FormData();
	formData.append('file', files[0]);
	
	formData.append('command', 'uploadDB');

	// now post a new XHR request
	var xhr = new XMLHttpRequest();
	xhr.open('POST', 'inc/utils/exec_command.php', true);
	xhr.onload = function (data) {
	  if (xhr.status === 200) {
		  $('#file-upload-progress').hide();
		  var response = xhr.responseText;
		  $('span.new-db-name').show();
		  if( response.length !== 0 ) {
			  // upload terminato con errori
			  $('span.new-db-name').text('<h3>'+xhr.responseText+'</h3>');
		  } else {
			  $('span.new-db-name').text(name+' caricato');
			  setTimeout( function() {
				  processData(data);
			  }, 10000);
		  }
	  }
	};
	
	xhr.onreadystatechange = function() {
		if (xhr.readyState == 4 && xhr.status == 200) {
			if(xhr.responseText !== "") {
			  var dump = xhr.responseText;
			}
		}
	  };

	xhr.upload.onprogress = updateProgress;

	xhr.send(formData);
}

function transferFiles(obj) {
	"use strict";
	
	$('.upload-files').removeClass('hover');
	if( !$('.upload-files').hasClass('filedropped') ) {
		$('button[type=submit]').removeAttr('disabled');
		$('.upload-files').addClass('filedropped');
		$('.upload-files .upload-iniziale').hide();
		$('.upload-files').append('<ul></ul>');
	}
	for(var i = 0; i < obj.length; i++) {
	  var name = obj[i].name;
	  files.push(obj[i]);
	  $('.upload-files ul').append('<li><span class="glyphicon glyphicon-remove pull-right" aria-hidden="true" onClick="deleteFile(this);"></span>'+name+'</li>');
	}
	
	return false;
}

function deleteFile(obj) {
	"use strict";
	
	var index = $(obj).closest('li').index();
	$(obj).closest('li').remove();
	files.splice(index, 1);
//	delete files[index];
	if( $('.upload-files ul li').length == 0 ) {
		$('button[type=submit]').attr('disabled', 'disabled');
		$('.upload-files').removeClass('filedropped');
		$('.upload-files .upload-iniziale').show();
		$('.upload-files ul').remove();
	}
}

function deleteAllJobs() {
	"use strict";
		
	var r = confirm("Sei sicuro di voler eliminare tutti i lavori programmati?");
	if (r == true) {
		$.post('inc/utils/exec_command.php', {
			command: 'deleteAllJobs'
		}, function(data) {
			processData(data);
		});
	}
}

function uploadFirmware() {
	"use strict";
	
	$('.schedule').attr('disabled', 'disabled');
	$('button[type=submit]').attr('disabled', 'disabled');
	$('#file-upload-progress').show();

	var formData = new FormData();
	for (var i = 0; i < files.length; i++) {
	  formData.append('file[]', files[i]);
	}
	
	var schedule = new Date();
	var date = Math.floor((schedule.getTime()/1000)+(schedule.getTimezoneOffset()*(-60)));
	formData.append('date', date);

	formData.append('command', 'uploadFirmware');
	if($('div[aria-label=scheduled]').attr('data-selected')) {
		if($('.schedule #datetimepicker1 input').val() !== "") {
			schedule = $('.schedule #datetimepicker1 input').val();
		}
	}

	formData.append('scheduled', schedule);

	// now post a new XHR request
	var xhr = new XMLHttpRequest();
	xhr.open('POST', 'inc/utils/exec_command.php', true);
	xhr.onload = function (data) {
	  if (xhr.status === 200) {
		  $('#file-upload-progress').hide();
		  $('.upload-files').empty();
		  $('.upload-files').removeClass('filedropped');
		  $('.upload-files').addClass('uploaded');
		  $('.schedule').remove();
		  $('button[type=submit]').remove();
		  var response = xhr.responseText;
		  if( response.length !== 0 ) {
			  // upload terminato con errori
			  $('.upload-files').html('<h3>'+xhr.responseText+'</h3>');
		  } else {
			  $('.upload-files').html('<h3>Tutti i files sono stati caricati correttamente</h3>');
			  setTimeout( function() {
				  processData(data);
			  }, 5000);
		  }
	  }
	};
	
	xhr.onreadystatechange = function() {
		if (xhr.readyState == 4 && xhr.status == 200) {
			if(xhr.responseText !== "") {
			  var dump = xhr.responseText;
			}
		}
	  };

	xhr.upload.onprogress = updateProgress;

	xhr.send(formData);
	
}

function updateProgress(event) {
	"use strict";
	
	if (event.lengthComputable) {
		var complete = (event.loaded / event.total * 100);
		$('#file-upload-progress .progress-bar').attr('aria-valuenow', complete);
		$('#file-upload-progress .progress-bar').css('width', complete+'%');
	}
}

$(window).on('scroll', function(e) {
	"use strict";

	if($(window).scrollTop() > ($('table.eventstable').scrollTop() + $('.navbar-brand').height() + 200)) {
		$('.fixed-header-bg').fadeIn();
		$('.eventstable thead tr:last-child div').fadeIn();
	}
	else {
		$('.fixed-header-bg').fadeOut();
		$('.eventstable thead tr:last-child div').fadeOut();
	}

	if( $(document).height() == ($(window).scrollTop() + $(window).height()) ) {
        if(window.location.search.search('eventi') != -1) {
            if(!startedRealTime)	 {
                getOldEvents();
            }
        }
	}
});

var timeout;
var rowid = 0;
var startedRealTime = 0;

function startRealTime() {
	"use strict";

	$('.eventstable	tbody').empty();
	$('#start-real-time').hide();
	$('#last-events').hide();
	$('#stop-real-time').show();
	
	$.post('inc/utils/exec_command.php', {
		command: 'getLastRowid'
	}, function(data) {
		rowid = data;
		startedRealTime = 1;
		
		timeout = setTimeout( getLastEvents, 1000);
	});
}

function stopRealTime() {
	"use strict";
	
	clearTimeout(timeout);
	startedRealTime = 0;
	$('#start-real-time').show();
	$('#last-events').show();
	$('#stop-real-time').hide();
}

function getLastEvents() {
	"use strict";
	
	var filters = getFilters();
	
	$.post('inc/utils/exec_command.php', {
		command: 'getLastEvents',
		startdate: filters[0],
		enddate: filters[1],
		badges: filters[2],
		titolari: filters[3],
		event_type: filters[4],
		terminals: filters[5],
		causals: filters[6],
		areas: filters[7],
		logic: filters[8],
		rowid: rowid
	}, function(data) {
		var lines = printData(data);
		$('.eventstable	tbody').prepend(lines);
		timeout = setTimeout( getLastEvents, 1000);
	}, "json").fail(function(e) {
		console.log(e.responseText);
	});
}

function getOldEvents() {
	"use strict";
	
	var filters = getFilters();
	
	$.post('inc/utils/exec_command.php', {
		command: 'getOldEvents',
		startdate: filters[0],
		enddate: filters[1],
		badges: filters[2],
		titolari: filters[3],
		event_type: filters[4],
		terminals: filters[5],
		causals: filters[6],
		areas: filters[7],
		logic: filters[8],
		rowid: rowid
	}, function(data) {
		var lines = printData(data);
		$('.eventstable	tbody').append(lines);
	}, "json", false).fail(function(e) {
		console.log(e.responseText);
	});
}

/*function printData(data) {
	"use strict";
	
	var lines;
	
	for(var i = 0; i < data.length; i++) {
		var line = "";
		var timestamp = data[i].timestamp.split(' ');
		line += '<td data-th="Data">'+timestamp[0]+'</td>';
		line += '<td data-th="Ora">'+timestamp[1]+'</td>';
		if(data[i].badge_id !== NO_BADGE) {
			line += '<td data-th="Badge ID">'+data[i].badge_id+'</td>';
			line += '<td data-th="Titolare">'+usersByBadge[data[i].badge_id]+'</td>';
		}
		else {
			line += '<td data-th="Badge ID">-</td>';
			line += '<td data-th="Titolare">-</td>';
		}
		line += '<td data-th="Evento">'+$('#events-ids option[value="'+data[i].event+'"]').text()+'</td>';
		if($('#terminals-ids option[value="'+data[i].terminal_id+'"]').text() !== "") {
			line += '<td data-th="Varco">'+$('#terminals-ids option[value="'+data[i].terminal_id+'"]').text()+'</td>';
		}
		else {
			line += '<td data-th="Varco">Varco '+data[i].terminal_id+'</td>';
		}
		if((data[i].area === 1) || (data[i].area === 0)) {
			line += '<td data-th="Direzione">'+$('#causal-ids option[value="'+data[i].area+'"]').text()+'</td>';
		}
		else if(data[i].area === NO_AREA) {
			line += '<td data-th="Direzione">-</td>';
		}
		else {
			line += '<td data-th="Direzione">verso</td>';
		}
		if((data[i].area !== NO_AREA) || (data[i].area !== 1) || (data[i].area !== 0)) {
			line += '<td data-th="Area">'+$('#areas-ids option[value="'+data[i].area+'"]').text()+'</td>';
		}
		else {
			line += '<td data-th="Area">-</td>';
		}
		lines += '<tr>'+line+'</tr>';
		rowid = data[i].rowid;
	}
	return lines;
}*/

function printData(data) {
	"use strict";
	
	var lines;
	
	for(var i = 0; i < data.length; i++) {
		
		var line = "";
		line += '<td data-th="Data">'+data[i].date+'</td>';
		line += '<td data-th="Ora">'+data[i].time+'</td>';
		line += '<td data-th="Badge ID">'+data[i].badgeID+'</td>';
		line += '<td data-th="Titolare">'+data[i].titolare+'</td>';
		line += '<td data-th="Evento">'+data[i].event_name+'</td>';
		line += '<td data-th="Causali">'+data[i].causale+'</td>';
		line += '<td data-th="Varco">'+data[i].varco+'</td>';
		line += '<td data-th="Area">'+data[i].area_name+'</td>';
		
		lines += '<tr>'+line+'</tr>';
		rowid = data[i].rowid;
	}
	return lines;
}

function exportEvents() {
	"use strict";
	
	var filters = getFilters();
	
	$.post('inc/utils/export_events.php', {
		command: 'exportEv',
		startdate: filters[0],
		enddate: filters[1],
		badges: filters[2],
		titolari: filters[3],
		event_type: filters[4],
		terminals: filters[5],
		causals: filters[6],
		areas: filters[7],
		logic: filters[8],
		limit: 1000,
		rowid: 0
	}).done( function(data) {
		document.location.href = 'inc/utils/export_events.php';
	});
}

function getFilters() {
	"use strict";
	
	usersByBadge = {};
	$('#user-ids option').each(function(index, element) {
		var associatedBadges = $(this).val();
		var badgesForUser = associatedBadges.split('-');
		for(var i = 0; i < badgesForUser.length; i++) {
			usersByBadge[badgesForUser[i]] = $(this).text();
		}
    });
	
	var filters = [];
	
	// Get filters
	var startdate = $('#fromdate input').val();
	filters.push(startdate);
	var enddate = $('#todate input').val();
	filters.push(enddate);
	
	var badges = [];
	$('.filters #badge_ids_chosen ul.chosen-choices li.search-choice a').each(function(index, element) {
        badges.push($('#badge-ids option').eq($(this).attr('data-option-array-index')).val());
    });
	filters.push(badges);
	
	var titolari = [];
	$('.filters #user_ids_chosen ul.chosen-choices li.search-choice a').each(function(index, element) {
		var associatedBadges = $('#user-ids option').eq($(this).attr('data-option-array-index')).val();
		var badgesForUser = associatedBadges.split('-');
		for(var i = 0; i < badgesForUser.length; i++) {
	        titolari.push(badgesForUser[i]);
		}
    });
	filters.push(titolari);
	
	var event_type = [];
	$('.filters #events_ids_chosen ul.chosen-choices li.search-choice a').each(function(index, element) {
        event_type.push($('#events-ids option').eq($(this).attr('data-option-array-index')).val());
    });
	filters.push(event_type);
	
	var terminals = [];
	$('.filters #terminals_ids_chosen ul.chosen-choices li.search-choice a').each(function(index, element) {
        terminals.push($('#terminals-ids option').eq($(this).attr('data-option-array-index')).val());
    });
	filters.push(terminals);
	
	var causals = [];
	$('.filters #causal_ids_chosen ul.chosen-choices li.search-choice a').each(function(index, element) {
        causals.push($('#causal-ids option').eq($(this).attr('data-option-array-index')).val());
    });
	filters.push(causals);
	
	var areas = [];
	$('.filters #areas_ids_chosen ul.chosen-choices li.search-choice a').each(function(index, element) {
        areas.push($('#areas-ids option').eq($(this).attr('data-option-array-index')).val());
    });
	filters.push(areas);
	
	var logic = $('div[aria-label="Logic"]').attr('data-selected');
	filters.push(logic);
	
	return filters;
}

function applyFilters() {
	"use strict";
	
	if(!startedRealTime) {
		rowid = 0;
		$('.eventstable	tbody').empty();
		getOldEvents();
	}
}

function applyBadgeFilters() {
	"use strict";
	
//	$('#badge .table-body').empty();
	
	var filters = getBadgeFilters();
	
	if(filters[0].length || filters[1].length) {		
		var i;

		$('#badge .table-body li.draggable').hide();
				
		if(filters[0].length) {		
			for(i = 0; i < filters[0].length; i++) {
				$('#badge .table-body li.draggable[data-id="'+filters[0][i]+'"]').show();
			}
		}
			
		if(filters[1].length) {		
			for(i = 0; i < filters[1].length; i++) {
				$('#badge .table-body li.draggable[data-id="'+filters[1][i]+'"]').show();
			}
		}

		if(filters[2].length) {		
			for(i = 0; i < filters[2].length; i++) {
				$('#badge .table-body li.draggable[data-id="'+filters[2][i]+'"]').show();
			}
		}
	} else {
		$('#badge .table-body li.draggable').show();
	}
}

function getBadgeFilters() {
	"use strict";
	
	usersByBadge = {};
	$('#user-ids option').each(function(index, element) {
		var associatedBadges = $(this).val();
		var badgesForUser = associatedBadges.split('-');
		for(var i = 0; i < badgesForUser.length; i++) {
			usersByBadge[badgesForUser[i]] = $(this).text();
		}
    });
	
	var filters = [];
	
	var badges = [];
	$('.filters #badge_ids_chosen ul.chosen-choices li.search-choice a').each(function(index, element) {
        badges.push($('#badge-ids option').eq($(this).attr('data-option-array-index')).val());
    });
	filters.push(badges);
	
	var titolari = [];
	$('.filters #user_ids_chosen ul.chosen-choices li.search-choice a').each(function(index, element) {
		var associatedBadges = $('#user-ids option').eq($(this).attr('data-option-array-index')).val();
		var badgesForUser = associatedBadges.split('-');
		for(var i = 0; i < badgesForUser.length; i++) {
	        titolari.push(badgesForUser[i]);
		}
    });
	filters.push(titolari);
	
	var badgePrintedCodes = [];
	$('.filters #printed_ids_chosen ul.chosen-choices li.search-choice a').each(function(index, element) {
        badges.push($('#printed-ids option').eq($(this).attr('data-option-array-index')).val());
    });
	filters.push(badgePrintedCodes);

	return filters;
}


function applyUserFilters() {
	"use strict";
		
	var filters = getUserFilters();
	
	if(filters[0].length) {		
		$('#utenti .table-body li.badge-droppable').hide();
		
		for(var i = 0; i < filters[0].length; i++) {
			$('#utenti .table-body li.badge-droppable[data-id="'+filters[0][i]+'"]').show();
		}
	} else {
		$('#utenti .table-body li.badge-droppable').show();
	}
}

function getUserFilters() {
	"use strict";
	
	var filters = [];
	
	var titolari = [];
	$('.filters #users_ids_chosen ul.chosen-choices li.search-choice a').each(function(index, element) {
        titolari.push($('#users-ids option').eq($(this).attr('data-option-array-index')).val());
    });
	filters.push(titolari);
	
	return filters;
}

$(document).on('click', '.btn-group[role="and-or"] button', function() {
	"use strict";
	
	$(this).siblings().removeClass('btn-danger').removeClass('btn-primary');
	if ( $(this).is(':first-child') ) {
		$(this).addClass('btn-primary');
		$(this).parent().attr('data-selected', '1');
		$(this).siblings('input').val(1);
	}
	else {
		$(this).addClass('btn-danger');
		$(this).parent().attr('data-selected', '0');
		$(this).siblings('input').val(0);
	}
	
	applyFilters();
});

/* ADMIN FUNCTIONS */
function sendRtc() {
	"use strict";
		
	var t = new Date();
	var h=t.getHours();
	var m=t.getMinutes();
	var s=t.getSeconds();
	var d=t.getDate();
	var mo=t.getMonth()+1;
	var y=t.getFullYear();

	if(h<10)h="0"+h;
	if(m<10)m="0"+m;
	if(s<10)s="0"+s;
	if(d<10)d="0"+d;
	if(mo<10)mo="0"+mo;
	var time = y.toString()+mo.toString()+d.toString()+h.toString()+m.toString()+s.toString();

	$.post('inc/utils/exec_command.php', {
		command: 'sendRtc',
		time: time
	}).done( function(data) {
		$('.rtcbutton').removeClass('btn-primary');
		$('.rtcbutton').addClass('btn-success');
	});

}

function changeSupervisorIP() {
	"use strict";
	
	var errors = false;
	
	var ip = $('#ipaddr').val();
	if(ip === '___.___.___.___') {
		errors = true;
		$('input#ipaddr').after(alertError('Campo obbligatorio'));
	}

	var subnet = $('#subnet').val();
	if(subnet === '___.___.___.___') {
		errors = true;
		$('input#subnet').after(alertError('Campo obbligatorio'));
	}

	var gateway = $('#gateway').val();
	if(gateway === '___.___.___.___') {
		errors = true;
		$('input#gateway').after(alertError('Campo obbligatorio'));
	}

	var DNS1 = $('#DNS1').val();
	if(DNS1 === '___.___.___.___') {
		errors = true;
		$('input#DNS1').after(alertError('Campo obbligatorio'));
	}

	var DNS2 = $('#DNS2').val();
	
	if(!errors) {
		$('button#changeIPSubmit').attr('disabled', 'disabled');
		$('button#changeIPSubmit').addClass('disabled');
		$('button#changeIPSubmit span.glyphicon-refresh.gly-spin').fadeIn();
		
		$.post('inc/utils/exec_command.php', {
			command: 'changeSupervisorIP',
			ip: ip,
			subnet: subnet,
			gateway: gateway,
			DNS1: DNS1,
			DNS2: DNS2
		}).done( function(data) {
			location.replace('http://'+ip);
		});
	}

}

function checkUserName(username) {
	"use strict";
	
	$.post('inc/utils/exec_command.php', {
		command: 'checkUserName',
		username: username
	}).done( function(data) {
		if(data !== '0') {
			$('.username').removeClass('has-success');
			$('.username').addClass('has-error');
			$('#edit-adminusers .submit-buttons button.btn-success').addClass('disabled');
			$('#edit-adminusers .submit-buttons button.btn-success').attr('disabled', 'disabled');
			$('input[aria-describedby="username"]').after(alertError('Lo username esiste gi&agrave;'));
		} else {
			$('.username').removeClass('has-error');
			$('.username').addClass('has-success');
			$('#edit-adminusers .submit-buttons button.btn-success').removeClass('disabled');
			$('#edit-adminusers .submit-buttons button.btn-success').removeAttr('disabled');
		}
	});

}

function saveAdminUser(id, newInsert) {
	"use strict";
	
	var errors = false;
	
	var username = $('input[aria-describedby="username"]').val();
	if( (name === '') || ($('.username').hasClass('has-error')) ) {
		errors = true;
		$('input[aria-describedby="username"]').after(alertError('Campo obbligatorio'));
	}
	
	var name = $('input[aria-describedby="name"]').val();
	if(name === '') {
		errors = true;
		$('input[aria-describedby="name"]').after(alertError('Campo obbligatorio'));
	}
	
	var lastname = $('input[aria-describedby="lastname"]').val();
	if(lastname === '') {
		errors = true;
		$('input[aria-describedby="lastname"]').after(alertError('Campo obbligatorio'));
	}

	var role = $('.btn-group[aria-label="role"]').attr('data-selected');
		
	if(!errors) {
		$('.loaderScreen').fadeIn();
	
		$.post('inc/utils/exec_command.php', {
			command: 'saveAdminUser',
			newInsert: newInsert,
			id: id,
			username: username,
			name: name,
			lastname: lastname,
			role: role
		}).done( function(data) {
			processData(data);
		});
	}
}

function deleteAdminUser(id) {
	"use strict";
	
	var r = confirm("Sei sicuro di voler eliminare l'utente?");

	if (r == true) {
		$('.loaderScreen').fadeIn();
		
		$.post('inc/utils/exec_command.php', {
			command: 'deleteAdminUser',
			id: id
		}).done( function(data) {
			processData(data);
		});
	}
}

function deleteBadgesFromTerminal() {
	"use strict";
	
	var r = confirm("Sei sicuro di voler eliminare i badge da tutti i terminali?");
	
	if (r == true) {
		$('.loaderScreen').fadeIn();

		$.post('inc/utils/exec_command.php', {
			command: 'deleteBadgesFromTerminal'
		}).done( function(data) {
			$('.loaderScreen').fadeOut();
			$('#deleteBadgesFromTerminal').html('Badge eliminati <span class="glyphicon glyphicon-ok" aria-hidden="true"></span>');
			$('#deleteBadgesFromTerminal').removeClass('btn-warning');
			$('#deleteBadgesFromTerminal').addClass('btn-success');
			$('#deleteBadgesFromTerminal').removeAttr('onClick');
		});
	}
}

function deleteBadgesFromSupervisor() {
	"use strict";
	
	var r = confirm("Sei sicuro di voler eliminare i badge dal supervisore?");
	
	if (r == true) {
		$('.loaderScreen').fadeIn();

		$.post('inc/utils/exec_command.php', {
			command: 'deleteBadgesFromSupervisor'
		}).done( function(data) {
			$('.loaderScreen').fadeOut();
			$('#deleteBadgesFromSupervisor').html('Badge eliminati <span class="glyphicon glyphicon-ok" aria-hidden="true"></span>');
			$('#deleteBadgesFromSupervisor').removeClass('btn-danger');
			$('#deleteBadgesFromSupervisor').addClass('btn-success');
			$('#deleteBadgesFromSupervisor').removeAttr('onClick');
		});
	}
}

function restoreDefault() {
	"use strict";
	
	$('.loaderScreen').fadeIn();
	
	$.post('inc/utils/exec_command.php', {
		command: 'restoreDefault'
	}).done( function(data) {
		processData(data);
	});
}

function processData(data) {
    "use strict";
    
    if(data == 'ok') {
        location.reload();
    } else {
        $('.error_log').html(data).slideDown();
        $('.modal[role=dialog]').modal('hide');
        $('.loaderScreen').fadeOut();
    }
}
