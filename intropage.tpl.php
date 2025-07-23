<div class="ammo-content">
  <h1>Wijzigingsvoorstellen en moties</h1>
 
  <?php if ($has_any_access): ?>

    <div class="ammo-section-content">
      <h2>Bekijken</h2>
      <p>Voordat je een voorstel of motie indient, bekijk je eerst het overzicht van de ingediende voorstellen. 
        Vaak kun je bij een bestaand voorstel aansluiten, hier kun je in het overzicht steun bij aangeven.</p>
      <nav class="button-group">
        <ul>
          <li><a href="ammo/amendments">Overzicht wijzigingsvoorstellen</a></li>
          <?php if ($show_motions): ?>
          <li><a href="ammo/motions">Overzicht moties</a></li>
          <?php endif; ?>
        </ul>
      </nav>
    </div>

    <div class="ammo-section-content">   
      <h2>Indienen</h2>
      <p>Als er geen voorstellen zijn waar je bij kunt aansluiten kun je namens je afdeling een wijzigingsvoorstel<?php if ($show_motions): ?> of motie<?php endif; ?> indienen. 
        Deze moeten besproken zijn tijdens een ledenvergadering.</p>
      <p><strong>Let op!</strong> Tekstcorrecties, typefouten, tekstuele aanvullingen e.d. kunnen worden ingediend bij <a href="mailto:programmacommissie@sp.nl">programmacommissie@sp.nl</a>. Voorstellen en moties over kommaplaatsing, spelling, zinsstructuur etc. worden ter kennisgeving aangenomen met de status 'tekstuele wijziging' en komen niet in stemming op het congres.</p>
      <p>Loop je tegen problemen aan met het gebruik van dit programma? Stuur een e-mail naar <a href="mailto:webteam@sp.nl">webteam@sp.nl</a></p>
      <nav class="button-group">
        <ul>
          <li><a href="ammo/amendment">Wijzigingsvoorstel <span class="ammo-button-label">Voor inoudelijke toevoegingen en wijzigingen</span></a></li>
          <?php if ($show_motions): ?>
          <li><a href="ammo/motion">Motie <span class="ammo-button-label">Om de partij op te roepen tot een specifiek actie</span></a></li>
          <?php endif; ?>
        </ul>
      </nav>
    </div>
  
  <?php else: ?>
      <p><?php print t('You do not have the necessary permissions to use this module.'); ?></p>
  <?php endif; ?>

</div>
