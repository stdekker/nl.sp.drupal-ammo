<?php if (!empty($meeting['selected_document']) && empty($meeting['hide'])) : ?>
  <?php $document = $meeting['documents'][$meeting['selected_document']]; ?>
  <nav class="amendments-navigation" aria-label="Documenten, hoofdstukken en voorstellen">
    <?php print $chapter_navigation . $amendment_navigation; ?>
  </nav>
  <div class="first">
    <?php if ($meeting['admin_access']):
      $states = ammo_states();
      $header = array();
      $counts = array();
      foreach ($document['totals'] as $key => $value) {
        $header[] = isset($states[$key]) ? $states[$key] : check_plain($key);
        $counts[] = $value;
      }
      ?>
      <div class="ammo-state-totals">
        <?php print theme('table', array('header' => $header, 'rows' => array($counts))); ?>
      </div>
    <?php endif; ?>
    <ul class="amendments">
      <?php foreach ($document['amendments'] as $amendment): ?>
        <li class="ammo-element <?php print check_plain($amendment['state']); ?>">
          <?php print theme('amendment', array('entity_id' => $amendment['id'], 'destination' => $destination)); ?>
        </li>
      <?php endforeach; ?>
    </ul>
  </div>
  <nav class="amendments-navigation" aria-label="Documenten en hoofdstukken onderaan">
    <?php print $chapter_navigation; ?>
  </nav>
<?php elseif (empty($meeting['hide'])): ?>
  <p>Er zijn geen wijzigingsvoorstellen beschikbaar.<?php if (!empty($submit_link)) : ?> <?php print $submit_link; ?><?php endif; ?></p>
<?php endif; ?>
