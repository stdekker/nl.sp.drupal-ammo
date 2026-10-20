<?php

/**
 * Run with ddev drush scr. All fixtures and captured mail are rolled back.
 */
module_load_include('inc', 'ammo', 'ammo');
module_load_include('inc', 'ammo', 'amendment.form');
module_load_include('inc', 'ammo', 'motion.form');

function ammo_delete_test_assert($condition, $message) {
  if (!$condition) {
    throw new Exception($message);
  }
}

/**
 * Builds the amendment or motion form for an action, as its menu item does.
 */
function ammo_delete_test_form($type, $action, $id) {
  $form_state = array('build_info' => array('args' => array($action, $id)), 'no_cache' => TRUE);
  return drupal_build_form('ammo_' . $type . '_form', $form_state);
}

global $user, $conf;
$previous_user = $user;
$previous_conf = $conf;
$previous_get = $_GET;
$conf['mail_system'] = array('default-system' => 'TestingMailSystem', 'ammo' => 'TestingMailSystem');
$transaction = db_transaction();
try {
  $user = user_save(NULL, array('name' => 'ammo-delete-test-' . uniqid(), 'mail' => 'ammo-test@example.invalid', 'status' => 1));
  $permissions = &drupal_static('user_access');
  $permissions[$user->uid] = array('administer amendments and motions' => TRUE, 'add amendments and motions' => TRUE);
  $meeting = ammo_save_entity('meeting', array('title' => 'Delete test', 'date' => '2099-01-01', 'submission_closing_date' => '2099-01-01', 'withdrawal_closing_date' => '2099-01-01', 'email' => 'ammo-test@example.invalid', 'hide_meeting' => 0));
  $document = ammo_save_entity('document', array('title' => 'Delete document', 'meeting_id' => $meeting->id));
  $amendment = ammo_save_entity('amendment', array('document_id' => $document->id, 'chapter' => 2, 'chapterized_id' => 1, 'page' => 1, 'line' => 1, 'amendment_text' => 'Tekst', 'state' => 'submitted'));
  $motion = ammo_save_entity('motion', array('meeting_id' => $meeting->id, 'motion_id' => 1, 'consideration_body' => 'Overweging', 'state' => 'submitted'));
  foreach (array('amendment' => $amendment, 'motion' => $motion) as $type => $entity) {
    ammo_save_entity('contact_relation', array('relation_type' => 'owner', 'target_type' => $type, 'target_id' => $entity->id, 'contact_type' => 'member', 'contact_id' => 99999999, 'contact_display_name' => 'Indiener', 'contact_email' => 'owner@example.invalid'));
  }

  ammo_delete_test_assert(!isset(ammo_editable_states('submitted')['blocked']) && isset(ammo_editable_states('blocked')['blocked']), 'Verwijderd is only offered for deleted items');

  foreach (array('amendment' => $amendment, 'motion' => $motion) as $type => $entity) {
    $label = $type === 'amendment' ? 'Wijzigingsvoorstel 2.1' : 'Motie 1';

    // The status/advies step offers no Verwijderd, but a delete button.
    $_GET = array('destination' => 'ammo/item-test');
    $form = ammo_delete_test_form($type, 'advice', $entity->id);
    ammo_delete_test_assert(isset($form['state']) && !isset($form['state']['#options']['blocked']), $type . ': status dropdown has no Verwijderd');
    ammo_delete_test_assert($form['buttons']['delete']['#href'] === 'ammo/' . $type . '/delete/' . $entity->id && $form['buttons']['delete']['#options']['query']['destination'] === 'ammo/item-test', $type . ': delete button opens the confirmation page and keeps the destination');

    // The confirmation page asks and requires an explanation.
    $form = ammo_delete_test_form($type, 'delete', $entity->id);
    ammo_delete_test_assert(strpos($form['question']['#markup'], 'Weet je zeker') !== FALSE && !empty($form['mail_text']['#required']), $type . ': confirmation page asks and requires an explanation');
    ammo_delete_test_assert(strpos($form['actions']['cancel']['#markup'], 'ammo/item-test') !== FALSE, $type . ': cancel returns to the destination');
    $form_state = array('values' => array('mail_text' => '', 'delete_amot' => 'Verwijderen'), 'build_info' => array('args' => array('delete', $entity->id)));
    drupal_form_submit('ammo_' . $type . '_form', $form_state, 'delete', $entity->id);
    ammo_delete_test_assert(form_get_errors() && entity_load_single($type, $entity->id)->state === 'submitted', $type . ': empty explanation rejected');
    drupal_static_reset('form_set_error');
    drupal_get_messages();

    variable_set('drupal_test_email_collector', array());
    $form_state = array('values' => array('mail_text' => 'Dubbel ingediend.', 'delete_amot' => 'Verwijderen'), 'build_info' => array('args' => array('delete', $entity->id)));
    drupal_form_submit('ammo_' . $type . '_form', $form_state, 'delete', $entity->id);
    entity_get_controller($type)->resetCache();
    ammo_delete_test_assert(!form_get_errors() && entity_load_single($type, $entity->id)->state === 'blocked', $type . ': confirmed delete sets Verwijderd');
    $messages = drupal_get_messages('status');
    ammo_delete_test_assert(in_array($label . ' is verwijderd.', $messages['status'], TRUE), $type . ': confirmation message names the item');
    $mails = variable_get('drupal_test_email_collector', array());
    $recipients = array_map(function ($mail) { return $mail['to']; }, $mails);
    ammo_delete_test_assert(in_array('owner@example.invalid', $recipients, TRUE) && strpos($mails[0]['body'], 'Dubbel ingediend.') !== FALSE, $type . ': explanation mailed to the submitters');

    // A deleted item keeps Verwijderd selectable and has no delete button.
    $form = ammo_delete_test_form($type, 'advice', $entity->id);
    ammo_delete_test_assert(isset($form['state']['#options']['blocked']) && !isset($form['buttons']['delete']), $type . ': deleted item can be restored, not deleted again');
    $form = ammo_delete_test_form($type, 'delete', $entity->id);
    ammo_delete_test_assert(!isset($form['mail_text']) && strpos($form['deleted']['#markup'], 'al verwijderd') !== FALSE, $type . ': already deleted item is reported');
  }

  // Without admin rights there is no delete page.
  $entity = ammo_save_entity('amendment', array('document_id' => $document->id, 'chapter' => 2, 'chapterized_id' => 2, 'state' => 'submitted'));
  $permissions[$user->uid] = array('add amendments and motions' => TRUE);
  $form = ammo_delete_test_form('amendment', 'delete', $entity->id);
  ammo_delete_test_assert(!isset($form['mail_text']), 'Members cannot open the delete page');
  $form_state = array('values' => array('mail_text' => 'x', 'entity_type' => 'amendment', 'entity_id' => $entity->id));
  ammo_delete_entity_form_submit(array(), $form_state);
  drupal_get_messages();
  ammo_delete_test_assert(entity_load_single('amendment', $entity->id)->state === 'submitted', 'Members cannot delete through the submit handler');

  $transaction->rollback();
  print "PASS: dropdown without Verwijderd, delete button, confirmation page, required explanation, soft delete, mail, restore option, already deleted and access. Fixtures rolled back.\n";
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
