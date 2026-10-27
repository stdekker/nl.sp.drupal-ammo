<?php

/** Run with ddev drush scr; fixtures are rolled back. */
module_load_include('inc', 'ammo', 'ammo');
module_load_include('inc', 'ammo', 'meeting.form');

function ammo_admin_owner_assert($condition, $message) {
  if (!$condition) {
    throw new Exception($message);
  }
}

function ammo_admin_owner_test_owner($type, $id, $contact_id, $name) {
  return ammo_save_entity('contact_relation', array('target_id' => $id, 'target_type' => $type, 'relation_type' => 'owner', 'contact_type' => 'branch', 'contact_id' => $contact_id, 'branch_id' => $contact_id, 'contact_display_name' => $name, 'branch_display_name' => $name))->id;
}

function ammo_admin_owner_test_names($relation_id) {
  return db_query('SELECT contact_display_name, branch_display_name FROM {ammo_contact_relations} WHERE id = :id', array(':id' => $relation_id))->fetchAssoc();
}

$transaction = db_transaction();
try {
  $meeting_values = array('date' => '2099-01-01', 'email' => 'ammo-test@example.invalid', 'amendment_withdraw_end' => '2099-01-01', 'motion_withdraw_end' => '2099-01-01', 'amendment_support_withdraw_end' => '2099-01-01', 'motion_support_withdraw_end' => '2099-01-01', 'amendment_vote_request_end' => '2099-01-01', 'motion_vote_request_end' => '2099-01-01', 'motion_submission_end' => '2099-01-01', 'description' => '', 'hide_meeting' => 0, 'hide_state' => 0, 'hide_advice' => 0, 'disable_confirmation_checkbox' => 0, 'admin_owner' => 'de congrescommissie');
  $meeting = ammo_save_entity('meeting', array('title' => 'Admin owner test ' . uniqid()) + $meeting_values);
  $other = ammo_save_entity('meeting', array('title' => 'Admin owner other ' . uniqid()) + $meeting_values);
  $document = ammo_save_entity('document', array('title' => 'Admin owner document', 'meeting_id' => $meeting->id));
  $other_document = ammo_save_entity('document', array('title' => 'Admin owner other document', 'meeting_id' => $other->id));
  $amendment = ammo_save_entity('amendment', array('document_id' => $document->id, 'chapter' => 1, 'chapterized_id' => 1, 'page' => 1, 'line' => 1, 'state' => 'submitted'));
  $motion = ammo_save_entity('motion', array('meeting_id' => $meeting->id, 'motion_id' => 1, 'state' => 'submitted'));
  $other_amendment = ammo_save_entity('amendment', array('document_id' => $other_document->id, 'chapter' => 1, 'chapterized_id' => 1, 'page' => 1, 'line' => 1, 'state' => 'submitted'));

  $admin_amendment = ammo_admin_owner_test_owner('amendment', $amendment->id, 0, '- de congrescommissie');
  $co_owner = ammo_admin_owner_test_owner('amendment', $amendment->id, 99999999, 'Testafdeling');
  $admin_motion = ammo_admin_owner_test_owner('motion', $motion->id, 0, '- de congrescommissie');
  $admin_other = ammo_admin_owner_test_owner('amendment', $other_amendment->id, 0, '- de congrescommissie');

  $form_state = array();
  $form = ammo_meeting_edit_form(array(), $form_state, $meeting);
  ammo_admin_owner_assert(empty($form['admin_owner']['#disabled']), 'Admin owner can be changed');
  $form_state = array('meeting' => $meeting, 'values' => array('admin_owner' => 'het partijbestuur') + (array) $meeting);
  ammo_meeting_edit_form_submit($form, $form_state);
  drupal_get_messages();

  $renamed = array('contact_display_name' => '- het partijbestuur', 'branch_display_name' => '- het partijbestuur');
  ammo_admin_owner_assert(entity_load_single('meeting', $meeting->id)->admin_owner === 'het partijbestuur', 'Meeting keeps the new name');
  ammo_admin_owner_assert(ammo_admin_owner_test_names($admin_amendment) === $renamed, 'Admin owner of an amendment is renamed');
  ammo_admin_owner_assert(ammo_admin_owner_test_names($admin_motion) === $renamed, 'Admin owner of a motion is renamed');
  ammo_admin_owner_assert(ammo_admin_owner_test_names($co_owner) === array('contact_display_name' => 'Testafdeling', 'branch_display_name' => 'Testafdeling'), 'Branch co-owner keeps its name');
  ammo_admin_owner_assert(ammo_admin_owner_test_names($admin_other) === array('contact_display_name' => '- de congrescommissie', 'branch_display_name' => '- de congrescommissie'), 'Admin owner of another meeting keeps its name');

  $transaction->rollback();
  print "PASS: renaming the admin owner updates the admin-owned amendments and motions of the meeting only. Fixtures rolled back.\n";
}
catch (Throwable $e) {
  $transaction->rollback();
  throw $e;
}
