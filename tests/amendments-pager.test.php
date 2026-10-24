<?php

/** Run with ddev drush scr; all fixtures are rolled back. */
module_load_include('inc', 'ammo', 'ammo');
module_load_include('inc', 'ammo', 'amendments-pager');
module_load_include('inc', 'ammo', 'amendments.form');

function ammo_pager_assert($condition, $message) {
  if (!$condition) {
    throw new Exception($message);
  }
}

global $user;
$previous_user = $user;
$previous_get = $_GET;
$transaction = db_transaction();
try {
  $user = user_load(1);
  $meeting = ammo_save_entity('meeting', array('title' => 'Pager test', 'hide_meeting' => 0));
  $first = ammo_save_entity('document', array('title' => 'Z document', 'meeting_id' => $meeting->id));
  $second = ammo_save_entity('document', array('title' => 'A document', 'meeting_id' => $meeting->id));
  $without = ammo_save_entity('document', array('title' => 'M document', 'meeting_id' => $meeting->id));
  $ids = array();
  foreach (array(1 => 'submitted', 2 => 'assessed', 3 => 'blocked', 4 => 'concept') as $chapter => $state) {
    $entry = ammo_save_entity('amendment', array('document_id' => $first->id, 'chapter' => $chapter, 'chapterized_id' => 1, 'state' => $state));
    $ids[$chapter] = $entry->id;
  }
  ammo_save_entity('amendment', array('document_id' => $second->id, 'chapter' => NULL, 'state' => 'submitted'));
  ammo_save_entity('amendment', array('document_id' => $second->id, 'chapter' => 0, 'state' => 'submitted'));
  Database::startLog('ammo_pager');
  $page = ammo_amendments_page($meeting->id, array('document' => $first->id, 'chapter' => 2));
  $queries = Database::getLog('ammo_pager');
  $document = $page['meeting']['documents'][$first->id];
  ammo_pager_assert(array_keys($document['amendments']) === array((int) $ids[2]), 'Only selected chapter records loaded');
  ammo_pager_assert(!isset($page['meeting']['documents'][$second->id]['amendments']), 'Other document records not loaded');
  ammo_pager_assert(array_sum($document['totals']) == 4, 'Totals cover entire document');
  $navigation = $page['chapter_navigation'];
  ammo_pager_assert(strpos($navigation, '<span class="navigation-label">Document:</span>') !== FALSE && strpos($navigation, '<span class="navigation-label">Hoofdstuk:</span>') !== FALSE && strpos($page['amendment_navigation'], '<span class="navigation-label">Voorstel:</span>') !== FALSE, 'Document, Hoofdstuk and Voorstel rows');
  ammo_pager_assert(preg_match('/<a [^>]*document=' . $first->id . '"[^>]*aria-current="page"/', $navigation) && !preg_match('/<a [^>]*document=' . $second->id . '"[^>]*aria-current/', $navigation), 'A link per document, the selected one current');
  ammo_pager_assert(substr_count($navigation, 'chapter=') === 4 && strpos($navigation, 'document=' . $second->id . '&amp;chapter') === FALSE && strpos($page['amendment_navigation'], '#amendment' . $ids[2]) !== FALSE, 'Only the selected document has chapter links, and the chapter its amendment links');
  ammo_pager_assert(count($document['chapters']) == 4, 'Admin sees all chapters');
  $_GET = array('document' => $first->id, 'chapter' => 2);
  $variables = array('meeting_id' => $meeting->id);
  template_preprocess_overview_amendments($variables);
  $destination = $variables['destination']['destination'];
  ammo_pager_assert(strpos($destination, 'document=' . $first->id) !== FALSE && strpos($destination, 'chapter=2') !== FALSE, 'Return destination preserves document and chapter');
  $_GET = $previous_get;
  $record_queries = 0;
  foreach ($queries as $query) {
    if (strpos($query['query'], 'a.id AS id') !== FALSE) {
      $record_queries++;
      ammo_pager_assert(strpos($query['query'], 'a.chapter =') !== FALSE && strpos($query['query'], 'a.document_id =') !== FALSE, 'Record SQL is chapter and document scoped');
    }
  }
  ammo_pager_assert($record_queries === 1, 'Exactly one amendment record query');
  $default = ammo_amendments_page($meeting->id, array());
  ammo_pager_assert($default['meeting']['selected_document'] == $first->id && $default['meeting']['selected_chapter'] == 1, 'Default ordering preserved');
  $invalid = ammo_amendments_page($meeting->id, array('document' => array(1), 'chapter' => 'invalid'));
  ammo_pager_assert($invalid['meeting']['selected_document'] == $first->id && $invalid['meeting']['selected_chapter'] == 1, 'Invalid selection falls back');
  $unchaptered = ammo_amendments_page($meeting->id, array('document' => $second->id, 'chapter' => 0));
  ammo_pager_assert(count($unchaptered['meeting']['documents'][$second->id]['amendments']) === 2, 'Null and zero chapters accessible');
  ammo_pager_assert(strpos($unchaptered['chapter_navigation'], 'Zonder hoofdstuk') !== FALSE, 'Unchaptered navigation label');
  ammo_pager_assert(preg_match('/<a [^>]*document=' . $without->id . '"[^>]*>M document</', $navigation) === 1, 'Document without amendments listed in navigation');
  $no_amendments = ammo_amendments_page($meeting->id, array('document' => $without->id));
  ammo_pager_assert($no_amendments['meeting']['selected_document'] == $without->id && $no_amendments['meeting']['documents'][$without->id]['amendments'] === array(), 'Document without amendments selectable');
  ammo_pager_assert(preg_match('/<a [^>]*document=' . $without->id . '"[^>]*aria-current="page"/', $no_amendments['chapter_navigation']) && strpos($no_amendments['chapter_navigation'], 'chapter=') === FALSE && $no_amendments['amendment_navigation'] === '', 'Document without amendments has no chapter or amendment rows');
  $_GET = array('document' => $without->id);
  $variables = array('meeting_id' => $meeting->id);
  template_preprocess_overview_amendments($variables);
  ammo_pager_assert(strpos($variables['destination']['destination'], 'chapter') === FALSE, 'Return destination without chapter for document without amendments');
  $_GET = $previous_get;
  $user = drupal_anonymous_user();
  $public = ammo_amendments_page($meeting->id, array('document' => $first->id, 'chapter' => 4));
  ammo_pager_assert(count($public['meeting']['documents'][$first->id]['chapters']) === 2, 'Hidden statuses absent from navigation');
  ammo_pager_assert($public['meeting']['selected_chapter'] == 1, 'Hidden chapter falls back');
  $user = user_save(NULL, array('name' => 'pager-test-' . uniqid(), 'status' => 1, 'field_contact_id' => array(LANGUAGE_NONE => array(array('value' => 99999999)))));
  ammo_save_entity('contact_relation', array('target_id' => $ids[4], 'target_type' => 'amendment', 'relation_type' => 'owner', 'contact_type' => 'member', 'contact_id' => 99999999));
  $owned = ammo_amendments_page($meeting->id, array('chapter' => 4));
  ammo_pager_assert($owned['meeting']['selected_chapter'] == 4 && count($owned['meeting']['documents'][$first->id]['chapters']) === 3, 'Owner sees own draft but not blocked chapter');
  $empty = ammo_save_entity('meeting', array('title' => 'Empty meeting', 'hide_meeting' => 0));
  ammo_pager_assert(!ammo_amendments_page($empty->id, array())['meeting']['documents'], 'Empty meeting handled');
  $meeting->hide_meeting = 1;
  entity_save('meeting', $meeting);
  ammo_pager_assert(!ammo_amendments_page($meeting->id, array())['meeting']['documents'], 'Hidden meeting loads no entries');
  $state = array('values' => array('meeting' => $meeting->id));
  ammo_overview_amendments_form_submit(array(), $state);
  ammo_pager_assert($state['redirect'] === 'ammo/amendments/' . $meeting->id, 'Meeting selection redirects');
  $transaction->rollback();
  print "PASS: scoped queries, chapters, documents, permissions, totals, defaults and meeting redirect. Fixtures rolled back.\n";
}
catch (Throwable $e) {
  $transaction->rollback();
  throw $e;
}
finally {
  $user = $previous_user;
  $_GET = $previous_get;
}
