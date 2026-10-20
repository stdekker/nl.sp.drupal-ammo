<?php

/** Run with ddev drush scr; fixtures are rolled back. */
module_load_include('inc', 'ammo', 'ammo');
module_load_include('inc', 'ammo', 'support.form');

function ammo_co_submit_assert($condition, $message) {
  if (!$condition) {
    throw new Exception($message);
  }
}

/**
 * The number of owner relations of the test branch on an item.
 */
function ammo_co_submit_test_count($type, $id) {
  return (int) db_query('SELECT COUNT(*) FROM {ammo_contact_relations} WHERE target_type = :type AND target_id = :id AND relation_type = :relation AND contact_id = :contact', array(':type' => $type, ':id' => $id, ':relation' => 'owner', ':contact' => 99999999))->fetchField();
}

global $user;
$previous_user = $user;
$previous_path = $_GET['q'];
$transaction = db_transaction();
try {
  $user = user_save(NULL, array('name' => 'ammo-co-submit-' . uniqid(), 'status' => 1, 'field_contact_id' => array(LANGUAGE_NONE => array(array('value' => 99999990)))));
  $permissions = &drupal_static('user_access');
  $permissions[$user->uid] = array('add amendments and motions' => TRUE);
  $branches = &drupal_static('ammo_get_afdelingen');
  $branches[99999990] = array(99999999 => 'Andere afdeling');
  $meeting = ammo_save_entity('meeting', array('title' => 'Co-submit test', 'date' => '2099-01-01', 'withdrawal_closing_date' => '2099-01-01'));
  $document = ammo_save_entity('document', array('title' => 'Co-submit document', 'meeting_id' => $meeting->id));
  $items = array(
    'amendment' => ammo_save_entity('amendment', array('document_id' => $document->id, 'chapter' => 1, 'state' => 'submitted')),
    'motion' => ammo_save_entity('motion', array('meeting_id' => $meeting->id, 'motion_id' => 1, 'state' => 'submitted')),
  );
  foreach ($items as $type => $item) {
    ammo_save_entity('contact_relation', array('target_id' => $item->id, 'target_type' => $type, 'relation_type' => 'owner', 'contact_type' => 'branch', 'contact_id' => 99999998, 'branch_id' => 99999998, 'contact_display_name' => 'Indienende afdeling', 'branch_display_name' => 'Indienende afdeling'));
  }

  foreach (array_keys(ammo_states()) as $state) {
    foreach ($items as $type => $item) {
      $item->state = $state;
      $item->advice = $state === 'assessed' ? 'accept' : NULL;
      entity_save($type, $item);
      $link = 'ammo/support/add/branch/' . $type . '/' . $item->id;
      $markup = theme($type, array('entity_id' => $item->id));
      $state_form = array();
      $form = ammo_support_form(array(), $state_form, 'add', 'branch', $type, $item->id);
      if ($state === 'submitted') {
        ammo_co_submit_assert(ammo_support_co_submit_allowed('add', 'branch', $type, $item->id), 'Submitted ' . $type . ' allows co-submission');
        ammo_co_submit_assert(strpos($markup, $link) !== FALSE, 'Submitted ' . $type . ' shows the co-submission link');
        ammo_co_submit_assert(isset($form['submit']), 'Submitted ' . $type . ' opens the co-submission form');
        continue;
      }
      ammo_co_submit_assert(!ammo_support_co_submit_allowed('add', 'branch', $type, $item->id), $state . ' ' . $type . ' rejects co-submission');
      ammo_co_submit_assert(strpos($markup, $link) === FALSE, $state . ' ' . $type . ' hides the co-submission link');
      ammo_co_submit_assert(empty($form['submit']), 'Direct URL has no submit button for ' . $state . ' ' . $type);
      $_GET['q'] = $link;
      $state_form['values'] = array('branch_id' => array(99999999 => 99999999));
      ammo_support_form_submit(array(), $state_form);
      ammo_co_submit_assert(ammo_co_submit_test_count($type, $item->id) === 0, 'No co-submission saved for ' . $state . ' ' . $type);
      form_clear_error();
      drupal_get_messages('error');
    }
  }

  // Member support and withdrawing are not bound by this rule.
  ammo_co_submit_assert(ammo_support_co_submit_allowed('add', 'member', 'amendment', $items['amendment']->id), 'Member support unaffected');
  ammo_co_submit_assert(ammo_support_co_submit_allowed('withdraw', 'branch', 'amendment', $items['amendment']->id), 'Withdrawal unaffected');

  // A submitted item passes validation; the submit itself is not run because it mails the branches.
  foreach ($items as $type => $item) {
    $item->state = 'submitted';
    $item->advice = NULL;
    entity_save($type, $item);
    $_GET['q'] = 'ammo/support/add/branch/' . $type . '/' . $item->id;
    $state_form = array('values' => array('branch_id' => array(99999999 => 99999999)));
    ammo_support_form_validate(array(), $state_form);
    ammo_co_submit_assert(!form_get_errors(), 'Submitted ' . $type . ' passes validation');
  }

  // A co-submitter can revoke after the item left Ingediend, until the withdrawal date passes.
  foreach ($items as $type => $item) {
    ammo_save_entity('contact_relation', array('target_id' => $item->id, 'target_type' => $type, 'relation_type' => 'owner', 'contact_type' => 'branch', 'contact_id' => 99999999, 'branch_id' => 99999999, 'contact_display_name' => 'Andere afdeling', 'branch_display_name' => 'Andere afdeling'));
    $item->state = 'changed';
    entity_save($type, $item);
    $state_form = array();
    $form = ammo_support_form(array(), $state_form, 'withdraw', 'branch', $type, $item->id);
    ammo_co_submit_assert(isset($form['submit']), 'Co-submitter can revoke a changed ' . $type . ' before the withdrawal date');
    ammo_co_submit_assert(strpos(theme($type, array('entity_id' => $item->id)), 'ammo/support/withdraw/branch/' . $type . '/' . $item->id) !== FALSE, 'Changed ' . $type . ' shows the revoke link');
  }
  $meeting->withdrawal_closing_date = '2000-01-01';
  entity_save('meeting', $meeting);
  drupal_static_reset('ammo_passed_date');
  foreach ($items as $type => $item) {
    $state_form = array();
    $form = ammo_support_form(array(), $state_form, 'withdraw', 'branch', $type, $item->id);
    ammo_co_submit_assert(empty($form['submit']), 'Revoking a ' . $type . ' closes after the withdrawal date');
  }
  form_clear_error();
  drupal_get_messages('error');

  $user = user_load(1);
  foreach ($items as $type => $item) {
    $item->state = 'changed';
    entity_save($type, $item);
    ammo_co_submit_assert(ammo_support_co_submit_allowed('add', 'branch', $type, $item->id) && ammo_can_co_submit('changed'), 'Administrator can co-submit a changed ' . $type);
  }
  $transaction->rollback();
  print "PASS: branches can co-submit amendments and motions only while they are submitted, on open, submit and in the links; revoking stays open until the withdrawal date; member support and administrators unaffected. Fixtures rolled back.\n";
}
catch (Throwable $e) {
  $transaction->rollback();
  throw $e;
}
finally {
  $user = $previous_user;
  $_GET['q'] = $previous_path;
}
