<?php

/**
 * Shared visibility for chapter summaries and the selected chapter query.
 */
function ammo_amendments_visible_query($meeting_id) {
  $query = db_select('ammo_amendments', 'a');
  $query->join('ammo_documents', 'd', 'd.id = a.document_id');
  $query->condition('d.meeting_id', $meeting_id);
  if (!ammo_admin_access()) {
    global $user;
    $contact_id = NULL;
    $wrapper = entity_metadata_wrapper('user', $user);
    try {
      if (isset($wrapper->field_contact_id)) {
        $contact_id = $wrapper->field_contact_id->value();
      }
    }
    catch (EntityMetadataWrapperException $e) {
      // Accounts without a contact cannot own member drafts.
    }
    $query->condition('a.state', 'blocked', '<>');
    $visible = db_or()->condition('a.state', 'concept', '<>');
    if (!empty($contact_id)) {
      $owned = db_select('ammo_contact_relations', 'r');
      $owned->addExpression('1');
      $owned->where('r.target_id = a.id');
      $owned->condition('r.target_type', 'amendment')->condition('r.relation_type', 'owner')->condition('r.contact_type', 'member')->condition('r.contact_id', $contact_id);
      $visible->exists($owned);
    }
    $query->condition($visible);
  }
  return $query;
}

function ammo_amendments_page($meeting_id, array $selection) {
  $entity = entity_load_single('meeting', $meeting_id);
  $meeting = array('documents' => array(), 'hide' => TRUE);
  if (!$entity) {
    return array('meeting' => $meeting, 'chapter_navigation' => '');
  }
  $meeting += array('id' => $entity->id, 'admin_access' => ammo_admin_access());
  $meeting['hide'] = $entity->hide_meeting;
  if ($meeting['hide']) {
    return array('meeting' => $meeting, 'chapter_navigation' => '');
  }
  $documents = db_query('SELECT id, title FROM {ammo_documents} WHERE meeting_id = :meeting ORDER BY title DESC', array(':meeting' => $meeting_id))->fetchAllKeyed();
  $summary = ammo_amendments_visible_query($meeting_id);
  $summary->fields('a', array('document_id', 'state'));
  // Legacy rows group by chapter, where NULL and 0 share the ungrouped bucket.
  $summary->addExpression('COALESCE(a.group_value, a.chapter, 0)', 'chapter_number');
  $summary->addExpression('COUNT(*)', 'total');
  $summary->groupBy('a.document_id')->groupBy('a.state')->groupBy('chapter_number');
  foreach ($summary->execute() as $row) {
    $id = $row->document_id;
    if (!isset($meeting['documents'][$id])) {
      $meeting['documents'][$id] = array('id' => $id, 'title' => check_plain($documents[$id]), 'chapters' => array(), 'totals' => array_fill_keys(array_keys(ammo_states()), 0));
    }
    $meeting['documents'][$id]['chapters'][$row->chapter_number] = $row->chapter_number;
    if (!isset($meeting['documents'][$id]['totals'][$row->state])) {
      $meeting['documents'][$id]['totals'][$row->state] = 0;
    }
    $meeting['documents'][$id]['totals'][$row->state] += $row->total;
  }
  $meeting['documents'] = array_replace(array_intersect_key($documents, $meeting['documents']), $meeting['documents']);
  if (!$meeting['documents']) {
    return array('meeting' => $meeting, 'chapter_navigation' => '');
  }
  $entities = entity_load('document', array_keys($meeting['documents']));
  $groupings = array();
  foreach ($meeting['documents'] as $id => &$document) {
    $groupings[$id] = ammo_grouping_locator(ammo_document_locators($entities[$id]));
    $type = $groupings[$id]['type'];
    uksort($document['chapters'], function ($a, $b) use ($type) {
      return ammo_compare_locator_values($type, $a, $b);
    });
  }
  unset($document);
  $document_id = isset($selection['document']) && is_scalar($selection['document']) ? (string) $selection['document'] : '';
  if (!isset($meeting['documents'][$document_id])) {
    $document_id = key($meeting['documents']);
  }
  $chapters = $meeting['documents'][$document_id]['chapters'];
  $chapter = isset($selection['chapter']) && is_scalar($selection['chapter']) ? ammo_normalize_locator_value($groupings[$document_id], $selection['chapter']) : '';
  if (!isset($chapters[$chapter])) {
    $chapter = key($chapters);
  }
  $meeting['selected_document'] = $document_id;
  $meeting['selected_chapter'] = $chapter;
  $query = ammo_amendments_visible_query($meeting_id);
  $query->fields('a', array('id', 'chapter', 'group_value', 'chapterized_id', 'state'));
  $query->condition('a.document_id', $document_id);
  if (empty($groupings[$document_id]['legacy'])) {
    $query->condition('a.group_value', $chapter);
  }
  elseif ((string) $chapter === '0') {
    $query->condition(db_or()->condition('a.chapter', 0)->isNull('a.chapter'));
  }
  else {
    $query->condition('a.chapter', $chapter);
  }
  $query->orderBy('a.chapterized_id')->orderBy('a.id');
  $amendments = $query->execute()->fetchAllAssoc('id', PDO::FETCH_ASSOC);
  foreach ($amendments as &$amendment) {
    $amendment['number'] = check_plain(ammo_amendment_number($amendment));
  }
  unset($amendment);
  $meeting['documents'][$document_id]['amendments'] = $amendments;
  // Each row is a label and links; the current document and chapter decide the rows below them.
  $items = array();
  foreach ($meeting['documents'] as $document) {
    $items[] = ammo_amendments_navigation_link($document['title'], $meeting_id, array('document' => $document['id']), $document['id'] == $document_id);
  }
  $navigation = ammo_amendments_navigation_row('Document', 'documents', $items);
  $grouping = $groupings[$document_id];
  $items = array();
  foreach ($meeting['documents'][$document_id]['chapters'] as $number) {
    $title = !$number && !empty($grouping['legacy']) ? 'Zonder ' . ammo_locator_inline_label($grouping) : check_plain($number);
    $items[] = ammo_amendments_navigation_link($title, $meeting_id, array('document' => $document_id, 'chapter' => $number), $number == $chapter);
  }
  $navigation .= ammo_amendments_navigation_row(check_plain($grouping['label']), 'chapters', $items);
  $items = array();
  foreach ($amendments as $amendment) {
    $items[] = '<a href="#amendment' . $amendment['id'] . '">' . $amendment['number'] . '</a>';
  }
  $amendment_navigation = $items ? ammo_amendments_navigation_row('Voorstel', 'amendments', $items) : '';
  return array('meeting' => $meeting, 'chapter_navigation' => $navigation, 'amendment_navigation' => $amendment_navigation);
}

/**
 * Link in the amendments navigation; the current one is marked for screen readers and styling.
 */
function ammo_amendments_navigation_link($title, $meeting_id, array $query, $current) {
  $options = array('query' => $query, 'html' => TRUE);
  if ($current) {
    $options['attributes'] = array('aria-current' => 'page');
  }
  return l($title, 'ammo/amendments/' . $meeting_id, $options);
}

/**
 * One row of the amendments navigation: a label followed by its links.
 */
function ammo_amendments_navigation_row($label, $class, array $items) {
  return '<div class="navigation-row ' . $class . '"><span class="navigation-label">' . $label . ':</span><ul><li>' . implode('</li><li>', $items) . '</li></ul></div>';
}
