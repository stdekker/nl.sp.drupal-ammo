<?php

/**
 * Rows per page of the status overview.
 */
define('AMMO_STATUS_OVERVIEW_PAGE_SIZE', 50);

/**
 * The meeting, document and chapter shown, from the query with fallbacks; the document 'motions' lists the motions.
 */
function ammo_status_overview_selection(array $query) {
  $selection = array('meeting' => NULL, 'document' => NULL, 'chapter' => '', 'page' => isset($query['page']) && is_scalar($query['page']) ? max(0, (int) $query['page']) : 0, 'meetings' => array(), 'documents' => array(), 'chapters' => array(), 'grouping' => NULL);
  $selection['meetings'] = db_query('SELECT id, title FROM {ammo_meetings} ORDER BY id DESC')->fetchAllKeyed();
  $meeting_id = isset($query['meeting']) && is_scalar($query['meeting']) ? (string) $query['meeting'] : '';
  if (!isset($selection['meetings'][$meeting_id])) {
    $meeting_id = key($selection['meetings']);
  }
  if (!$meeting_id) {
    return $selection;
  }
  $selection['meeting'] = $meeting_id;
  $selection['documents'] = db_query('SELECT id, title FROM {ammo_documents} WHERE meeting_id = :meeting ORDER BY title', array(':meeting' => $meeting_id))->fetchAllKeyed();
  if (ammo_meeting_has_motions(entity_load_single('meeting', $meeting_id))) {
    $selection['documents']['motions'] = 'Moties';
  }
  $document_id = isset($query['document']) && is_scalar($query['document']) ? (string) $query['document'] : '';
  if (!isset($selection['documents'][$document_id])) {
    $document_id = key($selection['documents']);
  }
  $selection['document'] = $document_id;
  if ($document_id === NULL || $document_id === 'motions') {
    return $selection;
  }
  $selection['grouping'] = ammo_grouping_locator(ammo_document_locators($document_id));
  // Legacy rows group by chapter, where NULL and 0 share the ungrouped bucket, as on the amendments overview.
  $chapters = db_query('SELECT DISTINCT COALESCE(group_value, chapter, 0) FROM {ammo_amendments} WHERE document_id = :document', array(':document' => $document_id))->fetchCol();
  $type = $selection['grouping']['type'];
  usort($chapters, function ($a, $b) use ($type) {
    return ammo_compare_locator_values($type, $a, $b);
  });
  foreach ($chapters as $chapter) {
    $selection['chapters'][$chapter] = !$chapter && !empty($selection['grouping']['legacy']) ? 'Zonder ' . ammo_locator_inline_label($selection['grouping']) : $chapter;
  }
  $chapter = isset($query['chapter']) && is_scalar($query['chapter']) ? (string) $query['chapter'] : '';
  $selection['chapter'] = isset($selection['chapters'][$chapter]) ? $chapter : '';
  return $selection;
}

/**
 * The query that leads back to a selection and its page, for links and redirects.
 */
function ammo_status_overview_query(array $selection) {
  return array_filter(array('meeting' => $selection['meeting'], 'document' => $selection['document'], 'chapter' => $selection['chapter'], 'page' => $selection['page'] ?: ''), 'strlen');
}

/**
 * The first ten words of a proposal text, followed by an ellipsis when there are more.
 */
function ammo_status_overview_preview($text) {
  $words = preg_split('/\s+/u', trim(decode_entities(strip_tags((string) $text))), -1, PREG_SPLIT_NO_EMPTY);
  return implode(' ', array_slice($words, 0, 10)) . (count($words) > 10 ? ' …' : '');
}

/**
 * Table rows of the selection, keyed by "type:id", with the number, location, preview, status and advice.
 */
function ammo_status_overview_rows(array $selection) {
  if (empty($selection['document'])) {
    return array();
  }
  $type = $selection['document'] === 'motions' ? 'motion' : 'amendment';
  if ($type === 'motion') {
    $items = db_query('SELECT id, motion_id, consideration_body AS text, state, advice, advice_supplement, created, changed FROM {ammo_motions} WHERE meeting_id = :meeting ORDER BY motion_id, id', array(':meeting' => $selection['meeting']))->fetchAllAssoc('id', PDO::FETCH_ASSOC);
  }
  else {
    $query = db_select('ammo_amendments', 'a')
      ->fields('a', array('id', 'chapter', 'group_value', 'chapterized_id', 'page', 'line', 'locator_values', 'state', 'advice', 'advice_supplement', 'created', 'changed'))
      ->condition('a.document_id', $selection['document']);
    $query->addField('a', 'amendment_text', 'text');
    if ($selection['chapter'] !== '') {
      $query->where('COALESCE(a.group_value, a.chapter, 0) = :chapter', array(':chapter' => $selection['chapter']));
    }
    $items = $query->execute()->fetchAllAssoc('id', PDO::FETCH_ASSOC);
    $document = entity_load_single('document', $selection['document']);
    ammo_sort_amendments($items, ammo_document_locators($document));
  }
  // The submitter and the actions take several queries per item, so only one page of rows is built.
  $page = pager_default_initialize(count($items), AMMO_STATUS_OVERVIEW_PAGE_SIZE);
  $items = array_slice($items, $page * AMMO_STATUS_OVERVIEW_PAGE_SIZE, AMMO_STATUS_OVERVIEW_PAGE_SIZE, TRUE);
  $destination = array('destination' => 'admin/ammo/status' . (($query = drupal_http_build_query(ammo_status_overview_query($selection))) ? '?' . $query : ''));
  $states = ammo_states();
  $advices = $type === 'amendment' ? ammo_amendment_advice() : ammo_motion_advice();
  $rows = array();
  foreach ($items as $id => $item) {
    $path = ammo_item_view_path($type, $id);
    $preview = ammo_status_overview_preview($item['text']);
    // The same submitter and actions as on the item itself.
    $variables = array('entity_id' => $id, 'destination' => $destination);
    $type === 'amendment' ? template_preprocess_amendment($variables) : template_preprocess_motion($variables);
    $rows[$type . ':' . $id] = array(
      'number' => l(ammo_status_overview_number($type, $item), $path),
      'location' => $type === 'amendment' ? check_plain(ammo_amendment_location_short($item, $document)) : '',
      'first_owner' => empty($variables['first_owner']) ? '' : ammo_contact_names(array($variables['first_owner'])),
      'text' => $preview === '' ? '' : l($preview, $path),
      'state' => isset($states[$item['state']]) ? $states[$item['state']] : check_plain($item['state']),
      'advice' => isset($advices[$item['advice']]) ? $advices[$item['advice']] : check_plain($item['advice']),
      'advice_supplement' => nl2br(check_plain($item['advice_supplement'])),
      'created' => ammo_item_date($item, 'created'),
      'changed' => ammo_item_date($item, 'changed'),
      'actions' => ammo_status_overview_actions($variables['actions']),
      '#attributes' => array('class' => array(drupal_html_class($item['state']))),
    );
  }
  return $rows;
}

/**
 * Actions of an item as a dropdown: the CTools dropbutton, as on the Views listing; a plain list without CTools.
 */
function ammo_status_overview_actions(array $links) {
  if (!$links) {
    return '';
  }
  if (module_exists('ctools')) {
    return '<div class="ammo-status-actions">' . theme('links__ctools_dropbutton', array('title' => 'Acties', 'links' => $links)) . '</div>';
  }
  return theme('links', array('links' => $links, 'attributes' => array('class' => array('ammo-status-actions'))));
}

/**
 * The number an amendment or motion is known by.
 */
function ammo_status_overview_number($type, $item) {
  $item = (array) $item;
  if ($type === 'amendment') {
    return ammo_amendment_number($item);
  }
  return !empty($item['motion_id']) ? $item['motion_id'] : $item['id'];
}

/**
 * Whether the user may change statuses of the meeting here, as on the status/advies form: admins until the meeting date, superadmins always.
 */
function ammo_status_overview_edit_access($meeting_id) {
  return ammo_superadmin_access() || (ammo_admin_access() && ammo_advice_access($meeting_id));
}

/**
 * Sets the status of one item and mails it as a status/advies save; returns changed, unchanged, no_advice or failed.
 */
function ammo_status_overview_set_state($type, $id, $state) {
  $entity = entity_load_single($type, $id);
  if (!$entity) {
    return 'failed';
  }
  if ($entity->state === $state) {
    return 'unchanged';
  }
  // Beoordeeld needs an advice, which gives the outcome; see ammo_validate_assessed_advice().
  if ($state === 'assessed' && empty($entity->advice)) {
    return 'no_advice';
  }
  $previous = $entity->state;
  if (!ammo_save_entity($type, array('state' => $state), $id)) {
    return 'failed';
  }
  $meeting_id = ammo_get_meeting_id($type, $id);
  $meeting = entity_load_single('meeting', $meeting_id);
  // As in ammo_store(), a hidden status is not mailed.
  if (empty($meeting->hide_state)) {
    $states = ammo_states();
    $field = ammo_diff_fields('state');
    $differences = array('state' => array('fieldname' => $field['name'], 'diff' => (isset($states[$previous]) ? $states[$previous] : $previous) . ' → ' . $states[$state]));
    ammo_changes_update_mail($type, $id, $differences, $meeting_id, 'advice');
  }
  return 'changed';
}
