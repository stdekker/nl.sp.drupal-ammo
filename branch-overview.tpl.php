<?php if (empty($meeting_available)) : ?>
  <p>Deze bijeenkomst is niet beschikbaar.</p>
<?php elseif (empty($branches)) : ?>
  <p>Er is geen afdeling aan uw account gekoppeld.</p>
<?php else : ?>
  <?php foreach ($branches as $branch) : ?>
    <?php foreach ($branch as $part => $content) : ?>
      <section class="branch-overview <?php print drupal_html_class($part); ?>">
        <h2><?php print $content['title']; ?></h2>
        <?php foreach ($content['tables'] as $table) : ?>
          <?php print theme('table', $table); ?>
        <?php endforeach; ?>
      </section>
    <?php endforeach; ?>
  <?php endforeach; ?>
<?php endif; ?>
