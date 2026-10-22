<?php

/**
 * Run with ddev drush scr. All fixtures and captured mail are rolled back.
 */
module_load_include('inc', 'ammo', 'ammo');
module_load_include('inc', 'ammo', 'status-overview');
module_load_include('inc', 'ammo', 'status-overview.form');

function ammo_status_overview_test_assert($condition, $message) {
  if (!$condition) {
    throw new Exception($message);
  }
}

/**
 * Submits a bulk status change of the given items and returns the messages.
 */
function ammo_status_overview_test_apply(array $selection, array $keys, $state) {
  $form_state = array('values' => array('state' => $state, 'items' => drupal_map_assoc($keys)), 'ammo_selection' => $selection);
  ammo_status_overview_form_submit(array(), $form_state);
  return drupal_get_messages();
}

global $user, $conf;
$previous_user = $user;
$previous_conf = $conf;
$previous_get = $_GET;
$conf['mail_system'] = array('default-system' => 'TestingMailSystem', 'ammo' => 'TestingMailSystem');
$transaction = db_transaction();
try {
  $user = user_save(NULL, array('name' => 'ammo-status-test-' . uniqid(), 'mail' => 'ammo-test@example.invalid', 'status' => 1));
  $permissions = &drupal_static('user_access');
  $permissions[$user->uid] = array('administer amendments and motions' => TRUE);
  // Mail as with default settings, whatever the site has switched off.
  variable_del('ammo_mail_enabled');
  variable_del('ammo_mail_steps');
  $meeting = ammo_save_entity('meeting', array('title' => 'Status test', 'date' => '2099-01-01', 'amendment_submission_end' => '2099-01-01', 'motion_submission_end' => '2099-01-01', 'withdrawal_closing_date' => '2099-01-01', 'email' => 'secretariaat@example.invalid', 'hide_meeting' => 0));
  $document = ammo_save_entity('document', array('title' => 'Status document', 'meeting_id' => $meeting->id));
  $long = '<p>Een twee drie vier vijf zes zeven acht negen tien elf twaalf</p>';
  $first = ammo_save_entity('amendment', array('document_id' => $document->id, 'chapter' => 1, 'chapterized_id' => 1, 'page' => 3, 'line' => 10, 'amendment_text' => $long, 'state' => 'submitted', 'advice' => 'reject', 'advice_supplement' => "Niet nodig.\nZie artikel 4."));
  $second = ammo_save_entity('amendment', array('document_id' => $document->id, 'chapter' => 2, 'chapterized_id' => 1, 'page' => 5, 'line' => 1, 'amendment_text' => 'Kort', 'state' => 'submitted'));
  $motion = ammo_save_entity('motion', array('meeting_id' => $meeting->id, 'motion_id' => 7, 'consideration_body' => 'Overweging', 'state' => 'submitted'));
  ammo_save_entity('contact_relation', array('relation_type' => 'owner', 'target_type' => 'amendment', 'target_id' => $first->id, 'contact_type' => 'branch', 'contact_id' => 99999999, 'branch_id' => 99999999, 'contact_display_name' => 'Testafdeling', 'branch_display_name' => 'Testafdeling', 'contact_email' => 'owner@example.invalid'));

  // Filters: document, chapter and the motions.
  $all = ammo_status_overview_selection(array('meeting' => $meeting->id, 'document' => $document->id));
  ammo_status_overview_test_assert($all['documents'] === array($document->id => 'Status document', 'motions' => 'Moties') && array_keys($all['chapters']) === array(1, 2) && $all['chapter'] === '', 'Documents with Moties, and the chapters of the document');
  $rows = ammo_status_overview_rows($all);
  ammo_status_overview_test_assert(array_keys($rows) === array('amendment:' . $first->id, 'amendment:' . $second->id), 'All chapters by default, sorted');
  $chapter = ammo_status_overview_selection(array('meeting' => $meeting->id, 'document' => $document->id, 'chapter' => 2));
  ammo_status_overview_test_assert(array_keys(ammo_status_overview_rows($chapter)) === array('amendment:' . $second->id), 'The chapter filter limits the rows');
  $motions = ammo_status_overview_selection(array('meeting' => $meeting->id, 'document' => 'motions', 'chapter' => 2));
  ammo_status_overview_test_assert(!$motions['chapters'] && array_keys(ammo_status_overview_rows($motions)) === array('motion:' . $motion->id), 'Moties lists the motions without chapters');

  // Row content.
  $row = $rows['amendment:' . $first->id];
  ammo_status_overview_test_assert(strpos($row['number'], '>1.1</a>') !== FALSE && strpos($row['number'], ammo_item_view_path('amendment', $first->id)) !== FALSE, 'Number links to the proposal');
  ammo_status_overview_test_assert($row['location'] === '1 / 3 / 10', 'Location of a legacy document');
  ammo_status_overview_test_assert(strpos($row['text'], 'Een twee drie vier vijf zes zeven acht negen tien …</a>') !== FALSE && strpos($row['text'], 'elf') === FALSE, 'Preview of ten words, linked');
  ammo_status_overview_test_assert($row['first_owner'] === 'Testafdeling' && $rows['amendment:' . $second->id]['first_owner'] === '', 'Hoofdindiener, empty without owners');
  ammo_status_overview_test_assert(strpos($row['actions'], '>status/advies</a>') !== FALSE && strpos($row['actions'], '>indieners</a>') !== FALSE && strpos($row['actions'], 'destination=admin/ammo/status%3Fmeeting%3D' . $meeting->id) !== FALSE, 'The actions of the item, returning to the overview');
  ammo_status_overview_test_assert($rows['amendment:' . $second->id]['actions'] === '', 'No actions where the item itself shows none');
  ammo_status_overview_test_assert(strpos($row['actions'], 'ctools-dropbutton') !== FALSE && strpos($row['actions'], 'ctools-twisty') !== FALSE, 'The actions are a dropbutton');
  $now = format_date(REQUEST_TIME, 'custom', 'j F Y, H:i');
  ammo_status_overview_test_assert(entity_load_single('amendment', $first->id)->created == REQUEST_TIME && entity_load_single('amendment', $first->id)->changed == REQUEST_TIME && $row['created'] === $now && $row['changed'] === $now, 'Saves record the creation and change dates, shown in their columns');
  ammo_status_overview_test_assert($row['state'] === 'Ingediend' && $row['advice'] === 'Afwijzen' && $row['advice_supplement'] === "Niet nodig.<br />\nZie artikel 4.", 'Status, advice and the full advice explanation');

  // One page of 50 rows at a time.
  for ($i = 2; $i <= 51; $i++) {
    ammo_save_entity('amendment', array('document_id' => $document->id, 'chapter' => 3, 'chapterized_id' => $i, 'page' => 1, 'line' => $i, 'amendment_text' => 'Extra', 'state' => 'submitted'));
  }
  $_GET = array('q' => 'admin/ammo/status', 'page' => 1);
  $paged = ammo_status_overview_selection(array('meeting' => $meeting->id, 'document' => $document->id, 'page' => 1));
  ammo_status_overview_test_assert(count(ammo_status_overview_rows($paged)) === 2 && ammo_status_overview_query($paged)['page'] === 1, 'The second page holds the last rows and is kept in the query');
  $_GET = array('q' => 'admin/ammo/status');
  ammo_status_overview_test_assert(count(ammo_status_overview_rows($all)) === 50, 'The first page holds 50 rows');

  // The page: filters, bulk change and a checkbox per row.
  $_GET = array('q' => 'admin/ammo/status', 'meeting' => $meeting->id, 'document' => $document->id);
  $form = drupal_get_form('ammo_status_overview_form');
  ammo_status_overview_test_assert(isset($form['filters']['document'], $form['filters']['chapter'], $form['bulk']['state']) && !isset($form['bulk']['state']['#options']['blocked']) && $form['items']['#type'] === 'tableselect', 'Filters, a status dropdown without Verwijderd and a table with checkboxes');
  $html = drupal_render($form);
  ammo_status_overview_test_assert(substr_count($html, 'type="checkbox"') >= 2 && strpos($html, 'Advies toelichting') !== FALSE, 'The page renders the rows');

  // Validation.
  $form_state = array('values' => array('state' => '', 'items' => array('amendment:' . $first->id => 0)));
  ammo_status_overview_form_validate(array(), $form_state);
  $errors = form_get_errors();
  form_clear_error();
  drupal_get_messages();
  ammo_status_overview_test_assert(isset($errors['state'], $errors['items']), 'A status and a selection are required');

  // A bulk change saves the ticked items only and mails like the status/advies form.
  variable_set('drupal_test_email_collector', array());
  ammo_status_overview_test_apply($all, array('amendment:' . $first->id), 'changed');
  entity_get_controller('amendment')->resetCache();
  ammo_status_overview_test_assert(entity_load_single('amendment', $first->id)->state === 'changed' && entity_load_single('amendment', $second->id)->state === 'submitted', 'Only the ticked item changes');
  db_update('ammo_amendments')->fields(array('created' => 1000))->condition('id', $first->id)->execute();
  entity_get_controller('amendment')->resetCache();
  ammo_save_entity('amendment', array('supplement' => 'Later'), $first->id);
  entity_get_controller('amendment')->resetCache();
  ammo_status_overview_test_assert(entity_load_single('amendment', $first->id)->created == 1000, 'A later save keeps the creation date');
  $mails = variable_get('drupal_test_email_collector', array());
  $recipients = array_map(function ($mail) { return $mail['to']; }, $mails);
  ammo_status_overview_test_assert(in_array('owner@example.invalid', $recipients, TRUE) && in_array('secretariaat@example.invalid', $recipients, TRUE) && strpos($mails[0]['body'], 'Ingediend → Gewijzigd') !== FALSE, 'The change is mailed to the owners and the meeting');

  // Beoordeeld needs an advice; an unchanged status is skipped.
  variable_set('drupal_test_email_collector', array());
  $messages = ammo_status_overview_test_apply($all, array('amendment:' . $first->id, 'amendment:' . $second->id), 'assessed');
  entity_get_controller('amendment')->resetCache();
  ammo_status_overview_test_assert(entity_load_single('amendment', $first->id)->state === 'assessed' && entity_load_single('amendment', $second->id)->state === 'submitted', 'Beoordeeld only for the item with an advice');
  ammo_status_overview_test_assert(strpos(implode(' ', $messages['error']), 'geen advies: 2.1') !== FALSE, 'The item without advice is reported');
  $messages = ammo_status_overview_test_apply($all, array('amendment:' . $first->id), 'assessed');
  ammo_status_overview_test_assert(strpos(implode(' ', $messages['status']), 'Had deze status al: 1.1') !== FALSE, 'An unchanged status is skipped');

  // Items of another meeting are ignored.
  $other = ammo_save_entity('motion', array('meeting_id' => $meeting->id + 1000, 'motion_id' => 1, 'state' => 'submitted'));
  ammo_status_overview_test_apply($all, array('motion:' . $other->id), 'changed');
  entity_get_controller('motion')->resetCache();
  ammo_status_overview_test_assert(entity_load_single('motion', $other->id)->state === 'submitted', 'Items outside the meeting on screen are not changed');

  // Access: admins until the meeting date, superadmins always.
  ammo_status_overview_test_assert(menu_get_item('admin/ammo/status')['access'] && ammo_status_overview_edit_access($meeting->id), 'Admins can open the page and change statuses');
  $meeting->date = '2000-01-01';
  entity_save('meeting', $meeting);
  entity_get_controller('meeting')->resetCache();
  drupal_static_reset('ammo_get_form_cache');
  $form = drupal_get_form('ammo_status_overview_form');
  ammo_status_overview_test_assert(!ammo_status_overview_edit_access($meeting->id) && isset($form['closed']) && !isset($form['bulk']) && $form['items']['#theme'] === 'table', 'After the meeting date the page is read-only for admins');
  $permissions[$user->uid]['administer ammo'] = TRUE;
  ammo_status_overview_test_assert(ammo_status_overview_edit_access($meeting->id), 'Superadmins can still change statuses');
  $permissions[$user->uid] = array('add amendments and motions' => TRUE);
  drupal_static_reset('menu_get_item');
  ammo_status_overview_test_assert(!menu_get_item('admin/ammo/status')['access'], 'Without admin rights the page is closed');

  $transaction->rollback();
  print "PASS: document, chapter and motion filters, row content with submitter and actions, paging, page, validation, bulk change with mail, Beoordeeld needs an advice, unchanged and foreign items skipped, read-only after the meeting date and access. Fixtures rolled back.\n";
}
catch (Throwable $e) {
  $transaction->rollback();
  throw $e;
}
finally {
  $user = $previous_user;
  $conf = $previous_conf;
  $_GET = $previous_get;
}
