<?php

/**
 * Local integration test: ddev drush scr <path-to-this-file>.
 * Fixtures and captured mail are rolled back; no external mail is sent.
 */
module_load_include('inc', 'ammo', 'ammo');
module_load_include('inc', 'ammo', 'amendment.form');
module_load_include('inc', 'ammo', 'motion.form');

function ammo_edit_test_assert($condition, $message) {
  if (!$condition) {
    throw new Exception($message);
  }
}

/**
 * Opens or closes the change range of a type.
 */
function ammo_edit_test_change_range($meeting, $type, $open) {
  $meeting->{$type . '_change_start'} = NULL;
  $meeting->{$type . '_change_end'} = $open ? '2099-01-01 00:00:00' : NULL;
  entity_save('meeting', $meeting);
}

global $user, $conf;
$previous_user = $user;
$previous_conf = $conf;
$conf['mail_system'] = array('default-system' => 'TestingMailSystem', 'ammo' => 'TestingMailSystem');
$transaction = db_transaction();
try {
  $user = user_save(NULL, array('name' => 'ammo-edit-test-' . uniqid(), 'mail' => 'ammo-test@example.invalid', 'status' => 1, 'field_contact_id' => array(LANGUAGE_NONE => array(array('value' => 99999999)))));
  $permissions = &drupal_static('user_access');
  $permissions[$user->uid] = array('add amendments and motions' => TRUE);
  $branches = &drupal_static('ammo_get_afdelingen');
  $branches[99999999] = array(99999999 => 'Testafdeling', 99999997 => 'Mede-indiener');
  // Submission closed, changes open: only the change range matters for submitted items.
  $meeting = ammo_save_entity('meeting', array('title' => 'AMMO edit test', 'date' => '2099-02-01', 'amendment_submission_end' => '1999-01-01', 'motion_submission_end' => '1999-01-01', 'amendment_change_end' => '2099-01-01', 'motion_change_end' => '2099-01-01', 'withdrawal_closing_date' => '1999-01-01', 'email' => 'ammo-test@example.invalid'));
  $document = ammo_save_entity('document', array('title' => 'Testdocument', 'meeting_id' => $meeting->id));
  $original = ammo_save_entity('amendment', array('document_id' => $document->id, 'chapter' => 1, 'chapterized_id' => 1, 'page' => 1, 'line' => 1, 'amendment_text' => 'Oude tekst', 'supplement' => 'Toelichting', 'state' => 'assessed', 'advice' => 'accept', 'advice_supplement' => 'Advies', 'state_supplement' => 'Beoordeling'));
  $owner = ammo_save_entity('contact_relation', array('relation_type' => 'owner', 'target_type' => 'amendment', 'target_id' => $original->id, 'contact_type' => 'branch', 'contact_id' => 99999999, 'branch_id' => 99999999, 'contact_display_name' => 'Testafdeling', 'branch_display_name' => 'Testafdeling', 'contact_email' => 'owner@example.invalid'));
  $coowner = ammo_save_entity('contact_relation', array('relation_type' => 'owner', 'target_type' => 'amendment', 'target_id' => $original->id, 'contact_type' => 'branch', 'contact_id' => 99999997, 'branch_id' => 99999997, 'contact_display_name' => 'Mede-indiener', 'branch_display_name' => 'Mede-indiener', 'contact_email' => 'coowner@example.invalid'));
  ammo_edit_test_assert(ammo_add_access() && ammo_access(99999999) && ammo_access(99999997), 'Fixture must have submission permission and both branches.');

  ammo_edit_test_assert(ammo_edit_access($meeting->id, 99999999, 'amendment', $original->id), 'Owner may change inside the change range, also when assessed.');
  ammo_edit_test_assert(!ammo_edit_access($meeting->id, 99999997, 'amendment', $original->id), 'A co-submitting branch may not change it.');
  ammo_edit_test_assert(!ammo_edit_access($meeting->id, 123, 'amendment', $original->id), 'Another branch may not change it.');
  foreach (array('withdrawn', 'blocked') as $state) {
    $original->state = $state;
    entity_save('amendment', $original);
    ammo_edit_test_assert(!ammo_edit_access($meeting->id, 99999999, 'amendment', $original->id), 'Closed state ' . $state . ' must be denied.');
  }
  $original->state = 'assessed';
  entity_save('amendment', $original);

  $form_state = array();
  ammo_edit_test_assert(ammo_populate_form('amendment', $original->id, $form_state), 'Edit form must populate.');
  $storage = $form_state['storage'];
  $form_state['storage']['first_step'] = 5;
  $form_state['step'] = 6;
  $form = ammo_amendment_form(array(), $form_state);
  ammo_edit_test_assert(strpos($form['confirm']['#title'], 'Gewijzigd') !== FALSE && strpos($form['confirm']['#title'], 'mede-indieners') !== FALSE, 'Confirmation explains the new status and the mail.');
  ammo_edit_test_assert(ammo_save_submitted_edit('amendment', $storage) === NULL, 'Unchanged form must be a no-op.');

  ammo_edit_test_change_range($meeting, 'amendment', FALSE);
  ammo_edit_test_assert(!ammo_edit_access($meeting->id, 99999999, 'amendment', $original->id), 'An empty change range denies changes.');
  ammo_edit_test_assert(ammo_save_submitted_edit('amendment', $storage) === FALSE, 'An open form must respect the closed range.');
  drupal_get_messages('error');
  ammo_edit_test_change_range($meeting, 'amendment', TRUE);

  variable_set('drupal_test_email_collector', array());
  $storage['amendment_text'] = 'Gewijzigde tekst';
  $saved = ammo_save_submitted_edit('amendment', $storage);
  ammo_edit_test_assert($saved && $saved->id == $original->id && $saved->chapterized_id == 1, 'The change is made in place and keeps its number.');
  ammo_edit_test_assert($saved->state === 'changed' && $saved->advice === 'accept' && $saved->advice_supplement === 'Advies', 'The status becomes Gewijzigd and the advice is kept.');
  ammo_edit_test_assert($saved->state_supplement === '', 'The explanation of the old status is cleared.');
  ammo_edit_test_assert(entity_load_single('contact_relation', $owner->id)->target_id == $original->id && entity_load_single('contact_relation', $coowner->id)->target_id == $original->id, 'Owner and co-submitter stay attached.');
  $recipients = array_map(function ($mail) { return $mail['to']; }, variable_get('drupal_test_email_collector', array()));
  ammo_edit_test_assert(in_array('coowner@example.invalid', $recipients, TRUE) && in_array('owner@example.invalid', $recipients, TRUE), 'The change mail reaches the owner and the co-submitter.');
  ammo_edit_test_assert(ammo_save_submitted_edit('amendment', $storage) === FALSE, 'Stale submission must fail.');
  drupal_get_messages();

  // A changed item can be changed again, and its new location renumbers it within the new group.
  $form_state = array();
  ammo_populate_form('amendment', $original->id, $form_state);
  $storage = $form_state['storage'];
  $storage['chapter'] = 3;
  $saved = ammo_save_submitted_edit('amendment', $storage);
  ammo_edit_test_assert($saved && $saved->id == $original->id && ammo_amendment_number($saved) === '3.1', 'A change of group renumbers in place.');
  drupal_get_messages();

  // Motions follow the same rules.
  $motion = ammo_save_entity('motion', array('meeting_id' => $meeting->id, 'motion_id' => 4, 'consideration_opening' => 'Overwegende dat', 'consideration_body' => 'Oude overweging', 'follow_up_opening' => 'Verzoekt het congres', 'follow_up_body' => 'Iets te doen', 'state' => 'submitted', 'advice' => 'reject'));
  ammo_save_entity('contact_relation', array('relation_type' => 'owner', 'target_type' => 'motion', 'target_id' => $motion->id, 'contact_type' => 'branch', 'contact_id' => 99999999, 'branch_id' => 99999999, 'contact_display_name' => 'Testafdeling', 'branch_display_name' => 'Testafdeling', 'contact_email' => 'owner@example.invalid'));
  ammo_save_entity('contact_relation', array('relation_type' => 'owner', 'target_type' => 'motion', 'target_id' => $motion->id, 'contact_type' => 'branch', 'contact_id' => 99999997, 'branch_id' => 99999997, 'contact_display_name' => 'Mede-indiener', 'branch_display_name' => 'Mede-indiener', 'contact_email' => 'coowner@example.invalid'));
  ammo_edit_test_assert(ammo_edit_access($meeting->id, 99999999, 'motion', $motion->id) && !ammo_edit_access($meeting->id, 99999997, 'motion', $motion->id), 'Only the submitting branch may change a motion.');
  ammo_edit_test_change_range($meeting, 'motion', FALSE);
  ammo_edit_test_assert(!ammo_edit_access($meeting->id, 99999999, 'motion', $motion->id), 'An empty motion change range denies changes.');
  ammo_edit_test_change_range($meeting, 'motion', TRUE);

  $form_state = array('build_info' => array('args' => array('edit', $motion->id)), 'no_cache' => TRUE);
  $form = drupal_build_form('ammo_motion_form', $form_state);
  ammo_edit_test_assert(isset($form['consideration_text']) && !empty($form['confirm']['#required']), 'Motion edit form opens with a required confirmation.');
  // The form's last step ends in an AJAX redirect that exits, so save as its step 4 does.
  $edit = array('consideration_text' => 'Nieuwe overweging') + $form_state['storage'];
  variable_set('drupal_test_email_collector', array());
  ammo_edit_test_assert(ammo_save_submitted_edit('motion', $edit) !== FALSE, 'Motion change is saved.');
  entity_get_controller('motion')->resetCache();
  $changed = entity_load_single('motion', $motion->id);
  ammo_edit_test_assert($changed->consideration_body === 'Nieuwe overweging' && $changed->state === 'changed' && $changed->advice === 'reject', 'Motion is changed in place to Gewijzigd with its advice kept.');
  $recipients = array_map(function ($mail) { return $mail['to']; }, variable_get('drupal_test_email_collector', array()));
  ammo_edit_test_assert(in_array('coowner@example.invalid', $recipients, TRUE), 'The motion co-submitter gets the change mail.');
  drupal_get_messages();

  $permissions[$user->uid] = array();
  ammo_edit_test_assert(!ammo_edit_access($meeting->id, 99999999, 'amendment', $original->id), 'Submission permission is required.');
  $permissions[$user->uid] = array('administer ammo' => TRUE);
  ammo_edit_test_change_range($meeting, 'amendment', FALSE);
  ammo_edit_test_assert(ammo_edit_access($meeting->id, 99999999, 'amendment', $original->id), 'Admins are not bound by the ranges.');
  $edit_state = array();
  ammo_populate_form('amendment', $original->id, $edit_state);
  ammo_edit_test_assert(empty($edit_state['storage']['submitted_edit']), 'Admins edit through the normal save.');
  print "PASS: owner-only access, change ranges, in-place change to Gewijzigd with advice kept, numbering, mail to co-submitters, stale saves, motions, admin bypass.\n";
}
finally {
  $transaction->rollback();
  $user = $previous_user;
  $conf = $previous_conf;
}
