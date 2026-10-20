<?php

/**
 * Run with ddev drush scr. All fixtures are rolled back.
 */
module_load_include('inc', 'ammo', 'ammo');
module_load_include('inc', 'ammo', 'amendment.form');
module_load_include('inc', 'ammo', 'motion.form');

function ammo_assessed_advice_assert($condition, $message) {
  if (!$condition) {
    throw new Exception($message);
  }
}

/**
 * The form errors of one validation call on the status/advies step, cleared afterwards.
 */
function ammo_assessed_advice_errors($type, $step, array $values) {
  drupal_static_reset('form_set_error');
  $form_state = array('step' => $step, 'values' => $values, 'storage' => array());
  call_user_func_array('ammo_' . $type . '_form_validate', array(array(), &$form_state));
  $errors = form_get_errors() ?: array();
  drupal_static_reset('form_set_error');
  drupal_get_messages('error');
  return $errors;
}

global $user;
$previous_user = $user;
$transaction = db_transaction();
try {
  $user = user_load(1);
  $meeting = ammo_save_entity('meeting', array('title' => 'Advice test', 'date' => '2099-01-01', 'amendment_submission_end' => '2099-01-01', 'motion_submission_end' => '2099-01-01', 'withdrawal_closing_date' => '2099-01-01', 'hide_meeting' => 0));
  $document = ammo_save_entity('document', array('title' => 'Advice document', 'meeting_id' => $meeting->id));
  $amendment = ammo_save_entity('amendment', array('document_id' => $document->id, 'chapter' => 1, 'chapterized_id' => 1, 'page' => 1, 'line' => 1, 'amendment_text' => 'Tekst', 'state' => 'submitted'));
  // The status/advies form only opens items with a first owner.
  ammo_save_entity('contact_relation', array('relation_type' => 'owner', 'target_type' => 'amendment', 'target_id' => $amendment->id, 'contact_type' => 'branch', 'contact_id' => 99999999, 'branch_id' => 99999999, 'contact_display_name' => 'Testafdeling', 'branch_display_name' => 'Testafdeling'));

  foreach (array('amendment' => 7, 'motion' => 5) as $type => $step) {
    $errors = ammo_assessed_advice_errors($type, $step, array('state' => 'assessed', 'advice' => ''));
    ammo_assessed_advice_assert(isset($errors['advice']) && strpos($errors['advice'], 'Beoordeeld') !== FALSE, $type . ': Beoordeeld without advice is refused');
    ammo_assessed_advice_assert(!ammo_assessed_advice_errors($type, $step, array('state' => 'assessed', 'advice' => 'reject')), $type . ': Beoordeeld with advice is accepted');
    ammo_assessed_advice_assert(!ammo_assessed_advice_errors($type, $step, array('state' => 'submitted', 'advice' => '')), $type . ': other statuses need no advice');
  }

  // The real form: the advice field is marked required for Beoordeeld, and the save is refused.
  $form_state = array('build_info' => array('args' => array('advice', $amendment->id)), 'no_cache' => TRUE);
  $form = drupal_build_form('ammo_amendment_form', $form_state);
  ammo_assessed_advice_assert($form['advice']['#states']['required'][':input[name="state"]']['value'] === 'assessed', 'Advice field is marked required for Beoordeeld');
  $form_state = array('values' => array('state' => 'assessed', 'advice' => '', 'action_button' => $form['buttons']['forward']['#value']), 'build_info' => array('args' => array('advice', $amendment->id)));
  drupal_form_submit('ammo_amendment_form', $form_state, 'advice', $amendment->id);
  entity_get_controller('amendment')->resetCache();
  ammo_assessed_advice_assert(isset(form_get_errors()['advice']) && entity_load_single('amendment', $amendment->id)->state === 'submitted', 'Form refuses Beoordeeld without advice and saves nothing');
  drupal_get_messages();

  $transaction->rollback();
  print "PASS: Beoordeeld requires an advice on the amendment and motion status/advies steps; other statuses and the saved item unaffected. Fixtures rolled back.\n";
}
catch (Throwable $e) {
  $transaction->rollback();
  throw $e;
}
finally {
  $user = $previous_user;
}
