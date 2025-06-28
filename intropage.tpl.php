<div class="ammo-content">
  <h1>Wijzigingsvoorstellen en moties</h1>
 
  <?php if ($has_any_access): ?>

    <div class="ammo-section-content">
      <h2>Bekijken</h2>
      <p>Voordat je een voorstel of motie indient, bekijk je eerst het overzicht van de ingediende voorstellen. Vaak kun je bij een bestand voorstel aansluiten, hier kun je in het overzicht steun bij aangeven.</p>
      <nav class="button-group">
        <ul>
          <li><a href="ammo/amendments">Overzicht wijzigingsvoorstellen</a></li>
          <li><a href="ammo/motions">Overzicht moties</a></li>
        </ul>
      </nav>
    </div>

    <div class="ammo-section-content">   
      <h2>Indienen</h2>
      <p>Als er geen voorstellen zijn waar je bij kunt aansluiten kun je namens je afdeling(en) een wijzigingsvoorstel of motie indienen. Deze moeten besproken zijn tijdens een ledenvergadering.</p>
      <p><strong>Let op!</strong> Tekstcorrecties, typefouten, e.d. kunnen worden ingediend bij <a href="mailto:secretariaat@sp.nl">secretariaat@sp.nl </a></p>
      <nav class="button-group">
        <ul>
          <li><a href="ammo/amendment">Wijzigingsvoorstel <span class="ammo-button-label">Voor inoudelijke toevoegingen en wijzigingen</span></a></li>
          <li><a href="ammo/motion">Motie <span class="ammo-button-label">Om de partij op te roepen tot een specifiek actie</span></a></li>
        </ul>
      </nav>
    </div>
  
  <?php else: ?>
      <p><?php print t('You do not have the necessary permissions to use this module.'); ?></p>
  <?php endif; ?>

</div>
