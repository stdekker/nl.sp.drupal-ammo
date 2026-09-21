<?php

/**
 * Read ownership without merging member and branch contact identifiers.
 */
function ammo_amendment_edit_owners($id) {
  return db_select('ammo_contact_relations', 'r')->fields('r')
    ->condition('target_type', 'amendment')->condition('target_id', $id)
    ->condition('relation_type', 'owner')->orderBy('id')->execute()->fetchAllAssoc('id');
}

/**
 * Detect changes since the edit form was opened, including changed ownership.
 */
function ammo_amendment_edit_signature($id) {
  $amendment = db_select('ammo_amendments', 'a')->fields('a')
    ->condition('id', $id)->execute()->fetchAssoc();
  return hash('sha256', serialize(array($amendment, ammo_amendment_edit_owners($id))));
}

/**
 * Save a confirmed edit, keeping other owners' support attached to their text.
 * Returns NULL when nothing changed, FALSE on failure, or the saved amendment.
 */
function ammo_save_submitted_amendment($storage) {
  $id = $storage['submitted_edit'];
  $transaction = db_transaction();
  try {
    // Serialize edits and numbering within this document.
    db_select('ammo_documents', 'd')->fields('d', array('id'))
      ->condition('id', $storage['document'])->forUpdate()->execute()->fetchField();
    db_select('ammo_amendments', 'a')->fields('a', array('id'))
      ->condition('id', $id)->forUpdate()->execute()->fetchField();
    db_select('ammo_meetings', 'm')->fields('m', array('id'))
      ->condition('id', $storage['meeting'])->forUpdate()->execute()->fetchField();
    db_select('ammo_contact_relations', 'r')->fields('r', array('id'))
      ->condition('target_type', 'amendment')->condition('target_id', $id)
      ->condition('relation_type', 'owner')->forUpdate()->execute()->fetchAll();
    entity_get_controller('amendment')->resetCache(array($id));
    entity_get_controller('meeting')->resetCache(array($storage['meeting']));
    entity_get_controller('contact_relation')->resetCache();
    if (!ammo_edit_access($storage['meeting'], $storage['branch_id'], 'amendment', $id)
      || ammo_amendment_edit_signature($id) !== $storage['submitted_edit_signature']) {
      $transaction->rollback();
      drupal_set_message('Het voorstel, de indieners of de rechten zijn gewijzigd. Open het voorstel opnieuw.', 'error');
      return FALSE;
    }
    $original = entity_load_single('amendment', $id);
    $fields = array();
    foreach (array('chapter', 'page', 'line', 'amendment_text', 'supplement') as $field) {
      if ((string) $original->$field !== (string) $storage[$field]) {
        $fields[$field] = $storage[$field];
      }
    }
    if (!$fields) {
      unset($transaction);
      return NULL;
    }
    $owners = ammo_amendment_edit_owners($id);
    $first_owner = ammo_get_first_owner('amendment', $id);
    $split = count($owners) > 1;
    $saved = $split ? entity_create('amendment', array('document_id' => $original->document_id)) : clone $original;
    foreach (array('chapter', 'page', 'line', 'amendment_text', 'supplement') as $field) {
      $saved->$field = $storage[$field];
    }
    $saved->state = 'submitted';
    $saved->state_supplement = '';
    $saved->advice = '';
    $saved->advice_supplement = '';
    if ($split || $original->chapter != $saved->chapter || empty($original->chapterized_id)) {
      $saved->chapterized_id = 1 + db_query('SELECT MAX(chapterized_id) FROM {ammo_amendments} WHERE document_id = :document AND chapter = :chapter', array(':document' => $saved->document_id, ':chapter' => $saved->chapter))->fetchField();
    }
    if (entity_save('amendment', $saved) === FALSE) {
      throw new Exception('Het gewijzigde voorstel kon niet worden opgeslagen.');
    }
    if ($split) {
      $relation = entity_load_single('contact_relation', $first_owner['id']);
      $relation->target_id = $saved->id;
      if (entity_save('contact_relation', $relation) === FALSE) {
        throw new Exception('De indiener kon niet worden verplaatst.');
      }
      global $base_url;
      $short_url = shurly_shorten($base_url . '/ammo/support/add/member/amendment/' . $saved->id, ammo_shurly('amendment', $saved->id));
      if (empty($short_url['success'])) {
        watchdog('ammo', 'Steunlink voor voorstel @id mislukt: @error', array('@id' => $saved->id, '@error' => $short_url['error']), WATCHDOG_ERROR);
        throw new Exception('De steunlink kon niet worden aangemaakt. Het voorstel is niet gewijzigd.');
      }
    }
    // Commit before sending mail: an email failure must not undo the proposal.
    unset($transaction);
  }
  catch (Exception $e) {
    if (isset($transaction)) {
      $transaction->rollback();
    }
    entity_get_controller('amendment')->resetCache();
    entity_get_controller('contact_relation')->resetCache();
    watchdog_exception('ammo', $e);
    drupal_set_message($e->getMessage(), 'error');
    return FALSE;
  }

  try {
    if ($split) {
      ammo_amendment_replacement_mail($original, $saved, $owners, $first_owner, $storage['meeting']);
    }
    else {
      $differences = array();
      foreach ($fields as $field => $value) {
        $description = ammo_diff_fields($field);
        $differences[$field] = array('fieldname' => $description['name'], 'diff' => ammo_diffline($original->$field, $value));
      }
      if (!ammo_changes_update_mail('amendment', $saved->id, $differences, $storage['meeting'])) {
        drupal_set_message('Het voorstel is opgeslagen, maar niet alle e-mails konden worden verzonden.', 'warning');
      }
    }
  }
  catch (Exception $e) {
    watchdog_exception('ammo', $e);
    drupal_set_message('Het voorstel is opgeslagen, maar niet alle e-mails konden worden verzonden.', 'warning');
  }
  return $saved;
}

function ammo_amendment_replacement_mail($original, $saved, $owners, $first_owner, $meeting_id) {
  global $language;
  $sender = ammo_mailadress($meeting_id);
  $params = ammo_mail_get_params('amendment', $saved->id, $meeting_id, $sender);
  $params['old_id'] = $original->id;
  $params['old_number'] = $original->chapter . '.' . $original->chapterized_id;
  $recipients = array();
  foreach ($owners as $owner) {
    if ($owner->id != $first_owner['id'] && !empty($owner->contact_email)) {
      $recipients[strtolower(trim($owner->contact_email))] = trim($owner->contact_email);
    }
  }
  foreach ($recipients as $recipient) {
    $result = drupal_mail('ammo', 'amendment_replacement', $recipient, $language, $params, $sender, TRUE);
    if (empty($result['result'])) {
      watchdog('ammo', 'Notificatie voor nieuw voorstel @id kon niet worden verzonden.', array('@id' => $saved->id), WATCHDOG_ERROR);
      drupal_set_message('Het voorstel is opgeslagen, maar niet alle e-mails konden worden verzonden.', 'warning');
    }
  }
}
