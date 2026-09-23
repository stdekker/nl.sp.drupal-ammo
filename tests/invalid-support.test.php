<?php

/** Run with ddev drush scr; fixtures are rolled back. */
module_load_include('inc', 'ammo', 'ammo');
module_load_include('inc', 'ammo', 'support.form');

function ammo_invalid_support_assert($condition, $message) {
  if (!$condition) {
    throw new Exception($message);
  }
}

global $user;
$previous_user = $user;
$previous_path = $_GET['q'];
$transaction = db_transaction();
try {
  $user = user_save(NULL, array('name' => 'ammo-invalid-support-' . uniqid(), 'status' => 1, 'field_contact_id' => array(LANGUAGE_NONE => array(array('value' => 99999999)))));
  $permissions = &drupal_static('user_access');
  $permissions[$user->uid] = array('add amendments and motions' => TRUE);
  $meeting = ammo_save_entity('meeting', array('title' => 'Invalid support test', 'date' => '2099-01-01', 'withdrawal_closing_date' => '2099-01-01'));
  $document = ammo_save_entity('document', array('title' => 'Invalid support document', 'meeting_id' => $meeting->id));
  $amendment = ammo_save_entity('amendment', array('document_id' => $document->id, 'chapter' => 1, 'state' => 'submitted'));
  ammo_invalid_support_assert(ammo_support_add_allowed('add', 'amendment', $amendment->id), 'Submitted amendment accepts support');
  $state = array();
  $form = ammo_support_form(array(), $state, 'add', 'member', 'amendment', $amendment->id);
  ammo_invalid_support_assert(isset($form['submit']), 'Member can open support form before status change');
  foreach (array('invalid', 'rejected') as $blocked_state) {
    $amendment->state = $blocked_state;
    entity_save('amendment', $amendment);
    ammo_invalid_support_assert(!ammo_support_add_allowed('add', 'amendment', $amendment->id), $blocked_state . ' amendment rejects new support');
    ammo_invalid_support_assert(ammo_support_add_allowed('withdraw', 'amendment', $amendment->id), 'Withdrawal remains allowed');
    ammo_invalid_support_assert(ammo_support_add_allowed('add', 'motion', $amendment->id), 'Motion access unchanged');
    foreach (array('member', 'branch') as $contact_type) {
      $state = array();
      $form = ammo_support_form(array(), $state, 'add', $contact_type, 'amendment', $amendment->id);
      ammo_invalid_support_assert(empty($form['submit']), 'Direct ' . $contact_type . ' URL has no submit button for ' . $blocked_state);
      $_GET['q'] = 'ammo/support/add/' . $contact_type . '/amendment/' . $amendment->id;
      $state['values'] = array('support' => 1, 'branch_id' => array(99999999 => 99999999));
      ammo_support_form_validate(array(), $state);
      ammo_support_form_submit(array(), $state);
      ammo_invalid_support_assert(!db_query('SELECT COUNT(*) FROM {ammo_contact_relations} WHERE target_type = :type AND target_id = :id', array(':type' => 'amendment', ':id' => $amendment->id))->fetchField(), 'No ' . $contact_type . ' relation saved for ' . $blocked_state);
    }
  }
  form_clear_error();
  drupal_get_messages('error');
  ammo_save_entity('contact_relation', array('target_id' => $amendment->id, 'target_type' => 'amendment', 'relation_type' => 'owner', 'contact_type' => 'branch', 'contact_id' => 99999998, 'branch_id' => 99999998, 'contact_display_name' => 'Bestaande indiener', 'branch_display_name' => 'Bestaande indiener'));
  $branches = &drupal_static('ammo_get_afdelingen');
  $branches[99999999] = array(99999999 => 'Andere afdeling');
  foreach (array('invalid', 'rejected') as $blocked_state) {
    $amendment->state = $blocked_state;
    entity_save('amendment', $amendment);
    $blocked_markup = theme('amendment', array('entity_id' => $amendment->id));
    ammo_invalid_support_assert(strpos($blocked_markup, 'mede indienen') === FALSE, $blocked_state . ' amendment hides co-submission link');
    if ($blocked_state === 'invalid') {
      ammo_invalid_support_assert(strpos($blocked_markup, ' - Ongeldig</span>') !== FALSE, 'Invalid status visible in header');
      ammo_invalid_support_assert(strpos($blocked_markup, 'class="amendment-details-toggle"') !== FALSE && strpos($blocked_markup, 'class="amendment-content-collapsed"') !== FALSE, 'Invalid details collapsed with toggle');
    }
    else {
      ammo_invalid_support_assert(strpos($blocked_markup, 'amendment-details-toggle') === FALSE, 'Rejected details remain expanded');
    }
  }
  $amendment->state = 'submitted';
  entity_save('amendment', $amendment);
  $submitted_markup = theme('amendment', array('entity_id' => $amendment->id));
  ammo_invalid_support_assert(strpos($submitted_markup, 'mede indienen') !== FALSE, 'Submitted amendment still shows co-submission link');
  ammo_invalid_support_assert(strpos($submitted_markup, 'amendment-details-toggle') === FALSE, 'Submitted details remain expanded');
  $amendment->state = 'withdrawn';
  entity_save('amendment', $amendment);
  $withdrawn_markup = theme('amendment', array('entity_id' => $amendment->id));
  ammo_invalid_support_assert(strpos($withdrawn_markup, ' - Ingetrokken</span>') !== FALSE && strpos($withdrawn_markup, 'class="amendment-details-toggle"') !== FALSE, 'Withdrawn details still use toggle');
  $motion = ammo_save_entity('motion', array('meeting_id' => $meeting->id, 'motion_id' => 1, 'state' => 'withdrawn'));
  $motion_markup = theme('motion', array('entity_id' => $motion->id));
  ammo_invalid_support_assert(strpos($motion_markup, 'class="withdrawn-toggle"') !== FALSE, 'Withdrawn motion retains its details toggle');
  $user = user_load(1);
  foreach (array('invalid', 'rejected') as $blocked_state) {
    $amendment->state = $blocked_state;
    entity_save('amendment', $amendment);
    ammo_invalid_support_assert(ammo_support_add_allowed('add', 'amendment', $amendment->id), 'Administrator retains access for ' . $blocked_state);
    $state = array();
    $form = ammo_support_form(array(), $state, 'add', 'member', 'amendment', $amendment->id);
    ammo_invalid_support_assert(isset($form['submit']), 'Administrator can open support form for ' . $blocked_state);
  }
  $transaction->rollback();
  print "PASS: invalid and rejected amendments block ordinary member and branch support on open and submit; withdrawal, motions and administrator access preserved. Fixtures rolled back.\n";
}
catch (Exception $e) {
  $transaction->rollback();
  throw $e;
}
finally {
  $user = $previous_user;
  $_GET['q'] = $previous_path;
}
