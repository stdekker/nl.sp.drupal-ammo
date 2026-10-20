<?php

/**
 * Run with ddev drush scr. All fixtures are rolled back.
 */
module_load_include('inc', 'ammo', 'ammo');
module_load_include('inc', 'ammo', 'test-data');

function ammo_test_data_assert($condition, $message) {
  if (!$condition) {
    throw new Exception($message);
  }
}

$transaction = db_transaction();
try {
  $branches = ammo_test_data_branches();
  $branch_ids = array_keys($branches);
  ammo_test_data_assert(count($branches) > 1, 'Integration fixture requires multiple existing branches');
  $branch_count = db_query('SELECT COUNT(*) FROM {node} WHERE type = :type', array(':type' => 'afdeling'))->fetchField();
  $meeting = ammo_save_entity('meeting', array('title' => 'Generator test', 'date' => '2000-01-01', 'motion_submission_end' => '2099-01-01'));
  $other = ammo_save_entity('meeting', array('title' => 'Other generator test', 'date' => '2000-01-01', 'motion_submission_end' => '2099-01-01'));
  $document = ammo_save_entity('document', array('title' => 'Test document', 'meeting_id' => $meeting->id, 'chapter_min' => 3, 'chapter_max' => 4, 'page_min' => 7, 'page_max' => 9, 'line_max' => 2));
  $real = ammo_save_entity('amendment', array('document_id' => $document->id, 'chapter' => 3, 'chapterized_id' => 1, 'amendment_text' => 'Real entry'));
  foreach (array('amendment', 'motion') as $type) {
    foreach (array(10, 25, 50, 100) as $count) {
      $before = ammo_test_data_count($meeting->id);
      ammo_test_data_generate($meeting->id, $type, $count, $document->id);
      ammo_test_data_assert(ammo_test_data_count($meeting->id) === $before + $count, 'Exact count for ' . $type . ': ' . $count);
      $owners = db_query('SELECT r.* FROM {ammo_contact_relations} r INNER JOIN {ammo_test_entries} t ON t.entry_type = r.target_type AND t.entry_id = r.target_id WHERE t.meeting_id = :meeting AND t.entry_type = :type AND r.relation_type = :relation ORDER BY r.id DESC', array(':meeting' => $meeting->id, ':type' => $type, ':relation' => 'owner'))->fetchAll();
      $owners = array_reverse(array_slice($owners, 0, $count));
      foreach ($owners as $i => $owner) {
        $expected_id = $branch_ids[$i % count($branch_ids)];
        ammo_test_data_assert($owner->contact_id == $expected_id && $owner->branch_id == $expected_id && $owner->contact_type === 'branch', 'Round-robin branch IDs');
        ammo_test_data_assert($owner->contact_display_name === ammo_cleanup_branch_name($branches[$expected_id]) && $owner->branch_display_name === $owner->contact_display_name, 'Matching branch names');
      }
    }
  }
  $rows = db_query('SELECT a.* FROM {ammo_amendments} a INNER JOIN {ammo_test_entries} t ON t.entry_id = a.id AND t.entry_type = :type WHERE t.meeting_id = :meeting', array(':type' => 'amendment', ':meeting' => $meeting->id))->fetchAll();
  $states = array();
  $numbers = array();
  $chapters = array();
  foreach ($rows as $row) {
    ammo_test_data_assert(!ammo_validate_document_ranges($document->id, $row->chapter, $row->page, $row->line), 'Valid document coordinates');
    ammo_test_data_assert($row->chapter != 3 || $row->chapterized_id > 1, 'Existing number preserved');
    $states[$row->state] = TRUE;
    $numbers[$row->chapter . '.' . $row->chapterized_id] = TRUE;
    $chapters[$row->chapter] = TRUE;
    ammo_test_data_assert(ammo_get_first_owner('amendment', $row->id), 'Owner exists');
  }
  ammo_test_data_assert(count($states) === count(ammo_states()), 'All statuses represented');
  ammo_test_data_assert(count($numbers) === count($rows), 'Unique amendment numbering');
  ammo_test_data_assert(count($chapters) === 2 && isset($chapters[3], $chapters[4]), 'Spread across allowed chapters');
  ammo_test_data_assert((int) db_query('SELECT COUNT(DISTINCT motion_id) FROM {ammo_motions} WHERE meeting_id = :id', array(':id' => $meeting->id))->fetchField() === 185, 'Unique motion numbering');
  $edited = ammo_save_entity('amendment', array('amendment_text' => 'Edited sample without marker'), $rows[0]->id);
  foreach (array('backer', 'vote_request') as $relation_type) {
    ammo_save_entity('contact_relation', array('target_type' => 'amendment', 'target_id' => $edited->id, 'relation_type' => $relation_type, 'contact_id' => 0, 'contact_type' => 'branch'));
  }
  ammo_test_data_generate($other->id, 'motion', 10);
  $no_branches = db_transaction();
  db_update('node')->fields(array('status' => 0))->condition('type', 'afdeling')->execute();
  ammo_test_data_assert(!ammo_test_data_branches(), 'Unpublished branches excluded');
  $missing_rejected = FALSE;
  try {
    ammo_test_data_generate($other->id, 'motion', 10);
  }
  catch (InvalidArgumentException $e) {
    $missing_rejected = TRUE;
  }
  ammo_test_data_assert($missing_rejected && ammo_test_data_count($other->id) === 10, 'No branches rejects generation without writes');
  $missing_state = array();
  $missing_form = ammo_test_data_form(array(), $missing_state, $other);
  ammo_test_data_assert(!isset($missing_form['submit']) && isset($missing_form['delete']), 'No branches blocks adding but permits cleanup');
  ammo_test_data_assert(ammo_test_data_delete($other->id) === 10, 'Cleanup works without branches');
  $no_branches->rollback();
  unset($no_branches);
  $meeting->motion_submission_end = NULL;
  entity_save('meeting', $meeting);
  $invalid = array(array($meeting->id, 'motion', 10, NULL), array($meeting->id, 'amendment', 11, $document->id), array($other->id, 'amendment', 10, $document->id), array($other->id, 'amendment', 10, NULL), array($meeting->id, 'invalid', 10, NULL));
  foreach ($invalid as $arguments) {
    $rejected = FALSE;
    try {
      call_user_func_array('ammo_test_data_generate', $arguments);
    }
    catch (InvalidArgumentException $e) {
      $rejected = TRUE;
    }
    ammo_test_data_assert($rejected, 'Invalid selection rejected');
  }
  $form_state = array();
  $form = ammo_test_data_form(array(), $form_state, $meeting);
  ammo_test_data_assert(!isset($form['entry_type']['#options']['motion']) && isset($form['delete']), 'Disabled motions hidden; cleanup available');
  ammo_test_data_assert(ammo_test_data_delete($meeting->id) === 370, 'Cleanup all series and both types');
  ammo_test_data_assert(ammo_test_data_count($meeting->id) === 0, 'Tracking cleared');
  ammo_test_data_assert(ammo_test_data_count($other->id) === 10, 'Other meeting preserved');
  ammo_test_data_assert(entity_load_single('amendment', $real->id), 'Real entry preserved');
  ammo_test_data_assert(!entity_load_single('amendment', $edited->id), 'Edited test entry deleted');
  ammo_test_data_assert(!db_query('SELECT COUNT(*) FROM {ammo_contact_relations} WHERE target_type = :type AND target_id = :id', array(':type' => 'amendment', ':id' => $edited->id))->fetchField(), 'All related rows deleted');
  ammo_test_data_assert(ammo_test_data_delete($meeting->id) === 0, 'Empty cleanup succeeds');
  ammo_test_data_assert(db_query('SELECT COUNT(*) FROM {node} WHERE type = :type', array(':type' => 'afdeling'))->fetchField() == $branch_count, 'Branch nodes preserved');
  $empty_form = ammo_test_data_form(array(), $form_state, $other);
  ammo_test_data_assert(!isset($empty_form['entry_type']['#options']['amendment']), 'No documents means no amendments');
  $routes = ammo_menu();
  ammo_test_data_assert($routes['admin/ammo/meeting/%meeting/test-data']['access callback'] === 'ammo_superadmin_access', 'Admin permission required');
  foreach (array(NULL, 20) as $maximum) {
    $wide = ammo_save_entity('document', array('title' => 'Chapters test', 'meeting_id' => $meeting->id, 'chapter_min' => 1, 'chapter_max' => $maximum));
    ammo_test_data_generate($meeting->id, 'amendment', 25, $wide->id);
    $distribution = db_query('SELECT chapter, COUNT(*) FROM {ammo_amendments} WHERE document_id = :id GROUP BY chapter ORDER BY chapter', array(':id' => $wide->id))->fetchAllKeyed();
    ammo_test_data_assert(array_map('intval', array_keys($distribution)) === range(1, 10), 'Uses exactly chapters 1 through 10');
    ammo_test_data_assert(max($distribution) - min($distribution) <= 1, 'Even chapter distribution');
  }
  $wide->chapter_min = 11;
  $wide->chapter_max = 20;
  entity_save('document', $wide);
  $rejected = FALSE;
  try {
    ammo_test_data_generate($meeting->id, 'amendment', 10, $wide->id);
  }
  catch (InvalidArgumentException $e) {
    $rejected = TRUE;
  }
  ammo_test_data_assert($rejected, 'Reject document with no chapters in 1 through 10');
  $transaction->rollback();
  print "PASS: counts, statuses, chapters, numbering, ownership, validation, forms and isolated cleanup. Fixtures rolled back.\n";
}
catch (Exception $e) {
  $transaction->rollback();
  throw $e;
}
