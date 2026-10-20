<?php

/**
 * Local integration test: ddev drush scr <path-to-this-file>.
 * Fixtures and captured mail are rolled back; no external mail is sent.
 */
module_load_include('inc', 'ammo', 'ammo');
module_load_include('inc', 'ammo', 'amendment.form');
module_load_include('inc', 'ammo', 'meeting.form');

function ammo_edit_test_assert($condition, $message) {
  if (!$condition) {
    throw new Exception($message);
  }
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
  $branches[99999999] = array(99999999 => 'Testafdeling');
  $meeting = ammo_save_entity('meeting', array('title' => 'AMMO edit test', 'date' => '2000-01-01', 'submission_closing_date' => '1999-01-01', 'withdrawal_closing_date' => '1999-01-01', 'email' => 'ammo-test@example.invalid', 'allow_submitted_amendment_edit' => 1));
  $document = ammo_save_entity('document', array('title' => 'Testdocument', 'meeting_id' => $meeting->id));
  $original = ammo_save_entity('amendment', array('document_id' => $document->id, 'chapter' => 1, 'chapterized_id' => 1, 'page' => 1, 'line' => 1, 'amendment_text' => 'Oude tekst', 'supplement' => 'Toelichting', 'state' => 'assessed', 'advice' => 'accept', 'advice_supplement' => 'Advies', 'state_supplement' => 'Beoordeling'));
  $owner = ammo_save_entity('contact_relation', array('relation_type' => 'owner', 'target_type' => 'amendment', 'target_id' => $original->id, 'contact_type' => 'branch', 'contact_id' => 99999999, 'branch_id' => 99999999, 'contact_display_name' => 'Testafdeling', 'branch_display_name' => 'Testafdeling', 'contact_email' => 'owner@example.invalid'));
  ammo_edit_test_assert(ammo_add_access(), 'Fixture must have submission permission.');
  ammo_edit_test_assert(ammo_access(99999999), 'Fixture must have branch access.');
  ammo_edit_test_assert(ammo_get_first_owner('amendment', $original->id), 'Fixture must have a primary owner.');
  ammo_edit_test_assert(ammo_edit_access($meeting->id, 99999999, 'amendment', $original->id), 'Past dates and assessed state must allow editing.');
  ammo_edit_test_assert(!ammo_edit_access($meeting->id, 123, 'amendment', $original->id), 'Other branch must be denied.');
  foreach (array('withdrawn', 'blocked') as $state) {
    $original->state = $state;
    entity_save('amendment', $original);
    ammo_edit_test_assert(!ammo_edit_access($meeting->id, 99999999, 'amendment', $original->id), 'Closed state must be denied.');
  }
  $original->state = 'assessed';
  entity_save('amendment', $original);
  $form_state = array();
  ammo_edit_test_assert(ammo_populate_form('amendment', $original->id, $form_state), 'Edit form must populate.');
  $storage = $form_state['storage'];
  $form_state['storage']['first_step'] = 5;
  $form_state['step'] = 5;
  $form = ammo_amendment_form(array(), $form_state);
  ammo_edit_test_assert(isset($form['amendment_text']), 'Edit form must remain available after the deadline.');
  $form_state['step'] = 6;
  $form = ammo_amendment_form(array(), $form_state);
  ammo_edit_test_assert(strpos($form['confirm']['#title'], 'beoordeling vervallen') !== FALSE, 'Confirmation must explain assessment reset.');
  ammo_edit_test_assert(ammo_save_submitted_amendment($storage) === NULL, 'Unchanged form must be a no-op.');
  $meeting->allow_submitted_amendment_edit = 0;
  entity_save('meeting', $meeting);
  ammo_edit_test_assert(!ammo_edit_access($meeting->id, 99999999, 'amendment', $original->id), 'Checkbox off must deny editing.');
  ammo_edit_test_assert(ammo_save_submitted_amendment($storage) === FALSE, 'An open form must respect disabled checkbox.');
  drupal_get_messages('error');
  $meeting->allow_submitted_amendment_edit = 1;
  entity_save('meeting', $meeting);
  $backer = ammo_save_entity('contact_relation', array('relation_type' => 'backer', 'target_type' => 'amendment', 'target_id' => $original->id, 'contact_type' => 'member', 'contact_id' => 99999998, 'contact_display_name' => 'Testlid', 'contact_email' => 'backer@example.invalid'));
  $storage['amendment_text'] = 'Gewijzigde tekst';
  $saved = ammo_save_submitted_amendment($storage);
  ammo_edit_test_assert($saved && $saved->id == $original->id && $saved->state === 'submitted' && $saved->advice === '' && $saved->state_supplement === '', 'Backer alone must not split; assessment must reset.');
  ammo_edit_test_assert(entity_load_single('contact_relation', $backer->id)->target_id == $original->id, 'Backer must remain.');
  ammo_edit_test_assert(ammo_save_submitted_amendment($storage) === FALSE, 'Stale submission must fail.');
  drupal_get_messages('error');
  $coowner = ammo_save_entity('contact_relation', array('relation_type' => 'owner', 'target_type' => 'amendment', 'target_id' => $original->id, 'contact_type' => 'branch', 'contact_id' => 99999997, 'branch_id' => 99999997, 'contact_display_name' => 'Mede-indiener', 'branch_display_name' => 'Mede-indiener', 'contact_email' => 'coowner@example.invalid'));
  $coowner2 = clone $coowner;
  unset($coowner2->id);
  $coowner2->is_new = TRUE;
  $coowner2->contact_id = 99999996;
  $coowner2->branch_id = 99999996;
  entity_save('contact_relation', $coowner2);
  $form_state = array();
  ammo_populate_form('amendment', $original->id, $form_state);
  $storage = $form_state['storage'];
  $storage['amendment_text'] = 'Afgesplitste tekst';
  variable_set('drupal_test_email_collector', array());
  $split = ammo_save_submitted_amendment($storage);
  ammo_edit_test_assert($split && $split->id != $original->id && $split->chapterized_id == 2, 'Coowners must cause a newly numbered proposal.');
  $old = entity_load_single('amendment', $original->id);
  ammo_edit_test_assert($old->amendment_text === 'Gewijzigde tekst' && $old->state === 'submitted', 'Old proposal must remain intact.');
  ammo_edit_test_assert(entity_load_single('contact_relation', $owner->id)->target_id == $split->id && entity_load_single('contact_relation', $coowner->id)->target_id == $original->id, 'Only primary owner must move.');
  $mails = variable_get('drupal_test_email_collector', array());
  ammo_edit_test_assert(count($mails) === 1 && $mails[0]['to'] === 'coowner@example.invalid', 'Only coowners receive deduplicated replacement mail.');
  ammo_edit_test_assert(strpos($mails[0]['body'], 'ammo/amendment/view/' . $old->id) !== FALSE && strpos($mails[0]['body'], 'ammo/amendment/view/' . $split->id) !== FALSE, 'Mail must link both proposals.');
  ammo_edit_test_assert(ammo_save_submitted_amendment($storage) === FALSE, 'Duplicate split must fail.');
  drupal_get_messages('error');
  $permissions[$user->uid] = array();
  ammo_edit_test_assert(!ammo_edit_access($meeting->id, 99999999, 'amendment', $split->id), 'Submission permission is required.');
  $permissions[$user->uid] = array('administer ammo' => TRUE);
  ammo_edit_test_assert(ammo_edit_access($meeting->id, 99999999, 'amendment', $old->id), 'Admins retain access.');
  $edit_state = array();
  ammo_populate_form('amendment', $old->id, $edit_state);
  ammo_edit_test_assert(empty($edit_state['storage']['submitted_edit']), 'Admins must bypass splitting.');
  print "PASS: access, deadlines, no-op, assessment reset, member support, split, numbering, ownership, mail, stale and duplicate saves, admin bypass.\n";
}
finally {
  $transaction->rollback();
  $user = $previous_user;
  $conf = $previous_conf;
}
