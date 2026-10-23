<?php

/**
 * Run with ddev drush scr. All fixtures are rolled back.
 */
module_load_include('inc', 'ammo', 'ammo');
module_load_include('inc', 'ammo', 'meeting.form');

function ammo_ranges_test_assert($condition, $message) {
  if (!$condition) {
    throw new Exception($message);
  }
}

/**
 * The form errors of one ranges validation, cleared afterwards.
 */
function ammo_ranges_test_errors(array $values) {
  drupal_static_reset('form_set_error');
  ammo_meeting_ranges_validate($values);
  $errors = form_get_errors() ?: array();
  drupal_static_reset('form_set_error');
  drupal_get_messages('error');
  return $errors;
}

global $user;
$previous_user = $user;
$previous_get = $_GET;
$transaction = db_transaction();
try {
  $past = date('Y-m-d H:i:s', strtotime('-2 days'));
  $yesterday = date('Y-m-d H:i:s', strtotime('-1 day'));
  $tomorrow = date('Y-m-d H:i:s', strtotime('+1 day'));
  $later = date('Y-m-d H:i:s', strtotime('+2 days'));

  // Open, closed and empty ranges.
  $meeting = (object) array('amendment_submission_start' => $yesterday, 'amendment_submission_end' => $tomorrow);
  ammo_ranges_test_assert(ammo_meeting_range_open($meeting, 'amendment_submission'), 'Inside the range is open');
  $meeting->amendment_submission_start = $tomorrow;
  $meeting->amendment_submission_end = $later;
  ammo_ranges_test_assert(!ammo_meeting_range_open($meeting, 'amendment_submission'), 'Before the range is closed');
  $meeting->amendment_submission_start = $past;
  $meeting->amendment_submission_end = $yesterday;
  ammo_ranges_test_assert(!ammo_meeting_range_open($meeting, 'amendment_submission'), 'After the range is closed');
  $meeting->amendment_submission_start = NULL;
  $meeting->amendment_submission_end = $tomorrow;
  ammo_ranges_test_assert(ammo_meeting_range_open($meeting, 'amendment_submission'), 'Without Van the range is open right away');
  ammo_ranges_test_assert(!ammo_meeting_range_open($meeting, 'motion_submission') && !ammo_meeting_has_motions($meeting), 'An empty range is closed, and an empty motion range means no motions');
  $meeting->motion_submission_end = '0000-00-00 00:00:00';
  ammo_ranges_test_assert(!ammo_meeting_has_motions($meeting), 'A zero date counts as empty');

  // Validation of the meeting form.
  $errors = ammo_ranges_test_errors(array('amendment_submission_end' => ''));
  ammo_ranges_test_assert(isset($errors['amendment_submission_end']), 'The amendment submission range is required');
  $errors = ammo_ranges_test_errors(array('amendment_submission_end' => $tomorrow, 'motion_change_start' => $yesterday));
  ammo_ranges_test_assert(array_keys($errors) === array('motion_change_end'), 'Van without Tot is refused');
  $errors = ammo_ranges_test_errors(array('amendment_submission_start' => $later, 'amendment_submission_end' => $tomorrow));
  ammo_ranges_test_assert(array_keys($errors) === array('amendment_submission_start'), 'Van after Tot is refused');
  ammo_ranges_test_assert(!ammo_ranges_test_errors(array('amendment_submission_end' => $tomorrow)), 'Only the amendment submission Tot is needed');
  $values = ammo_meeting_ranges_values(array('amendment_submission_start' => '2099-01-05 00:00:00', 'amendment_submission_end' => '2099-01-05', 'motion_submission_start' => '', 'motion_submission_end' => ''));
  ammo_ranges_test_assert($values['motion_submission_end'] === NULL && $values['motion_submission_start'] === NULL && count($values) === 20, 'Empty dates are stored as NULL');
  ammo_ranges_test_assert($values['amendment_submission_start'] === '2099-01-05 00:00:00' && $values['amendment_submission_end'] === '2099-01-05 23:59:59', 'Van starts at 00:00 and Tot ends at 23:59:59');
  ammo_ranges_test_assert(!ammo_ranges_test_errors(array('amendment_submission_start' => '2099-01-05', 'amendment_submission_end' => '2099-01-05')), 'A range of one day is allowed');

  // A saved meeting without motions keeps its empty ranges on the edit form.
  $user = user_load(1);
  $saved = ammo_save_entity('meeting', array('title' => 'Ranges test ' . uniqid(), 'date' => '2099-01-01', 'email' => 'ammo-test@example.invalid', 'amendment_withdraw_end' => '2099-01-01', 'motion_withdraw_end' => '2099-01-01', 'amendment_support_withdraw_end' => '2099-01-01', 'motion_support_withdraw_end' => '2099-01-01', 'amendment_vote_request_end' => '2099-01-01', 'motion_vote_request_end' => '2099-01-01', 'hide_meeting' => 0, 'admin_owner' => 'de congrescommissie') + ammo_meeting_ranges_values(array('amendment_submission_end' => $tomorrow)));
  $form_state = array('build_info' => array('args' => array($saved)));
  $form = drupal_build_form('ammo_meeting_edit_form', $form_state);
  foreach (array('amendment', 'motion') as $type) {
    foreach (array('submission', 'change', 'withdraw', 'support_withdraw', 'vote_request') as $kind) {
      ammo_ranges_test_assert(isset($form['ranges']['columns'][$type][$type . '_' . $kind][$type . '_' . $kind . '_start'], $form['ranges']['columns'][$type][$type . '_' . $kind][$type . '_' . $kind . '_end']), 'The edit form has Van and Tot for ' . $type . '_' . $kind);
    }
  }
  ammo_ranges_test_assert(!isset($form['ranges']['withdrawal_closing_date']), 'The withdrawal date is gone from the edit form');
  ammo_ranges_test_assert(empty($form['ranges']['columns']['motion']['motion_submission']['motion_submission_end']['#default_value']) && $form['ranges']['columns']['amendment']['amendment_submission']['amendment_submission_end']['#default_value'] === date('Y-m-d', strtotime($tomorrow)), 'Empty ranges stay empty on the edit form');
  $form_state = array('build_info' => array('args' => array($saved)), 'values' => array('title' => $saved->title, 'description' => '', 'email' => 'ammo-test@example.invalid', 'date' => array('date' => '01-01-2099'), 'amendment_submission_end' => array('date' => date('d-m-Y', strtotime($later))), 'amendment_withdraw_end' => array('date' => '01-01-2099'), 'op' => 'Opslaan'));
  drupal_form_submit('ammo_meeting_edit_form', $form_state, $saved);
  ammo_ranges_test_assert(!form_get_errors(), 'The edit form saves: ' . implode(' ', form_get_errors() ?: array()));
  entity_get_controller('meeting')->resetCache();
  $reloaded = entity_load_single('meeting', $saved->id);
  ammo_ranges_test_assert($reloaded->motion_submission_end === NULL && $reloaded->motion_change_start === NULL && $reloaded->amendment_submission_end === date('Y-m-d', strtotime($later)) . ' 23:59:59' && $reloaded->amendment_withdraw_end === '2099-01-01 23:59:59', 'Saving with empty ranges stores NULL; set ends run to the end of the day');
  drupal_get_messages();
  $form_state = array('build_info' => array('args' => array($reloaded)), 'values' => array('title' => $saved->title, 'email' => 'ammo-test@example.invalid', 'date' => array('date' => '01-01-2099'), 'amendment_submission_end' => array('date' => 'abc'), 'op' => 'Opslaan'));
  drupal_form_submit('ammo_meeting_edit_form', $form_state, $reloaded);
  ammo_ranges_test_assert(isset(form_get_errors()['amendment_submission_end']), 'An unreadable date is refused without a crash');
  drupal_static_reset('form_set_error');
  drupal_get_messages();
  module_load_include('inc', 'ammo', 'meeting.form');
  ammo_ranges_test_assert(strpos(ammo_meeting_list_page(), 'Termijnen') === FALSE, 'The meeting list leaves the ranges to the edit form');

  // Meeting choice and motion visibility follow the ranges.
  ammo_ranges_test_assert(isset(ammo_get_meetings('amendment_submission')[$saved->id]) && !isset(ammo_get_meetings('motion_submission')[$saved->id]), 'Only meetings with an open range can be chosen');
  $reloaded->motion_submission_end = $later;
  entity_save('meeting', $reloaded);
  ammo_ranges_test_assert(isset(ammo_get_meetings('motion_submission')[$saved->id]) && ammo_show_motions(), 'A motion range turns motions on');

  // Concepts follow the submission range; submitted items the change range.
  $user = user_save(NULL, array('name' => 'ammo-ranges-test-' . uniqid(), 'mail' => 'ammo-test@example.invalid', 'status' => 1, 'field_contact_id' => array(LANGUAGE_NONE => array(array('value' => 99999999)))));
  $permissions = &drupal_static('user_access');
  $permissions[$user->uid] = array('add amendments and motions' => TRUE);
  $branches = &drupal_static('ammo_get_afdelingen');
  $branches[99999999] = array(99999999 => 'Testafdeling');
  $document = ammo_save_entity('document', array('title' => 'Ranges document', 'meeting_id' => $saved->id));
  $concept = ammo_save_entity('amendment', array('document_id' => $document->id, 'chapter' => 1, 'chapterized_id' => 1, 'state' => 'concept'));
  $submitted = ammo_save_entity('amendment', array('document_id' => $document->id, 'chapter' => 1, 'chapterized_id' => 2, 'state' => 'submitted'));
  $assessed = ammo_save_entity('amendment', array('document_id' => $document->id, 'chapter' => 1, 'chapterized_id' => 3, 'state' => 'assessed', 'advice' => 'reject'));
  foreach (array($concept, $submitted, $assessed) as $entity) {
    ammo_save_entity('contact_relation', array('relation_type' => 'owner', 'target_type' => 'amendment', 'target_id' => $entity->id, 'contact_type' => 'branch', 'contact_id' => 99999999, 'branch_id' => 99999999, 'contact_display_name' => 'Testafdeling', 'branch_display_name' => 'Testafdeling'));
  }
  ammo_ranges_test_assert(ammo_edit_access($saved->id, 99999999, 'amendment', $concept->id) && !ammo_edit_access($saved->id, 99999999, 'amendment', $assessed->id), 'Inside the submission range a concept can be changed, an assessed item without a change range cannot');
  $reloaded->amendment_submission_end = $yesterday;
  $reloaded->amendment_change_end = $tomorrow;
  entity_save('meeting', $reloaded);
  ammo_ranges_test_assert(!ammo_edit_access($saved->id, 99999999, 'amendment', $concept->id) && ammo_edit_access($saved->id, 99999999, 'amendment', $assessed->id) && !ammo_edit_access($saved->id, 99999999, 'amendment', $submitted->id), 'After the submission range a concept is closed, and the change range opens the assessed item but not one that is only submitted');
  ammo_ranges_test_assert(!ammo_edit_access($saved->id, 99999999, 'amendment'), 'No new amendment after the submission range');

  // Each action follows its own range per type; the status/advies form closes at the meeting.
  $reloaded->date = '2099-01-01 00:00:00';
  $reloaded->amendment_support_withdraw_end = date('Y-m-d') . ' 23:59:59';
  $reloaded->amendment_withdraw_end = $reloaded->amendment_vote_request_end = $reloaded->motion_support_withdraw_end = $reloaded->motion_withdraw_end = NULL;
  entity_save('meeting', $reloaded);
  ammo_ranges_test_assert(ammo_support_access($saved->id, 'amendment') && ammo_support_withdraw_access($saved->id, 'amendment') && ammo_advice_access($saved->id), 'Supporting and leaving are open through the whole last day of Steun intrekken');
  ammo_ranges_test_assert(!ammo_withdraw_access($saved->id, 'amendment') && !ammo_vote_request_access($saved->id, 'assessed', 'amendment') && !ammo_support_access($saved->id, 'motion'), 'Steun intrekken opens neither withdrawing, vote requests nor the motion range');
  $reloaded->amendment_withdraw_end = $tomorrow;
  entity_save('meeting', $reloaded);
  ammo_ranges_test_assert(ammo_withdraw_access($saved->id, 'amendment') && !ammo_withdraw_access($saved->id, 'motion'), 'Voorstel intrekken opens withdrawing per type');
  ammo_ranges_test_assert(ammo_co_submit_access($saved->id, 'amendment') === ammo_meeting_range_open($reloaded, 'amendment_submission'), 'Co-submitting follows the submission range');
  $reloaded->amendment_support_withdraw_end = date('Y-m-d', strtotime('-1 day')) . ' 23:59:59';
  entity_save('meeting', $reloaded);
  ammo_ranges_test_assert(!ammo_support_access($saved->id, 'amendment') && ammo_advice_access($saved->id), 'After Steun intrekken supporting closes, before the meeting the advice stays open');
  module_load_include('inc', 'ammo', 'support.form');
  $form_state = array('build_info' => array('args' => array('add', 'member', 'amendment', $submitted->id)), 'no_cache' => TRUE);
  $form = drupal_build_form('ammo_support_form', $form_state);
  ammo_ranges_test_assert(!isset($form['submit']) && strpos(implode(' ', form_get_errors() ?: array()), 'niet (meer) mogelijk te ondersteunen') !== FALSE, 'The support form refuses after the Steun intrekken range');
  drupal_static_reset('form_set_error');
  drupal_get_messages();
  $reloaded->date = date('Y-m-d', strtotime('-1 day')) . ' 00:00:00';
  entity_save('meeting', $reloaded);
  ammo_ranges_test_assert(!ammo_advice_access($saved->id), 'After the meeting starts the advice closes');

  $transaction->rollback();
  print "PASS: open, closed and empty ranges, validation, NULL storage, edit form defaults, meeting list, meeting choice, motions on and off, and concept and change access, support and advice dates. Fixtures rolled back.\n";
}
catch (Throwable $e) {
  $transaction->rollback();
  throw $e;
}
finally {
  $user = $previous_user;
  $_GET = $previous_get;
}
