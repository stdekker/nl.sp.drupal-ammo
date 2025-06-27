<h1 id="inhoud">Ingediende moties</h1>

<?php if ($meeting['admin_access']) : ?>
  <?php $states = ammo_states(); ?>
  <?php if (isset($meeting['motion_totals'])) : ?>
    <?php foreach ($meeting['motion_totals'] as $key => $value) : ?>
      <?php $rows[] = array($states[$key], $value); ?>
    <?php endforeach; ?>
  <?php endif; ?>
  <?php if (!empty($rows)) : ?>
    <?php print theme('table', array('rows' => $rows)); ?>
  <?php endif; ?>
<?php endif; ?>

<?php if (!empty($meeting['motions']) && empty($meeting['hide'])) : ?>
  <h2 id="inhoud" class="hidden">Inhoud</h2>
  <div class="navigation">
    <ul class="motions">
      <?php foreach ($meeting['motions'] as $motion) : ?>
        <li>
          <a href="#motion<?php print $motion['id']; ?>"><?php print (!empty($motion['motion_id'])) ? $motion['motion_id'] : $motion['id']; ?></a>
        </li>
      <?php endforeach; ?>
    </ul>
  </div>
  <ul>
    <?php foreach ($meeting['motions'] as $motion) : ?>
      <li class="ammo-element <?php print $motion['state']; ?>">
        <?php print theme('motion', array('entity_id' => $motion['id'], 'destination' => $destination)); ?>
      </li>
    <?php endforeach; ?>
  </ul>

<?php endif; ?>
