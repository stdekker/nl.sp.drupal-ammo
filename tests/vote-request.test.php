<?php

/** Run with ddev drush scr; fixtures are rolled back. */
module_load_include('inc', 'ammo', 'ammo');
module_load_include('inc', 'ammo', 'vote_request.form');

function ammo_vote_request_assert($condition, $message) {
  if (!$condition) {
    throw new Exception($message);
  }
}

global $user;
$previous_user = $user;
$previous_path = $_GET['q'];
$transaction = db_transaction();
try {
  $user = user_save(NULL, array('name' => 'ammo-vote-request-' . uniqid(), 'status' => 1, 'field_contact_id' => array(LANGUAGE_NONE => array(array('value' => 99999999)))));
  $permissions = &drupal_static('user_access');
  $permissions[$user->uid] = array('add amendments and motions' => TRUE);
  $branches = &drupal_static('ammo_get_afdelingen');
  $branches[99999999] = array(99999999 => 'Andere afdeling', 99999998 => 'Indiener');
  $meeting = ammo_save_entity('meeting', array('title' => 'Vote request test', 'date' => '2099-01-01', 'amendment_withdraw_end' => '2099-01-01', 'motion_withdraw_end' => '2099-01-01', 'amendment_support_withdraw_end' => '2099-01-01', 'motion_support_withdraw_end' => '2099-01-01', 'amendment_vote_request_end' => '2099-01-01', 'motion_vote_request_end' => '2099-01-01'));
  $document = ammo_save_entity('document', array('title' => 'Vote request document', 'meeting_id' => $meeting->id));
  $amendment = ammo_save_entity('amendment', array('document_id' => $document->id, 'chapter' => 1, 'chapterized_id' => 1, 'state' => 'assessed', 'advice' => 'accept'));
  ammo_save_entity('contact_relation', array('target_id' => $amendment->id, 'target_type' => 'amendment', 'relation_type' => 'owner', 'contact_type' => 'branch', 'contact_id' => 99999998, 'branch_id' => 99999998, 'contact_display_name' => 'Indiener', 'branch_display_name' => 'Indiener'));
  ammo_save_entity('contact_relation', array('target_id' => $amendment->id, 'target_type' => 'amendment', 'relation_type' => 'owner', 'contact_type' => 'member', 'contact_id' => 99999990, 'branch_id' => 99999997, 'contact_display_name' => 'Lid', 'branch_display_name' => 'Ledenafdeling'));

  // Assessed as accepted: only branches that did not (co-)submit may request a vote.
  ammo_vote_request_assert(!ammo_vote_request_branch_allowed('amendment', $amendment, 99999998), 'Submitting branch may not request a vote on an accepted amendment');
  ammo_vote_request_assert(!ammo_vote_request_branch_allowed('amendment', $amendment, 99999997), 'Branch of a member submitter may not request a vote on an accepted amendment');
  ammo_vote_request_assert(ammo_vote_request_branch_allowed('amendment', $amendment, 99999999), 'Another branch may request a vote on an accepted amendment');
  $state = array();
  $form = ammo_vote_request_form(array(), $state, 'add', 'branch', 'amendment', $amendment->id);
  ammo_vote_request_assert(array_keys($form['branch_id']['#options']) == array(99999999), 'Vote request form offers only branches that did not submit');
  $_GET['q'] = 'ammo/vote_request/add/branch/amendment/' . $amendment->id;
  $state['values'] = array('branch_id' => array(99999998 => 99999998));
  ammo_vote_request_form_validate($form, $state);
  $errors = form_get_errors();
  ammo_vote_request_assert(isset($errors['vote_request']) && strpos($errors['vote_request'], 'overgenomen') !== FALSE, 'A forced vote request by the submitting branch is refused');
  form_clear_error();
  drupal_get_messages();
  $markup = theme('amendment', array('entity_id' => $amendment->id));
  ammo_vote_request_assert(strpos($markup, 'stemming aanvragen') !== FALSE, 'The link shows while another branch of the user may request a vote');
  $branches[99999999] = array(99999998 => 'Indiener');
  $markup = theme('amendment', array('entity_id' => $amendment->id));
  ammo_vote_request_assert(strpos($markup, 'stemming aanvragen') === FALSE, 'The link is hidden when all branches of the user submitted it');

  // Assessed as rejected: anyone may request a vote.
  $amendment->advice = 'reject';
  entity_save('amendment', $amendment);
  ammo_vote_request_assert(ammo_vote_request_branch_allowed('amendment', $amendment, 99999998), 'Submitting branch may request a vote on a rejected amendment');
  ammo_vote_request_assert(strpos(theme('amendment', array('entity_id' => $amendment->id)), 'stemming aanvragen') !== FALSE, 'The link shows on a rejected amendment for the submitting branch');

  // Motions follow the same rule.
  $motion = ammo_save_entity('motion', array('meeting_id' => $meeting->id, 'motion_id' => 1, 'state' => 'assessed', 'advice' => 'accept'));
  ammo_save_entity('contact_relation', array('target_id' => $motion->id, 'target_type' => 'motion', 'relation_type' => 'owner', 'contact_type' => 'branch', 'contact_id' => 99999998, 'branch_id' => 99999998, 'contact_display_name' => 'Indiener', 'branch_display_name' => 'Indiener'));
  ammo_vote_request_assert(!ammo_vote_request_branch_allowed('motion', $motion, 99999998) && ammo_vote_request_branch_allowed('motion', $motion, 99999999), 'Accepted motion: only other branches may request a vote');

  // The vote requests are the last part of an item, after the advice and its Toelichting.
  foreach (array('amendment' => $amendment, 'motion' => $motion) as $type => $item) {
    $item->supplement = 'Toelichting voorstel';
    $item->advice_supplement = 'Toelichting advies';
    entity_save($type, $item);
    ammo_save_entity('contact_relation', array('target_id' => $item->id, 'target_type' => $type, 'relation_type' => 'vote_request', 'contact_type' => 'branch', 'contact_id' => 99999999, 'branch_id' => 99999999, 'contact_display_name' => 'Andere afdeling', 'branch_display_name' => 'Andere afdeling'));
    $markup = theme($type, array('entity_id' => $item->id));
    $vote_heading = strpos($markup, '<h4>Stemming aangevraagd door:</h4>');
    ammo_vote_request_assert($vote_heading !== FALSE && $vote_heading === strrpos($markup, '<h4>') && $vote_heading > strpos($markup, 'Toelichting advies'), 'Stemming aangevraagd door is the last part of the ' . $type);
  }

  // Vote requests follow their own range per type, not the withdraw ranges.
  $meeting->amendment_withdraw_end = $meeting->amendment_support_withdraw_end = '2000-01-01 23:59:59';
  entity_save('meeting', $meeting);
  ammo_vote_request_assert(ammo_vote_request_access($meeting->id, 'assessed', 'amendment'), 'Closing the withdraw ranges keeps vote requests open');
  $meeting->amendment_vote_request_end = '2000-01-01 23:59:59';
  entity_save('meeting', $meeting);
  ammo_vote_request_assert(!ammo_vote_request_access($meeting->id, 'assessed', 'amendment') && ammo_vote_request_access($meeting->id, 'assessed', 'motion'), 'The amendment vote request range closes amendments only');
  $state = array();
  $form = ammo_vote_request_form(array(), $state, 'add', 'branch', 'amendment', $amendment->id);
  ammo_vote_request_assert(empty($form['submit']) && strpos(implode(' ', form_get_errors() ?: array()), 'termijn voor stemming aanvragen') !== FALSE, 'The vote request form refuses after its range');
  form_clear_error();
  drupal_get_messages();

  // Only the allowed statuses take vote requests, except for admins.
  ammo_vote_request_assert(!ammo_vote_request_access($meeting->id, 'submitted', 'motion'), 'A regular user may not manage vote requests on a submitted item');
  $permissions[$user->uid]['administer amendments and motions'] = TRUE;
  ammo_vote_request_assert(ammo_vote_request_access($meeting->id, 'submitted', 'amendment'), 'Admins may manage vote requests on a submitted item, also after the range');
  $amendment->advice = 'accept';
  entity_save('amendment', $amendment);
  ammo_vote_request_assert(ammo_vote_request_branch_allowed('amendment', $amendment, 99999998), 'Admins are not bound by the submitter rule');

  $transaction->rollback();
  print "PASS: accepted items only take vote requests from branches that did not (co-)submit, member submitters included; rejected items from anyone; form, validation, links, vote requests shown last, motions, the vote request range per type, the allowed statuses and admins. Fixtures rolled back.\n";
}
catch (Throwable $e) {
  $transaction->rollback();
  throw $e;
}
finally {
  $user = $previous_user;
  $_GET['q'] = $previous_path;
}
