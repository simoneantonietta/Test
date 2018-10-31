<nav class="navbar navbar-default navbar-fixed-top">
  <div class="container">
    <!-- Brand and toggle get grouped for better mobile display -->
    <div class="navbar-header">
      <button type="button" class="navbar-toggle collapsed" data-toggle="collapse" data-target="#main-menu-collapse" aria-expanded="false">
        <span class="sr-only">Menu</span>
        <span class="icon-bar"></span>
        <span class="icon-bar"></span>
        <span class="icon-bar"></span>
      </button>
      <a class="navbar-brand" href="index.php"><img src="img/saetis.png" height="75" alt=""/></a>
    </div>

    <!-- Collect the nav links, forms, and other content for toggling -->
    <div class="collapse navbar-collapse" id="main-menu-collapse">
      <ul class="nav navbar-nav navbar-left">
      <?php
	  	if($loggeduser < 10) { ?>
        <li class="dropdown <?php if($menu == 'sistema' || $menu == 'fasce-orarie-list' || $menu == 'profili-list' || $menu == 'terminali-list' || $menu == 'aree' || $menu == 'aggiornamento-firmware'  ) echo 'active'; ?>">
          <a href="#" class="dropdown-toggle" data-toggle="dropdown" role="button" aria-haspopup="true" aria-expanded="false"><span class="glyphicon glyphicon-cog" aria-hidden="true"></span> configurazioni <span class="caret"></span></a>
          <ul class="dropdown-menu">
            <li <?php if($menu == 'terminali-list') echo 'class="active"'; ?>><a href="?menu=terminali-list"><span class="glyphicon glyphicon-modal-window" aria-hidden="true"></span> terminali</a></li>
            <li <?php if($menu == 'fasce-orarie-list') echo 'class="active"'; ?>><a href="?menu=fasce-orarie-list"><span class="glyphicon glyphicon-time" aria-hidden="true"></span> fasce orarie</a></li>
            <li <?php if($menu == 'aree') echo 'class="active"'; ?>><a href="?menu=aree"><span class="glyphicon glyphicon-th-large" aria-hidden="true"></span> aree</a></li>
            <li <?php if($menu == 'profili-list') echo 'class="active"'; ?>><a href="?menu=profili-list"><span class="glyphicon glyphicon-tasks" aria-hidden="true"></span> profili</a></li>
            <li <?php if($menu == 'sistema') echo 'class="active"'; ?>><a href="?menu=sistema"><span class="glyphicon glyphicon-dashboard" aria-hidden="true"></span> sistema</a></li>
            <li <?php if($menu == 'badge-codes') echo 'class="active"'; ?>><a href="?menu=badge-codes"><span class="glyphicon glyphicon-certificate" aria-hidden="true"></span> codici causali</a></li>
            <li role="separator" class="divider"></li>
            <li <?php if($menu == 'aggiornamento-firmware') echo 'class="active"'; ?>><a href="?menu=aggiornamento-firmware"><span class="glyphicon glyphicon-download-alt" aria-hidden="true"></span> aggiornamento firmware</a></li>
          </ul>
        </li>
        <?php }
        if($loggeduser < 100) { ?>
        <li <?php if($menu == 'associa-badge-list') echo 'class="active"'; ?>><a href="?menu=associa-badge-list"><span class="glyphicon glyphicon-tag" aria-hidden="true"></span> badge</a></li>
        <?php }
		if($loggeduser < 1000) { ?>
        <li <?php if($menu == 'eventi') echo 'class="active"'; ?>><a href="?menu=eventi"><span class="glyphicon glyphicon-calendar" aria-hidden="true"></span> eventi</a></li>
        <li <?php if($menu == 'queries' || $menu == 'custom-query-output') echo 'class="active"'; ?>><a href="?menu=queries"><span class="glyphicon glyphicon-th-list" aria-hidden="true"></span> query personalizzate</a></li>
        <?php } ?>
      </ul>
      <?php if($loggeduser < 1000) { ?>
      <ul class="nav navbar-nav navbar-right">
        <li class="dropdown <?php if($menu == 'admin_settings') echo 'active'; ?>">
        <a href="#" class="dropdown-toggle" data-toggle="dropdown" role="button" aria-haspopup="true" aria-expanded="false"><span class="glyphicon glyphicon-lock" aria-hidden="true"></span> <?php echo $_SESSION['username']; ?></a>
            <ul class="dropdown-menu">
      <?php if($loggeduser < 10) { ?>
                <li <?php if($menu == 'admin_adminusers') echo 'class="active"'; ?>><a href="?menu=admin_adminusers"><span class="glyphicon glyphicon-user" aria-hidden="true"></span> Gestione utenti</a></li>
                <li <?php if($menu == 'admin_settings') echo 'class="active"'; ?>><a href="?menu=admin_settings"><span class="glyphicon glyphicon-wrench" aria-hidden="true"></span> Impostazioni di sistema</a></li>
                <li <?php if($menu == 'admin_change-krypt') echo 'class="active"'; ?>><a href="?menu=admin_change-krypt"><span class="glyphicon glyphicon-barcode" aria-hidden="true"></span> Modifica chiave di cifratura</a></li>
	            <li role="separator" class="divider"></li>
      <?php } ?>
                <li <?php if($menu == 'admin_change-password') echo 'class="active"'; ?>><a href="?menu=admin_change-password"><span class="glyphicon glyphicon-lock" aria-hidden="true"></span> cambia la password</a></li>
                <li><a href="?menu=logout"><span class="glyphicon glyphicon-log-out" aria-hidden="true"></span> logout</a></li>
            </ul>
        </li>
      </ul>
      <?php } ?>
    </div><!-- /.navbar-collapse -->
  </div><!-- /.container-fluid -->
</nav>