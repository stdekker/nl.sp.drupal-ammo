<h2 id="inhoud">Ingediende wijzigingsvoorstelen</h2>

<?php if (!empty($meeting['selected_document']) && empty($meeting['hide'])) : ?>
  <nav class="chapter-navigation" aria-label="Hoofdstukken bovenaan">
    <?php print $chapter_navigation; ?>
  </nav>
  <?php $document = $meeting['documents'][$meeting['selected_document']]; ?>
  <?php if (!empty($document['amendments'])): ?>
    <nav class="amendment-navigation" aria-label="Wijzigingsvoorstellen in dit hoofdstuk">
      <ul class="amendments">
        <?php foreach ($document['amendments'] as $amendment): ?>
          <li class="amendment">
            <a href="#amendment<?php print $amendment['id']; ?>"><?php print ($amendment['chapter'] ?: 'X') . '.' . ($amendment['chapterized_id'] ?: $amendment['id']); ?></a>
          </li>
        <?php endforeach; ?>
      </ul>
    </nav>
  <?php endif; ?>
  <div class="first">
    <?php if ($meeting['admin_access']):
      $states = ammo_states();
      $rows = array();
      foreach ($document['totals'] as $key => $value) {
        $rows[] = array(isset($states[$key]) ? $states[$key] : check_plain($key), $value);
      }
      print theme('table', array('rows' => $rows));
    endif; ?>
    <ul class="amendments">
      <?php foreach ($document['amendments'] as $amendment): ?>
        <li class="ammo-element <?php print check_plain($amendment['state']); ?>">
          <?php print theme('amendment', array('entity_id' => $amendment['id'], 'destination' => $destination)); ?>
        </li>
      <?php endforeach; ?>
    </ul>
  </div>
  <nav class="chapter-navigation" aria-label="Hoofdstukken onderaan">
    <?php print $bottom_chapter_navigation; ?>
  </nav>
<?php elseif (empty($meeting['hide'])): ?>
  <p>Er zijn geen wijzigingsvoorstellen beschikbaar.</p>
<?php endif; ?>
