<?php

/**
 * Run with ddev drush scr. All fixtures, settings and captured mail are rolled back.
 */
module_load_include('inc', 'ammo', 'ammo');
module_load_include('inc', 'ammo', 'mail_settings.form');

function ammo_mail_settings_test_assert($condition, $message) {
  if (!$condition) {
    throw new Exception($message);
  }
}

/**
 * Addresses of the mails collected since the last call.
 */
function ammo_mail_settings_test_recipients() {
  $recipients = array_map(function ($mail) { return $mail['to']; }, variable_get('drupal_test_email_collector', array()));
  variable_set('drupal_test_email_collector', array());
  sort($recipients);
  return array_values(array_unique($recipients));
}

/**
 * Submits the settings form with the main switch and the step checkboxes as posted.
 */
function ammo_mail_settings_test_submit($enabled, array $steps) {
  $form_state = array('values' => array('ammo_mail_enabled' => $enabled, 'steps' => $steps));
  ammo_mail_settings_form_submit(array(), $form_state);
  drupal_get_messages();
}

global $user, $conf;
$previous_user = $user;
$previous_conf = $conf;
$previous_get = $_GET;
$conf['mail_system'] = array('default-system' => 'TestingMailSystem', 'ammo' => 'TestingMailSystem');
$transaction = db_transaction();
try {
  $user = user_save(NULL, array('name' => 'ammo-mail-test-' . uniqid(), 'mail' => 'ammo-test@example.invalid', 'status' => 1));
  $permissions = &drupal_static('user_access');
  $permissions[$user->uid] = array('administer ammo mail settings' => TRUE);
  variable_del('ammo_mail_enabled');
  variable_del('ammo_mail_steps');

  // Without saved settings only the contacts of the proposal get the mails of these steps, and the meeting's address none.
  $user_steps = array('submitted_edit', 'support', 'support_withdraw', 'withdraw', 'vote_request', 'vote_request_withdraw', 'delete');
  foreach (ammo_mail_step_settings() as $step => $recipients) {
    $user_default = in_array($step, $user_steps, TRUE);
    ammo_mail_settings_test_assert($recipients === array('user' => $user_default, 'admin' => FALSE) && ammo_mail_enabled($step, 'user') === $user_default && !ammo_mail_enabled($step, 'admin'), $step . ' has its default');
  }
  ammo_mail_settings_test_assert(count(ammo_mail_steps()) === 12, 'Every mail step is listed');

  // A save through ammo_store() picks its step from the mapping and the states.
  $submitted = (object) array('state' => 'submitted');
  $concept = (object) array('state' => 'concept');
  ammo_mail_settings_test_assert(ammo_store_mail_step(array('advice' => 'advice', 'advice_supplement' => 'advice_supplement', 'state' => 'state'), $submitted, array('state' => 'assessed')) === 'advice', 'Status/advies step');
  ammo_mail_settings_test_assert(ammo_store_mail_step(array('amendment_text' => 'amendment_text', 'state' => 'state'), $concept, array('amendment_text' => 'x')) === 'concept', 'Concept step');
  ammo_mail_settings_test_assert(ammo_store_mail_step(array('amendment_text' => 'amendment_text', 'state' => 'state'), $concept, array('state' => 'submitted')) === 'submit', 'Submit step');
  ammo_mail_settings_test_assert(ammo_store_mail_step(array('amendment_text' => 'amendment_text', 'state' => 'state'), $submitted, array('amendment_text' => 'x')) === 'change', 'Change step');

  $meeting = ammo_save_entity('meeting', array('title' => 'Mail test', 'date' => '2099-01-01', 'amendment_submission_end' => '2099-01-01', 'motion_submission_end' => '2099-01-01', 'amendment_withdraw_end' => '2099-01-01', 'motion_withdraw_end' => '2099-01-01', 'amendment_support_withdraw_end' => '2099-01-01', 'motion_support_withdraw_end' => '2099-01-01', 'amendment_vote_request_end' => '2099-01-01', 'motion_vote_request_end' => '2099-01-01', 'email' => 'secretariaat@example.invalid', 'hide_meeting' => 0));
  $document = ammo_save_entity('document', array('title' => 'Mail document', 'meeting_id' => $meeting->id));
  $amendment = ammo_save_entity('amendment', array('document_id' => $document->id, 'chapter' => 2, 'chapterized_id' => 1, 'page' => 1, 'line' => 1, 'amendment_text' => 'Tekst', 'state' => 'concept'));
  $owner = ammo_save_entity('contact_relation', array('relation_type' => 'owner', 'target_type' => 'amendment', 'target_id' => $amendment->id, 'contact_type' => 'branch', 'contact_id' => 99999999, 'branch_id' => 99999999, 'contact_display_name' => 'Testafdeling', 'branch_display_name' => 'Testafdeling', 'contact_email' => 'owner@example.invalid'));
  $both = array('owner@example.invalid', 'secretariaat@example.invalid');

  // With every step on, saving a concept mails both, until the concept step is off for the submitters.
  variable_set('ammo_mail_steps', array_fill_keys(array_keys(ammo_mail_steps()), array('user' => TRUE, 'admin' => TRUE)));
  $store = function ($text) use ($amendment, $meeting, $document) {
    $form_state = array('values' => array('amendment_text' => $text, 'state' => 'concept'), 'storage' => array('meeting' => $meeting->id, 'document' => $document->id, 'state' => 'concept'));
    ammo_store('amendment', $amendment->id, array('amendment_text' => 'amendment_text', 'state' => 'state'), array(), $form_state);
  };
  variable_set('drupal_test_email_collector', array());
  $store('Eerste versie');
  ammo_mail_settings_test_assert(ammo_mail_settings_test_recipients() === $both, 'A concept save mails the submitters and the meeting address');
  ammo_mail_settings_test_submit(1, array('concept' => array('user' => 0, 'admin' => 1)) + ammo_mail_step_settings());
  ammo_mail_settings_test_assert(!ammo_mail_enabled('concept', 'user') && ammo_mail_enabled('concept', 'admin') && ammo_mail_enabled('concept'), 'The concept step is off for the submitters only');
  $store('Tweede versie');
  ammo_mail_settings_test_assert(ammo_mail_settings_test_recipients() === array('secretariaat@example.invalid'), 'A concept save then only mails the meeting address');

  // Other steps are not affected.
  ammo_delete_mail('amendment', $amendment->id, $meeting->id, 'Toelichting');
  ammo_mail_settings_test_assert(ammo_mail_settings_test_recipients() === $both, 'The delete mail still reaches everyone');

  // The submitter and a co-submitter withdrawing are separate steps.
  ammo_mail_settings_test_submit(1, array('support_withdraw' => array('user' => 0, 'admin' => 0)) + ammo_mail_step_settings());
  ammo_withdraw_mail('amendment', $amendment->id, $owner, $meeting->id, 'support_withdraw');
  ammo_mail_settings_test_assert(ammo_mail_settings_test_recipients() === array(), 'A co-submitter withdrawing mails nobody when its step is off');
  ammo_withdraw_mail('amendment', $amendment->id, $owner, $meeting->id);
  ammo_mail_settings_test_assert(ammo_mail_settings_test_recipients() === $both, 'The submitter withdrawing still mails everyone');

  // The main switch stops every mail, reports success, and keeps the steps even though disabled checkboxes are not posted.
  $saved = ammo_mail_step_settings();
  ammo_mail_settings_test_submit(0, array_map(function ($recipients) { return array_fill_keys(array_keys($recipients), 0); }, $saved));
  ammo_mail_settings_test_assert(!variable_get('ammo_mail_enabled', TRUE) && ammo_mail_step_settings() === $saved, 'Switching off keeps the step settings');
  foreach (array_keys(ammo_mail_steps()) as $step) {
    ammo_mail_settings_test_assert(!ammo_mail_enabled($step) && !ammo_mail_enabled($step, 'admin'), $step . ' is off with the main switch');
  }
  ammo_delete_mail('amendment', $amendment->id, $meeting->id, 'Toelichting');
  ammo_mail_settings_test_assert(ammo_changes_update_mail('amendment', $amendment->id, array('x' => array('fieldname' => 'x', 'diff' => 'x')), $meeting->id, 'submitted_edit') === TRUE, 'A switched off change mail counts as sent');
  ammo_mail_settings_test_assert(ammo_mail_settings_test_recipients() === array(), 'Nothing is mailed with the main switch off');

  // Switching back on restores the earlier choices.
  ammo_mail_settings_test_submit(1, $saved);
  ammo_mail_settings_test_assert(ammo_mail_enabled('concept', 'admin') && !ammo_mail_enabled('concept', 'user') && !ammo_mail_enabled('support_withdraw') && ammo_mail_enabled('delete', 'user'), 'Switching on restores the step settings');

  // The page shows the main switch and a checkbox per step and recipient, greyed out with the switch.
  $form = drupal_get_form('ammo_mail_settings_form');
  $html = drupal_render($form);
  ammo_mail_settings_test_assert(strpos($html, 'E-mailmeldingen versturen') !== FALSE && strpos($html, 'ammo-mail-settings') !== FALSE && substr_count($html, 'type="checkbox"') === 1 + 2 * count(ammo_mail_steps()), 'The page has the main switch and a checkbox per step and recipient');
  ammo_mail_settings_test_assert($form['steps']['concept']['user']['#default_value'] === FALSE && $form['steps']['concept']['admin']['#default_value'] === TRUE && isset($form['steps']['concept']['user']['#states']['disabled']), 'Checkboxes show the saved choice and follow the main switch');
  ammo_mail_settings_test_assert(menu_get_item('admin/ammo/mail-settings')['access'], 'The mail settings permission opens the page');
  foreach (array('administer ammo', 'administer amendments and motions') as $permission) {
    $permissions[$user->uid] = array($permission => TRUE);
    drupal_static_reset('menu_get_item');
    ammo_mail_settings_test_assert(!menu_get_item('admin/ammo/mail-settings')['access'], 'The page needs its own permission, not ' . $permission);
  }

  $transaction->rollback();
  print "PASS: defaults, step per save, per-recipient switches, separate withdraw steps, main switch keeps the steps, page and access. Fixtures rolled back.\n";
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
