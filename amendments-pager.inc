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
  $summary->addExpression('COALESCE(a.chapter, 0)', 'chapter_number');
  $summary->addExpression('COUNT(*)', 'total');
  $summary->groupBy('a.document_id')->groupBy('a.state')->groupBy('a.chapter');
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
  foreach ($meeting['documents'] as &$document) {
    ksort($document['chapters'], SORT_NUMERIC);
  }
  unset($document);
  $document_id = isset($selection['document']) && is_scalar($selection['document']) ? (string) $selection['document'] : '';
  if (!isset($meeting['documents'][$document_id])) {
    $document_id = key($meeting['documents']);
  }
  $chapters = $meeting['documents'][$document_id]['chapters'];
  $chapter = isset($selection['chapter']) && is_scalar($selection['chapter']) ? (string) $selection['chapter'] : '';
  if (!isset($chapters[$chapter])) {
    $chapter = key($chapters);
  }
  $meeting['selected_document'] = $document_id;
  $meeting['selected_chapter'] = $chapter;
  $query = ammo_amendments_visible_query($meeting_id);
  $query->fields('a', array('id', 'chapter', 'chapterized_id', 'state'));
  $query->condition('a.document_id', $document_id);
  if ((int) $chapter === 0) {
    $query->condition(db_or()->condition('a.chapter', 0)->isNull('a.chapter'));
  }
  else {
    $query->condition('a.chapter', $chapter);
  }
  $query->orderBy('a.chapterized_id')->orderBy('a.id');
  $meeting['documents'][$document_id]['amendments'] = $query->execute()->fetchAllAssoc('id', PDO::FETCH_ASSOC);
  $navigation = '';
  $bottom_navigation = '';
  foreach ($meeting['documents'] as $document) {
    $title = $document['title'];
    $heading = $document['id'] == $document_id ? 'h1' : 'h2';
    $links = '<ul class="chapters tab"><li class="chapter"><span>Hoofdstuk:</span></li>';
    foreach ($document['chapters'] as $number) {
      $options = array('query' => array('document' => $document['id'], 'chapter' => $number));
      if ($document['id'] == $document_id && $number == $chapter) {
        $options['attributes'] = array('class' => array('active'), 'aria-current' => 'page');
      }
      $links .= '<li class="chapter">' . l($number ? $number : t('Zonder hoofdstuk'), 'ammo/amendments/' . $meeting_id, $options) . '</li>';
    }
    $navigation .= '<div class="document"><' . $heading . '>' . $title . '</' . $heading . '>' . $links . '</ul></div>';
    $bottom_navigation .= '<div class="document"><span class="document-name">' . $title . '</span>' . $links . '</ul></div>';
  }
  return array('meeting' => $meeting, 'chapter_navigation' => $navigation, 'bottom_chapter_navigation' => $bottom_navigation);
}
