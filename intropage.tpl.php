<?php $enmoties = ($show_motions) ? ' en moties ': ''; ?>
<?php $ofmoties = ($show_motions) ? ' of moties ': ''; ?>
<div class="ammo-content">
  <h1>Wijzigingsvoorstellen<?php print $enmoties; ?></h1>
  <?php if ($has_any_access): ?>
    <p><strong>U kunt in dit scherm:</strong></p>
    <ul>
      <li>Een overzicht van <?php print l('wijzigingsvoorstellen bekijken', 'ammo/amendments'); ?><?php print ($show_motions) ? ' of ' . l('moties', 'ammo/motions') : ''; ?> .</li>
      <?php if ($add_access): ?>
        <li>Nieuwe <?php print l('wijzigingsvoorstellen', 'ammo/amendment'); ?><?php print ($show_motions) ? ' en ' . l('moties', 'ammo/motion') : ''; ?> indienen.</li>
      <?php endif; ?>
      <?php if ($superadmin_access): ?>
        <li>Een nieuwe <?php print l('bijeenkomst toevoegen', 'ammo/meeting'); ?>.</li>
        <li>Een nieuw <?php print l('stuk toevoegen', 'ammo/document'); ?> bij een bijeenkomst.</li>
      <?php endif; ?>
    </ul>
    <?php if ($add_access): ?>
      <p><strong>In een overzicht kunt u:</strong></p>
      <ul>
        <li>Wijzigingsvoorstellen<?php print $enmoties; ?> van andere afdelingen mede indienen.</li>
        <?php if (!$admin_access): ?>
          <li>Uw ingediende wijzigingsvoorstellen<?php print $enmoties; ?> aanpassen.</li>
        <?php endif; ?>
        <?php if ($admin_access): ?>
          <li>Ingediende wijzigingsvoorstellen<?php print  $enmoties; ?> aanpassen.</li>
          <li>Ingediende wijzigingsvoorstellen<?php print  $enmoties; ?> voorzien van advies.</li>
          <li>De status van ingediende wijzigingsvoorstellen<?php $enmoties; ?> aanpassen.</li>
        <?php endif; ?>
        <li>Uw ingediende wijzigingsvoorstellen<?php print $enmoties; ?> intrekken.</li>
      </ul>
      <?php endif; ?>
    <p><strong>Klik op een van de grijze tabjes bovenaan voor de gewenste functies.</strong></p>
  <?php else: ?>
    <p><?php print t('You do not have the necessary permissions to use this module.'); ?></p>
  <?php endif; ?>
</div>
