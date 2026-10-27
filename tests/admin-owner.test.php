<?php

/** Run with ddev drush scr; fixtures are rolled back. */
module_load_include('inc', 'ammo', 'ammo');
module_load_include('inc', 'ammo', 'meeting.form');
module_load_include('inc', 'ammo', 'branch-overview');

function ammo_admin_owner_assert($condition, $message) {
  if (!$condition) {
    throw new Exception($message);
  }
}

function ammo_admin_owner_test_owner($type, $id, $contact_id, $name) {
  return ammo_save_entity('contact_relation', array('target_id' => $id, 'target_type' => $type, 'relation_type' => 'owner', 'contact_type' => 'branch', 'contact_id' => $contact_id, 'branch_id' => $contact_id, 'contact_display_name' => $name, 'branch_display_name' => $name))->id;
}

function ammo_admin_owner_test_stored($relation_id) {
  return db_query('SELECT contact_display_name, branch_display_name FROM {ammo_contact_relations} WHERE id = :id', array(':id' => $relation_id))->fetchAssoc();
}

function ammo_admin_owner_test_names($type, $id) {
  entity_get_controller('contact_relation')->resetCache();
  $names = array();
  foreach (ammo_get_related_contacts($id, $type, 'owner') as $contact_id => $contact) {
    $names[$contact_id] = $contact['contact_display_name'] . '|' . $contact['branch_display_name'];
  }
  return $names;
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

  $admin_amendment = ammo_admin_owner_test_owner('amendment', $amendment->id, 0, 'de congrescommissie');
  $co_owner = ammo_admin_owner_test_owner('amendment', $amendment->id, 99999999, 'Testafdeling');
  // Submitting through the branch select passes the option label with its dash.
  $admin_motion = ammo_save_relation('owner', 'motion', $motion->id, 0, 'branch', '- de congrescommissie', 0, '- de congrescommissie')->id;
  $admin_other = ammo_admin_owner_test_owner('amendment', $other_amendment->id, 0, 'de congrescommissie');

  ammo_admin_owner_assert(ammo_admin_owner_test_stored($admin_motion) === array('contact_display_name' => 'de congrescommissie', 'branch_display_name' => 'de congrescommissie'), 'Admin owner is saved without the dash of the select');

  // The edit page gets the meeting from the entity cache, which saving updates in place.
  $loaded = entity_load_single('meeting', $meeting->id);
  $form_state = array();
  $form = ammo_meeting_edit_form(array(), $form_state, $loaded);
  ammo_admin_owner_assert(empty($form['admin_owner']['#disabled']), 'Admin owner can be changed');
  $form_state = array('meeting' => $loaded, 'values' => array('admin_owner' => 'het partijbestuur') + (array) $loaded);
  ammo_meeting_edit_form_submit($form, $form_state);
  drupal_get_messages();

  ammo_admin_owner_assert(ammo_admin_owner_test_names('amendment', $amendment->id) === array(0 => 'het partijbestuur|het partijbestuur', 99999999 => 'Testafdeling|Testafdeling'), 'Amendment shows the new admin owner and keeps its co-owner');
  ammo_admin_owner_assert(ammo_admin_owner_test_names('motion', $motion->id) === array(0 => 'het partijbestuur|het partijbestuur'), 'Motion shows the new admin owner');
  ammo_admin_owner_assert(ammo_admin_owner_test_names('amendment', $other_amendment->id) === array(0 => 'de congrescommissie|de congrescommissie'), 'Another meeting keeps its admin owner');
  $overview = ammo_branch_overview_items('amendment', $meeting->id, array(99999999));
  ammo_admin_owner_assert($overview[99999999]['co_submitted'][$amendment->id]['others'] === array(0 => 'het partijbestuur'), 'Branch overview shows the new admin owner');

  $transaction->rollback();
  print "PASS: the admin owner can be renamed and its proposals, the branch overview and only its own meeting show the new name. Fixtures rolled back.\n";
}
catch (Throwable $e) {
  $transaction->rollback();
  throw $e;
}
