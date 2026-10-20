<?php

/**
 * Run with ddev drush scr. All fixtures and captured mail are rolled back.
 */
module_load_include('inc', 'ammo', 'ammo');
module_load_include('inc', 'ammo', 'document.form');
module_load_include('inc', 'ammo', 'amendments-pager');
module_load_include('inc', 'ammo', 'test-data');
module_load_include('inc', 'ammo', 'amendment.form');

function ammo_locators_assert($condition, $message) {
  if (!$condition) {
    throw new Exception($message);
  }
}

/**
 * The form errors of one locator editor validation, cleared afterwards.
 */
function ammo_locators_editor_errors(array $rows, $document) {
  drupal_static_reset('form_set_error');
  ammo_document_locators_validate($rows, $document);
  $errors = form_get_errors() ?: array();
  drupal_static_reset('form_set_error');
  drupal_get_messages('error');
  return $errors;
}

global $user, $conf;
$previous_user = $user;
$previous_conf = $conf;
$conf['mail_system'] = array('default-system' => 'TestingMailSystem', 'ammo' => 'TestingMailSystem');
$transaction = db_transaction();
try {
  $user = user_load(1);
  $meeting = ammo_save_entity('meeting', array('title' => 'Locator test', 'date' => '2000-01-01', 'hide_meeting' => 0, 'email' => 'ammo-test@example.invalid'));

  // Editor rows: a letter grouping, a numeric locator and optional free text.
  $rows = array(
    array('key' => '', 'label' => 'Toelichting', 'type' => 'text', 'min' => '1', 'max' => '9', 'grouping' => 0, 'required' => 0, 'weight' => 2),
    array('key' => '', 'label' => ' Deel ', 'type' => 'letter', 'min' => 'a', 'max' => 'f', 'grouping' => 1, 'required' => 0, 'weight' => 0),
    array('key' => '', 'label' => 'Artikel', 'type' => 'numeric', 'min' => '01', 'max' => '50', 'grouping' => 0, 'required' => 1, 'weight' => 1),
    array('key' => '', 'label' => '', 'type' => 'numeric', 'min' => '', 'max' => '', 'grouping' => 0, 'required' => 1, 'weight' => 3),
  );
  ammo_locators_assert(!ammo_locators_editor_errors($rows, NULL), 'Valid editor rows pass');
  $document = ammo_save_entity('document', array('title' => 'Locator document', 'meeting_id' => $meeting->id, 'locators' => ammo_document_locators_value($rows, NULL)));
  $locators = ammo_document_locators($document);
  ammo_locators_assert(array_keys($locators) === array('loc1', 'loc2', 'loc3'), 'Keys follow the chosen order and skip empty rows');
  ammo_locators_assert($locators['loc1']['label'] === 'Deel' && $locators['loc1']['min'] === 'A' && $locators['loc1']['max'] === 'F' && $locators['loc1']['required'], 'Grouping letter is normalized and required');
  ammo_locators_assert($locators['loc2']['min'] === '1' && $locators['loc3']['min'] === '' && !$locators['loc3']['required'], 'Numeric range normalized, text range dropped');
  ammo_locators_assert(!ammo_document_is_legacy($document) && !ammo_document_locators_locked($document), 'New document is configured and unlocked');

  // Editor validation.
  $bad = $rows;
  $bad[0]['grouping'] = 1;
  ammo_locators_assert(isset(ammo_locators_editor_errors($bad, NULL)['locators']), 'Two grouping locators rejected');
  $bad = $rows;
  $bad[1]['type'] = 'text';
  ammo_locators_assert(isset(ammo_locators_editor_errors($bad, NULL)['locators][1][grouping']), 'Free text cannot group');
  $bad = $rows;
  $bad[1]['min'] = 'G';
  ammo_locators_assert(isset(ammo_locators_editor_errors($bad, NULL)['locators][1][max']), 'Letter range order checked');
  $bad = $rows;
  $bad[2]['label'] = 'deel';
  ammo_locators_assert(isset(ammo_locators_editor_errors($bad, NULL)['locators][2][label']), 'Labels must be unique');

  // Amendment input validation.
  $errors = ammo_validate_locators($document, array('loc1' => 'g', 'loc2' => '0'));
  ammo_locators_assert(isset($errors['loc1'], $errors['loc2']) && !isset($errors['loc3']), 'Ranges checked, optional text may stay empty');
  $errors = ammo_validate_locators($document, array('loc1' => 'BB', 'loc2' => 'x', 'loc3' => str_repeat('a', 256)));
  ammo_locators_assert(count($errors) === 3 && strpos($errors['loc2'], 'Artikel') === 0, 'Formats checked with the label in the message');
  ammo_locators_assert(isset(ammo_validate_locators($document, array('loc2' => '3'))['loc1']), 'Grouping locator is required');
  ammo_locators_assert(!ammo_validate_locators($document, array('loc1' => ' b ', 'loc2' => '07')), 'Lowercase letter accepted');

  // Storage, numbering and display.
  $fields = ammo_amendment_locator_fields($document, array('loc1' => 'b', 'loc2' => '07', 'loc3' => ' Zie bijlage '));
  ammo_locators_assert($fields === array('locator_values' => '{"loc1":"B","loc2":"7","loc3":"Zie bijlage"}', 'group_value' => 'B'), 'Values stored normalized with the group');
  $save = function ($group, $article) use ($document) {
    $fields = ammo_amendment_locator_fields($document, array('loc1' => $group, 'loc2' => $article));
    return ammo_save_entity('amendment', $fields + array('document_id' => $document->id, 'state' => 'submitted', 'amendment_text' => 'Tekst', 'chapterized_id' => ammo_next_chapterized_id($document->id, $fields['group_value'])));
  };
  $b1 = $save('B', '10');
  $b2 = $save('B', '9');
  $a1 = $save('A', '3');
  $c1 = $save('C', '2');
  ammo_locators_assert(ammo_amendment_number($b1) === 'B.1' && ammo_amendment_number($b2) === 'B.2' && ammo_amendment_number($a1) === 'A.1', 'Numbered per letter group');
  ammo_locators_assert($b1->chapter === NULL && $b1->page === NULL, 'Legacy columns stay empty');
  ammo_locators_assert(ammo_amendment_location($b1, $locators) === 'Deel B, Artikel 10' && ammo_amendment_location($b1, $locators, FALSE, TRUE) === 'Artikel 10', 'Location text skips empty and grouping values');
  ammo_locators_assert(ammo_document_locators_locked($document), 'Document with amendments is locked');

  // Sorting by location and by number.
  $sorted = db_query('SELECT * FROM {ammo_amendments} WHERE document_id = :id', array(':id' => $document->id))->fetchAllAssoc('id');
  ammo_sort_amendments($sorted, $locators);
  ammo_locators_assert(array_keys($sorted) == array($a1->id, $b2->id, $b1->id, $c1->id), 'Location order sorts numbers numerically within a group');
  ammo_sort_amendments($sorted, $locators, FALSE);
  ammo_locators_assert(array_keys($sorted) == array($a1->id, $b1->id, $b2->id, $c1->id), 'Number order follows the sequence');
  $variables = array('values' => array('meeting' => $meeting->id, 'status' => array('submitted' => 'submitted')));
  template_preprocess_print_amendments_by_location($variables);
  $printed = $variables['meeting']['documents'][$document->id]['chapters'];
  ammo_locators_assert(array_keys($printed) === array('A', 'B', 'C') && array_keys($printed['B']['amendments']) == array($b2->id, $b1->id), 'Print groups by letter in location order');
  ammo_locators_assert($printed['B']['amendments'][$b1->id]['number'] === 'B.1' && $printed['B']['amendments'][$b1->id]['location'] === 'Deel B, Artikel 10', 'Print shows number and location');

  // Overview tabs by letter, without the legacy ungrouped bucket.
  $page = ammo_amendments_page($meeting->id, array('document' => $document->id, 'chapter' => 'b'));
  ammo_locators_assert($page['meeting']['selected_chapter'] === 'B', 'Lowercase group selection normalized');
  ammo_locators_assert(array_keys($page['meeting']['documents'][$document->id]['chapters']) === array('A', 'B', 'C'), 'Tabs sorted by letter');
  ammo_locators_assert(array_keys($page['meeting']['documents'][$document->id]['amendments']) == array($b1->id, $b2->id), 'Only the selected group loaded');
  ammo_locators_assert(strpos($page['chapter_navigation'], 'Deel:') !== FALSE && strpos($page['chapter_navigation'], 'Zonder') === FALSE, 'Tabs use the grouping label');

  // A changed group renumbers, and the change mail lists each locator.
  $original = clone entity_load_single('amendment', $b2->id);
  $form_state = array('values' => array(), 'storage' => array('document' => $document->id, 'meeting' => $meeting->id) + ammo_amendment_locator_fields($document, array('loc1' => 'C', 'loc2' => '9')));
  $moved = ammo_store('amendment', $b2->id, drupal_map_assoc(array('locator_values', 'group_value')), array(), $form_state);
  ammo_locators_assert(ammo_amendment_number($moved) === 'C.2', 'Moving to another group renumbers');
  $differences = ammo_locator_differences($document, $original, array('locator_values' => $moved->locator_values));
  ammo_locators_assert(array_keys($differences) === array('locator_loc1') && $differences['locator_loc1']['diff'] === 'B → C' && $differences['locator_loc1']['fieldname'] === 'Deel', 'Change mail lists the changed locator');

  // Lock: labels and ranges may change, structure may not.
  $stored_rows = array();
  foreach (array_values($locators) as $weight => $locator) {
    $stored_rows[] = array('key' => $locator['key'], 'label' => $locator['label'], 'type' => $locator['type'], 'min' => $locator['min'], 'max' => $locator['max'], 'grouping' => (int) $locator['grouping'], 'required' => (int) $locator['required'], 'weight' => $weight);
  }
  $edit = $stored_rows;
  $edit[0]['label'] = 'Onderdeel';
  $edit[1]['max'] = '60';
  ammo_locators_assert(!ammo_locators_editor_errors($edit, $document), 'Labels and ranges stay editable');
  $edit = $stored_rows;
  $edit[1]['type'] = 'text';
  ammo_locators_assert(ammo_locators_editor_errors($edit, $document), 'Type change rejected when locked');
  $edit = $stored_rows;
  $edit[2]['label'] = '';
  ammo_locators_assert(ammo_locators_editor_errors($edit, $document), 'Removal rejected when locked');
  $edit = $stored_rows;
  $edit[1]['type'] = 'text';
  $edit[0]['label'] = 'Onderdeel';
  $saved = json_decode(ammo_document_locators_value($edit, $document), TRUE);
  ammo_locators_assert($saved[1]['type'] === 'numeric' && $saved[0]['label'] === 'Onderdeel', 'Locked structure is kept on save');

  // Reordering locators keeps stored values byte-equal, so edits stay no-ops.
  $before = ammo_amendment_locator_fields($document, array('loc1' => 'A', 'loc2' => '3', 'loc3' => 'x'));
  $reordered = array_reverse($stored_rows);
  foreach ($reordered as $weight => &$row) {
    $row['weight'] = $weight;
  }
  unset($row);
  ammo_locators_assert(!ammo_locators_editor_errors($reordered, $document), 'Reordering allowed when locked');
  $document->locators = ammo_document_locators_value($reordered, $document);
  entity_save('document', $document);
  ammo_locators_assert(array_keys(ammo_document_locators($document)) === array('loc3', 'loc2', 'loc1'), 'New order saved');
  ammo_locators_assert(ammo_amendment_locator_fields($document, array('loc1' => 'A', 'loc2' => '3', 'loc3' => 'x')) === $before, 'Stored values do not depend on the order');

  // The wizard stores the locators in step 5 and saves them in step 6 without a change mail.
  $meeting->hide_state = 1;
  entity_save('meeting', $meeting);
  variable_set('drupal_test_email_collector', array());
  drupal_static_reset('form_set_error');
  $branches = ammo_test_data_branches();
  ammo_locators_assert($branches, 'Integration fixture requires an existing branch');
  $wizard = array(
    'step' => 5,
    'values' => array('loc1' => 'd', 'loc2' => '05', 'loc3' => '', 'amendment_text' => 'Wizardtekst', 'supplement' => '', 'submission_type' => 'submit'),
    'triggering_element' => array('#name' => 'action_button'),
    'storage' => array('first_step' => 1, 'last_step' => 7, 'meeting' => $meeting->id, 'meeting_display_name' => 'M', 'document' => $document->id, 'document_display_name' => 'D', 'branch_id' => key($branches), 'branch_display_name' => current($branches), 'branch_selected' => TRUE, 'destination' => array('destination' => 'ammo')),
  );
  ammo_amendment_form_validate(array(), $wizard);
  ammo_locators_assert(!form_get_errors(), 'Wizard step 5 validates');
  ammo_amendment_form_submit(array(), $wizard);
  ammo_locators_assert($wizard['step'] == 6 && $wizard['storage']['group_value'] === 'D' && $wizard['storage']['loc1'] === 'D', 'Step 5 stores normalized locators');
  $wizard['values'] = array('confirm' => 1);
  ammo_amendment_form_validate(array(), $wizard);
  ammo_amendment_form_submit(array(), $wizard);
  $created = entity_load_single('amendment', $wizard['storage']['amendment']);
  ammo_locators_assert($created->state === 'submitted' && $created->group_value === 'D' && ammo_amendment_number($created) === 'D.1' && $created->locator_values === '{"loc1":"D","loc2":"5"}', 'Wizard saves a numbered amendment');
  foreach (variable_get('drupal_test_email_collector', array()) as $mail) {
    ammo_locators_assert(strpos($mail['body'], '→') === FALSE, 'First submission sends no change mail');
  }
  drupal_get_messages();

  // Test data rotates the letter group and leaves optional text empty.
  $groups = array();
  for ($i = 0; $i < 8; $i++) {
    $input = ammo_test_data_locator_input($locators, $i);
    $groups[] = $input['loc1'];
    ammo_locators_assert(!ammo_validate_locators($document, $input) && $input['loc3'] === '', 'Test data input is valid');
  }
  ammo_locators_assert($groups === array('A', 'B', 'C', 'D', 'E', 'F', 'A', 'B'), 'Test data rotates within the letter range');

  // Legacy documents keep chapter, page and line.
  $legacy = ammo_save_entity('document', array('title' => 'Legacy document', 'meeting_id' => $meeting->id, 'chapter_min' => 1, 'chapter_max' => 7, 'line_max' => 500));
  $legacy_locators = ammo_document_locators($legacy);
  ammo_locators_assert(array_keys($legacy_locators) === array('chapter', 'page', 'line') && $legacy_locators['chapter']['grouping'], 'Legacy document uses chapter, page and line');
  ammo_locators_assert(array_keys(ammo_amendment_locator_fields($legacy, array('chapter' => '2', 'page' => '14', 'line' => '220'))) === array('chapter', 'page', 'line'), 'Legacy values go to their columns');
  $old = ammo_save_entity('amendment', array('document_id' => $legacy->id, 'chapter' => 2, 'chapterized_id' => 1, 'page' => 14, 'line' => 220, 'state' => 'submitted'));
  ammo_locators_assert(ammo_amendment_number($old) === '2.1' && ammo_amendment_location($old, $legacy_locators) === 'Hoofdstuk 2, pagina 14, regel 220', 'Legacy number and location unchanged');
  ammo_locators_assert(ammo_next_chapterized_id($legacy->id, '2') === 2, 'Legacy numbering still counts chapters');
  ammo_locators_assert(ammo_validate_document_ranges($legacy->id, 8, 1, 501) === array('chapter' => 'Hoofdstuk nr. mag maximaal 7 zijn.', 'line' => 'Regelnummer mag maximaal 500 zijn.'), 'Legacy range messages unchanged');

  $transaction->rollback();
  print "PASS: editor, validation, storage, numbering, location, sorting, print, overview tabs, renumbering, change diff, lock, test data and legacy fallback. Fixtures rolled back.\n";
}
catch (Throwable $e) {
  $transaction->rollback();
  throw $e;
}
finally {
  $user = $previous_user;
  $conf = $previous_conf;
}
