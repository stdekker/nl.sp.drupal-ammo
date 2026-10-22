<?php

/** Run with ddev drush scr after drush cc all; fixtures are rolled back. */
module_load_include('inc', 'ammo', 'ammo');
module_load_include('inc', 'ammo', 'item-view');

function ammo_item_view_assert($condition, $message) {
  if (!$condition) {
    throw new Exception($message);
  }
}

function ammo_item_view_test_owner($type, $id, $contact_id, $contact_type = 'branch') {
  ammo_save_entity('contact_relation', array('target_id' => $id, 'target_type' => $type, 'relation_type' => 'owner', 'contact_type' => $contact_type, 'contact_id' => $contact_id, 'branch_id' => $contact_id, 'contact_display_name' => 'Indiener ' . $contact_id, 'branch_display_name' => 'Indiener ' . $contact_id));
}

function ammo_item_view_test_can($type, $id) {
  $loaded = ammo_item_view_load($type, $id);
  return $loaded && ammo_item_view_access($type, $loaded);
}

global $user;
$previous_user = $user;
$previous_path = $_GET['q'];
$a = 99999901;
$b = 99999902;
$transaction = db_transaction();
try {
  $permissions = &drupal_static('user_access');
  $viewer = user_save(NULL, array('name' => 'ammo-item-viewer-' . uniqid(), 'status' => 1));
  $permissions[$viewer->uid] = array('view submitted amendments and motions' => TRUE);
  $outsider = user_save(NULL, array('name' => 'ammo-item-outsider-' . uniqid(), 'status' => 1));
  $permissions[$outsider->uid] = array();
  $adder = user_save(NULL, array('name' => 'ammo-item-adder-' . uniqid(), 'status' => 1, 'field_contact_id' => array(LANGUAGE_NONE => array(array('value' => 99999999)))));
  $permissions[$adder->uid] = array('add amendments and motions' => TRUE);
  $branches = &drupal_static('ammo_get_afdelingen');
  $branches[99999999] = array($a => 'SP Afdeling A');

  $meeting = ammo_save_entity('meeting', array('title' => 'Item view test', 'date' => '2099-01-01', 'amendment_submission_end' => '2099-01-01', 'motion_submission_end' => '2099-01-01', 'withdrawal_closing_date' => '2099-01-01', 'hide_meeting' => 0));
  $document = ammo_save_entity('document', array('title' => 'Item view document', 'meeting_id' => $meeting->id));
  $orphan_document = ammo_save_entity('document', array('title' => 'Orphan document', 'meeting_id' => 99999999));
  $ids = array();
  foreach (array('foreign' => 'submitted', 'own_draft' => 'concept', 'other_draft' => 'concept', 'member_draft' => 'concept', 'blocked' => 'blocked', 'withdrawn' => 'withdrawn', 'orphan' => 'submitted') as $key => $state) {
    $entry = ammo_save_entity('amendment', array('document_id' => $key === 'orphan' ? $orphan_document->id : $document->id, 'chapter' => 2, 'chapterized_id' => count($ids) + 1, 'page' => 14, 'line' => 220, 'amendment_text' => 'Tekst ' . $key, 'state' => $state));
    $ids[$key] = (int) $entry->id;
  }
  ammo_item_view_test_owner('amendment', $ids['foreign'], $b);
  ammo_item_view_test_owner('amendment', $ids['own_draft'], $a);
  ammo_item_view_test_owner('amendment', $ids['other_draft'], $b);
  ammo_item_view_test_owner('amendment', $ids['member_draft'], 99999999, 'member');
  ammo_item_view_test_owner('amendment', $ids['blocked'], $a);
  ammo_item_view_test_owner('amendment', $ids['withdrawn'], $a);
  $motion = ammo_save_entity('motion', array('meeting_id' => $meeting->id, 'motion_id' => 7, 'state' => 'submitted'));
  ammo_item_view_test_owner('motion', $motion->id, $a);

  foreach (array('amendment' => $ids['foreign'], 'motion' => $motion->id) as $type => $id) {
    $router = menu_get_item(ammo_item_view_path($type, $id));
    ammo_item_view_assert($router['path'] === 'ammo/' . $type . '/view/%' && $router['page_callback'] === 'ammo_item_view_page', 'Item route overrides the ' . $type . ' form route (run drush cc all)');
  }

  $user = $outsider;
  ammo_item_view_assert(!ammo_item_view_test_can('amendment', $ids['foreign']), 'User without Ammo permissions is denied');
  $user = $viewer;
  ammo_item_view_assert(ammo_item_view_test_can('amendment', $ids['foreign']) && ammo_item_view_test_can('amendment', $ids['withdrawn']) && ammo_item_view_test_can('motion', $motion->id), 'View permission opens published items');
  ammo_item_view_assert(!ammo_item_view_test_can('amendment', $ids['own_draft']) && !ammo_item_view_test_can('amendment', $ids['blocked']), 'Viewer sees no drafts or blocked items');
  $user = $adder;
  ammo_item_view_assert(ammo_item_view_test_can('amendment', $ids['foreign']) && ammo_item_view_test_can('motion', $motion->id), 'Add permission opens published items');
  ammo_item_view_assert(ammo_item_view_test_can('amendment', $ids['own_draft']), 'Branch sees its own draft');
  ammo_item_view_assert(ammo_item_view_test_can('amendment', $ids['member_draft']), 'Member owner sees own draft');
  ammo_item_view_assert(!ammo_item_view_test_can('amendment', $ids['other_draft']), 'Draft of another branch is denied');
  ammo_item_view_assert(!ammo_item_view_test_can('amendment', $ids['blocked']), 'Blocked item is denied, even for its branch');
  ammo_item_view_assert(ammo_item_view_page('amendment', $ids['blocked']) === MENU_ACCESS_DENIED, 'Denied item returns access denied');
  foreach (array($ids['orphan'], 'invalid', '-1', array(1), 0, 99999999) as $missing) {
    ammo_item_view_assert(ammo_item_view_page('amendment', $missing) === MENU_NOT_FOUND, 'Missing item, meeting or invalid id returns not found');
  }

  $_GET['q'] = ammo_item_view_path('amendment', $ids['foreign']);
  drupal_static_reset('drupal_get_destination');
  $page = ammo_item_view_page('amendment', $ids['foreign']);
  $markup = drupal_render($page);
  ammo_item_view_assert(strpos(drupal_get_title(), 'Voorstel 2.1') === 0, 'Page title carries the amendment heading');
  ammo_item_view_assert(strpos($markup, 'Item view test: Item view document') !== FALSE && strpos($markup, '<h3') === FALSE && strpos($markup, 'Tekst foreign') !== FALSE, 'Page shows meeting, document and the item without its heading');
  ammo_item_view_assert(strpos($markup, 'ammo/support/add/branch/amendment/' . $ids['foreign'] . '?destination=ammo/amendment/view/' . $ids['foreign']) !== FALSE, 'Actions return to the item page');
  ammo_item_view_assert(strpos($markup, 'ammo/amendments/' . $meeting->id . '?document=' . $document->id . '&amp;chapter=2#amendment' . $ids['foreign']) !== FALSE, 'Back link targets document, chapter and anchor');
  ammo_item_view_assert(strpos(implode(' ', array_keys(drupal_add_js())), 'ammo.js') !== FALSE, 'Script for collapsed details attached');
  $now = format_date(REQUEST_TIME, 'custom', 'j F Y, H:i');
  ammo_item_view_assert(strpos($markup, '<p class="ammo-item-dates">Aangemaakt op ' . $now . ', laatst gewijzigd op ' . $now . '</p>') > strpos($markup, 'Tekst foreign'), 'The creation and last change dates are shown below the item');
  db_update('ammo_amendments')->fields(array('created' => NULL))->condition('id', $ids['foreign'])->execute();
  entity_get_controller('amendment')->resetCache();
  $page = ammo_item_view_page('amendment', $ids['foreign']);
  ammo_item_view_assert(strpos(drupal_render($page), '<p class="ammo-item-dates">Laatst gewijzigd op ' . $now . '</p>') !== FALSE, 'An unknown creation date is left out');
  db_update('ammo_amendments')->fields(array('changed' => NULL))->condition('id', $ids['foreign'])->execute();
  entity_get_controller('amendment')->resetCache();
  $page = ammo_item_view_page('amendment', $ids['foreign']);
  ammo_item_view_assert(strpos(drupal_render($page), 'ammo-item-dates') === FALSE, 'No dates for items from before the updates');
  $page = ammo_item_view_page('amendment', $ids['withdrawn']);
  ammo_item_view_assert(strpos(drupal_render($page), 'class="amendment-details-toggle"') !== FALSE, 'Withdrawn item keeps its details toggle');
  $page = ammo_item_view_page('amendment', $ids['own_draft']);
  $markup = drupal_render($page);
  ammo_item_view_assert(strpos($markup, 'ammo/afdeling/' . $meeting->id) !== FALSE && strpos($markup, 'ammo/amendment/edit/' . $ids['own_draft']) !== FALSE, 'Draft links back to the branch overview and to its form');
  $page = ammo_item_view_page('motion', $motion->id);
  $markup = drupal_render($page);
  ammo_item_view_assert(drupal_get_title() === 'Motie 7' && strpos($markup, 'ammo/motions/' . $meeting->id . '#motion' . $motion->id) !== FALSE, 'Motion page has title and back link');

  $params = array('reply_to' => 'ammo-test@example.invalid', 'entity_type' => 'amendment', 'entity_id' => $ids['foreign'], 'id' => '2.1', 'meeting_id' => $meeting->id, 'document_title' => 'Item view document', 'markup' => '', 'first_owner' => 'Indiener', 'differences' => array());
  foreach (array('change_message' => array('foreign')) as $key => $linked) {
    $message = array('subject' => '', 'body' => array(), 'headers' => array());
    ammo_mail($key, $message, $params);
    $body = implode(' ', $message['body']);
    foreach ($linked as $item) {
      ammo_item_view_assert(strpos($body, url(ammo_item_view_path('amendment', $ids[$item]), array('absolute' => TRUE)) . '"') !== FALSE, 'Mail ' . $key . ' links to the item page');
    }
    ammo_item_view_assert(strpos($body, 'ammo/amendments/') === FALSE, 'Mail ' . $key . ' has no overview anchor link');
  }

  $user = user_load(1);
  ammo_item_view_assert(ammo_item_view_test_can('amendment', $ids['blocked']) && ammo_item_view_test_can('amendment', $ids['other_draft']), 'Administrator sees blocked items and drafts');
  $meeting->hide_meeting = 1;
  entity_save('meeting', $meeting);
  ammo_item_view_assert(!ammo_item_view_test_can('amendment', $ids['foreign']), 'Hidden meeting is closed for administrators too');
  $transaction->rollback();
  print "PASS: route, permissions, drafts, blocked, hidden and orphaned items, page content, return destinations, back links and mail links. Fixtures rolled back.\n";
}
catch (Throwable $e) {
  $transaction->rollback();
  throw $e;
}
finally {
  $user = $previous_user;
  $_GET['q'] = $previous_path;
  drupal_static_reset('drupal_get_destination');
}
