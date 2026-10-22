<?php

/**
 * The table of an amendment or motion.
 */
function ammo_submitted_edit_table($type) {
  return $type === 'motion' ? 'ammo_motions' : 'ammo_amendments';
}

/**
 * Read ownership without merging member and branch contact identifiers.
 */
function ammo_submitted_edit_owners($type, $id) {
  return db_select('ammo_contact_relations', 'r')->fields('r')
    ->condition('target_type', $type)->condition('target_id', $id)
    ->condition('relation_type', 'owner')->orderBy('id')->execute()->fetchAllAssoc('id');
}

/**
 * Detect changes since the edit form was opened, including changed ownership.
 */
function ammo_submitted_edit_signature($type, $id) {
  $row = db_select(ammo_submitted_edit_table($type), 't')->fields('t')
    ->condition('id', $id)->execute()->fetchAssoc();
  return hash('sha256', serialize(array($row, ammo_submitted_edit_owners($type, $id))));
}

/**
 * The fields the owner may change, keyed by entity property with the form storage key as value.
 */
function ammo_submitted_edit_fields($type, $entity) {
  if ($type === 'motion') {
    return array(
      'consideration_opening' => 'consideration_opening',
      'consideration_body' => 'consideration_text',
      'follow_up_opening' => 'follow_up_opening',
      'follow_up_body' => 'follow_up_text',
      'supplement' => 'supplement',
    );
  }
  // The location, and with it the number, stays as submitted.
  return array('amendment_text' => 'amendment_text', 'supplement' => 'supplement');
}

/**
 * How much of the editable text a change alters, from 0 (nothing) to 100 percent.
 */
function ammo_submitted_edit_change_percentage($type, $values) {
  $original = entity_load_single($type, $values['submitted_edit']);
  $old = $new = array();
  foreach (ammo_submitted_edit_fields($type, $original) as $field => $key) {
    // Line endings and surrounding whitespace from the browser are not a change.
    $old[] = trim(str_replace("\r\n", "\n", (string) $original->$field));
    $new[] = trim(str_replace("\r\n", "\n", isset($values[$key]) ? (string) $values[$key] : ''));
  }
  $old = implode("\n", $old);
  $new = implode("\n", $new);
  if ($old === $new) {
    return 0;
  }
  similar_text($old, $new, $similarity);
  return max(100 - $similarity, 0.01);
}

/**
 * Whether a change is so small that the submitter is asked to make sure.
 */
function ammo_submitted_edit_small_change($percentage) {
  return $percentage > 0 && $percentage < 5;
}

/**
 * The warning shown above the confirmation of a small change.
 */
function ammo_submitted_edit_small_change_warning() {
  return '<div class="messages warning">Het lijkt er op dat de wijziging heel beperkt is, weet je zeker dat je dit zo wil indienen?</div>';
}

/**
 * The confirmation text for an owner's change to a submitted item.
 */
function ammo_submitted_edit_confirmation() {
  return 'Ik bevestig de wijziging van dit voorstel. De status wordt Gewijzigd tot het opnieuw is beoordeeld, en alle indieners, ook de mede-indieners, krijgen een e-mail met de wijziging.';
}

/**
 * Save the owner's change to a submitted amendment or motion in place and mail every owner.
 * Returns NULL when nothing changed, FALSE on failure, or the saved entity.
 */
function ammo_save_submitted_edit($type, $storage) {
  $id = $storage['submitted_edit'];
  $transaction = db_transaction();
  try {
    // Serialize edits and numbering within this document or meeting.
    if ($type === 'amendment') {
      db_select('ammo_documents', 'd')->fields('d', array('id'))
        ->condition('id', $storage['document'])->forUpdate()->execute()->fetchField();
    }
    db_select(ammo_submitted_edit_table($type), 't')->fields('t', array('id'))
      ->condition('id', $id)->forUpdate()->execute()->fetchField();
    db_select('ammo_meetings', 'm')->fields('m', array('id'))
      ->condition('id', $storage['meeting'])->forUpdate()->execute()->fetchField();
    db_select('ammo_contact_relations', 'r')->fields('r', array('id'))
      ->condition('target_type', $type)->condition('target_id', $id)
      ->condition('relation_type', 'owner')->forUpdate()->execute()->fetchAll();
    entity_get_controller($type)->resetCache(array($id));
    entity_get_controller('meeting')->resetCache(array($storage['meeting']));
    entity_get_controller('contact_relation')->resetCache();
    if (!ammo_edit_access($storage['meeting'], $storage['branch_id'], $type, $id)
      || ammo_submitted_edit_signature($type, $id) !== $storage['submitted_edit_signature']) {
      $transaction->rollback();
      drupal_set_message('Het voorstel, de indieners of de rechten zijn gewijzigd. Open het voorstel opnieuw.', 'error');
      return FALSE;
    }
    $original = entity_load_single($type, $id);
    $editable = ammo_submitted_edit_fields($type, $original);
    $fields = array();
    foreach ($editable as $field => $key) {
      if ((string) $original->$field !== (string) $storage[$key]) {
        $fields[$field] = $storage[$key];
      }
    }
    if (!$fields) {
      unset($transaction);
      return NULL;
    }
    $saved = clone $original;
    foreach ($fields as $field => $value) {
      $saved->$field = $value;
    }
    // The advice and its explanation stay so the assessment can be redone against the new text.
    $saved->state = 'changed';
    if ($type === 'amendment') {
      $group = ammo_amendment_group($saved);
      if ($group !== ammo_amendment_group($original) || empty($original->chapterized_id)) {
        $saved->chapterized_id = ammo_next_chapterized_id($saved->document_id, $group);
      }
    }
    if (entity_save($type, $saved) === FALSE) {
      throw new Exception('Het gewijzigde voorstel kon niet worden opgeslagen.');
    }
    // Commit before sending mail: an email failure must not undo the change.
    unset($transaction);
  }
  catch (Exception $e) {
    if (isset($transaction)) {
      $transaction->rollback();
    }
    entity_get_controller($type)->resetCache();
    entity_get_controller('contact_relation')->resetCache();
    watchdog_exception('ammo', $e);
    drupal_set_message($e->getMessage(), 'error');
    return FALSE;
  }

  try {
    $differences = array();
    foreach ($fields as $field => $value) {
      $description = ammo_diff_fields($editable[$field]);
      $differences[$field] = array('fieldname' => $description['name'], 'diff' => ammo_diffline($original->$field, $value));
    }
    // The change mail goes to every owner branch, co-submitters included.
    if (!ammo_changes_update_mail($type, $saved->id, $differences, $storage['meeting'], 'submitted_edit')) {
      drupal_set_message('Het voorstel is opgeslagen, maar niet alle e-mails konden worden verzonden.', 'warning');
    }
  }
  catch (Exception $e) {
    watchdog_exception('ammo', $e);
    drupal_set_message('Het voorstel is opgeslagen, maar niet alle e-mails konden worden verzonden.', 'warning');
  }
  return $saved;
}
