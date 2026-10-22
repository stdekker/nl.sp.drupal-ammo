<?php

/**
 * Local integration test: ddev drush scr <path-to-this-file>.
 * Fixtures and captured mail are rolled back; no external mail is sent.
 */
module_load_include('inc', 'ammo', 'ammo');
module_load_include('inc', 'ammo', 'amendment.form');
module_load_include('inc', 'ammo', 'motion.form');

function ammo_edit_test_assert($condition, $message) {
  if (!$condition) {
    throw new Exception($message);
  }
}

/**
 * Opens or closes the change range of a type.
 */
function ammo_edit_test_change_range($meeting, $type, $open) {
  $meeting->{$type . '_change_start'} = NULL;
  $meeting->{$type . '_change_end'} = $open ? '2099-01-01 00:00:00' : NULL;
  entity_save('meeting', $meeting);
}

global $user, $conf;
$previous_user = $user;
$previous_conf = $conf;
$conf['mail_system'] = array('default-system' => 'TestingMailSystem', 'ammo' => 'TestingMailSystem');
$transaction = db_transaction();
try {
  // Mail as with default settings, whatever the site has switched off.
  variable_del('ammo_mail_enabled');
  variable_del('ammo_mail_steps');
  $user = user_save(NULL, array('name' => 'ammo-edit-test-' . uniqid(), 'mail' => 'ammo-test@example.invalid', 'status' => 1, 'field_contact_id' => array(LANGUAGE_NONE => array(array('value' => 99999999)))));
  $permissions = &drupal_static('user_access');
  $permissions[$user->uid] = array('add amendments and motions' => TRUE);
  $branches = &drupal_static('ammo_get_afdelingen');
  $branches[99999999] = array(99999999 => 'Testafdeling', 99999997 => 'Mede-indiener');
  // Submission closed, changes open: only the change range matters for submitted items.
  $meeting = ammo_save_entity('meeting', array('title' => 'AMMO edit test', 'date' => '2099-02-01', 'amendment_submission_end' => '1999-01-01', 'motion_submission_end' => '1999-01-01', 'amendment_change_end' => '2099-01-01', 'motion_change_end' => '2099-01-01', 'withdrawal_closing_date' => '1999-01-01', 'email' => 'ammo-test@example.invalid'));
  $document = ammo_save_entity('document', array('title' => 'Testdocument', 'meeting_id' => $meeting->id));
  $original = ammo_save_entity('amendment', array('document_id' => $document->id, 'chapter' => 1, 'chapterized_id' => 1, 'page' => 1, 'line' => 1, 'amendment_text' => 'Oude tekst', 'supplement' => 'Toelichting', 'state' => 'assessed', 'advice' => 'reject', 'advice_supplement' => 'Advies'));
  $owner = ammo_save_entity('contact_relation', array('relation_type' => 'owner', 'target_type' => 'amendment', 'target_id' => $original->id, 'contact_type' => 'branch', 'contact_id' => 99999999, 'branch_id' => 99999999, 'contact_display_name' => 'Testafdeling', 'branch_display_name' => 'Testafdeling', 'contact_email' => 'owner@example.invalid'));
  $coowner = ammo_save_entity('contact_relation', array('relation_type' => 'owner', 'target_type' => 'amendment', 'target_id' => $original->id, 'contact_type' => 'branch', 'contact_id' => 99999997, 'branch_id' => 99999997, 'contact_display_name' => 'Mede-indiener', 'branch_display_name' => 'Mede-indiener', 'contact_email' => 'coowner@example.invalid'));
  ammo_edit_test_assert(ammo_add_access() && ammo_access(99999999) && ammo_access(99999997), 'Fixture must have submission permission and both branches.');

  ammo_edit_test_assert(ammo_edit_access($meeting->id, 99999999, 'amendment', $original->id), 'Owner may change inside the change range, also when assessed.');
  ammo_edit_test_assert(!ammo_edit_access($meeting->id, 99999997, 'amendment', $original->id), 'A co-submitting branch may not change it.');
  ammo_edit_test_assert(!ammo_edit_access($meeting->id, 123, 'amendment', $original->id), 'Another branch may not change it.');
  foreach (array('withdrawn', 'blocked') as $state) {
    $original->state = $state;
    entity_save('amendment', $original);
    ammo_edit_test_assert(!ammo_edit_access($meeting->id, 99999999, 'amendment', $original->id), 'Closed state ' . $state . ' must be denied.');
  }
  $original->state = 'assessed';
  $original->advice = 'accept';
  entity_save('amendment', $original);
  ammo_edit_test_assert(!ammo_edit_access($meeting->id, 99999999, 'amendment', $original->id), 'An amendment assessed as Overnemen may not be changed.');
  $original->advice = 'reject';
  entity_save('amendment', $original);

  $form_state = array();
  ammo_edit_test_assert(ammo_populate_form('amendment', $original->id, $form_state), 'Edit form must populate.');
  $storage = $form_state['storage'];
  $form_state['storage']['first_step'] = 5;
  $form = ammo_amendment_form(array(), $form_state);
  ammo_edit_test_assert(!isset($form['location']) && isset($form['amendment_text'], $form['supplement']) && drupal_get_title() === 'Voorstel wijzigen', 'The submitter changes only the text and explanation.');
  unset($form_state['step']);
  $permissions[$user->uid]['administer amendments and motions'] = TRUE;
  $admin_state = array();
  ammo_populate_form('amendment', $original->id, $admin_state);
  $admin_state['storage']['first_step'] = 5;
  $admin_form = ammo_amendment_form(array(), $admin_state);
  ammo_edit_test_assert(empty($admin_state['storage']['submitted_edit']) && isset($admin_form['location']['chapter']), 'Admins keep the location fields.');
  unset($permissions[$user->uid]['administer amendments and motions']);
  $form_state['step'] = 6;
  $form = ammo_amendment_form(array(), $form_state);
  ammo_edit_test_assert(strpos($form['confirm']['#title'], 'Gewijzigd') !== FALSE && strpos($form['confirm']['#title'], 'mede-indieners') !== FALSE, 'Confirmation explains the new status and the mail.');
  ammo_edit_test_assert(ammo_save_submitted_edit('amendment', $storage) === NULL, 'Unchanged form must be a no-op.');

  // The change is measured before confirming: none is refused, a small one is warned about.
  ammo_edit_test_assert(ammo_submitted_edit_change_percentage('amendment', $storage) === 0, 'Unchanged text is a change of 0%.');
  ammo_edit_test_assert(ammo_submitted_edit_change_percentage('amendment', array('amendment_text' => " Oude tekst\r\n", 'supplement' => "Toelichting\r\n") + $storage) === 0, 'Line endings and surrounding whitespace are not a change.');
  $long = str_repeat('Dit is een lange zin in het voorstel. ', 20);
  $original->amendment_text = $long;
  entity_save('amendment', $original);
  ammo_edit_test_assert(ammo_submitted_edit_small_change(ammo_submitted_edit_change_percentage('amendment', array('amendment_text' => $long . 'x') + $storage)), 'One extra character in a long text is a small change.');
  ammo_edit_test_assert(!ammo_submitted_edit_small_change(ammo_submitted_edit_change_percentage('amendment', array('amendment_text' => 'Een heel andere tekst') + $storage)), 'A rewritten text is not a small change.');
  $original->amendment_text = 'Oude tekst';
  entity_save('amendment', $original);
  $validate_state = array('step' => 5, 'storage' => $storage, 'values' => array('amendment_text' => 'Oude tekst', 'supplement' => 'Toelichting'));
  ammo_amendment_form_validate(array(), $validate_state);
  $errors = form_get_errors();
  ammo_edit_test_assert(isset($errors['amendment_text']) && strpos($errors['amendment_text'], 'niets gewijzigd') !== FALSE, 'An unchanged amendment is refused before confirming.');
  form_clear_error();
  drupal_get_messages();
  $warn_state = $form_state;
  $warn_state['storage']['submitted_edit_small_change'] = TRUE;
  $form = ammo_amendment_form(array(), $warn_state);
  ammo_edit_test_assert(isset($form['small_change_warning']) && strpos($form['small_change_warning']['#markup'], 'heel beperkt') !== FALSE, 'A small change is warned about above the confirmation.');
  $form = ammo_amendment_form(array(), $form_state);
  ammo_edit_test_assert(!isset($form['small_change_warning']), 'A larger change has no warning.');

  ammo_edit_test_change_range($meeting, 'amendment', FALSE);
  ammo_edit_test_assert(!ammo_edit_access($meeting->id, 99999999, 'amendment', $original->id), 'An empty change range denies changes.');
  ammo_edit_test_assert(ammo_save_submitted_edit('amendment', $storage) === FALSE, 'An open form must respect the closed range.');
  drupal_get_messages('error');
  ammo_edit_test_change_range($meeting, 'amendment', TRUE);

  variable_set('drupal_test_email_collector', array());
  $storage['amendment_text'] = 'Gewijzigde tekst';
  $saved = ammo_save_submitted_edit('amendment', $storage);
  ammo_edit_test_assert($saved && $saved->id == $original->id && $saved->chapterized_id == 1, 'The change is made in place and keeps its number.');
  ammo_edit_test_assert($saved->state === 'changed' && $saved->advice === 'reject' && $saved->advice_supplement === 'Advies', 'The status becomes Gewijzigd and the advice and its explanation are kept.');
  ammo_edit_test_assert(entity_load_single('contact_relation', $owner->id)->target_id == $original->id && entity_load_single('contact_relation', $coowner->id)->target_id == $original->id, 'Owner and co-submitter stay attached.');
  $recipients = array_map(function ($mail) { return $mail['to']; }, variable_get('drupal_test_email_collector', array()));
  ammo_edit_test_assert(in_array('coowner@example.invalid', $recipients, TRUE) && in_array('owner@example.invalid', $recipients, TRUE), 'The change mail reaches the owner and the co-submitter.');
  ammo_edit_test_assert(ammo_save_submitted_edit('amendment', $storage) === FALSE, 'Stale submission must fail.');
  drupal_get_messages();

  // A Gewijzigd item waits for a new assessment before the submitter can change it again.
  $edit_state = array();
  ammo_edit_test_assert(!ammo_edit_access($meeting->id, 99999999, 'amendment', $original->id) && !ammo_populate_form('amendment', $original->id, $edit_state), 'A Gewijzigd amendment cannot be changed again by the submitter.');
  drupal_get_messages();
  $reassessed = entity_load_single('amendment', $original->id);
  $reassessed->state = 'assessed';
  entity_save('amendment', $reassessed);
  ammo_edit_test_assert(ammo_edit_access($meeting->id, 99999999, 'amendment', $original->id), 'After a new assessment the submitter can change it again.');

  // The next change keeps the location and number.
  $form_state = array();
  ammo_populate_form('amendment', $original->id, $form_state);
  $storage = $form_state['storage'];
  $storage['chapter'] = 3;
  $storage['amendment_text'] = 'Nog eens gewijzigd';
  $saved = ammo_save_submitted_edit('amendment', $storage);
  ammo_edit_test_assert($saved && $saved->id == $original->id && $saved->amendment_text === 'Nog eens gewijzigd' && $saved->chapter == 1 && ammo_amendment_number($saved) === '1.1', 'A change keeps the location and the number.');
  drupal_get_messages();

  // Motions follow the same rules.
  $motion = ammo_save_entity('motion', array('meeting_id' => $meeting->id, 'motion_id' => 4, 'consideration_opening' => 'Overwegende dat', 'consideration_body' => 'Oude overweging', 'follow_up_opening' => 'Verzoekt het congres', 'follow_up_body' => 'Iets te doen', 'state' => 'submitted', 'advice' => 'reject'));
  ammo_save_entity('contact_relation', array('relation_type' => 'owner', 'target_type' => 'motion', 'target_id' => $motion->id, 'contact_type' => 'branch', 'contact_id' => 99999999, 'branch_id' => 99999999, 'contact_display_name' => 'Testafdeling', 'branch_display_name' => 'Testafdeling', 'contact_email' => 'owner@example.invalid'));
  ammo_save_entity('contact_relation', array('relation_type' => 'owner', 'target_type' => 'motion', 'target_id' => $motion->id, 'contact_type' => 'branch', 'contact_id' => 99999997, 'branch_id' => 99999997, 'contact_display_name' => 'Mede-indiener', 'branch_display_name' => 'Mede-indiener', 'contact_email' => 'coowner@example.invalid'));
  ammo_edit_test_assert(ammo_edit_access($meeting->id, 99999999, 'motion', $motion->id) && !ammo_edit_access($meeting->id, 99999997, 'motion', $motion->id), 'Only the submitting branch may change a motion.');
  $motion->state = 'assessed';
  $motion->advice = 'accept';
  entity_save('motion', $motion);
  ammo_edit_test_assert(!ammo_edit_access($meeting->id, 99999999, 'motion', $motion->id), 'A motion assessed as Overnemen may not be changed.');
  $motion->state = 'submitted';
  $motion->advice = 'reject';
  entity_save('motion', $motion);
  ammo_edit_test_change_range($meeting, 'motion', FALSE);
  ammo_edit_test_assert(!ammo_edit_access($meeting->id, 99999999, 'motion', $motion->id), 'An empty motion change range denies changes.');
  ammo_edit_test_change_range($meeting, 'motion', TRUE);

  $form_state = array('build_info' => array('args' => array('edit', $motion->id)), 'no_cache' => TRUE);
  $form = drupal_build_form('ammo_motion_form', $form_state);
  ammo_edit_test_assert(isset($form['consideration_text']) && !empty($form['confirm']['#required']), 'Motion edit form opens with a required confirmation.');
  // The form's last step ends in an AJAX redirect that exits, so save as its step 4 does.
  $edit = array('consideration_text' => 'Nieuwe overweging') + $form_state['storage'];
  variable_set('drupal_test_email_collector', array());
  ammo_edit_test_assert(ammo_save_submitted_edit('motion', $edit) !== FALSE, 'Motion change is saved.');
  entity_get_controller('motion')->resetCache();
  $changed = entity_load_single('motion', $motion->id);
  ammo_edit_test_assert($changed->consideration_body === 'Nieuwe overweging' && $changed->state === 'changed' && $changed->advice === 'reject', 'Motion is changed in place to Gewijzigd with its advice kept.');
  $recipients = array_map(function ($mail) { return $mail['to']; }, variable_get('drupal_test_email_collector', array()));
  ammo_edit_test_assert(in_array('coowner@example.invalid', $recipients, TRUE), 'The motion co-submitter gets the change mail.');
  ammo_edit_test_assert(!ammo_edit_access($meeting->id, 99999999, 'motion', $motion->id), 'A Gewijzigd motion cannot be changed again by the submitter.');
  drupal_get_messages();

  // An unchanged motion is refused; a small change needs a second submit after the warning.
  $long = str_repeat('Dit is een lange overweging van de motie. ', 20);
  $changed->consideration_body = $long;
  $changed->state = 'assessed';
  entity_save('motion', $changed);
  $form_state = array('build_info' => array('args' => array('edit', $motion->id)), 'no_cache' => TRUE);
  $form = drupal_build_form('ammo_motion_form', $form_state);
  $values = array('consideration_opening' => 'Overwegende dat', 'consideration_text' => $long, 'follow_up_opening' => 'Verzoekt het congres', 'follow_up_text' => 'Iets te doen', 'supplement' => '', 'confirm' => 1);
  $form_state['values'] = $values;
  ammo_motion_form_validate($form, $form_state);
  $errors = form_get_errors();
  ammo_edit_test_assert(isset($errors['consideration_text']) && strpos($errors['consideration_text'], 'niets gewijzigd') !== FALSE, 'An unchanged motion is refused.');
  form_clear_error();
  drupal_get_messages();
  $form_state['values'] = array('consideration_text' => $long . 'x') + $values;
  $form_state['triggering_element'] = array('#name' => 'action_button');
  ammo_motion_form_submit($form, $form_state);
  entity_get_controller('motion')->resetCache();
  ammo_edit_test_assert(!empty($form_state['storage']['submitted_edit_warned']) && !empty($form_state['rebuild']) && entity_load_single('motion', $motion->id)->consideration_body === $long, 'A small motion change is not saved on the first submit.');
  $form = ammo_motion_form(array(), $form_state, 'edit', $motion->id);
  ammo_edit_test_assert(isset($form['small_change_warning']) && $form_state['storage']['consideration_text'] === $long . 'x', 'The warning is shown with the changed text kept.');
  drupal_get_messages();

  $permissions[$user->uid] = array();
  ammo_edit_test_assert(!ammo_edit_access($meeting->id, 99999999, 'amendment', $original->id), 'Submission permission is required.');
  $permissions[$user->uid] = array('administer ammo' => TRUE);
  ammo_edit_test_change_range($meeting, 'amendment', FALSE);
  ammo_edit_test_assert(ammo_edit_access($meeting->id, 99999999, 'amendment', $original->id), 'Admins are not bound by the ranges.');
  $original->state = 'assessed';
  $original->advice = 'accept';
  entity_save('amendment', $original);
  ammo_edit_test_assert(ammo_edit_access($meeting->id, 99999999, 'amendment', $original->id), 'Admins may still change an item assessed as Overnemen.');
  $edit_state = array();
  ammo_populate_form('amendment', $original->id, $edit_state);
  ammo_edit_test_assert(empty($edit_state['storage']['submitted_edit']), 'Admins edit through the normal save.');
  print "PASS: owner-only access, change ranges, in-place change to Gewijzigd with advice kept, Overnemen is final, one change per assessment, text-only form, no-change refusal and small-change warning, fixed location and numbering, mail to co-submitters, stale saves, motions, admin bypass.\n";
}
finally {
  $transaction->rollback();
  $user = $previous_user;
  $conf = $previous_conf;
}
