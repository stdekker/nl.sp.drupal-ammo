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
  $meeting = ammo_save_entity('meeting', array('title' => 'Co-submit test', 'date' => '2099-01-01', 'amendment_submission_end' => '2099-01-01', 'motion_submission_end' => '2099-01-01', 'amendment_withdraw_end' => '2099-01-01', 'motion_withdraw_end' => '2099-01-01', 'amendment_support_withdraw_end' => '2099-01-01', 'motion_support_withdraw_end' => '2099-01-01', 'amendment_vote_request_end' => '2099-01-01', 'motion_vote_request_end' => '2099-01-01'));
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

  // A co-submitter can revoke after the item left Ingediend, within the Steun intrekken range.
  foreach ($items as $type => $item) {
    ammo_save_entity('contact_relation', array('target_id' => $item->id, 'target_type' => $type, 'relation_type' => 'owner', 'contact_type' => 'branch', 'contact_id' => 99999999, 'branch_id' => 99999999, 'contact_display_name' => 'Andere afdeling', 'branch_display_name' => 'Andere afdeling'));
    $item->state = 'changed';
    entity_save($type, $item);
    $state_form = array();
    $form = ammo_support_form(array(), $state_form, 'withdraw', 'branch', $type, $item->id);
    ammo_co_submit_assert(isset($form['submit']), 'Co-submitter can revoke a changed ' . $type . ' within the Steun intrekken range');
    ammo_co_submit_assert(strpos(theme($type, array('entity_id' => $item->id)), 'ammo/support/withdraw/branch/' . $type . '/' . $item->id) !== FALSE, 'Changed ' . $type . ' shows the revoke link');
  }
  // Closing Voorstel intrekken leaves co-submitters free to leave; the link then names only that.
  $meeting->amendment_withdraw_end = $meeting->motion_withdraw_end = '2000-01-01 23:59:59';
  entity_save('meeting', $meeting);
  $titles = array('amendment' => '>steun intrekken<', 'motion' => '>steun intrekken als afdeling<');
  foreach ($items as $type => $item) {
    $state_form = array();
    $form = ammo_support_form(array(), $state_form, 'withdraw', 'branch', $type, $item->id);
    ammo_co_submit_assert(isset($form['submit']), 'Closing Voorstel intrekken keeps leaving a ' . $type . ' open');
    ammo_co_submit_assert(strpos(theme($type, array('entity_id' => $item->id)), $titles[$type]) !== FALSE, 'The link on a ' . $type . ' says steun intrekken');
  }
  $meeting->amendment_support_withdraw_end = $meeting->motion_support_withdraw_end = '2000-01-01 23:59:59';
  entity_save('meeting', $meeting);
  foreach ($items as $type => $item) {
    $state_form = array();
    $form = ammo_support_form(array(), $state_form, 'withdraw', 'branch', $type, $item->id);
    ammo_co_submit_assert(empty($form['submit']), 'Revoking a ' . $type . ' closes after the Steun intrekken range');
    ammo_co_submit_assert(strpos(theme($type, array('entity_id' => $item->id)), 'ammo/support/withdraw/branch/' . $type . '/' . $item->id) === FALSE, 'The revoke link on a ' . $type . ' is gone after the range');
    $_GET['q'] = 'ammo/support/withdraw/branch/' . $type . '/' . $item->id;
    $state_form = array('values' => array('branch_id' => array(99999999 => 99999999)));
    ammo_support_form_validate(array(), $state_form);
    ammo_co_submit_assert((bool) form_get_errors(), 'A stale revoke form for a ' . $type . ' is refused');
    form_clear_error();
  }
  drupal_get_messages('error');

  // A user with the submitting branch and a co-submitting branch: each branch follows its own range.
  $branches[99999990][99999998] = 'Indienende afdeling';
  $cases = array(
    'both' => array('2099-01-01 23:59:59', '2099-01-01 23:59:59', array(99999998, 99999999), array('amendment' => '>voorstel / steun intrekken<', 'motion' => '>intrekken als afdeling<')),
    'support only' => array('2000-01-01 23:59:59', '2099-01-01 23:59:59', array(99999999), array('amendment' => '>steun intrekken<', 'motion' => '>steun intrekken als afdeling<')),
    'withdraw only' => array('2099-01-01 23:59:59', '2000-01-01 23:59:59', array(99999998), array('amendment' => '>voorstel intrekken<', 'motion' => '>motie intrekken als afdeling<')),
  );
  foreach ($cases as $case => $setup) {
    list($withdraw_end, $support_withdraw_end, $offered, $titles) = $setup;
    $meeting->amendment_withdraw_end = $meeting->motion_withdraw_end = $withdraw_end;
    $meeting->amendment_support_withdraw_end = $meeting->motion_support_withdraw_end = $support_withdraw_end;
    entity_save('meeting', $meeting);
    foreach ($items as $type => $item) {
      $state_form = array();
      $form = ammo_support_form(array(), $state_form, 'withdraw', 'branch', $type, $item->id);
      $options = isset($form['branch_id']) ? array_keys($form['branch_id']['#options']) : array();
      sort($options);
      ammo_co_submit_assert($options == $offered, ucfirst($case) . ': the ' . $type . ' form offers ' . implode(', ', $offered));
      ammo_co_submit_assert(strpos(theme($type, array('entity_id' => $item->id)), $titles[$type]) !== FALSE, ucfirst($case) . ': the ' . $type . ' link says ' . trim($titles[$type], '<>'));
      foreach (array(99999998, 99999999) as $branch_id) {
        $_GET['q'] = 'ammo/support/withdraw/branch/' . $type . '/' . $item->id;
        $state_form = array('values' => array('branch_id' => array($branch_id => $branch_id)));
        ammo_support_form_validate(array(), $state_form);
        ammo_co_submit_assert(!form_get_errors() === in_array($branch_id, $offered), ucfirst($case) . ': validation of branch ' . $branch_id . ' on the ' . $type . ' follows its range');
        form_clear_error();
      }
    }
  }
  unset($branches[99999990][99999998]);
  drupal_get_messages('error');

  // Co-submitting follows the submission range.
  $meeting->amendment_submission_end = $meeting->motion_submission_end = '2000-01-01 23:59:59';
  entity_save('meeting', $meeting);
  foreach ($items as $type => $item) {
    $item->state = 'submitted';
    entity_save($type, $item);
    db_delete('ammo_contact_relations')->condition('target_type', $type)->condition('target_id', $item->id)->condition('contact_id', 99999999)->execute();
    $state_form = array();
    $form = ammo_support_form(array(), $state_form, 'add', 'branch', $type, $item->id);
    ammo_co_submit_assert(empty($form['submit']), 'Co-submitting a ' . $type . ' closes after the submission range');
    ammo_co_submit_assert(strpos(theme($type, array('entity_id' => $item->id)), 'ammo/support/add/branch/' . $type . '/' . $item->id) === FALSE, 'The co-submission link on a ' . $type . ' is gone after the submission range');
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
  print "PASS: branches can co-submit amendments and motions only while they are submitted, on open, submit and in the links; co-submitting follows the submission range, withdrawing by the submitter the Voorstel intrekken range and revoking the Steun intrekken range, in the form, validation and link titles; member support and administrators unaffected. Fixtures rolled back.\n";
}
catch (Throwable $e) {
  $transaction->rollback();
  throw $e;
}
finally {
  $user = $previous_user;
  $_GET['q'] = $previous_path;
}
