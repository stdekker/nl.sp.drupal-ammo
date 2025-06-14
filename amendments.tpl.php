<h2 id="inhoud">Overzicht alle ingediende wijzigingsvoorstelen</h2>

<?php if (!empty($meeting['documents']) && empty($meeting['hide'])) : ?>
  <div class="first navigation">
    <?php foreach ($meeting['documents'] as $document) : ?>
      <?php if (!empty($document['amendments'])) : ?>
      <div class="document">
        <a href="#document<?php print $document['id']; ?>"><strong>Document: <?php print $document['title']; ?></strong></a>
        <?php if (!empty($document['chapters'])) : ?>
          <ul class="chapters tab">
            <li class="chapter"><span>Hoofdstuk:</span></li>
            <?php foreach ($document['chapters'] as $chapter) : ?>
              <?php if (!isset($firstchapter)) $firstchapter = $chapter['nr']; ?>
              <li class="chapter">
                <a href="#" class="tablinks <?php print ($firstchapter == $chapter['nr']) ? 'active' : ''; ?>" id="<?php print 'tab-' . $document['id'] . '-' . $chapter['nr']; ?>"><?php print $chapter['nr']; ?></a>
              </li>
            <?php endforeach; ?>
          </ul>
          <?php foreach ($document['chapters'] as $chapter) : ?>
            <div id="<?php print 'tabcontent-' . $document['id'] . '-' . $chapter['nr']; ?>" class="tabcontent <?php print ($firstchapter == $chapter['nr']) ? 'active' : ''; ?>">

              <ul class="amendments">
                <?php $amendment_ids = $document['amendment_index'][$chapter['nr']]; ?>
                <?php foreach ($amendment_ids as $amendment_id) : ?>
                  <?php $amendment = $document['amendments'][$amendment_id]; ?>
                  <li class="amendment">
                    <a href="#amendment<?php print $amendment['id']; ?>"><?php print (!empty($amendment['chapterized_id'])) ? $amendment['chapter'] . '.' . $amendment['chapterized_id'] : $amendment['chapter'] . '.' . $amendment_id; ?></a>
                  </li>
                <?php endforeach; ?>
              </ul>
            </div>
          <?php endforeach; ?>
        <?php endif; ?>
      </div>
      <?php endif; ?>
    <?php endforeach; ?>
  </div>

  <div class="first">
    <?php foreach ($meeting['documents'] as $document) : ?>
      <?php if (!empty($document['amendments'])) : ?>
        <h2 id="document<?php print $document['id']; ?>" class="document-title"><?php print $document['title']; ?></h2>
          <?php if ($meeting['admin_access']): 
            $states = ammo_states();
            if (!empty($meeting['documents'][$document['id']]['totals'])):
              foreach ($meeting['documents'][$document['id']]['totals'] as $key => $value):
                $rows[] = array($states[$key], $value);
              endforeach;
              print theme('table', array('rows' => $rows));
            endif;
          endif;

          if (!empty($document['chapters'])):
            foreach ($document['chapters'] as $chapter):
              $amendment_ids = $document['amendment_index'][$chapter['nr']];
              foreach ($amendment_ids as $amendment_id):
                $amendment = $document['amendments'][$amendment_id]; ?>
                <ul>
                  <li class="ammo-element <?php print $amendment['state']; ?>">
                    <?php print theme('amendment', array('entity_id' => $amendment['id'], 'destination' => $destination)); ?>
                  </li>
                </ul>
              <?php endforeach;
            endforeach;
          endif;
          ?>
      <?php endif; ?>
    <?php endforeach; ?>
  </div>
<?php endif; ?>
