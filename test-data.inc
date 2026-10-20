<?php

/**
 * Meeting-scoped test data forms and storage. No notification handlers are used.
 */
function ammo_test_data_documents($meeting_id) {
  return db_query('SELECT id, title FROM {ammo_documents} WHERE meeting_id = :meeting ORDER BY title', array(':meeting' => $meeting_id))->fetchAllKeyed();
}

function ammo_test_data_count($meeting_id) {
  return (int) db_query('SELECT COUNT(*) FROM {ammo_test_entries} WHERE meeting_id = :meeting', array(':meeting' => $meeting_id))->fetchField();
}

function ammo_test_data_branches() {
  $branches = (array) ammo_get_all_afdelingen_list();
  foreach ($branches as $id => $name) {
    if (!ctype_digit((string) $id) || $id <= 0) {
      unset($branches[$id]);
    }
  }
  return $branches;
}

function ammo_test_data_form($form, &$form_state, $meeting) {
  $form['meeting'] = array('#type' => 'value', '#value' => $meeting->id);
  $form['summary'] = array('#markup' => '<p>' . t('Bijeenkomst: @title. Gegenereerde testgegevens: @count.', array('@title' => $meeting->title, '@count' => ammo_test_data_count($meeting->id))) . '</p>');
  $documents = ammo_test_data_documents($meeting->id);
  $types = array();
  if ($documents) {
    $types['amendment'] = t('Wijzigingsvoorstellen');
  }
  if (ammo_meeting_has_motions($meeting)) {
    $types['motion'] = t('Moties');
  }
  if (!ammo_test_data_branches()) {
    $form['empty'] = array('#markup' => '<p>' . t('Er zijn geen gepubliceerde, erkende afdelingen met een geldig afdelingsnummer beschikbaar. Toevoegen van testgegevens is niet mogelijk.') . '</p>');
  }
  elseif ($types) {
    $form['entry_type'] = array('#type' => 'select', '#title' => t('Soort'), '#options' => $types, '#required' => TRUE);
    $form['count'] = array('#type' => 'select', '#title' => t('Aantal'), '#options' => array(10 => 10, 25 => 25, 50 => 50, 100 => 100), '#default_value' => 10, '#required' => TRUE);
    if ($documents) {
      $form['document'] = array('#type' => 'select', '#title' => t('Document'), '#options' => $documents, '#states' => array('visible' => array(':input[name="entry_type"]' => array('value' => 'amendment'))));
    }
    $form['description'] = array('#markup' => '<p>' . t('Voegt een nieuwe serie toe met verschillende statussen en herkenbare Nederlandse testteksten. Bestaande afdelingen worden afwisselend als indiener gekoppeld. Er worden geen e-mails verzonden.') . '</p>');
    $form['submit'] = array('#type' => 'submit', '#value' => t('Testgegevens toevoegen'));
  }
  else {
    $form['empty'] = array('#markup' => '<p>' . t('Voeg een document toe of schakel moties in om testgegevens te maken.') . '</p>');
  }
  if (ammo_test_data_count($meeting->id)) {
    $form['delete'] = array('#markup' => '<p>' . l(t('Alle testgegevens verwijderen'), 'admin/ammo/meeting/' . $meeting->id . '/test-data/delete') . '</p>');
  }
  return $form;
}

function ammo_test_data_validate_selection($meeting_id, $type, $count, $document_id) {
  $meeting = entity_load_single('meeting', $meeting_id);
  if (!$meeting || !in_array((string) $count, array('10', '25', '50', '100'), TRUE)) {
    throw new InvalidArgumentException('Kies een bestaande bijeenkomst en een geldig aantal.');
  }
  if ($type === 'motion' && ammo_meeting_has_motions($meeting)) {
    return;
  }
  if ($type === 'amendment' && !empty($document_id)) {
    $document = entity_load_single('document', $document_id);
    if ($document && $document->meeting_id == $meeting_id) {
      if (ammo_document_is_legacy($document) && $document->chapter_min > 10) {
        throw new InvalidArgumentException('Dit document staat geen hoofdstukken binnen 1–10 toe.');
      }
      $locators = ammo_document_locators($document);
      $errors = ammo_validate_locators($document, ammo_test_data_locator_input($locators, 0), $locators);
      if (!$errors) {
        return;
      }
      throw new InvalidArgumentException(implode(' ', $errors));
    }
  }
  throw new InvalidArgumentException('Kies een ingeschakeld soort en een document van deze bijeenkomst.');
}

function ammo_test_data_form_validate($form, &$form_state) {
  $values = $form_state['values'];
  try {
    ammo_test_data_validate_selection($values['meeting'], $values['entry_type'], $values['count'], isset($values['document']) ? $values['document'] : NULL);
    if (!ammo_test_data_branches()) {
      throw new InvalidArgumentException('Er zijn geen afdelingen beschikbaar voor de testgegevens.');
    }
  }
  catch (InvalidArgumentException $e) {
    form_set_error('entry_type', $e->getMessage());
  }
}

function ammo_test_data_form_submit($form, &$form_state) {
  $values = $form_state['values'];
  try {
    ammo_test_data_generate($values['meeting'], $values['entry_type'], $values['count'], isset($values['document']) ? $values['document'] : NULL);
    drupal_set_message(t('@count testgegevens toegevoegd.', array('@count' => $values['count'])));
  }
  catch (Exception $e) {
    watchdog_exception('ammo', $e);
    drupal_set_message(t('De serie kon niet worden toegevoegd. Er zijn geen testgegevens opgeslagen.'), 'error');
  }
  $form_state['redirect'] = 'admin/ammo/meeting/' . $values['meeting'] . '/test-data';
}

/**
 * Locator input for test entry $i; the group rotates over at most ten values.
 */
function ammo_test_data_locator_input(array $locators, $i) {
  $input = array();
  foreach ($locators as $key => $locator) {
    if (empty($locator['required'])) {
      $input[$key] = '';
      continue;
    }
    if ($locator['type'] === 'text') {
      $input[$key] = 'Testlocatie';
      continue;
    }
    $letter = $locator['type'] === 'letter';
    $first = $letter ? ord($locator['min'] !== '' ? $locator['min'] : 'A') : max(1, (int) $locator['min']);
    $value = $first;
    if (!empty($locator['grouping'])) {
      $last = $letter ? ord($locator['max'] !== '' ? $locator['max'] : 'Z') : ($locator['max'] !== '' ? (int) $locator['max'] : PHP_INT_MAX);
      // Legacy chapters stay within 1–10.
      $last = !empty($locator['legacy']) ? min($last, 10) : min($last, $first + 9);
      $value = $first + ($last >= $first ? $i % ($last - $first + 1) : 0);
    }
    $input[$key] = $letter ? chr($value) : (string) $value;
  }
  return $input;
}

function ammo_test_data_generate($meeting_id, $type, $count, $document_id = NULL) {
  ammo_test_data_validate_selection($meeting_id, $type, $count, $document_id);
  $branches = ammo_test_data_branches();
  if (!$branches) {
    throw new InvalidArgumentException('Er zijn geen afdelingen beschikbaar voor de testgegevens.');
  }
  $branch_ids = array_keys($branches);
  $transaction = db_transaction();
  try {
    // Serialize generated series and cleanup for this meeting.
    db_query('SELECT id FROM {ammo_meetings} WHERE id = :id FOR UPDATE', array(':id' => $meeting_id))->fetchField();
    $states = array_keys(ammo_states());
    $topics = array('betaalbare woningen', 'goed openbaar vervoer', 'toegankelijke zorg', 'groene buurten', 'gelijke kansen');
    $document = $type === 'amendment' ? entity_load_single('document', $document_id) : NULL;
    $locators = $document ? ammo_document_locators($document) : array();
    for ($i = 0; $i < $count; $i++) {
      $topic = $topics[$i % count($topics)];
      $label = '[TEST] ' . ($i + 1) . ': ' . $topic;
      $values = array('state' => $states[$i % count($states)], 'supplement' => $label . '. Dit is gegenereerde testdata.', 'advice' => $i % 2 ? 'reject' : 'accept', 'advice_supplement' => 'Testadvies over ' . $topic . '.', 'state_supplement' => 'Teststatus.');
      if ($document) {
        $fields = ammo_amendment_locator_fields($document, ammo_test_data_locator_input($locators, $i));
        $values += $fields + array('document_id' => $document_id, 'amendment_text' => $label . '. Voeg toe: Wij zetten ons in voor ' . $topic . '.', 'chapterized_id' => ammo_next_chapterized_id($document_id, ammo_amendment_group($fields)));
      }
      else {
        $values += array('meeting_id' => $meeting_id, 'motion_id' => 1 + db_query('SELECT MAX(motion_id) FROM {ammo_motions} WHERE meeting_id = :meeting', array(':meeting' => $meeting_id))->fetchField(), 'consideration_opening' => '[TEST] constaterende dat', 'consideration_body' => $label . '. Er behoefte is aan ' . $topic . '.', 'follow_up_opening' => 'verzoekt', 'follow_up_body' => 'Een voorstel uit te werken voor ' . $topic . '.');
      }
      $entry = ammo_save_entity($type, $values);
      if (!$entry) {
        throw new RuntimeException('Test entry could not be saved.');
      }
      $branch_id = $branch_ids[$i % count($branch_ids)];
      $branch_name = ammo_cleanup_branch_name($branches[$branch_id]);
      $owner = ammo_save_entity('contact_relation', array('target_type' => $type, 'target_id' => $entry->id, 'relation_type' => 'owner', 'contact_id' => $branch_id, 'contact_type' => 'branch', 'contact_display_name' => $branch_name, 'branch_id' => $branch_id, 'branch_display_name' => $branch_name, 'contact_email' => ''));
      if (!$owner) {
        throw new RuntimeException('Test owner could not be saved.');
      }
      db_insert('ammo_test_entries')->fields(array('meeting_id' => $meeting_id, 'entry_type' => $type, 'entry_id' => $entry->id))->execute();
    }
  }
  catch (Exception $e) {
    $transaction->rollback();
    throw $e;
  }
}

function ammo_test_data_delete_form($form, &$form_state, $meeting) {
  $form['meeting'] = array('#type' => 'value', '#value' => $meeting->id);
  return confirm_form($form, t('Alle @count testgegevens van @title verwijderen?', array('@count' => ammo_test_data_count($meeting->id), '@title' => $meeting->title)), 'admin/ammo/meeting/' . $meeting->id . '/test-data', t('Dit verwijdert alle gegenereerde wijzigingsvoorstellen en moties van deze bijeenkomst, inclusief hun relaties. Gewone gegevens blijven behouden.'));
}

function ammo_test_data_delete_form_submit($form, &$form_state) {
  $meeting_id = $form_state['values']['meeting'];
  try {
    $count = ammo_test_data_delete($meeting_id);
    drupal_set_message(t('@count testgegevens verwijderd.', array('@count' => $count)));
  }
  catch (Exception $e) {
    watchdog_exception('ammo', $e);
    drupal_set_message(t('De testgegevens konden niet worden verwijderd.'), 'error');
  }
  $form_state['redirect'] = 'admin/ammo/meeting/' . $meeting_id . '/test-data';
}

function ammo_test_data_delete($meeting_id) {
  $transaction = db_transaction();
  try {
    db_query('SELECT id FROM {ammo_meetings} WHERE id = :id FOR UPDATE', array(':id' => $meeting_id))->fetchField();
    $entries = db_query('SELECT entry_type, entry_id FROM {ammo_test_entries} WHERE meeting_id = :meeting', array(':meeting' => $meeting_id))->fetchAll();
    foreach ($entries as $entry) {
      $relations = db_query('SELECT id FROM {ammo_contact_relations} WHERE target_type = :type AND target_id = :id', array(':type' => $entry->entry_type, ':id' => $entry->entry_id))->fetchCol();
      if ($relations) {
        entity_delete_multiple('contact_relation', $relations);
      }
      entity_delete($entry->entry_type, $entry->entry_id);
    }
    db_delete('ammo_test_entries')->condition('meeting_id', $meeting_id)->execute();
    return count($entries);
  }
  catch (Exception $e) {
    $transaction->rollback();
    throw $e;
  }
}
